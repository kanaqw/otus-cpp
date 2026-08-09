cmake_minimum_required(VERSION 3.16)
project(control_sim_ground CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
# Skip building nlohmann's own tests
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(json)

find_package(Threads REQUIRED)

add_executable(sim_runner
  src/main.cpp
  src/cli_args.cpp
  src/sim_loop.cpp
  src/mockingjay.cpp
  src/scenario.cpp
  src/vehicle_model.cpp
  src/control_interface.cpp
  src/mock_modules.cpp
  src/teleop_input.cpp
  src/ws_server.cpp
)

target_include_directories(sim_runner PRIVATE include)
target_link_libraries(sim_runner PRIVATE nlohmann_json::nlohmann_json Threads::Threads)

# ws_server.hpp uses raw POSIX sockets (Linux/macOS). On Windows you'd need
# Winsock2 (see the #ifdef stub at the top of include/ws_server.hpp).
