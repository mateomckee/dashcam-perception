#pragma once

#include "core/frame.hpp"
#include "core/world_state.hpp"

namespace dcp {

struct RenderFrame {
  Frame frame;          // Raw frame (full resolution, original timing)
  WorldState world;     // Tracks/state aligned to this frame
};

} // namespace dcp