#include "infra/thread_runner.hpp"

#include <stdexcept>
#include <utility>

namespace dcp {

ThreadRunner::ThreadRunner(std::string name) : name_(std::move(name)) {}

// Destructor safely stops thread on death
ThreadRunner::~ThreadRunner() {
  request_stop();
  if (thread_.joinable()) thread_.join();
}

// start, create a new worker thread
void ThreadRunner::start(StopToken global_stop, Fn fn) {
  if (thread_.joinable()) {
    throw std::runtime_error("ThreadRunner '" + name_ + "' already started");
  }

  local_stop_.store(false, std::memory_order_relaxed);
  global_stop_ = global_stop;

  thread_ = std::thread([this, fn = std::move(fn)]() mutable {
    fn(global_stop_, local_stop_);
  });
}

// request_stop, simply update local_stop to true
void ThreadRunner::request_stop() {
  local_stop_.store(true, std::memory_order_relaxed);
}

// Checks global_stop through StopToken, and local_stop to see if any stoppage has happened
bool ThreadRunner::stop_requested() const {
  return global_stop_.stop_requested() || local_stop_.load(std::memory_order_relaxed);
}

void ThreadRunner::join() {
  if (thread_.joinable()) thread_.join();
}

bool ThreadRunner::joinable() const {
  return thread_.joinable();
}

} // namespace dcp