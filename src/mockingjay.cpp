#include "mockingjay.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <nlohmann/json.hpp>

#include "csv_logger.hpp"
#include "mock_modules.hpp"

using nlohmann::json;

void runMockingjayLoop(const sim::Scenario& scenario, const Args& args, sim::WsServer& ws,
                        const std::string& csv_path, std::atomic<bool>& stop_flag, std::atomic<bool>& pause_flag,
                        sim::IRawInputSource& source, const std::string& mode_label,
                        std::vector<sim::RawInputFrame>* out_recording,
                        sim::VehicleState* out_end_state,
                        bool reverse_undo) {
    stop_flag = false;
    pause_flag = false;
    std::cout << "[sim_runner] mockingjay '" << mode_label << "' starting -> " << csv_path << "\n";

    sim::VehicleState initial;
    initial.x = scenario.vehicle.x;
    initial.y = scenario.vehicle.y;
    initial.heading_rad = scenario.vehicle.heading_deg * M_PI / 180.0;

    const auto& vc = scenario.vehicle_config;
    sim::VehicleParams vparams;
    vparams.max_speed_mps = vc.max_speed_mps;
    vparams.max_steer_rad = vc.max_steer_deg * M_PI / 180.0;
    vparams.max_accel_mps2 = vc.max_accel_mps2;
    vparams.max_decel_mps2 = vc.max_decel_mps2;
    vparams.wheelbase_m = vc.wheelbase_m;
    vparams.gear = sim::gearFromString(vc.gear);
    sim::VehicleModel vehicle(initial, vparams);

    sim::MockPerception perception(scenario);

    sim::CsvLogger csv(csv_path, {
        "t", "vehicle_x", "vehicle_y", "vehicle_heading_deg", "vehicle_speed_mps",
        "target_x", "target_y", "target_speed_mps",
        "steering_request_deg", "acceleration_request_mps2", "control_error",
        "pedal", "steering_norm", "gear_code"
    });

    json started;
    started["type"] = "run_started";
    started["scenario_name"] = scenario.meta.name;
    started["mode"] = mode_label;
    ws.broadcast(started.dump());

    double t = 0.0;
    int ticks = 0;
    bool stopped = false;
    while (t < args.max_time_s) {
        if (pause_flag.load()) {
            while (pause_flag.load() && !stop_flag.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        if (stop_flag.load()) { stopped = true; break; }

        sim::PerceptionInput percept = perception.step(args.dt);

        if (source.done()) break; // checked before next() so the last frame isn't silently dropped
        sim::RawInputFrame f = source.next();
        if (out_recording) f.pre_tick_speed_mps = vehicle.state().speed_mps; // capture pre-tick speed for future reverse undo

        double accel = f.pedal >= 0 ? f.pedal * vparams.max_accel_mps2
                                     : f.pedal * (-vparams.max_decel_mps2);
        double steer = f.steering * vparams.max_steer_rad;
        vehicle.setGear(sim::gearFromString(f.gear));
        if (reverse_undo) {
            double beta = std::atan(0.5 * std::tan(steer));
            double v = f.pre_tick_speed_mps;
            double dtheta = (v / vparams.wheelbase_m) * std::sin(beta) * 2.0 * args.dt;
            sim::VehicleState ns = vehicle.state();
            double h_k = ns.heading_rad - dtheta; // undo this tick's forward heading change first
            ns.x += -v * std::cos(h_k + beta) * args.dt;
            ns.y += -v * std::sin(h_k + beta) * args.dt;
            ns.heading_rad = h_k;
            ns.speed_mps = v;
            vehicle.setState(ns);
        } else {
            vehicle.step(args.dt, steer, accel);
        }
        const auto& vs = vehicle.state();

        if (out_recording) out_recording->push_back(f);

        csv.writeRow({
            t, vs.x, vs.y, vs.heading_rad * 180.0 / M_PI, vs.speed_mps,
            vs.x, vs.y, vs.speed_mps,
            steer * 180.0 / M_PI, accel, 0.0,
            f.pedal, f.steering, static_cast<double>(static_cast<int>(vehicle.gear()))
        });

        json tick;
        tick["type"] = "tick";
        tick["t"] = t;
        tick["vehicle"] = { {"x", vs.x}, {"y", vs.y},
                             {"heading_deg", vs.heading_rad * 180.0 / M_PI},
                             {"speed_mps", vs.speed_mps},
                             {"gear", sim::gearToString(vehicle.gear())} };
        tick["control"] = { {"steering_deg", steer * 180.0 / M_PI},
                             {"accel_mps2", accel},
                             {"error", false} };
        tick["target"] = { {"x", vs.x}, {"y", vs.y}, {"speed_mps", vs.speed_mps} };
        tick["trajectory"] = json::array();
        json obs = json::array();
        for (const auto& o : percept.obstacles) {
            obs.push_back({ {"id", o.id}, {"type", o.type}, {"x", o.x}, {"y", o.y},
                             {"w", o.w}, {"h", o.h}, {"heading_deg", o.heading_rad * 180.0 / M_PI} });
        }
        tick["obstacles"] = obs;
        tick["route_finished"] = false;
        tick["finished_by_zone"] = false;
        ws.broadcast(tick.dump());
        ++ticks;

        if (stop_flag.load()) { stopped = true; break; }
        if (args.realtime) std::this_thread::sleep_for(std::chrono::duration<double>(args.dt));
        t += args.dt;
    }

    if (out_end_state) *out_end_state = vehicle.state();

    json complete;
    complete["type"] = "run_complete";
    complete["csv_path"] = csv_path;
    complete["ticks"] = ticks;
    complete["duration_s"] = t;
    complete["stopped"] = stopped;
    complete["mode"] = mode_label;
    complete["frames_recorded"] = out_recording ? static_cast<int>(out_recording->size()) : 0;
    ws.broadcast(complete.dump());
    std::cout << "[sim_runner] mockingjay " << (stopped ? "stopped" : "complete") << ": " << ticks
              << " ticks, " << t << "s, wrote " << csv_path << "\n";
}

std::vector<std::pair<double, double>> mockingjayPreviewPath(
    const sim::Scenario& base_scenario, const sim::VehicleState& start_state,
    const std::vector<sim::RawInputFrame>& frames, bool reverse, double dt) {
    std::vector<std::pair<double, double>> path;
    const auto& vc = base_scenario.vehicle_config;
    sim::VehicleParams vparams;
    vparams.max_speed_mps = vc.max_speed_mps;
    vparams.max_steer_rad = vc.max_steer_deg * M_PI / 180.0;
    vparams.max_accel_mps2 = vc.max_accel_mps2;
    vparams.max_decel_mps2 = vc.max_decel_mps2;
    vparams.wheelbase_m = vc.wheelbase_m;
    sim::VehicleModel vehicle(start_state, vparams);
    sim::ReplayInputSource source(frames, reverse);

    path.push_back({start_state.x, start_state.y});
    while (!source.done()) {
        sim::RawInputFrame f = source.next();
        double steer = f.steering * vparams.max_steer_rad;
        vehicle.setGear(sim::gearFromString(f.gear));
        if (reverse) {
            // Mirrors runMockingjayLoop's reverse_undo branch — must stay in
            // sync so the preview line matches what Replay Reverse actually does.
            double beta = std::atan(0.5 * std::tan(steer));
            double v = f.pre_tick_speed_mps;
            double dtheta = (v / vparams.wheelbase_m) * std::sin(beta) * 2.0 * dt;
            sim::VehicleState ns = vehicle.state();
            double h_k = ns.heading_rad - dtheta;
            ns.x += -v * std::cos(h_k + beta) * dt;
            ns.y += -v * std::sin(h_k + beta) * dt;
            ns.heading_rad = h_k;
            ns.speed_mps = v;
            vehicle.setState(ns);
        } else {
            double accel = f.pedal >= 0 ? f.pedal * vparams.max_accel_mps2
                                         : f.pedal * (-vparams.max_decel_mps2);
            vehicle.step(dt, steer, accel);
        }
        path.push_back({vehicle.state().x, vehicle.state().y});
    }
    return path;
}

void broadcastMockingjayPreview(sim::WsServer& ws, const MockingjayRecording& rec,
                                 const sim::VehicleState& current_state, double dt) {
    auto fwd = mockingjayPreviewPath(rec.base_scenario, current_state, rec.frames, false, dt);
    auto rev = mockingjayPreviewPath(rec.base_scenario, current_state, rec.frames, true, dt);
    json msg;
    msg["type"] = "mockingjay_preview";
    json fj = json::array();
    for (auto& p : fwd) fj.push_back({ {"x", p.first}, {"y", p.second} });
    json rj = json::array();
    for (auto& p : rev) rj.push_back({ {"x", p.first}, {"y", p.second} });
    msg["forward"] = fj;
    msg["reverse"] = rj;
    ws.broadcast(msg.dump());
}
