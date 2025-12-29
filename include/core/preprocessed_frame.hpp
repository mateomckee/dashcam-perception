#pragma once

#include <cstdint>
#include <chrono>

#include <opencv2/core.hpp>

#include "core/frame.hpp"

namespace dcp {

struct PreprocessInfo {
  bool roi_applied{false};
  cv::Rect roi{};
  int resize_width{0};
  int resize_height{0};
};

// PreprocessFrame derives from Frame, and can easily adapt to whatever needs I may have in the future for preprocessing purposes, for now its simple
struct PreprocessedFrame {
  // Original frame data passed in
  std::uint64_t source_frame_id{0};
  std::chrono::steady_clock::time_point capture_time{};

  // Useful for debugging later
  std::chrono::steady_clock::time_point preprocess_time{};

  // Main preprocessing data
  cv::Mat image;        // What inference uses (roi/resize applied)
  PreprocessInfo info;  // Useful for mapping boxes back later since we are altering frames in this stage
};

} // namespace dcp