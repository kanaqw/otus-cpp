#include "mock_modules.hpp"

#include <cmath>

namespace sim {

MockPerception::MockPerception(const Scenario& scenario) {
    for (const auto& o : scenario.obstacles) {
        ObstacleState s;
        s.id = o.id; s.type = o.type;
        s.x = o.x; s.y = o.y; s.w = o.w; s.h = o.h;
        s.heading_rad = o.heading_deg * M_PI / 180.0;
        s.speed_mps = o.speed_mps;
        obstacles_.push_back(s);
    }
}

PerceptionInput MockPerception::step(double dt) {
    for (auto& o : obstacles_) {
        if (o.type == "dynamic") {
            o.x += o.speed_mps * std::cos(o.heading_rad) * dt;
            o.y += o.speed_mps * std::sin(o.heading_rad) * dt;
        }
    }
    return PerceptionInput{obstacles_};
}

Trajectory MockPlanner::step(const VehicleState& vehicle) {
    Trajectory traj;
    if (route_.empty()) return traj;

    // Advance progress index: drop waypoints we've effectively reached.
    while (cur_idx_ + 1 < route_.size() &&
           std::hypot(route_[cur_idx_].x - vehicle.x, route_[cur_idx_].y - vehicle.y) < spacing_m_) {
        ++cur_idx_;
    }

    if (cur_idx_ == route_.size() - 1 &&
        std::hypot(route_.back().x - vehicle.x, route_.back().y - vehicle.y) < 1.0) {
        return traj; // empty => route finished
    }

    // Walk the remaining polyline, resampling at spacing_m_ up to horizon_m_.
    double accumulated = 0.0;
    double px = vehicle.x, py = vehicle.y;
    size_t idx = cur_idx_;
    double distToNext = std::hypot(route_[idx].x - px, route_[idx].y - py);
    double nextSample = spacing_m_;

    while (accumulated < horizon_m_ && idx < route_.size()) {
        while (nextSample <= distToNext && accumulated < horizon_m_) {
            double t = distToNext > 1e-6 ? nextSample / distToNext : 0.0;
            double sx = px + (route_[idx].x - px) * t;
            double sy = py + (route_[idx].y - py) * t;
            traj.push_back({sx, sy, route_[idx].target_speed_mps});
            nextSample += spacing_m_;
            accumulated += spacing_m_;
        }
        nextSample -= distToNext;
        px = route_[idx].x; py = route_[idx].y;
        ++idx;
        if (idx < route_.size()) distToNext = std::hypot(route_[idx].x - px, route_[idx].y - py);
    }

    if (traj.empty()) traj.push_back({route_[cur_idx_].x, route_[cur_idx_].y, route_[cur_idx_].target_speed_mps});
    return traj;
}

Trajectory FixedTrajectorySource::step(const VehicleState& vehicle) {
    Trajectory traj;
    if (points_.empty()) return traj;

    while (cur_idx_ + 1 < points_.size() &&
           std::hypot(points_[cur_idx_].x - vehicle.x, points_[cur_idx_].y - vehicle.y) < 1.0) {
        ++cur_idx_;
    }
    if (cur_idx_ == points_.size() - 1 &&
        std::hypot(points_.back().x - vehicle.x, points_.back().y - vehicle.y) < 1.0) {
        return traj; // finished
    }

    double accumulated = 0.0;
    double px = vehicle.x, py = vehicle.y;
    for (size_t i = cur_idx_; i < points_.size() && accumulated < horizon_m_; ++i) {
        traj.push_back({points_[i].x, points_[i].y, points_[i].target_speed_mps});
        accumulated += std::hypot(points_[i].x - px, points_[i].y - py);
        px = points_[i].x; py = points_[i].y;
    }
    return traj;
}

} // namespace sim
