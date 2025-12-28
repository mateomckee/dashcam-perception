#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include "infra/stop_token.hpp"

/*
    ThreadRunner is a simple utility for basic thread usage. It owns one worker thread
    It provides:
        - Consistent start/stop behavior
        - A local_stop flag for stopping the singular worker thread
        - Read-only access to a global_stop flag for stopping on entire system shutdowns

*/

namespace dcp {

class ThreadRunner {
public:
  // Function signature for thread
  // Any callable that takes a global stop token and a local stop flag, and returns nothing
  using Fn = std::function<void(const StopToken&, const std::atomic_bool&)>;

  // Default constructor, allow naming threads
  ThreadRunner() = default;
  explicit ThreadRunner(std::string name);

  // Remove copy/move
  ThreadRunner(const ThreadRunner&) = delete;
  ThreadRunner& operator=(const ThreadRunner&) = delete;

  ~ThreadRunner();

  // Start the thread
  // Pass in StopToken to check for when to stop thread, and fn to do the actual work 
  void start(StopToken global_stop, Fn fn);

  // Request this specific thread to stop. Does NOT affect other threads
  void request_stop();
  // Returns true if either global or local stop flags are true
  bool stop_requested() const;

  void join();
  bool joinable() const;

  const std::string& name() const { return name_; }

// Basic member variables used in each thread instance
private:
  std::thread thread_;                    // The thread itself
  std::atomic_bool local_stop_{false};    // Flag that stops this thread only
  StopToken global_stop_{};               // Class that is used to check on StopSource's global_stop flag. Causes all threads to stop
  std::string name_{"thread"};            // Thread name, used for simple debugging/profiling
};

} // namespace dcp