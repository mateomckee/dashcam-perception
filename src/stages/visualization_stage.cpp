#include "stages/visualization_stage.hpp"

#include <chrono>
#include <iostream>

namespace dcp {

VisualizationStage::VisualizationStage(VisualizationConfig cfg, std::shared_ptr<BoundedQueue<RenderFrame>> in)
    : Stage("visualization_stage"), cfg_(std::move(cfg)), in_(std::move(in)) {}

void VisualizationStage::run(const StopToken& global, const std::atomic_bool& local) {
    using namespace std::chrono_literals;

    while (!global.stop_requested() && !local.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}

} // namespace dcp