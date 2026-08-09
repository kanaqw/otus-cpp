#include "scenario.hpp"

#include <fstream>
#include <stdexcept>

namespace sim {

void from_json(const nlohmann::json& j, Meta& m) {
    if (j.contains("name")) j.at("name").get_to(m.name);
    if (j.contains("grid_size_m")) j.at("grid_size_m").get_to(m.grid_size_m);
}

void from_json(const nlohmann::json& j, VehicleStart& v) {
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    if (j.contains("heading_deg")) j.at("heading_deg").get_to(v.heading_deg);
}

void from_json(const nlohmann::json& j, VehicleConfig& c) {
    if (j.contains("max_speed_mps")) j.at("max_speed_mps").get_to(c.max_speed_mps);
    if (j.contains("max_steer_deg")) j.at("max_steer_deg").get_to(c.max_steer_deg);
    if (j.contains("max_accel_mps2")) j.at("max_accel_mps2").get_to(c.max_accel_mps2);
    if (j.contains("max_decel_mps2")) j.at("max_decel_mps2").get_to(c.max_decel_mps2);
    if (j.contains("wheelbase_m")) j.at("wheelbase_m").get_to(c.wheelbase_m);
    if (j.contains("gear")) j.at("gear").get_to(c.gear);
}

void from_json(const nlohmann::json& j, Zone& z) {
    j.at("x").get_to(z.x);
    j.at("y").get_to(z.y);
    if (j.contains("radius")) j.at("radius").get_to(z.radius);
    z.present = true;
}

void from_json(const nlohmann::json& j, Waypoint& w) {
    if (j.contains("id")) j.at("id").get_to(w.id);
    j.at("x").get_to(w.x);
    j.at("y").get_to(w.y);
    if (j.contains("target_speed_mps")) j.at("target_speed_mps").get_to(w.target_speed_mps);
    if (j.contains("curved")) j.at("curved").get_to(w.curved);
    if (j.contains("ctrl_x")) j.at("ctrl_x").get_to(w.ctrl_x);
    if (j.contains("ctrl_y")) j.at("ctrl_y").get_to(w.ctrl_y);
}

void from_json(const nlohmann::json& j, TrajectoryPointDef& p) {
    if (j.contains("id")) j.at("id").get_to(p.id);
    j.at("x").get_to(p.x);
    j.at("y").get_to(p.y);
    if (j.contains("target_speed_mps")) j.at("target_speed_mps").get_to(p.target_speed_mps);
    if (j.contains("t_s")) j.at("t_s").get_to(p.t_s);
    if (j.contains("curved")) j.at("curved").get_to(p.curved);
    if (j.contains("ctrl_x")) j.at("ctrl_x").get_to(p.ctrl_x);
    if (j.contains("ctrl_y")) j.at("ctrl_y").get_to(p.ctrl_y);
}

void from_json(const nlohmann::json& j, Obstacle& o) {
    if (j.contains("id")) j.at("id").get_to(o.id);
    if (j.contains("type")) j.at("type").get_to(o.type);
    if (j.contains("shape")) j.at("shape").get_to(o.shape);
    j.at("x").get_to(o.x);
    j.at("y").get_to(o.y);
    if (j.contains("w")) j.at("w").get_to(o.w);
    if (j.contains("h")) j.at("h").get_to(o.h);
    if (j.contains("radius")) j.at("radius").get_to(o.radius);
    if (j.contains("heading_deg")) j.at("heading_deg").get_to(o.heading_deg);
    if (j.contains("speed_mps")) j.at("speed_mps").get_to(o.speed_mps);
}

Scenario parse_scenario(const nlohmann::json& j, bool require_route) {
    Scenario s;
    if (j.contains("meta")) s.meta = j.at("meta").get<Meta>();
    if (j.contains("vehicle") && !j.at("vehicle").is_null()) {
        s.vehicle = j.at("vehicle").get<VehicleStart>();
        s.has_vehicle = true;
    }
    if (j.contains("vehicle_config") && !j.at("vehicle_config").is_null())
        s.vehicle_config = j.at("vehicle_config").get<VehicleConfig>();
    if (j.contains("start_zone") && !j.at("start_zone").is_null())
        s.start_zone = j.at("start_zone").get<Zone>();
    if (j.contains("finish_zone") && !j.at("finish_zone").is_null())
        s.finish_zone = j.at("finish_zone").get<Zone>();
    if (j.contains("route"))
        for (auto& item : j.at("route")) s.route.push_back(item.get<Waypoint>());
    if (j.contains("trajectory"))
        for (auto& item : j.at("trajectory")) s.trajectory.push_back(item.get<TrajectoryPointDef>());
    if (j.contains("obstacles"))
        for (auto& item : j.at("obstacles")) s.obstacles.push_back(item.get<Obstacle>());

    if (!s.has_vehicle) throw std::runtime_error("scenario: no vehicle start pose set");
    if (require_route && s.route.empty()) throw std::runtime_error("scenario: no route waypoints");
    return s;
}

Scenario load_scenario(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("scenario: could not open " + path);
    nlohmann::json j;
    f >> j;
    return parse_scenario(j);
}

} // namespace sim
