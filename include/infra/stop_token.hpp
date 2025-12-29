#pragma once
#include <atomic>

/*
    StopToken / StopSource is a small utility for cooperative thread shutdown.

    The StopSource is owned by the pipeline (for example, live_pipeline.cpp) and represents
    a global stop request for the entire system.

    A StopToken is a read-only view into StopSource given to each thread, allowing them to
    observe whether a global stop has been requested by StopSource.

    Each thread also maintains its own local stop flag (via ThreadRunner), allowing
    a thread to stop independently. A thread should exit when either the global stop
    or its local stop is requested.
*/

namespace dcp {

class StopToken {
public:
  StopToken() = default;
  explicit StopToken(const std::atomic_bool* flag) : flag_(flag) {}

  bool stop_requested() const {
    return flag_ && flag_->load(std::memory_order_relaxed);
  }

private:
  const std::atomic_bool* flag_ = nullptr;
};

class StopSource {
public:
  StopSource() = default;

  StopToken token() const { return StopToken(&stop_); }

  // Same method names as ThreadRunner to keep uniformity between StopSource (pipeline) and ThreadRunners (stages)
  void request_stop() { stop_.store(true, std::memory_order_relaxed); }

  bool stop_requested() const { return stop_.load(std::memory_order_relaxed); }

private:
  std::atomic_bool stop_{false};
};

} // namespace dcp