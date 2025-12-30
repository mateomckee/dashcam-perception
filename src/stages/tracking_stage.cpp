#include "stages/tracking_stage.hpp"

#include <chrono>
#include <iostream>

namespace dcp {

TrackingStage::TrackingStage(TrackingConfig cfg, std::shared_ptr<BoundedQueue<Frame>> in, std::shared_ptr<LatestStore<Detections>> detections_latest_store)
    : Stage("tracking_stage"), cfg_(std::move(cfg)), in_(std::move(in)), detections_latest_store_(std::move(detections_latest_store)) {}

void TrackingStage::run(const StopToken& global, const std::atomic_bool& local) {
    using namespace std::chrono_literals;

    while (!global.stop_requested() && !local.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}

} // namespace dcp