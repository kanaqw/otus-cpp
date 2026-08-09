// cli_args.hpp — sim_runner command-line argument parsing.
//
//   1. One-shot file mode:
//        sim_runner scenario.json [--port 8765] [--dt 0.02] [--out run.csv] [--fast] [--max-time S]
//      Loads the file, runs it once, exits.
//
//   2. Serve mode (for the "hot plug from the editor" workflow):
//        sim_runner --serve [--port 8765] [--dt 0.02] [--max-time S]
//      Starts the WebSocket server and waits.
#pragma once
#include <string>

struct Args {
    std::string scenario_path;   // empty in serve mode
    bool serve = false;
    int port = 8765;
    double dt = 0.02;
    std::string csv_path;        // one-shot mode only; serve mode auto-names each run
    bool realtime = true;
    double max_time_s = 120.0;
};

Args parseArgs(int argc, char** argv);

// Builds a filesystem-safe, timestamped CSV filename for a serve-mode run,
// e.g. "run_my_scenario_1712345678.csv".
std::string timestampedCsvName(const std::string& scenario_name);
