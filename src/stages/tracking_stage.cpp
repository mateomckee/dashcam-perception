#include "stages/tracking_stage.hpp"

#include <algorithm>
#include <chrono>
#include <optional>
#include <vector>
#include "iostream"

namespace dcp {

static float IoU(const BBoxF& a, const BBox& b) {
  const float ax1 = a.x;
  const float ay1 = a.y;
  const float ax2 = a.x + a.w;
  const float ay2 = a.y + a.h;

  const float bx1 = b.x;
  const float by1 = b.y;
  const float bx2 = b.x + b.w;
  const float by2 = b.y + b.h;

  const float ix1 = std::max(ax1, bx1);
  const float iy1 = std::max(ay1, by1);
  const float ix2 = std::min(ax2, bx2);
  const float iy2 = std::min(ay2, by2);

  const float iw = std::max(0.f, ix2 - ix1);
  const float ih = std::max(0.f, iy2 - iy1);
  const float inter = iw * ih;

  const float area_a = std::max(0.f, a.w) * std::max(0.f, a.h);
  const float area_b = std::max(0.f, b.w) * std::max(0.f, b.h);
  const float uni = area_a + area_b - inter;

  if (uni <= 0.f) return 0.f;
  return inter / uni;
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
  std::vector<Track> tracks; // Where tracks are stored
  std::uint64_t next_id = 1;

  while (!global.stop_requested() && !local.load(std::memory_order_relaxed)) {
    Frame f;
    if (!in_->try_pop_for(f, 5ms)) continue;

    const auto t0 = std::chrono::steady_clock::now();

    // Read latest detections (if available) and cache them
    if (auto dets_opt = detections_latest_store_->read_latest()) {
      cached_dets = std::move(*dets_opt);
    }
    const Detections* dets = cached_dets ? &(*cached_dets) : nullptr;

    // Age all existing tracks; assume missed until matched
    for (auto& tr : tracks) {
      tr.age_frames++;
      tr.missed_frames++;
    }

    std::vector<bool> used(tracks.size(), false);

    // Associate detections to tracks by max IoU (same class only)
    if (dets) {

        // For each detection
        for (const auto& det : dets->items) {
        int best_i = -1;
        float best_iou = 0.f;

        // Simple brute force approach, check every detection against every track using IoU

        // For each track
        // Check every track against new detections
        for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
          if (used[static_cast<std::size_t>(i)]) continue;
          if (tracks[static_cast<std::size_t>(i)].class_id != det.class_id) continue;

          // Compute Intersection over Union (IoU) of this track and this detection
          const float iou = IoU(tracks[static_cast<std::size_t>(i)].bbox, det.bbox);
          if (iou > best_iou) {
            best_iou = iou;
            best_i = i;
          }
        }

        // After checking against all tracks, update track based on best match found

        if (best_i >= 0 && best_iou >= cfg_.iou_threshold) {
          // Update matched track
          Track& tr = tracks[static_cast<std::size_t>(best_i)];
          tr.bbox = BBoxF{det.bbox.x, det.bbox.y, det.bbox.w, det.bbox.h};
          tr.confidence = det.confidence;
          tr.class_id = static_cast<int>(det.class_id);
          tr.last_update_frame_id = f.sequence_id;
          tr.missed_frames = 0;
          tr.confirmed = (tr.age_frames >= cfg_.min_confirmed_frames);

          used[static_cast<std::size_t>(best_i)] = true;
        } else {
          // Spawn new track
          Track tr;
          tr.id = next_id++;
          tr.bbox = BBoxF{det.bbox.x, det.bbox.y, det.bbox.w, det.bbox.h};
          tr.class_id = static_cast<int>(det.class_id);
          tr.confidence = det.confidence;
          tr.last_update_frame_id = f.sequence_id;
          tr.age_frames = 1;
          tr.missed_frames = 0;
          tr.confirmed = (cfg_.min_confirmed_frames <= 1);

          tracks.push_back(tr);
          used.push_back(true);
        }
      }
    }

    // Drop tracks that have been missed too long
    tracks.erase(std::remove_if(tracks.begin(), tracks.end(),
                                [&](const Track& tr) {
                                  return tr.missed_frames > cfg_.max_missed_frames;
                                }),
                 tracks.end());

    // Build world state for this frame
    WorldState ws;
    ws.frame_id = f.sequence_id;
    ws.timestamp = std::chrono::steady_clock::now();
    ws.tracks = tracks;

    if (dets) {
      ws.detections_source_frame_id = dets->source_frame_id;
      ws.detections_inference_time = dets->inference_time;
    } else {
      ws.detections_source_frame_id = 0;
      ws.detections_inference_time = {};
    }

    RenderFrame rf;
    rf.frame = std::move(f);
    rf.world = std::move(ws);

    out_->try_push(std::move(rf));

    // End work time, store in metrics
    const auto work_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - t0).count();
    if (metrics_) metrics_->on_item(static_cast<std::uint64_t>(work_ns));
  }
}

} // namespace dcp