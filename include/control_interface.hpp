// control_interface.hpp
//
// This defines the seam between the sim harness and real control stack,
// matching ControlManager's actual signature:
//
//   step(vehicle_position, trajectory) -> { acceleration_request, steering_request, error }
//
// `trajectory` is a sequence of points the vehicle is meant to move along
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

    bool debug_target_valid = false;
    double debug_target_x = 0.0, debug_target_y = 0.0, debug_target_speed_mps = 0.0;
};

class IControlManager {
public:
    virtual ~IControlManager() = default;
    virtual ControlOutput step(const VehicleState& vehicle_position,
                                const Trajectory& trajectory) = 0;
};

// Picks the trajectory point nearest to (but not under) a fixed lookahead
// distance and steers/accelerates toward it. 
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
