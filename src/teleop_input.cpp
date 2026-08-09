#include "teleop_input.hpp"

#include <algorithm>

namespace sim {

ReplayInputSource::ReplayInputSource(std::vector<RawInputFrame> frames, bool reverse)
    : frames_(std::move(frames)) {
    if (reverse) {
        for (auto& f : frames_) f.gear = flipGear(f.gear);
        std::reverse(frames_.begin(), frames_.end());
    }
}

} // namespace sim
