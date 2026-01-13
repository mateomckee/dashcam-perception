#pragma once

#include <string>
#include <vector>

namespace dcp {

inline const std::vector<std::string>& GeneralLabels() {
  static const std::vector<std::string> labels = {
    "person","bicycle","car","motorcycle","airplane","bus","train","truck","boat",
    "traffic light","fire hydrant","stop sign","parking meter","bench","bird","cat","dog",
    "horse","sheep","cow","elephant","bear","zebra","giraffe","backpack","umbrella",
    "handbag","tie","suitcase","frisbee","skis","snowboard","sports ball","kite",
    "baseball bat","baseball glove","skateboard","surfboard","tennis racket","bottle",
    "wine glass","cup","fork","knife","spoon","bowl","banana","apple","sandwich","orange",
    "broccoli","carrot","hot dog","pizza","donut","cake","chair","couch","potted plant",
    "bed","dining table","toilet","tv","laptop","mouse","remote","keyboard","cell phone",
    "microwave","oven","toaster","sink","refrigerator","book","clock","vase","scissors",
    "teddy bear","hair drier","toothbrush"
  };
  return labels;
}

inline std::string GeneralClassName(int class_id) {
  const auto& labels = GeneralLabels();
  if (class_id < 0 || class_id >= static_cast<int>(labels.size()))
    return "unknown";
  return labels[class_id];
}

} // namespace dcp