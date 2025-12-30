#include <chrono>
#include <iostream>

#include "stages/preprocess_stage.hpp"

namespace dcp {

// Simple helper functions to encapsulate ROI calculation logic (only used in preprocess stage)
static cv::Rect ClampRect(const cv::Rect& r, int w, int h) {
    cv::Rect bounds(0, 0, w, h);
    cv::Rect out = r & bounds;
    return out;
}

static cv::Rect ComputeRoiRect(const cv::Mat& img, const RoiConfig& cfg) {
    if (!cfg.enabled) return cv::Rect(0, 0, img.cols, img.rows);

    cv::Rect roi;

    if (cfg.use_normalized) {
        auto clamp01 = [](float v) {
            if (v < 0.f) return 0.f;
            if (v > 1.f) return 1.f;
            return v;
        };

        const float x0 = clamp01(cfg.x_norm);
        const float y0 = clamp01(cfg.y_norm);
        const float w0 = clamp01(cfg.w_norm);
        const float h0 = clamp01(cfg.h_norm);

        const int x = static_cast<int>(x0 * img.cols);
        const int y = static_cast<int>(y0 * img.rows);
        const int w = static_cast<int>(w0 * img.cols);
        const int h = static_cast<int>(h0 * img.rows);

        roi = cv::Rect(x, y, w, h);
    } else {
        roi = cv::Rect(cfg.x, cfg.y, cfg.width, cfg.height);
    }

    roi = ClampRect(roi, img.cols, img.rows);

    if (roi.width <= 0 || roi.height <= 0) {
        // Fallback values for invalid configurations, bottomg half of the frame
        roi = cv::Rect(0, img.rows / 2, img.cols, img.rows - (img.rows / 2));
        roi = ClampRect(roi, img.cols, img.rows);
    }

    return roi;
}

PreprocessStage::PreprocessStage(PreprocessConfig cfg, std::shared_ptr<BoundedQueue<Frame>> in, std::shared_ptr<BoundedQueue<Frame>> out, std::shared_ptr<LatestStore<PreprocessedFrame>> preprocessed_latest_store)
    : Stage("preprocess_stage"), cfg_(std::move(cfg)), in_(std::move(in)), out_(std::move(out)), preprocessed_latest_store_(std::move(preprocessed_latest_store)) {}

void PreprocessStage::run(const StopToken& global, const std::atomic_bool& local) {
  using namespace std::chrono_literals;

  while (!global.stop_requested() && !local.load(std::memory_order_relaxed)) {
    // Frame read from input queue
    Frame f;

    // Try popping from input queue, store Frame in f
    // Returns false on failure after timeout period if queue is empty. Serves as the preprocess_stage's heartbeat for when no input exists
    // On failure, keep trying until input exists
    // Later implement a heartbeat shared value for other stages, for now keep here
    if(!in_->try_pop_for(f, 5ms)) {
        continue; // Try again
    }

    // If frame doesn't have data, skip
    const cv::Mat& src = f.image;
    if (src.empty()) {
        continue;
    }

    // Push raw frame to output queue (fast path), copy
    out_->try_push(f);

    // Begin preprocess operation for inference stage (slow path)

    const auto t_pre = std::chrono::steady_clock::now();

    // Perform ROI crop, nothing changes if disabled
    const cv::Rect roi = ComputeRoiRect(src, cfg_.crop_roi);
    const cv::Mat roi_view = src(roi);

    // Resize to configured size, nothing changes if disabled
    cv::Mat resized;
    cv::resize(roi_view, resized, cv::Size(cfg_.resize_width, cfg_.resize_height), 0, 0, cv::INTER_LINEAR);

    // Now that preprocessing is done, build new PreprocessedFrame and send it through to slow stream, even if no changes were made
    PreprocessedFrame pf;
    pf.source_frame_id = f.sequence_id;
    pf.capture_time = f.capture_time;
    pf.preprocess_time = t_pre;
    pf.image = std::move(resized);
    pf.info.roi_applied = cfg_.crop_roi.enabled;
    pf.info.roi = roi;
    pf.info.resize_width = cfg_.resize_width;
    pf.info.resize_height = cfg_.resize_height;

    // Write preprocessed frame to preprocessed latest_store (slow path), move
    preprocessed_latest_store_->write(std::move(pf));
  }
}

} // namespace dcp