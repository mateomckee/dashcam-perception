#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include "core/track.hpp"

namespace dcp {

using SteadyTP = std::chrono::steady_clock::time_point;

struct WorldState {
  std::uint64_t frame_id{0};
  SteadyTP timestamp{};
  std::vector<Track> tracks;

  std::uint64_t detections_source_frame_id{0};
  SteadyTP detections_inference_time{};
};

} // namespace dcp