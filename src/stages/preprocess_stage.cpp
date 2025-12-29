#include "stages/preprocess_stage.hpp"

#include <chrono>
#include <iostream>

namespace dcp {

PreprocessStage::PreprocessStage(PreprocessConfig cfg, std::shared_ptr<BoundedQueue<Frame>> in, std::shared_ptr<BoundedQueue<PreprocessedFrame>> out)
    : Stage("preprocess_stage"), cfg_(std::move(cfg)), in_(std::move(in)), out_(std::move(out)) {}

void ComputeROI(RoiConfig roi_cfg) {

}

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
    
    // Here we now have a frame read and ready to be preprocessed

    // If we want to perform ROI cropping
    if(cfg_.crop_roi.enabled) {

        // If values passed in are normalized
        if(cfg_.crop_roi.use_normalized) {

        }
        // Otherwise, values are static pixel
        else {

        }
        
    }

    // New PreprocessedFrame
    PreprocessedFrame pf;

    // Push preprocessed_frame to output queue (fast path)
    out_->try_push(std::move(pf));

    // Push preprocessed_frame to latest_store (slow path)
    //
  }
}

} // namespace dcp