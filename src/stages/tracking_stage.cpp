#include <chrono>
#include <iostream>

#include "stages/tracking_stage.hpp"

namespace dcp {

TrackingStage::TrackingStage(StageMetrics* metrics, TrackingConfig cfg, std::shared_ptr<BoundedQueue<Frame>> in, std::shared_ptr<LatestStore<Detections>> detections_latest_store, std::shared_ptr<BoundedQueue<RenderFrame>> out)
    : Stage("tracking_stage"), metrics_(metrics), cfg_(std::move(cfg)), in_(std::move(in)), detections_latest_store_(std::move(detections_latest_store)), out_(std::move(out)) {}

void TrackingStage::run(const StopToken& global, const std::atomic_bool& local) {
    using namespace std::chrono_literals;

    while (!global.stop_requested() && !local.load(std::memory_order_relaxed)) {
        // First thing to do in Tracking stage is to always pop and read the next raw frame from the fast path
        Frame f;
        if(!in_->try_pop_for(f, 5ms)) {
            continue; // Try again
        }

        // Start work time
        const auto t0 = std::chrono::steady_clock::now();

        WorldState ws;

        RenderFrame rf;
        rf.frame = f;
        rf.world = ws;

        // Push RenderFrame (a wrapper containing WorldState and Frame) to visualization queue
        out_->try_push(rf);

        // End work time, store in metrics
        const auto work_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count();
        if (metrics_) metrics_->on_item(static_cast<std::uint64_t>(work_ns));
    }
}

} // namespace dcp