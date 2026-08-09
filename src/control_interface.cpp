#include "control_interface.hpp"

#include <cmath>

namespace sim {

ControlOutput ControlManagerStub::step(const VehicleState& vehicle, const Trajectory& trajectory) {
    ControlOutput out;
    if (trajectory.empty()) {
        out.acceleration_request_mps2 = -4.0; // brake to stop, nothing left to track
        return out;
    }

    const TrajectoryPoint* target = &trajectory.front();
    for (const auto& p : trajectory) {
        double d = std::hypot(p.x - vehicle.x, p.y - vehicle.y);
        target = &p;
        if (d >= lookahead_m_) break;
    }

    out.debug_target_valid = true;
    out.debug_target_x = target->x;
    out.debug_target_y = target->y;
    out.debug_target_speed_mps = target->target_speed_mps;

    double dx = target->x - vehicle.x;
    double dy = target->y - vehicle.y;
    double dist = std::hypot(dx, dy);
    if (dist < 1e-3) return out;

    double target_heading = std::atan2(dy, dx);
    double alpha = wrapAngle(target_heading - vehicle.heading_rad);
    out.steering_request_rad = std::atan2(2.0 * wheelbase_m_ * std::sin(alpha), dist);

    double speed_err = target->target_speed_mps - vehicle.speed_mps;
    out.acceleration_request_mps2 = 1.2 * speed_err;
    return out;
}

double ControlManagerStub::wrapAngle(double a) {
    while (a > M_PI) a -= 2 * M_PI;
    while (a < -M_PI) a += 2 * M_PI;
    return a;
}

} // namespace sim
