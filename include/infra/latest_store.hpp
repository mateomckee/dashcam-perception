#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>

/*
    LatestStore is my key to the two-stream pipeline design.

    Inference (object detection) is the slowest stage. If we connect it with a normal queue in a single-stream pipeline,
    frames can backlog behind inference and end-to-end latency grows.

    With a two-stream design, the main stream (camera -> preprocess -> tracking/visualization) runs at the camera rate,
    while inference runs asynchronously at its own rate on the most recent available preprocessed frame.

    LatestStore enables this decoupling:
    - preprocess writes the newest frame into a LatestStore for inference to consume (overwriting older frames)
    - inference writes the newest detections into a LatestStore for tracking to consume (overwriting older detections)

    Tracking runs every frame and uses the latest available detections (which may be stale for a few frames) to update tracks/world state.
    When inference produces a new result, it replaces the old one in LatestStore and tracking immediately begins using it.

    This bounds latency by preventing unbounded inference backlog; the system degrades by increasing detection staleness rather than increasing end-to-end delay.
*/

namespace dcp {

template <typename T>
class LatestStore {
public:
  LatestStore() = default;

  LatestStore(const LatestStore&) = delete;
  LatestStore& operator=(const LatestStore&) = delete;

  void write(T value) {
    std::lock_guard<std::mutex> lock(mu_);
    latest_ = std::move(value);
    has_value_ = true;
    ++version_;
  }

  std::optional<T> read_latest() const {
    std::lock_guard<std::mutex> lock(mu_);
    if (!has_value_) return std::nullopt;
    return latest_;
  }

  std::uint64_t version() const {
    std::lock_guard<std::mutex> lock(mu_);
    return version_;
  }

  bool has_value() const {
    std::lock_guard<std::mutex> lock(mu_);
    return has_value_;
  }

private:
  mutable std::mutex mu_;
  T latest_{};
  bool has_value_{false};
  std::uint64_t version_{0};
};

} // namespace dcp