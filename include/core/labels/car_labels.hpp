#pragma once

#include <string>
#include <vector>

namespace dcp {
inline const std::vector<std::string>& CarLabels() {
  static const std::vector<std::string> labels = {
    "person",          // pedestrians
    "bicycle",         // cyclists
    "car",
    "motorcycle",
    "bus",
    "truck",
    "train",           // maybe in some environments
    "traffic light",   // signal lights
    "stop sign",       // road sign
    "parking meter",   // roadside object
    "bench",           // roadside
    // Optional depending on environment:
    "dog",             // animals crossing
    "cat",             // ——
    "fire hydrant",    // roadside
    "traffic sign"     // if your model supports it
  };
  return labels;
}

inline std::string CarClassName(int class_id) {
  const auto& labels = CarLabels();
  if (class_id < 0 || class_id >= static_cast<int>(labels.size()))
    return "unknown";
  return labels[class_id];
}
} // namespace dcp