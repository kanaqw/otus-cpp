// sim_runner
//
// Two ways to run it:
//
//   1. One-shot file mode (unchanged from before):
//        sim_runner scenario.json [--port 8765] [--dt 0.02] [--out run.csv] [--fast] [--max-time S]
//      Loads the file, runs it once, exits.
//
//   2. Serve mode (new — for the "hot plug from the editor" workflow):
//        sim_runner --serve [--port 8765] [--dt 0.02] [--max-time S]
//      Starts the WebSocket server and waits. Every time web/sim-ground.html
//      sends a {"type":"run_scenario","scenario":{...}} message (the "Run"
//      button in the Editor/Replay tabs), this runs that scenario
//      immediately — no export/import round-trip, no restart. Runs are
//      serialized (one at a time); each writes its own timestamped CSV.
//
// Either way, ticks stream over WebSocket to whatever's connected, and every
// run is logged to CSV for post-run analysis.
//
// This file just wires the pieces together: parses args (cli_args.hpp),
// starts the WsServer, and dispatches incoming messages to the sim loop
// (sim_loop.hpp) or the Mockingjay record/replay loop (mockingjay.hpp).
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <nlohmann/json.hpp>

#include "cli_args.hpp"
#include "mockingjay.hpp"
#include "scenario.hpp"
#include "sim_loop.hpp"
#include "teleop_input.hpp"
#include "ws_server.hpp"

using nlohmann::json;

int main(int argc, char** argv) {
    Args args = parseArgs(argc, argv);

    sim::WsServer ws(args.port);
    std::mutex run_mutex;
    std::atomic<bool> stop_requested{false};
    std::atomic<bool> pause_requested{false};
    sim::LiveKeyboardSource keyboard_source;
    std::mutex mockingjay_mutex;
    std::optional<MockingjayRecording> mockingjay_recording;
    sim::VehicleState current_vehicle_state;

    if (args.serve) {
        ws.onMessage = [&](const std::string& payload) {
            try {
                json j = json::parse(payload);
                std::string type = j.value("type", "");
                if (type == "stop_run") { stop_requested = true; return; }
                if (type == "pause_run") {
                    pause_requested = true;
                    json m; m["type"] = "run_paused"; ws.broadcast(m.dump());
                    return;
                }
                if (type == "resume_run") {
                    pause_requested = false;
                    json m; m["type"] = "run_resumed"; ws.broadcast(m.dump());
                    return;
                }
                if (type == "teleop_input") {
                    sim::RawInputFrame f;
                    f.pedal = j.value("pedal", 0.0);
                    f.steering = j.value("steering", 0.0);
                    f.gear = j.value("gear", std::string("drive"));
                    keyboard_source.setLatest(f);
                    return;
                }
                if (type == "mockingjay_start_record") {
                    sim::Scenario scenario = sim::parse_scenario(j.at("scenario"), /*require_route=*/false);
                    keyboard_source.setLatest(sim::RawInputFrame{}); 
                    {
                        std::lock_guard<std::mutex> mjlock(mockingjay_mutex);
                        current_vehicle_state = sim::VehicleState{};
                        current_vehicle_state.x = scenario.vehicle.x;
                        current_vehicle_state.y = scenario.vehicle.y;
                        current_vehicle_state.heading_rad = scenario.vehicle.heading_deg * M_PI / 180.0;
                    }
                    json clear; clear["type"] = "mockingjay_preview"; clear["forward"] = json::array(); clear["reverse"] = json::array();
                    ws.broadcast(clear.dump()); 
                    std::thread([&, scenario]() {
                        std::lock_guard<std::mutex> lock(run_mutex);
                        auto frames = std::make_shared<std::vector<sim::RawInputFrame>>();
                        sim::VehicleState end_state;
                        runMockingjayLoop(scenario, args, ws, timestampedCsvName(scenario.meta.name + "_mj_record"),
                                           stop_requested, pause_requested, keyboard_source,
                                           "mockingjay_record", frames.get(), &end_state);
                        MockingjayRecording new_rec{scenario, *frames};
                        {
                            std::lock_guard<std::mutex> mjlock(mockingjay_mutex);
                            mockingjay_recording = new_rec;
                            current_vehicle_state = end_state;
                        }
                        if (!new_rec.frames.empty()) broadcastMockingjayPreview(ws, new_rec, end_state, args.dt);
                    }).detach();
                    return;
                }
                if (type == "mockingjay_stop_record") { stop_requested = true; return; }
                if (type == "mockingjay_replay") {
                    std::string direction = j.value("direction", "forward");
                    bool reverse = (direction == "reverse");
                    std::optional<MockingjayRecording> rec;
                    sim::VehicleState start_state;
                    {
                        std::lock_guard<std::mutex> mjlock(mockingjay_mutex);
                        rec = mockingjay_recording;
                        start_state = current_vehicle_state;
                    }
                    if (!rec) {
                        json err; err["type"] = "run_error"; err["message"] = "mockingjay: no recording to replay yet";
                        ws.broadcast(err.dump());
                        return;
                    }
                    sim::Scenario replay_scenario = rec->base_scenario;
                    replay_scenario.vehicle.x = start_state.x;
                    replay_scenario.vehicle.y = start_state.y;
                    replay_scenario.vehicle.heading_deg = start_state.heading_rad * 180.0 / M_PI;
                    std::string mode_label = reverse ? "mockingjay_replay_reverse" : "mockingjay_replay_forward";
                    std::string csv_suffix = reverse ? "_mj_replay_rev" : "_mj_replay_fwd";
                    std::thread([&, replay_scenario, rec, reverse, mode_label, csv_suffix]() {
                        std::lock_guard<std::mutex> lock(run_mutex);
                        sim::ReplayInputSource source(rec->frames, reverse);
                        sim::VehicleState end_state;
                        runMockingjayLoop(replay_scenario, args, ws,
                                           timestampedCsvName(replay_scenario.meta.name + csv_suffix),
                                           stop_requested, pause_requested, source, mode_label, nullptr, &end_state,
                                           /*reverse_undo=*/reverse);
                        {
                            std::lock_guard<std::mutex> mjlock(mockingjay_mutex);
                            current_vehicle_state = end_state;
                        }
                        broadcastMockingjayPreview(ws, *rec, end_state, args.dt);
                    }).detach();
                    return;
                }
                if (type != "run_scenario") return;
                sim::Scenario scenario = sim::parse_scenario(j.at("scenario"));
                std::thread([&ws, &run_mutex, &args, &stop_requested, &pause_requested, scenario]() {
                    std::lock_guard<std::mutex> lock(run_mutex);
                    runSimLoop(scenario, args, ws, timestampedCsvName(scenario.meta.name), stop_requested, pause_requested);
                }).detach();
            } catch (const std::exception& e) {
                std::cerr << "[sim_runner] bad message: " << e.what() << "\n";
                json err; err["type"] = "run_error"; err["message"] = e.what();
                ws.broadcast(err.dump());
            }
        };
        ws.start();
        std::cout << "[sim_runner] serve mode — open web/sim-ground.html, connect, and hit Run.\n";
        while (true) std::this_thread::sleep_for(std::chrono::seconds(3600));
    } else {
        ws.onMessage = [&](const std::string& payload) {
            try {
                std::string type = json::parse(payload).value("type", "");
                if (type == "stop_run") stop_requested = true;
                else if (type == "pause_run") { pause_requested = true; json m; m["type"]="run_paused"; ws.broadcast(m.dump()); }
                else if (type == "resume_run") { pause_requested = false; json m; m["type"]="run_resumed"; ws.broadcast(m.dump()); }
            } catch (...) {}
        };
        ws.start();
        sim::Scenario scenario;
        try {
            scenario = sim::load_scenario(args.scenario_path);
        } catch (const std::exception& e) {
            std::cerr << "Failed to load scenario: " << e.what() << "\n";
            return 1;
        }
        std::cout << "[sim_runner] open web/sim-ground.html, connect to ws://localhost:" << args.port
                  << " — streaming immediately.\n";
        runSimLoop(scenario, args, ws, args.csv_path, stop_requested, pause_requested);
        std::cout << "[sim_runner] keeping WebSocket open for 5s so the browser can finish rendering...\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
}
