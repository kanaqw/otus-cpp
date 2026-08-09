// mockingjay.hpp — raw driver-input record & replay ("Mockingjay").
//
// Drives the vehicle directly from raw input (pedal/steering/gear) each
// tick instead of following a route through MockPlanner/IControlManager —
// used for both live recording (source = LiveKeyboardSource) and replay
// (source = ReplayInputSource). Mirrors sim_loop's tick-pacing / pause /
// stop / CSV / WS-broadcast shape so the browser's existing Live Replay

#pragma once
#include <atomic>
#include <string>
#include <utility>
#include <vector>

#include "cli_args.hpp"
#include "scenario.hpp"
#include "teleop_input.hpp"
#include "vehicle_model.hpp"
#include "ws_server.hpp"

struct MockingjayRecording {
    sim::Scenario base_scenario;
    std::vector<sim::RawInputFrame> frames;
};

void runMockingjayLoop(const sim::Scenario& scenario, const Args& args, sim::WsServer& ws,
                        const std::string& csv_path, std::atomic<bool>& stop_flag, std::atomic<bool>& pause_flag,
                        sim::IRawInputSource& source, const std::string& mode_label,
                        std::vector<sim::RawInputFrame>* out_recording,
                        sim::VehicleState* out_end_state = nullptr,
                        bool reverse_undo = false);

std::vector<std::pair<double, double>> mockingjayPreviewPath(
    const sim::Scenario& base_scenario, const sim::VehicleState& start_state,
    const std::vector<sim::RawInputFrame>& frames, bool reverse, double dt);

void broadcastMockingjayPreview(sim::WsServer& ws, const MockingjayRecording& rec,
                                 const sim::VehicleState& current_state, double dt);
