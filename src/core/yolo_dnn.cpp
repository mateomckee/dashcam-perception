#include "core/yolo_dnn.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace dcp {

static inline float Clamp(float v, float lo, float hi) {
  return std::max(lo, std::min(hi, v));
}

static inline float IoUBox(const BBox& a, const BBox& b) {
  const float ax2 = a.x + a.w;
  const float ay2 = a.y + a.h;
  const float bx2 = b.x + b.w;
  const float by2 = b.y + b.h;

  const float ix1 = std::max(a.x, b.x);
  const float iy1 = std::max(a.y, b.y);
  const float ix2 = std::min(ax2, bx2);
  const float iy2 = std::min(ay2, by2);

  const float iw = std::max(0.f, ix2 - ix1);
  const float ih = std::max(0.f, iy2 - iy1);
  const float inter = iw * ih;

  const float ua = a.w * a.h + b.w * b.h - inter;
  return (ua <= 0.f) ? 0.f : (inter / ua);
}

YoloDnn::YoloDnn(Params p) : p_(std::move(p)) {
  try {
    sess_opts_.SetIntraOpNumThreads(1);
    sess_opts_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    session_ = std::make_unique<Ort::Session>(env_, p_.onnx_path.c_str(), sess_opts_);

    {
      auto in = session_->GetInputNameAllocated(0, allocator_);
      input_name_ = in ? std::string(in.get()) : std::string{};
    }
    {
      auto out = session_->GetOutputNameAllocated(0, allocator_);
      output_name_ = out ? std::string(out.get()) : std::string{};
    }

    loaded_ = !input_name_.empty() && !output_name_.empty();
  } catch (const Ort::Exception& e) {
    std::cerr << "ONNX Runtime init failed: " << e.what() << "\n";
    loaded_ = false;
  }
}

Detections YoloDnn::infer(const PreprocessedFrame& pf) {
  Detections out;
  out.inference_time = std::chrono::steady_clock::now();
  out.source_frame_id = pf.source_frame_id;
  out.preprocess_info = pf.info;

  if (!loaded_ || !session_ || pf.image.empty()) return out;

  cv::Mat resized;
  cv::resize(pf.image, resized, cv::Size(p_.input_w, p_.input_h), 0, 0, cv::INTER_LINEAR);

  cv::Mat rgb;
  cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

  cv::Mat f32;
  rgb.convertTo(f32, CV_32F, 1.0 / 255.0);

  std::vector<float> input_tensor(1 * 3 * p_.input_h * p_.input_w);
  {
    std::vector<cv::Mat> ch(3);
    cv::split(f32, ch);
    const int hw = p_.input_h * p_.input_w;
    std::memcpy(input_tensor.data() + 0 * hw, ch[0].data, hw * sizeof(float));
    std::memcpy(input_tensor.data() + 1 * hw, ch[1].data, hw * sizeof(float));
    std::memcpy(input_tensor.data() + 2 * hw, ch[2].data, hw * sizeof(float));
  }

  std::array<int64_t, 4> in_shape{1, 3, p_.input_h, p_.input_w};
  auto mem_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

  Ort::Value in = Ort::Value::CreateTensor<float>(
      mem_info, input_tensor.data(), input_tensor.size(), in_shape.data(), in_shape.size());

  const char* in_names[] = {input_name_.c_str()};
  const char* out_names[] = {output_name_.c_str()};

  std::vector<Ort::Value> ort_out;
  try {
    ort_out = session_->Run(Ort::RunOptions{nullptr}, in_names, &in, 1, out_names, 1);
  } catch (const Ort::Exception& e) {
    std::cerr << "ORT Run failed: " << e.what() << "\n";
    return out;
  }

  if (ort_out.empty() || !ort_out[0].IsTensor()) return out;

  auto& t = ort_out[0];
  auto shape = t.GetTensorTypeAndShapeInfo().GetShape();
  if (shape.size() != 3 || shape[0] != 1) return out;

  const float* data = t.GetTensorData<float>();
  const int A = static_cast<int>(shape[1]);
  const int B = static_cast<int>(shape[2]);

  const bool layout_CxN = (A < B);
  const int C = layout_CxN ? A : B;
  const int N = layout_CxN ? B : A;
  if (C < 6) return out;

  const int num_classes = C - 4;

  auto at = [&](int c, int n) -> float {
    if (layout_CxN) return data[c * N + n];
    return data[n * C + c];
  };

  const float sx = static_cast<float>(pf.image.cols) / static_cast<float>(p_.input_w);
  const float sy = static_cast<float>(pf.image.rows) / static_cast<float>(p_.input_h);

  struct Cand { BBox box; int cls; float score; };
  std::vector<Cand> cands;
  cands.reserve(256);

  for (int i = 0; i < N; ++i) {
    const float cx = at(0, i);
    const float cy = at(1, i);
    const float w  = at(2, i);
    const float h  = at(3, i);

    int best_cls = -1;
    float best = 0.f;
    for (int c = 0; c < num_classes; ++c) {
      const float s = at(4 + c, i);
      if (s > best) { best = s; best_cls = c; }
    }

    if (best < p_.conf_thresh) continue;

    const float x = (cx - 0.5f * w) * sx;
    const float y = (cy - 0.5f * h) * sy;
    const float ww = w * sx;
    const float hh = h * sy;

    BBox bb;
    bb.x = Clamp(x, 0.f, (float)pf.image.cols - 1.f);
    bb.y = Clamp(y, 0.f, (float)pf.image.rows - 1.f);
    bb.w = Clamp(ww, 0.f, (float)pf.image.cols - bb.x);
    bb.h = Clamp(hh, 0.f, (float)pf.image.rows - bb.y);
    if (bb.w <= 1.f || bb.h <= 1.f) continue;

    cands.push_back({bb, best_cls, best});
  }

  std::sort(cands.begin(), cands.end(),
            [](const Cand& a, const Cand& b) { return a.score > b.score; });

  std::vector<Cand> kept;
  kept.reserve(cands.size());

  for (const auto& c : cands) {
    bool ok = true;
    for (const auto& k : kept) {
      if (IoUBox(c.box, k.box) > p_.nms_thresh) { ok = false; break; }
    }
    if (ok) kept.push_back(c);
  }

  out.items.reserve(kept.size());
  for (const auto& k : kept) {
    Detection d;
    d.class_id = k.cls;
    d.confidence = k.score;
    d.bbox = k.box;
    out.items.push_back(d);
  }

  return out;
}

} // namespace dcp