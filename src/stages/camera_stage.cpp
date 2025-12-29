#include "stages/camera_stage.hpp"

#include <chrono>
#include <iostream>

namespace dcp {

CameraStage::CameraStage(std::shared_ptr<BoundedQueue<Frame>> out)
    : Stage("camera_stage"), out_(std::move(out)) {}

void CameraStage::run(const StopToken& global, const std::atomic_bool& local) {
  using namespace std::chrono_literals;

  while (!global.stop_requested() && !local.load(std::memory_order_relaxed)) {
    Frame f;
    f.capture_time = std::chrono::steady_clock::now();
    f.sequence_id = next_id_++;

    out_->try_push(std::move(f));
    std::this_thread::sleep_for(33ms); // dummy ~30 FPS
  }
}

} // namespace dcp