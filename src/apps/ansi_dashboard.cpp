#include "apps/ansi_dashboard.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

namespace dcp {

static constexpr const char* kReset = "\033[0m";
static constexpr const char* kRed   = "\033[31m";
static constexpr const char* kGreen = "\033[32m";
static constexpr const char* kYellow= "\033[33m";

static double NsToMs(std::uint64_t ns) { return static_cast<double>(ns) / 1e6; }

// Fill a simple bar based on ratio of used/cap
static std::string Bar(std::size_t used, std::size_t cap, std::size_t width) {
  if (cap == 0) return std::string(width, '.');
  const double frac = static_cast<double>(used) / static_cast<double>(cap);
  
  const std::size_t filled = static_cast<std::size_t>(frac * width);
  std::string s;
  s.reserve(width);
  for (std::size_t i = 0; i < width; ++i) s.push_back(i < filled ? 'I' : '_');
  return s;
}

AnsiDashboard::AnsiDashboard(Metrics& metrics, std::vector<QueueView> queues, std::atomic_bool& sigint_flag): metrics_(metrics), queues_(std::move(queues)), sigint_(sigint_flag) {}

// Main draw function. Update every kHudPeriod ms, go through each metric stage and display calculates.
// Currently displays FPS, Busy % (thread utilization %), Latency in ms, and Last in ms (last time since stage processed an item, aka staleness)
void AnsiDashboard::run(const StopToken& stop) {
  using namespace std::chrono;
  using namespace std::chrono_literals;

  std::cout << "\033[2J\033[H" << std::flush;

  auto last = steady_clock::now();

  while (!stop.stop_requested()) {
    // Update CLI dashboard every X ms, currently set to 300ms. Later will provide a universal variable to not hardcode values
    std::this_thread::sleep_for(300ms);

    const auto now = steady_clock::now();
    const double dt = duration_cast<duration<double>>(now - last).count();
    last = now;

    const auto now_ns = dcp::NowNs();

    // Print dashboard title
    std::cout << "\033[H";
    std::cout << "PERCEPTION PIPELINE\n";
    std::cout << "SIGINT: " << (sigint_.load(std::memory_order_relaxed) ? "pending" : "ok") << "\n\n";

    // Print column names at specific positions
    std::cout << std::left
              << std::setw(14) << "STAGE"
              << std::setw(10) << "FPS"
              << std::setw(10) << "BUSY%"
              << std::setw(12) << "LAT(ms)"
              << std::setw(14) << "LAST(ms)"
              << "\n";
    std::cout << std::string(14 + 10 + 10 + 12 + 14, '-') << "\n";

    // For each stage
    for (const auto& up : metrics_.stages()) {
      const StageMetrics& m = *up;
      auto& p = prev_stage_[up.get()];

      // Compute FPS
      const auto c = m.count.load(std::memory_order_relaxed);
      const double fps = (dt > 0) ? (static_cast<double>(c - p.count) / dt) : 0.0;
      p.count = c;

      // Compute Work
      const auto work = m.work_ns_total.load(std::memory_order_relaxed);
      double busy = (dt > 0) ? static_cast<double>(work - p.work_ns) / (dt * 1e9) : 0.0;
      busy = std::max(0.0, std::min(1.0, busy));
      auto busy_color = (busy > 0.85) ? kRed : (busy > 0.60) ? kYellow : kGreen;
      p.work_ns = work;

      // Compute Latency and Staleness
      const double lat_ms = NsToMs(m.avg_latency_ns.load(std::memory_order_relaxed));
      const auto le = m.last_event_ns.load(std::memory_order_relaxed);
      const double last_ms = (le == 0) ? 0.0 : NsToMs(now_ns - le);

      // Print entire row of stats for this stage
      std::cout << std::left
                << std::setw(14) << m.name
                << std::setw(10)  << std::fixed << std::setprecision(1) << fps
                << busy_color << std::setw(10)  << std::fixed << std::setprecision(1) << (busy * 100.0) << kReset
                << std::setw(12) << std::fixed << std::setprecision(1) << lat_ms
                << std::setw(20) << std::fixed << std::setprecision(1) << last_ms
                << "\n";
    }

    // Queues sections, for each queue provided in pipeline, iterate and display its stats
    std::cout << "\nQUEUES\n";
    for (const auto& q : queues_) {
      const auto used = q.size_fn ? q.size_fn() : 0;
      const auto cap  = q.cap_fn ? q.cap_fn() : 0;

      // Get color based on fraction of usage/cap
      double frac = (cap == 0) ? 0.0 : (double)used / (double)cap;
      const char* color = (frac > 0.85) ? kRed : (frac > 0.60) ? kYellow : kGreen;

      std::uint64_t total_drops = q.drops_fn ? q.drops_fn() : 0;
      std::uint64_t& prev_total = prev_qdrops_[q.name];
      const double drop_ps = dt > 0 ? (static_cast<double>(total_drops - prev_total) / dt) : 0.0;
      prev_total = total_drops;

      // Print bar visual, using the Bar helper function to fill easily
      std::cout << "  " << std::setw(11) << std::left << q.name
                << " " << color << used << "/" << cap
                << " [" << Bar(used, cap, 24) << "]" << kReset
                << "  drop/s=" << std::fixed << std::setprecision(1) << drop_ps
                << "\n";
    }

    std::cout << "\n" << std::flush;
  }
}

} // namespace dcp