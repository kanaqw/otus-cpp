#include "vehicle_model.hpp"

#include <algorithm>
#include <cmath>

namespace sim {

namespace {
double clamp(double v, double lo, double hi) { return std::max(lo, std::min(hi, v)); }
} // namespace

void VehicleModel::step(double dt, double steering_rad, double accel_mps2) {
    steering_rad = clamp(steering_rad, -params_.max_steer_rad, params_.max_steer_rad);
    accel_mps2 = clamp(accel_mps2, params_.max_decel_mps2, params_.max_accel_mps2);

    double direction = 1.0;
    switch (params_.gear) {
        case Gear::Park:
            accel_mps2 = state_.speed_mps > 0.0 ? params_.max_decel_mps2 : 0.0;
            break;
        case Gear::Neutral:
            accel_mps2 = 0.0;
            break;
        case Gear::Reverse:
            direction = -1.0;
            break;
        case Gear::Drive:
        default:
            break;
    }

    double beta = std::atan(0.5 * std::tan(steering_rad)); // simplified bicycle slip angle
    state_.x += direction * state_.speed_mps * std::cos(state_.heading_rad + beta) * dt;
    state_.y += direction * state_.speed_mps * std::sin(state_.heading_rad + beta) * dt;
    state_.heading_rad += direction * (state_.speed_mps / params_.wheelbase_m) * std::sin(beta) * 2.0 * dt;
    state_.speed_mps = clamp(state_.speed_mps + accel_mps2 * dt, 0.0, params_.max_speed_mps);
}

const char* gearToString(Gear g) {
    switch (g) {
        case Gear::Park: return "park";
        case Gear::Reverse: return "reverse";
        case Gear::Neutral: return "neutral";
        case Gear::Drive: default: return "drive";
    }
}

Gear gearFromString(const std::string& s) {
    if (s == "park") return Gear::Park;
    if (s == "reverse") return Gear::Reverse;
    if (s == "neutral") return Gear::Neutral;
    return Gear::Drive;
}

} // namespace sim
