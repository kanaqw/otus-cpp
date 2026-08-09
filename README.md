# Control Core Test Ground

A minimal C++ sim harness + one browser page for testing the control command
layer (Executor / ControlManager / subsystem controllers) against scenarios
you build by hand.

## Layout

```
web/sim-ground.html      Editor / Live Replay / Analysis, all in one page, one shared scenario
include/scenario.hpp      Scenario JSON <-> C++ structs (must match the editor's schema)
include/ws_server.hpp     Minimal bidirectional WebSocket server (no external deps)
include/csv_logger.hpp    Per-tick CSV logging
include/vehicle_model.hpp Kinematic bicycle model
include/mock_modules.hpp  MockPlanner (route->trajectory) / FixedTrajectorySource (authored trajectory) / MockPerception
include/control_interface.hpp  IControlManager seam + ControlManagerStub (pure pursuit + P speed)
src/main.cpp              Sim loop, one-shot file mode + serve mode
```

## Build

Requires CMake 3.16+, a C++17 compiler, and internet access at configure time
(CMake FetchContent pulls `nlohmann/json`).

```
mkdir build && cd build
cmake ..
cmake --build . -j
```

## Run — hot-plug workflow

```
./sim_runner --serve --port 8765
```

Open `web/sim-ground.html` in a browser, click **Connect**. Build a route in
the **Editor** tab — vehicle start, waypoints, obstacles — then click **▶ Run
in sim_runner**. That sends the in-memory scenario straight over the
WebSocket; **Live Replay** - streams the run as it happens, at real dt-paced speed. A
run in progress shows **⏸ Pause** and **■ Stop** next to Run:
- **Pause** freezes simulated time and vehicle state exactly where it is
  (server-side — the tick loop just stops advancing) and turns into
  **▶ Resume**.
- **Stop** ends the run early; the vehicle brakes to a stop and the run
  reports `stopped: true`.
Run stays disabled while a run is active, so you can't accidentally queue
two overlapping runs. Tweak the map and hit Run again for another pass —
`sim_runner` keeps running and handles each request in turn, writing a fresh
timestamped CSV per run.

### Curving segments (smooth turns)

In **Select** mode, click the line between two consecutive route or
trajectory points to select that segment — a small diamond handle appears
at its midpoint. Drag the handle to bow the segment into a smooth arc
instead of a sharp corner; the side panel shows "Straighten segment" to snap
it back to a straight line.

Mechanically each segment is a quadratic Bezier through the handle's
position — a handle sitting exactly at the segment's own midpoint is
mathematically identical to a straight line, so "straight" isn't a special
case, just the default handle position. This is real geometry, not just a
prettier picture: `sim_runner` resamples every curved segment along its
actual Bezier (`expandCurvedSegments` in `mock_modules.hpp`) before handing
it to `MockPlanner` or `FixedTrajectorySource`, so the simulated vehicle
actually follows the smoothed path, not the sharp-cornered polyline.

### Start / Finish zones

`Start Zone` and `Finish Zone` in the toolbar place a circle each (draggable,
resizable via the side panel once selected). **Finish zone is functional**:
the moment the vehicle's position enters it, the run ends immediately —
takes priority over the usual "reached the end of the route and stopped"
completion check. **Start zone is informational only** right now — it's not
currently used to gate anything (the vehicle's actual start pose is still
the separate "Place Vehicle" marker). If you actually want run-start
semantics tied to it (e.g. the clock doesn't start until the vehicle leaves
the start zone), say so and I'll wire that in — didn't want to assume.

### Vehicle Config

The Editor tab has a **Vehicle Config** card, separate from the route —
switchable test-run conditions for the vehicle itself:

- **Gear** — Park (locked, brakes to a stop and ignores any accel request),
  Reverse (drives backward along its heading), Neutral (coasts — accel
  requests are ignored, whatever speed it already has just carries, no
  friction model), Drive (normal).
- **Max speed**, **max wheel turn** (steering angle), **max accel/decel**,
  **wheelbase** — hard limits enforced by the vehicle model regardless of
  what `ControlManager` requests, same as a real vehicle's physical limits
  would clip an over-eager request.

These are part of the scenario JSON (`vehicle_config`), so they save/load/run
exactly like the route does — no separate config file. Older scenario files
without this field just get the defaults (20 m/s, 34°, 3 / -6 m/s², 2.7 m
wheelbase, Drive).

### Route vs. Trajectory

- **Route** (`+ Route` tool): sparse waypoints with a target speed each.
  Normally this is all you need — `MockPlanner` resamples it into an evenly
  spaced trajectory window every tick, standing in for your real Planning
  module.
- **Trajectory** (`+ Trajectory` tool, optional): an explicit, densely
  authored path sent to `ControlManager` exactly as drawn, bypassing
  `MockPlanner` entirely. Use this to hand `ControlManager` a specific path
  you designed — an adversarial curve, a precise lane-change geometry,
  whatever the route/planner combination wouldn't reliably reproduce. Each
  point optionally carries a timestamp (`t_s`, -1 = unset) for future
  open-loop/timed-replay use; today `FixedTrajectorySource` just walks the
  points as an ordered path.
- If `scenario.trajectory` is empty, `sim_runner` falls back to the route +
  `MockPlanner` — fully backward compatible with scenarios that only have a
  route.

Once a run finishes, switch to **Analysis** and click **"Analyze last run
from this session"** — no file needed, it's still in the browser's memory.
You can also load any previously-saved CSV there instead.

## Run — one-shot file mode (for CI / headless runs)

```
./sim_runner scenario.json --port 8765 --out run.csv
```

Use **Export JSON** in the Editor tab to get a `scenario.json` for this.
Loads it, runs once, writes the CSV, exits. Same WebSocket streaming happens
if a browser tab is connected while it runs.

## Plugging in your real control stack

`main.cpp` currently drives a `ControlManagerStub` (pure-pursuit steering + a
P speed controller) that implements `IControlManager` from
`control_interface.hpp`, matching your real signature:

```
step(vehicle_position, trajectory) -> { acceleration_request, steering_request, error }
```

`trajectory` is an ordered list of `{x, y, target_speed_mps}` points ahead of
the vehicle (produced today by `MockPlanner`, resampled from the map editor's
route at a fixed spacing out to a fixed horizon).

## Notes / known limits

- `ws_server.hpp` is intentionally minimal: no TLS, single-frame messages
  only (no fragmentation), broadcast to all connected clients. It does
  correctly loop on partial `send()`/`recv()` calls (required for TCP
  correctness, not optional). Fine for a local dev tool; not meant for
  anything beyond localhost.
- In serve mode, runs are serialized — a second "Run" while one is in
  progress waits its turn rather than overlapping.
- The vehicle model is a kinematic bicycle model — good for validating
  steering/speed control logic, not for anything requiring real dynamics
  (tire slip, load transfer, etc). Swap `vehicle_model.hpp` if you need that.
- `MockPlanner` walks the route and resamples it into an evenly spaced
  trajectory window. Replace it the same way as the control stack once your
  real Planning module has an interface to mock against.
- Charts are drawn by a small built-in canvas plotter (`MiniChart` in
  `web/sim-ground.html`) — no Plotly, no CDN, no external JS at all, so the
  page works fully offline. Chart init is still lazy (first time the Live
  Replay tab is shown) and everything that touches the DOM/canvas is wrapped
  so a rendering hiccup can never abort the rest of the script. The only
  remaining network reference in the page is the Google Fonts link, which is
  purely cosmetic — the page falls back to system fonts offline and works
  exactly the same.
- The Editor and Live Replay canvases share one world-space camera
  (`sharedView`): pan or zoom in either and the other stays locked to the
  same world position, so switching tabs never leaves you looking at a
  different part of the map. The two canvases can be different pixel sizes;
  each derives its own pixel offset from the shared scale + world-centre on
  every draw.
