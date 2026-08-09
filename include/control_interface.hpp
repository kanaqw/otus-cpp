// control_interface.hpp
//
// This defines the seam between the sim harness and your real control stack,
// matching your ControlManager's actual signature:
//
//   step(vehicle_position, trajectory) -> { acceleration_request, steering_request, error }
//
// `trajectory` is a sequence of points the vehicle is meant to move along
// (produced by MockPlanner today, your real Planning module later). To plug
// in your real stack: implement IControlManager with a class that wraps your
// actual Executor/ControlManager, translating VehicleState/Trajectory into
// whatever your real message types are, and translating your subsystems'
// Action/Error outputs into ControlOutput. Everything else in this project
// (main.cpp, vehicle model, logging, replay) is agnostic to which
// implementation is plugged in here.
#pragma once
#include <vector>
#include <string>
#include "vehicle_model.hpp"

namespace sim {

struct ObstacleState {
    int id = 0;
    std::string type;      // "static" | "dynamic"
    double x = 0, y = 0, w = 1, h = 1, heading_rad = 0, speed_mps = 0;
};

// A single point along the trajectory the vehicle is meant to move through.
struct TrajectoryPoint {
    double x = 0, y = 0;
    double target_speed_mps = 0;
};

// Ordered points ahead of the vehicle, nearest first. Empty = route finished.
using Trajectory = std::vector<TrajectoryPoint>;

struct PerceptionInput {
    std::vector<ObstacleState> obstacles;
};

struct ControlOutput {
    double steering_request_rad = 0.0;
    double acceleration_request_mps2 = 0.0;
    bool error = false;
    std::string error_message;

    // Optional, for logging/visualization only: which trajectory point this
    // step actually aimed at. Real implementations can leave valid=false —
    // main.cpp falls back to trajectory.front() for the CSV/replay target.
    bool debug_target_valid = false;
    double debug_target_x = 0.0, debug_target_y = 0.0, debug_target_speed_mps = 0.0;
};

class IControlManager {
public:
    virtual ~IControlManager() = default;
    virtual ControlOutput step(const VehicleState& vehicle_position,
                                const Trajectory& trajectory) = 0;
};

// ---- Stub implementation: pure-pursuit steering + a P speed controller. ----
// Picks the trajectory point nearest to (but not under) a fixed lookahead
// distance and steers/accelerates toward it. Replace this class with your
// real ControlManager adapter; keep the interface so the rest of the harness
// doesn't change.
class ControlManagerStub : public IControlManager {
public:
    explicit ControlManagerStub(double lookahead_m = 4.0, double wheelbase_m = 2.7)
        : lookahead_m_(lookahead_m), wheelbase_m_(wheelbase_m) {}

    ControlOutput step(const VehicleState& vehicle, const Trajectory& trajectory) override;

private:
    static double wrapAngle(double a);

    double lookahead_m_;
    double wheelbase_m_;
};

} // namespace sim
