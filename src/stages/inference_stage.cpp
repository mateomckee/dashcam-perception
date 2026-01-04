#include <chrono>
#include <iostream>

#include "stages/inference_stage.hpp"

namespace dcp {

InferenceStage::InferenceStage(StageMetrics* metrics, InferenceConfig cfg, std::shared_ptr<LatestStore<PreprocessedFrame>> preprocessed_latest_store, std::shared_ptr<LatestStore<Detections>> detections_latest_store)
    : Stage("inference_stage"), metrics_(metrics), cfg_(std::move(cfg)), preprocessed_latest_store_(std::move(preprocessed_latest_store)), detections_latest_store_(std::move(detections_latest_store)) {}

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

        // Simulate a heavy inference operation
        //std::this_thread::sleep_for(std::chrono::milliseconds(72));

        // Store inference results
        dcp::Detections dets;
        dets.inference_time = std::chrono::steady_clock::now();
        dets.source_frame_id = pf_opt->source_frame_id;
        dets.preprocess_info = pf_opt->info;

        // This is where the detection mapping + storing would go, key to the whole system
        dcp::Detection d;
        d.class_id = 0;
        d.confidence = 0.9f;
        d.bbox = {50.f, 50.f, 100.f, 80.f};
        dets.items.push_back(d);
        //

        // Store detections in latest store
        detections_latest_store_->write(std::move(dets));

        // End work time, store in metrics
        const auto work_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count();
        if (metrics_) metrics_->on_item(static_cast<std::uint64_t>(work_ns));
    }
}

} // namespace dcp