#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "infra/metrics.hpp"
#include "infra/stop_token.hpp"

namespace dcp {

struct QueueView {
  std::string name;
  std::function<std::size_t()> size_fn;
  std::function<std::size_t()> cap_fn;
  std::function<std::uint64_t()> drops_fn;
};

class AnsiDashboard {
public:
  AnsiDashboard(Metrics& metrics,
                std::vector<QueueView> queues,
                std::atomic_bool& sigint_flag);

  void run(const StopToken& stop);

private:
  Metrics& metrics_;
  std::vector<QueueView> queues_;
  std::atomic_bool& sigint_;

  struct Prev { std::uint64_t count{0}; std::uint64_t work_ns{0}; };
  std::unordered_map<const StageMetrics*, Prev> prev_stage_;
  std::unordered_map<std::string, std::uint64_t> prev_qdrops_;
};

} // namespace dcp