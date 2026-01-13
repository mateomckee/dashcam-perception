#include "stages/tracking_stage.hpp"

#include <chrono>
#include <optional>

namespace dcp {

static BBoxF MapDetToRaw(const Detection& d, const PreprocessInfo& pi) {
  const auto& roi = pi.roi;

  const float rw = roi.width  > 0 ? static_cast<float>(roi.width)  : 1.f;
  const float rh = roi.height > 0 ? static_cast<float>(roi.height) : 1.f;

  const float sw = pi.resize_width  > 0 ? static_cast<float>(pi.resize_width)  : rw;
  const float sh = pi.resize_height > 0 ? static_cast<float>(pi.resize_height) : rh;

  const float sx = rw / sw;
  const float sy = rh / sh;

  BBoxF out;
  out.x = static_cast<float>(roi.x) + d.bbox.x * sx;
  out.y = static_cast<float>(roi.y) + d.bbox.y * sy;
  out.w = d.bbox.w * sx;
  out.h = d.bbox.h * sy;
  return out;
}

TrackingStage::TrackingStage(StageMetrics* metrics,
                             TrackingConfig cfg,
                             std::shared_ptr<BoundedQueue<Frame>> in,
                             std::shared_ptr<LatestStore<Detections>> detections_latest_store,
                             std::shared_ptr<BoundedQueue<RenderFrame>> out)
    : Stage("tracking_stage"),
      metrics_(metrics),
      cfg_(std::move(cfg)),
      in_(std::move(in)),
      detections_latest_store_(std::move(detections_latest_store)),
      out_(std::move(out)) {}

void TrackingStage::run(const StopToken& global, const std::atomic_bool& local) {
  using namespace std::chrono_literals;

  std::optional<Detections> cached_dets;

  std::uint64_t next_track_id = 1;

  while (!global.stop_requested() && !local.load(std::memory_order_relaxed)) {
    Frame f;
    if (!in_->try_pop_for(f, 5ms)) continue;
    if (f.image.empty()) continue;

    const auto t0 = std::chrono::steady_clock::now();

    auto dets_opt = detections_latest_store_->read_latest();
    if (dets_opt) {
      cached_dets = std::move(*dets_opt);
    }

    WorldState ws;
    ws.frame_id = f.sequence_id;
    ws.timestamp = std::chrono::steady_clock::now();

    if (cached_dets) {
      ws.detections_source_frame_id = cached_dets->source_frame_id;
      ws.detections_inference_time = cached_dets->inference_time;

      ws.tracks.reserve(cached_dets->items.size());

      for (const auto& d : cached_dets->items) {
        Track t;
        t.id = next_track_id++;
        t.class_id = d.class_id;
        t.confidence = d.confidence;

        const BBoxF raw = MapDetToRaw(d, cached_dets->preprocess_info);
        t.bbox = raw;

        t.last_update_frame_id = f.sequence_id;
        t.age_frames = 1;
        t.missed_frames = 0;
        t.confirmed = true;

        ws.tracks.push_back(t);
      }
    } else {
      ws.detections_source_frame_id = 0;
      ws.detections_inference_time = {};
    }

    RenderFrame rf;
    rf.frame = std::move(f);
    rf.world = std::move(ws);

    out_->try_push(std::move(rf));

    const auto work_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count();

    if (metrics_) metrics_->on_item(static_cast<std::uint64_t>(work_ns));
  }
}

} // namespace dcp