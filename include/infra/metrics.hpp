#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/*
  Metrics.hpp implements Metrics, an object owned by the pipeline that stores all stage metrics, and StageMetrics,
  objects created for each stage to store general performance stats in. Also includes a helper function NowNs which
  simplifies grabbing the current time in nanoseconds integer format.
*/

namespace dcp {

using SteadyClock = std::chrono::steady_clock;

inline std::uint64_t NowNs() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          SteadyClock::now().time_since_epoch())
          .count());
}

// StageMetrics is a class that contains general performance stats for a stage to log. These stats are used in visualization, e.g., the ANSI dashboard and HUD overlay.
struct StageMetrics {
  std::string name;

  std::atomic<std::uint64_t> count{0};
  std::atomic<std::uint64_t> avg_latency_ns{0};
  std::atomic<std::uint64_t> last_event_ns{0};

  std::atomic<std::uint64_t> work_ns_total{0};

  explicit StageMetrics(std::string n) : name(std::move(n)) {
    last_event_ns.store(NowNs(), std::memory_order_relaxed);
  }
  void on_item(std::uint64_t latency_ns) {
    count.fetch_add(1, std::memory_order_relaxed);

    auto prev = avg_latency_ns.load(std::memory_order_relaxed);
    auto next = (prev == 0) ? latency_ns : (prev * 7 + latency_ns) / 8;
    avg_latency_ns.store(next, std::memory_order_relaxed);

    work_ns_total.fetch_add(latency_ns, std::memory_order_relaxed);
    last_event_ns.store(NowNs(), std::memory_order_relaxed);
  }
};

// Metrics is a class that stores StageMetrics, allowing pipelines to own and control all metrics involved in it.
class Metrics {
public:
  StageMetrics* make_stage(std::string name) {
    stages_.push_back(std::make_unique<StageMetrics>(std::move(name)));
    return stages_.back().get();
  }

  const std::vector<std::unique_ptr<StageMetrics>>& stages() const { return stages_; }

private:
  std::vector<std::unique_ptr<StageMetrics>> stages_;
};

} // namespace dcp