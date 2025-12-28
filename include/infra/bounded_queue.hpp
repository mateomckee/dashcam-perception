#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>

#include "core/config.hpp" // For queue config

/*
    Implementation of bounded queue with capacity, drop policy, timed pop, non-blocking push, and statistics
*/

namespace dcp {

template <typename T>
class BoundedQueue {
public:
  explicit BoundedQueue(std::size_t capacity, DropPolicy policy): capacity_(capacity), policy_(policy) {}

  // No copy/move
  BoundedQueue(const BoundedQueue&) = delete;
  BoundedQueue& operator=(const BoundedQueue&) = delete;

  bool try_push(T item) {
    std::unique_lock<std::mutex> lock(mu_);

    ++pushes_;

    if (capacity_ == 0) {
      ++drops_;
      return false;
    }

    // If past capacity
    if (q_.size() >= capacity_) {
      if (policy_ == DropPolicy::DropNewest) {
        ++drops_;
        return false;
      }
      // DropOldest: remove one oldest element, then accept new one
      q_.pop_front();
      ++drops_;
    }

    q_.push_back(std::move(item));
    lock.unlock();
    cv_.notify_one();
    return true;
  }

  bool try_pop(T& out) {
    std::lock_guard<std::mutex> lock(mu_);

    if (q_.empty()) return false;

    out = std::move(q_.front());

    q_.pop_front();

    ++pops_;

    return true;
  }

  // Timeout variant of Pop function
  template <typename Rep, typename Period>
  bool try_pop_for(T& out, const std::chrono::duration<Rep, Period>& timeout) {
    std::unique_lock<std::mutex> lock(mu_);
    if (!cv_.wait_for(lock, timeout, [&] { return !q_.empty(); })) return false;

    out = std::move(q_.front());
    q_.pop_front();
    ++pops_;
    return true;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mu_);
    q_.clear();
  }

  // Getters

  std::size_t size() const {
    std::lock_guard<std::mutex> lock(mu_);
    return q_.size();
  }

  std::size_t capacity() const { return capacity_; }

  DropPolicy policy() const { return policy_; }

  std::uint64_t pushes_total() const {
    std::lock_guard<std::mutex> lock(mu_);
    return pushes_;
  }

  std::uint64_t pops_total() const {
    std::lock_guard<std::mutex> lock(mu_);
    return pops_;
  }

  std::uint64_t drops_total() const {
    std::lock_guard<std::mutex> lock(mu_);
    return drops_;
  }

private:
  const std::size_t capacity_;
  const DropPolicy policy_;

  mutable std::mutex mu_;
  std::condition_variable cv_;
  std::deque<T> q_;

  std::uint64_t pushes_{0};
  std::uint64_t pops_{0};
  std::uint64_t drops_{0};
};

} // namespace dcp