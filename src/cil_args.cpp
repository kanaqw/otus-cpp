#include "cli_args.hpp"

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

Args parseArgs(int argc, char** argv) {
    Args a;
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : ""; };
        if (arg == "--serve") a.serve = true;
        else if (arg == "--port") a.port = std::stoi(next());
        else if (arg == "--dt") a.dt = std::stod(next());
        else if (arg == "--out") a.csv_path = next();
        else if (arg == "--fast") a.realtime = false;
        else if (arg == "--max-time") a.max_time_s = std::stod(next());
        else positional.push_back(arg);
    }
    if (!a.serve) {
        if (positional.empty()) {
            std::cerr << "usage:\n"
                      << "  sim_runner <scenario.json> [--port N] [--dt S] [--out path.csv] [--fast] [--max-time S]\n"
                      << "  sim_runner --serve [--port N] [--dt S] [--fast] [--max-time S]\n";
            std::exit(1);
        }
        a.scenario_path = positional[0];
        if (a.csv_path.empty()) a.csv_path = "run.csv";
    }
    return a;
}

std::string timestampedCsvName(const std::string& scenario_name) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    long long secs = std::chrono::duration_cast<std::chrono::seconds>(now).count();
    std::string safe_name = scenario_name;
    for (auto& c : safe_name) if (!std::isalnum(static_cast<unsigned char>(c))) c = '_';
    return "run_" + safe_name + "_" + std::to_string(secs) + ".csv";
}
