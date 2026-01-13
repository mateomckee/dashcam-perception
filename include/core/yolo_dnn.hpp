#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "core/detections.hpp"
#include "core/preprocessed_frame.hpp"

#include <onnxruntime/onnxruntime_cxx_api.h>

namespace dcp {

class YoloDnn {
public:
  struct Params {
    std::string onnx_path;
    int input_w{640};
    int input_h{640};
    float conf_thresh{0.25f};
    float nms_thresh{0.45f};
  };

  explicit YoloDnn(Params p);

  bool is_loaded() const { return loaded_; }

  Detections infer(const PreprocessedFrame& pf);

private:
  Params p_;
  bool loaded_{false};

  Ort::Env env_{ORT_LOGGING_LEVEL_WARNING, "dcp-yolo"};
  Ort::SessionOptions sess_opts_{};
  std::unique_ptr<Ort::Session> session_;
  Ort::AllocatorWithDefaultOptions allocator_;

  std::string input_name_;
  std::string output_name_;

  static float IoU(const BBox& a, const BBox& b);
};

} // namespace dcp