#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include "core/preprocessed_frame.hpp"

namespace dcp {

using SteadyTP = std::chrono::steady_clock::time_point;

struct BBox {
  float x{0.f};
  float y{0.f};
  float w{0.f};
  float h{0.f};
};

// A singular Detection, location in the frame with a sepcified bounding box size, and the confidence
struct Detection {
  BBox bbox;
  std::int32_t class_id{-1};
  float confidence{0.f};
};

// The result of running inference on a frame. Multiple Detection objects containing their data
struct Detections {
  SteadyTP inference_time{}; // Useful timestamp for measuring inference staleness. Stores the time the inference result was produced
  std::uint64_t source_frame_id{0}; // Which frame this inference was produced from
  PreprocessInfo preprocess_info;  // Keep track of resize/crop values used in preprocess stage, useful in tracking stage
  std::vector<Detection> items;
};

} // namespace dcp