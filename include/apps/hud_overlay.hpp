#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

#include <opencv2/core.hpp>

#include "apps/ansi_dashboard.hpp"
#include "infra/metrics.hpp"

namespace dcp {

class HudOverlay {
public:
  HudOverlay() = default;

  void draw(cv::Mat& bgr,
            const Metrics& metrics,
            const std::vector<QueueView>& queues);

private:
  std::chrono::steady_clock::time_point last_refresh_{};
  cv::Mat panel_;

  struct Prev { std::uint64_t count{0}; std::uint64_t work_ns{0}; };

  std::unordered_map<const StageMetrics*, Prev> prev_stage_;
  std::unordered_map<std::string, std::uint64_t> prev_qdrops_;

  std::uint64_t last_tick_ns_{0};

  static double NsToMs(std::uint64_t ns);
  static std::string Bar(std::size_t used, std::size_t cap, std::size_t width);
};

} // namespace dcp