//Each mock reads directly from the loaded Scenario so a map-editor file fully
// determines a run. 
#pragma once
#include <vector>
#include "scenario.hpp"
#include "control_interface.hpp"

namespace sim {

// Expands any point list carrying the editor's curve fields (x, y,
// target_speed_mps, curved, ctrl_x, ctrl_y) into a dense straight-segment
// polyline: each curved segment (point i -> i+1 where point i has
// curved=true) gets resampled along its quadratic Bezier through
// (ctrl_x, ctrl_y); straight segments pass through as just their two
// endpoints. Used by both MockPlanner (route) and FixedTrajectorySource
// (authored trajectory) so a curve drawn in the editor actually bends the
// simulated vehicle's path, not just the picture of it.
template <typename PointT>
inline std::vector<Waypoint> expandCurvedSegments(const std::vector<PointT>& pts, int samples_per_curve = 10) {
    std::vector<Waypoint> out;
    if (pts.empty()) return out;
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        const auto& a = pts[i];
        const auto& b = pts[i + 1];
        Waypoint wa; wa.x = a.x; wa.y = a.y; wa.target_speed_mps = a.target_speed_mps;
        out.push_back(wa);
        if (a.curved) {
            for (int s = 1; s < samples_per_curve; ++s) {
                double t = static_cast<double>(s) / samples_per_curve;
                double mt = 1.0 - t;
                Waypoint wp;
                wp.x = mt * mt * a.x + 2 * mt * t * a.ctrl_x + t * t * b.x;
                wp.y = mt * mt * a.y + 2 * mt * t * a.ctrl_y + t * t * b.y;
                wp.target_speed_mps = a.target_speed_mps + (b.target_speed_mps - a.target_speed_mps) * t;
                out.push_back(wp);
            }
        }
    }
    Waypoint last;
    last.x = pts.back().x; last.y = pts.back().y; last.target_speed_mps = pts.back().target_speed_mps;
    out.push_back(last);
    for (size_t i = 0; i < out.size(); ++i) out[i].id = static_cast<int>(i) + 1;
    return out;
}

// Advances dynamic obstacles along their heading at constant speed each tick;
// static obstacles never move. Returns world-frame obstacle states. Not fed
// into ControlManager directly (its interface is vehicle position +
// trajectory only) — kept for scene visualization and, later, for whatever
// collision/obstacle-avoidance module sits between Perception and Planning.
class MockPerception {
public:
    explicit MockPerception(const Scenario& scenario);

    PerceptionInput step(double dt);

private:
    std::vector<ObstacleState> obstacles_;
};

// Common interface for anything that hands ControlManager its trajectory
class ITrajectorySource {
public:
    virtual ~ITrajectorySource() = default;
    virtual Trajectory step(const VehicleState& vehicle) = 0;
};

// Emits a local trajectory window each tick: the route waypoints from the
// vehicle's current progress out to `horizon_m` ahead, resampled at
// `spacing_m` intervals so ControlManager always sees evenly spaced points
// rather than the raw (possibly sparse) map-editor waypoints. Advances
// "current progress" based on nearest-ahead distance.
class MockPlanner : public ITrajectorySource {
public:
    explicit MockPlanner(std::vector<Waypoint> route, double horizon_m = 15.0, double spacing_m = 1.0)
        : route_(expandCurvedSegments(route)), horizon_m_(horizon_m), spacing_m_(spacing_m) {}

    Trajectory step(const VehicleState& vehicle) override;

private:
    std::vector<Waypoint> route_;
    double horizon_m_;
    double spacing_m_;
    size_t cur_idx_ = 0;
};

// Serves an explicitly authored trajectory (drawn in the editor's Trajectory
// tool, not derived from the route) as a moving window ahead of the
// vehicle's current progress along it. 
class FixedTrajectorySource : public ITrajectorySource {
public:
    explicit FixedTrajectorySource(const std::vector<TrajectoryPointDef>& points, double horizon_m = 15.0)
        : points_(expandCurvedSegments(points)), horizon_m_(horizon_m) {}

    Trajectory step(const VehicleState& vehicle) override;

private:
    std::vector<Waypoint> points_;
    double horizon_m_;
    size_t cur_idx_ = 0;
};

} // namespace sim
