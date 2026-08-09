// sim_loop.hpp — runs one scenario to completion through the mocked
// Planning -> IControlManager -> vehicle model pipeline, streaming ticks
// over WebSocket and logging to CSV.
#pragma once
#include <atomic>
#include <string>

#include "cli_args.hpp"
#include "scenario.hpp"
#include "ws_server.hpp"

// Runs one scenario to completion (or until stop_flag is set): mocked
// Planning -> IControlManager -> vehicle model, streaming ticks over `ws`
// and logging to a fresh CSV.
void runSimLoop(const sim::Scenario& scenario, const Args& args, sim::WsServer& ws,
                 const std::string& csv_path, std::atomic<bool>& stop_flag, std::atomic<bool>& pause_flag);
