// vehicle_model.hpp — kinematic bicycle model.
#pragma once
#include <string>

namespace sim {

struct VehicleState {
    double x = 0.0;
    double y = 0.0;
    double heading_rad = 0.0;
    double speed_mps = 0.0;
};

enum class Gear { Park, Reverse, Neutral, Drive };

struct VehicleParams {
    double wheelbase_m = 2.7;
    double max_steer_rad = 0.6;   // ~34 deg
    double max_accel_mps2 = 3.0;
    double max_decel_mps2 = -6.0;
    double max_speed_mps = 20.0;  // ~72 km/h
    Gear gear = Gear::Drive;
};

class VehicleModel {
public:
    explicit VehicleModel(VehicleState initial, VehicleParams params = {})
        : state_(initial), params_(params) {}

    // steering_rad: front wheel angle (positive = left). accel_mps2: signed
    // longitudinal acceleration request from the control stack. Gear governs
    // how that request gets applied: Park ignores it and brakes to a stop,
    // Neutral ignores it and coasts at whatever speed it already has (no
    // friction model here), Reverse applies it while moving backward along
    // the current heading instead of forward, Drive is the normal case.
    void step(double dt, double steering_rad, double accel_mps2);

    const VehicleState& state() const { return state_; }
    Gear gear() const { return params_.gear; }
    void setGear(Gear g) { params_.gear = g; }
    // Direct state override — used by Mockingjay's reverse replay, which
    // computes an exact undo of each recorded tick itself 
    void setState(const VehicleState& s) { state_ = s; }

private:
    VehicleState state_;
    VehicleParams params_;
};

const char* gearToString(Gear g);
Gear gearFromString(const std::string& s);

} // namespace sim
