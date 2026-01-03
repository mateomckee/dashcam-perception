#include <chrono>
#include <iostream>

#include <opencv2/videoio.hpp>

#include "stages/camera_stage.hpp"

namespace dcp {

CameraStage::CameraStage(StageMetrics* metrics, CameraConfig cfg, std::shared_ptr<BoundedQueue<Frame>> out)
    : Stage("camera_stage"), metrics_(metrics), cfg_(std::move(cfg)), out_(std::move(out)) {}

void CameraStage::run(const StopToken& global, const std::atomic_bool& local) {
  using namespace std::chrono_literals;

  // cap is our video capture, can be live video stream or recording
  cv::VideoCapture cap(cfg_.device_index);

  // On failure, camera doesn't open and we log error, keep pipeline alive
  if (!cap.isOpened()) return;

  // Set cv config
  cap.set(cv::CAP_PROP_FRAME_WIDTH, cfg_.width);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT, cfg_.height);
  cap.set(cv::CAP_PROP_FPS, cfg_.fps);

  while (!global.stop_requested() && !local.load(std::memory_order_relaxed)) {
    cv::Mat img;

    // Read one frame from capture, if unable, try again
    if (!cap.read(img)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }

    // Start work time
    const auto t0 = std::chrono::steady_clock::now();

    // Immediately handle frame adjustments once, make new canonical frame
    if (cfg_.flip_vertical) cv::flip(img, img, 0);
    if (cfg_.flip_horizontal) cv::flip(img, img, 1);

    // Create frame variable
    Frame f;
    f.capture_time = std::chrono::steady_clock::now();
    f.sequence_id = next_id_++;
    f.image = std::move(img); // Set frame data

    // Push frame to next queue
    out_->try_push(std::move(f));

    // End work time, store in metrics
    const auto work_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count();
    if (metrics_) metrics_->on_item(static_cast<std::uint64_t>(work_ns));
  }
}

} // namespace dcp