// teleop_input.hpp — "Mockingjay": raw driver-input record & replay.
//
// This defines the seam for wherever raw driver commands (pedal, steering
// wheel, gear) come from — mirrors the IControlManager seam in
// control_interface.hpp. Today the only implementation is
// LiveKeyboardSource, fed by WASD + gear keys relayed from the browser over
// the WebSocket (see web/sim-ground.html and main.cpp's "teleop_input"
// message). Later this becomes a ROS2 subscriber node holding the latest
// message off a real driver-input topic — same RawInputFrame shape, same
// IRawInputSource interface, nothing else in the sim loop needs to change.
#pragma once
#include <mutex>
#include <string>
#include <vector>

namespace sim {

// One tick's worth of raw driver input. pedal/steering are normalized
// (-1..1) rather than physical units so a recording replays consistently
// against whatever vehicle_config (max accel/decel/steer) it's run with —
// the sim loop scales these by the active VehicleParams each tick.
struct RawInputFrame {
    double pedal = 0.0;     // -1 (full brake/reverse-accel) .. +1 (full throttle)
    double steering = 0.0;  // -1 (full left) .. +1 (full right)
    std::string gear = "drive"; // "park" | "reverse" | "neutral" | "drive"
    // Speed the vehicle actually had *before* this frame was applied during
    // the original recording. Only ever set by the recording loop (see
    // mockingjay.cpp's runMockingjayLoop) — a live/forward source doesn't need
    // it, but reverse replay does: exactly undoing a tick's displacement
    // requires the speed that tick actually ran at, not whatever speed a
    // fresh accel-integration from rest would produce in reverse. See the
    // reverse_undo path in runMockingjayLoop for the math.
    double pre_tick_speed_mps = 0.0;
};

class IRawInputSource {
public:
    virtual ~IRawInputSource() = default;
    // Called once per sim tick to get the frame to apply this tick.
    virtual RawInputFrame next() = 0;
    // True once the source has nothing left to give (replay sources only —
    // a live source runs until externally stopped).
    virtual bool done() const { return false; }
};

// Live input source for recording: holds the most recently received frame,
// updated from the WS "teleop_input" handler on whatever thread that runs
// on. next() just hands back the latest known frame each tick, the same way
// a ROS2 subscriber callback would leave the latest topic message sitting
// there for the control loop to pick up.
class LiveKeyboardSource : public IRawInputSource {
public:
    void setLatest(const RawInputFrame& f) {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_ = f;
    }

    RawInputFrame next() override {
        std::lock_guard<std::mutex> lock(mutex_);
        return latest_;
    }

private:
    std::mutex mutex_;
    RawInputFrame latest_;
};

inline std::string flipGear(const std::string& g) {
    if (g == "drive") return "reverse";
    if (g == "reverse") return "drive";
    return g; // park/neutral have no meaningful inverse
}

// Replays a previously recorded sequence of frames.
class ReplayInputSource : public IRawInputSource {
public:
    ReplayInputSource(std::vector<RawInputFrame> frames, bool reverse);

    RawInputFrame next() override {
        if (idx_ >= frames_.size()) return RawInputFrame{};
        return frames_[idx_++];
    }

    bool done() const override { return idx_ >= frames_.size(); }

private:
    std::vector<RawInputFrame> frames_;
    size_t idx_ = 0;
};

} // namespace sim
