#include <chrono>
#include <iostream>

#include "stages/inference_stage.hpp"

namespace dcp {

InferenceStage::InferenceStage(StageMetrics* metrics, InferenceConfig cfg, std::shared_ptr<LatestStore<PreprocessedFrame>> preprocessed_latest_store, std::shared_ptr<LatestStore<Detections>> detections_latest_store)
    : Stage("inference_stage"), metrics_(metrics), cfg_(std::move(cfg)), preprocessed_latest_store_(std::move(preprocessed_latest_store)), detections_latest_store_(std::move(detections_latest_store))
{
    if (!cfg_.enabled) return;

    YoloDnn::Params p;
    p.onnx_path = cfg_.model.path;
    p.input_w = cfg_.model.input_width;
    p.input_h = cfg_.model.input_height;
    p.conf_thresh = cfg_.confidence_threshold;
    p.nms_thresh = 0.45f;

    yolo_ = std::make_unique<YoloDnn>(std::move(p));
    if (!yolo_->is_loaded()) {
        yolo_.reset();
    }
}

void InferenceStage::run(const StopToken& global, const std::atomic_bool& local) {
    using namespace std::chrono_literals;

    std::uint64_t last_seen_version = 0;

    while (!global.stop_requested() && !local.load(std::memory_order_relaxed)) {
        // Check if latest preprocessed frame has already been inferenced, if so, skip
        const auto ver = preprocessed_latest_store_->version();
        if (ver == last_seen_version) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        // Get next latest preprocessed frame and check that it exists
        auto pf_opt = preprocessed_latest_store_->read_latest();
        if (!pf_opt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        // Start work time
        const auto t0 = std::chrono::steady_clock::now();

        // Set for checking version against future frames
        last_seen_version = ver;

        PreprocessedFrame pf = std::move(*pf_opt);   // local stable snapshot
        dcp::Detections detections = yolo_->infer(pf);

        // Store detections in latest store
        detections_latest_store_->write(std::move(detections));

        // End work time, store in metrics
        const auto work_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count();
        if (metrics_) metrics_->on_item(static_cast<std::uint64_t>(work_ns));
    }
}

} // namespace dcp