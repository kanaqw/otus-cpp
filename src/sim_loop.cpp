#include "sim_loop.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <thread>
#include <nlohmann/json.hpp>

#include "control_interface.hpp"
#include "csv_logger.hpp"
#include "mock_modules.hpp"
#include "vehicle_model.hpp"

using nlohmann::json;

void runSimLoop(const sim::Scenario& scenario, const Args& args, sim::WsServer& ws,
                 const std::string& csv_path, std::atomic<bool>& stop_flag, std::atomic<bool>& pause_flag) {
    stop_flag = false;
    pause_flag = false;
    std::cout << "[sim_runner] running '" << scenario.meta.name << "' — "
              << scenario.route.size() << " waypoints, "
              << scenario.obstacles.size() << " obstacles -> " << csv_path << "\n";

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
    std::cout << "[sim_runner] vehicle config: max_speed=" << vc.max_speed_mps << "m/s, max_steer="
              << vc.max_steer_deg << "deg, accel/decel=" << vc.max_accel_mps2 << "/" << vc.max_decel_mps2
              << "m/s2, wheelbase=" << vc.wheelbase_m << "m, gear=" << vc.gear << "\n";

    sim::MockPerception perception(scenario);
    std::unique_ptr<sim::ITrajectorySource> trajectory_source;
    if (!scenario.trajectory.empty()) {
        std::cout << "[sim_runner] using authored trajectory (" << scenario.trajectory.size() << " points)\n";
        trajectory_source = std::make_unique<sim::FixedTrajectorySource>(scenario.trajectory);
    } else {
        std::cout << "[sim_runner] no authored trajectory — deriving one from the route via MockPlanner\n";
        trajectory_source = std::make_unique<sim::MockPlanner>(scenario.route);
    }

    //Control manager stub
    sim::ControlManagerStub control_manager;

    sim::CsvLogger csv(csv_path, {
        "t", "vehicle_x", "vehicle_y", "vehicle_heading_deg", "vehicle_speed_mps",
        "target_x", "target_y", "target_speed_mps",
        "steering_request_deg", "acceleration_request_mps2", "control_error"
    });

    json started;
    started["type"] = "run_started";
    started["scenario_name"] = scenario.meta.name;
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
        sim::Trajectory trajectory = trajectory_source->step(vehicle.state());
        bool route_finished = trajectory.empty();

        sim::ControlOutput out = control_manager.step(vehicle.state(), trajectory);
        vehicle.step(args.dt, out.steering_request_rad, out.acceleration_request_mps2);
        const auto& vs = vehicle.state();

        bool in_finish_zone = scenario.finish_zone.present &&
            std::hypot(vs.x - scenario.finish_zone.x, vs.y - scenario.finish_zone.y) <= scenario.finish_zone.radius;

        double tx = 0, ty = 0, tspeed = 0;
        if (out.debug_target_valid) { tx = out.debug_target_x; ty = out.debug_target_y; tspeed = out.debug_target_speed_mps; }
        else if (!trajectory.empty()) { tx = trajectory.front().x; ty = trajectory.front().y; tspeed = trajectory.front().target_speed_mps; }

        csv.writeRow({
            t, vs.x, vs.y, vs.heading_rad * 180.0 / M_PI, vs.speed_mps,
            tx, ty, tspeed,
            out.steering_request_rad * 180.0 / M_PI, out.acceleration_request_mps2,
            out.error ? 1.0 : 0.0
        });

        json tick;
        tick["type"] = "tick";
        tick["t"] = t;
        tick["vehicle"] = { {"x", vs.x}, {"y", vs.y},
                             {"heading_deg", vs.heading_rad * 180.0 / M_PI},
                             {"speed_mps", vs.speed_mps},
                             {"gear", sim::gearToString(vehicle.gear())} };
        tick["control"] = { {"steering_deg", out.steering_request_rad * 180.0 / M_PI},
                             {"accel_mps2", out.acceleration_request_mps2},
                             {"error", out.error} };
        tick["target"] = { {"x", tx}, {"y", ty}, {"speed_mps", tspeed} };
        json traj_json = json::array();
        for (const auto& p : trajectory) traj_json.push_back({ {"x", p.x}, {"y", p.y}, {"speed_mps", p.target_speed_mps} });
        tick["trajectory"] = traj_json;
        json obs = json::array();
        for (const auto& o : percept.obstacles) {
            obs.push_back({ {"id", o.id}, {"type", o.type}, {"x", o.x}, {"y", o.y},
                             {"w", o.w}, {"h", o.h}, {"heading_deg", o.heading_rad * 180.0 / M_PI} });
        }
        tick["obstacles"] = obs;
        tick["route_finished"] = route_finished;
        tick["finished_by_zone"] = in_finish_zone;
        ws.broadcast(tick.dump());
        ++ticks;

        bool finished = scenario.finish_zone.present ? in_finish_zone : (route_finished && vs.speed_mps < 0.05);
        if (finished) break;
        if (stop_flag.load()) { stopped = true; break; }
        if (args.realtime) std::this_thread::sleep_for(std::chrono::duration<double>(args.dt));
        t += args.dt;
    }

    json complete;
    complete["type"] = "run_complete";
    complete["csv_path"] = csv_path;
    complete["ticks"] = ticks;
    complete["duration_s"] = t;
    complete["stopped"] = stopped;
    ws.broadcast(complete.dump());
    std::cout << "[sim_runner] run " << (stopped ? "stopped" : "complete") << ": " << ticks
              << " ticks, " << t << "s, wrote " << csv_path << "\n";
}
