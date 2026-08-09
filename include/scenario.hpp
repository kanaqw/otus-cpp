// scenario.hpp
// Mirrors the JSON schema produced/consumed by web/sim-ground.html.
#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace sim {

struct Meta {
    std::string name = "unnamed_scenario";
    double grid_size_m = 1.0;
};

struct VehicleStart {
    double x = 0.0;
    double y = 0.0;
    double heading_deg = 0.0;
};

// Test-run conditions for the vehicle — switchable in the editor before
// running, independent of the route/trajectory/obstacles being tested.
// Feeds VehicleParams (see vehicle_model.hpp) plus the initial gear.
struct VehicleConfig {
    double max_speed_mps = 20.0;
    double max_steer_deg = 34.0;
    double max_accel_mps2 = 3.0;
    double max_decel_mps2 = -6.0;
    double wheelbase_m = 2.7;
    std::string gear = "drive"; // "park" | "reverse" | "neutral" | "drive"
};

// Each point optionally curves the segment FROM itself TO the next point
// into a quadratic Bezier through (ctrl_x, ctrl_y) — the editor's draggable
// midpoint handle. curved=false means "straight", which is exactly what you
// get anyway if ctrl were the segment's own midpoint (a quadratic Bezier
// with a midpoint control point degenerates to the straight chord), so the
// flag just avoids storing/depending on a redundant computed value.
// Meaningless on the last point in a route/trajectory (no outgoing segment).
// A circular marker on the map. Finish zones end the run the moment the
// vehicle enters them; start zones are informational only (not currently
// used to gate anything) — see sim_loop.cpp's termination check.
struct Zone {
    bool present = false;
    double x = 0.0;
    double y = 0.0;
    double radius = 2.0;
};

struct Waypoint {
    int id = 0;
    double x = 0.0;
    double y = 0.0;
    double target_speed_mps = 0.0;
    bool curved = false;
    double ctrl_x = 0.0;
    double ctrl_y = 0.0;
};

// An authored trajectory point. Distinct from Waypoint (the sparse route)
// because a trajectory can carry an explicit timestamp — useful for
// open-loop replay of a specific, hand-built path independent of whatever
// the (mocked or real) Planning module would generate from the route.
// t_s = -1 means "unspecified", i.e. treat as ordered points only.
struct TrajectoryPointDef {
    int id = 0;
    double x = 0.0;
    double y = 0.0;
    double target_speed_mps = 0.0;
    double t_s = -1.0;
    bool curved = false;
    double ctrl_x = 0.0;
    double ctrl_y = 0.0;
};

struct Obstacle {
    int id = 0;
    std::string type = "static";   // "static" | "dynamic"
    std::string shape = "rect";    // "rect" | "circle"
    double x = 0.0;
    double y = 0.0;
    double w = 1.0;                // rect only
    double h = 1.0;                // rect only
    double radius = 0.5;           // circle only
    double heading_deg = 0.0;      // direction of travel, dynamic only
    double speed_mps = 0.0;        // dynamic only
};

struct Scenario {
    Meta meta;
    bool has_vehicle = false;
    VehicleStart vehicle;
    VehicleConfig vehicle_config; // defaulted — optional in older scenario files
    Zone start_zone;              // present=false if not set
    Zone finish_zone;              // present=false if not set
    std::vector<Waypoint> route;
    std::vector<TrajectoryPointDef> trajectory; // optional — see FixedTrajectorySource
    std::vector<Obstacle> obstacles;
};

// ---- nlohmann::json (de)serialization ----

void from_json(const nlohmann::json& j, Meta& m);
void from_json(const nlohmann::json& j, VehicleStart& v);
void from_json(const nlohmann::json& j, VehicleConfig& c);
void from_json(const nlohmann::json& j, Zone& z);
void from_json(const nlohmann::json& j, Waypoint& w);
void from_json(const nlohmann::json& j, TrajectoryPointDef& p);
void from_json(const nlohmann::json& j, Obstacle& o);

// require_route=false is for callers that only need vehicle/vehicle_config/
// obstacles out of the JSON — e.g. Mockingjay record/replay, which drives
// the vehicle directly from raw input instead of following a route.
Scenario parse_scenario(const nlohmann::json& j, bool require_route = true);

Scenario load_scenario(const std::string& path);

} // namespace sim
