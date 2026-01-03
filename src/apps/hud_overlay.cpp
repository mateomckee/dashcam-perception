#include "apps/hud_overlay.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace dcp {

static constexpr auto kHudPeriod = std::chrono::milliseconds(300);

// Simple helper function to get color based on fraction (green -> yellow -> red), from 0.0 to 1.0
static cv::Scalar ColorByFrac(double frac) {
  if (frac > 0.85) return cv::Scalar(0, 0, 255);
  if (frac > 0.60) return cv::Scalar(0, 255, 255);
  return cv::Scalar(0, 255, 0);
}

double HudOverlay::NsToMs(std::uint64_t ns) {
  return static_cast<double>(ns) / 1e6;
}

// Fill a simple bar based on ratio of used/cap
std::string HudOverlay::Bar(std::size_t used, std::size_t cap, std::size_t width) {
  if (cap == 0) return std::string(width, '.');
  const double frac = static_cast<double>(used) / static_cast<double>(cap);
  const std::size_t filled = static_cast<std::size_t>(frac * width);
  std::string s;
  s.reserve(width);
  for (std::size_t i = 0; i < width; ++i) s.push_back(i < filled ? 'I' : '.');
  return s;
}

// Main draw function. Update every kHudPeriod ms, go through each metric stage and display calculates.
// Currently displays FPS, Busy % (thread utilization %), Latency in ms, and Last in ms (last time since stage processed an item, aka staleness)
void HudOverlay::draw(cv::Mat& bgr, const Metrics& metrics, const std::vector<QueueView>& queues) {
  using clock = std::chrono::steady_clock;
  const auto now = clock::now();

  const bool needs_refresh =
      (last_refresh_.time_since_epoch().count() == 0) || (now - last_refresh_ >= kHudPeriod);

  if (needs_refresh) {
    const auto now_ns = dcp::NowNs();

    double dt = 0.0;
    if (last_tick_ns_ != 0) dt = static_cast<double>(now_ns - last_tick_ns_) / 1e9;
    last_tick_ns_ = now_ns;
    last_refresh_ = now;

    // Display simple black box, where stats will be arranged and displayed
    const int line = 15;
    const int panel_w = 350;
    const int panel_h = 22 + line * (static_cast<int>(metrics.stages().size()) +
                                     static_cast<int>(queues.size()) + 3);

    panel_.create(panel_h, panel_w, bgr.type());
    panel_.setTo(cv::Scalar(0, 0, 0));
    cv::rectangle(panel_, cv::Rect(0, 0, panel_w, panel_h), cv::Scalar(80, 80, 80), 1);

    const int font = cv::FONT_HERSHEY_SIMPLEX;
    const double scale = 0.4;
    const int thickness = 1;

    auto put_at = [&](int x, int y, const std::string& s, const cv::Scalar& color = cv::Scalar(255, 255, 255)) {
      cv::putText(panel_, s, cv::Point(x, y), font, scale, color, thickness, cv::LINE_AA);
    };

    // Specific x positions of columns, I tinkered around with them and found a clean and consistent layout
    const int x_name = 6;
    const int x_fps  = 100;
    const int x_busy = 160;
    const int x_lat  = 220;
    const int x_last = 280;

    const int x_qname   = 6;
    const int x_usedcap = 100;
    const int x_bar     = 160;
    const int x_qdrop   = 280;

    int y = 18;

    // Print HUD title
    put_at(x_name, y, "PIPELINE STATS");
    y += line;

    // Print column names
    put_at(x_name, y, "STAGE");
    put_at(x_fps,  y, "FPS");
    put_at(x_busy, y, "BUSY%");
    put_at(x_lat,  y, "LAT(ms)");
    put_at(x_last, y, "LAST(ms)");
    y += line;

    cv::line(panel_, cv::Point(6, y - line + 4),
             cv::Point(panel_w - 6, y - line + 4),
             cv::Scalar(180, 180, 180), 1);

    // For each stage, get its metrics, perform basic calculations and print
    for (const auto& up : metrics.stages()) {
      const StageMetrics* mptr = up.get();
      const StageMetrics& m = *mptr;
      auto& p = prev_stage_[mptr];

      // Compute FPS
      const auto c = m.count.load(std::memory_order_relaxed);
      const double fps = (dt > 0.0) ? (static_cast<double>(c - p.count) / dt) : 0.0;
      p.count = c;

      // Compute work to find Busy%
      const auto work = m.work_ns_total.load(std::memory_order_relaxed);
      double busy = (dt > 0.0) ? (static_cast<double>(work - p.work_ns) / (dt * 1e9)) : 0.0;
      busy = std::max(0.0, std::min(1.0, busy));
      auto busy_color = ColorByFrac(busy);
      p.work_ns = work;

      // Compute latency and staleness in ms
      const double lat_ms = NsToMs(m.avg_latency_ns.load(std::memory_order_relaxed));
      const auto le = m.last_event_ns.load(std::memory_order_relaxed);
      const double last_ms = (le == 0) ? 0.0 : NsToMs(now_ns - le);

      std::ostringstream s_fps, s_busy, s_lat, s_last;
      s_fps  << std::fixed << std::setprecision(1) << fps;
      s_busy << std::fixed << std::setprecision(1) << (busy * 100.0);
      s_lat  << std::fixed << std::setprecision(1) << lat_ms;
      s_last << std::fixed << std::setprecision(1) << last_ms;

      // Print entire stats row for stage
      put_at(x_name, y, m.name);
      put_at(x_fps,  y, s_fps.str());
      put_at(x_busy, y, s_busy.str(), busy_color);
      put_at(x_lat,  y, s_lat.str());
      put_at(x_last, y, s_last.str());
      y += line;
    }

    y += 12;

    // Print Queues section columns
    put_at(x_qname,   y, "QUEUES");
    put_at(x_usedcap, y, "CAP");
    put_at(x_bar,     y, "DEPTH");
    put_at(x_qdrop,   y, "DROP/s");
    y += line;

    cv::line(panel_, cv::Point(6, y - line + 4),
             cv::Point(panel_w - 6, y - line + 4),
             cv::Scalar(180, 180, 180), 1);

    // For each queue we passed in from pipeline, compute its stats and print under queue section
    for (const auto& q : queues) {
      const auto used = q.size_fn ? q.size_fn() : 0;
      const auto cap  = q.cap_fn ? q.cap_fn() : 0;

      // Compute usage/cap
      const double frac = (cap == 0) ? 0.0 : static_cast<double>(used) / static_cast<double>(cap);
      const cv::Scalar qcolor = ColorByFrac(frac);

      // Compute drops per second
      const std::uint64_t total_drops = q.drops_fn ? q.drops_fn() : 0;
      std::uint64_t& prev_total = prev_qdrops_[q.name];
      const double drop_ps = (dt > 0.0) ? (static_cast<double>(total_drops - prev_total) / dt) : 0.0;
      prev_total = total_drops;

      std::ostringstream s_usedcap, s_drop;
      s_usedcap << used << "/" << cap;
      s_drop << std::fixed << std::setprecision(1) << drop_ps;

      const std::string bar = "[" + Bar(used, cap, 20) + "]";

      // Print stats for queue
      put_at(x_qname, y, q.name);
      put_at(x_usedcap, y, s_usedcap.str(), qcolor);
      put_at(x_bar, y, bar, qcolor);
      put_at(x_qdrop, y, s_drop.str());
      y += line;
    }
  }

  // At the end, position box in the bottom left of the frame
  if (!panel_.empty()) {
    const int margin = 6;

    const int w = std::min(panel_.cols, bgr.cols - 2 * margin);
    const int h = std::min(panel_.rows, bgr.rows - 2 * margin);

    const int x = margin;
    const int y = bgr.rows - h - margin;

    if (w > 0 && h > 0) {
      panel_(cv::Rect(0, 0, w, h))
          .copyTo(bgr(cv::Rect(x, y, w, h)));
    }
  }
}

} // namespace dcp