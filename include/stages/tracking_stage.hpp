#pragma once

#include <cstdint>
#include <memory>

#include "core/config.hpp"
#include "core/frame.hpp"
#include "core/detections.hpp"
#include "infra/bounded_queue.hpp"
#include "infra/latest_store.hpp"
#include "stages/stage.hpp"

namespace dcp {

class TrackingStage final : public Stage {
public:
  TrackingStage(TrackingConfig cfg, std::shared_ptr<BoundedQueue<Frame>> in, std::shared_ptr<LatestStore<Detections>> detections_latest_store);

protected:
  void run(const StopToken& global_stop,
           const std::atomic_bool& local_stop) override;

private:
  TrackingConfig cfg_;
  std::shared_ptr<BoundedQueue<Frame>> in_;
  std::shared_ptr<LatestStore<Detections>> detections_latest_store_;
};

} // namespace dcp