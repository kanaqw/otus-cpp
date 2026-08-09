// mockingjay.hpp — raw driver-input record & replay ("Mockingjay").
//
// Drives the vehicle directly from raw input (pedal/steering/gear) each
// tick instead of following a route through MockPlanner/IControlManager —
// used for both live recording (source = LiveKeyboardSource) and replay
// (source = ReplayInputSource). Mirrors sim_loop's tick-pacing / pause /
// stop / CSV / WS-broadcast shape so the browser's existing Live Replay
// canvas and Analysis tab need no changes to render these runs: the same
// "run_started"/"tick"/"run_complete" message types are used, with an added
// "mode" field the browser uses to drive its Mockingjay button state.
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

// A completed Mockingjay recording: the vehicle_config/obstacles it was
// captured with (base_scenario — its vehicle start pose is only the pose
// at the moment recording began, not necessarily where replays start from)
// and the raw input frames captured tick by tick. Where a replay actually
// starts from is tracked separately by the caller (main.cpp's
// current_vehicle_state): forward and reverse replay both continue from
// wherever the vehicle currently is, not the original recording's start pose.
struct MockingjayRecording {
    sim::Scenario base_scenario;
    std::vector<sim::RawInputFrame> frames;
};

// reverse_undo=true (Replay Reverse only) replaces the normal accel/steering
// integration with an exact per-tick undo: it uses each frame's cached
// pre_tick_speed_mps and steering to invert exactly the displacement and
// heading change that tick produced during the original recording, so
// reverse replay always lands precisely back where the segment being
// undone started — not an approximation. Re-deriving speed via accel
// integration from rest (like the forward path does) doesn't work in
// reverse: the reversed pedal sequence integrated from a fresh stop
// produces a different speed profile than the original, which is what let
// reverse replay visibly drift off-path on anything but a constant-input
// drive. See the derivation in the plan/PR notes for why this is exact.
void runMockingjayLoop(const sim::Scenario& scenario, const Args& args, sim::WsServer& ws,
                        const std::string& csv_path, std::atomic<bool>& stop_flag, std::atomic<bool>& pause_flag,
                        sim::IRawInputSource& source, const std::string& mode_label,
                        std::vector<sim::RawInputFrame>* out_recording,
                        sim::VehicleState* out_end_state = nullptr,
                        bool reverse_undo = false);

// Silently re-simulates a recording (no WS ticks, no CSV) from `start_state`
// to produce the ghost-trace points the browser draws so you can see where
// Replay Forward / Replay Reverse would actually take the vehicle before
// committing to either — same VehicleModel integration runMockingjayLoop
// uses, just collecting positions instead of streaming/logging them.
std::vector<std::pair<double, double>> mockingjayPreviewPath(
    const sim::Scenario& base_scenario, const sim::VehicleState& start_state,
    const std::vector<sim::RawInputFrame>& frames, bool reverse, double dt);

void broadcastMockingjayPreview(sim::WsServer& ws, const MockingjayRecording& rec,
                                 const sim::VehicleState& current_state, double dt);
