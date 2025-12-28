#pragma once

#include <chrono>
#include <cstdint>

#include <opencv2/core.hpp>

/*
    Defines the structure for a singular frame in a video stream
*/

namespace dcp {

using TimePoint = std::chrono::steady_clock::time_point;

struct Frame {
  // Monotonic timestamp when frame was captured
  TimePoint capture_time;

  // Frame sequence number (monotonic)
  std::uint64_t sequence_id{0};

  // Image data (shared, ref-counted)
  cv::Mat image;
};

} // namespace dcp