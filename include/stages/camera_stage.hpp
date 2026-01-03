#pragma once

#include <cstdint>
#include <memory>

#include "core/config.hpp"
#include "infra/metrics.hpp"
#include "core/frame.hpp"
#include "infra/bounded_queue.hpp"
#include "stages/stage.hpp"

namespace dcp {

class CameraStage final : public Stage {
public:
  CameraStage(StageMetrics* metrics, CameraConfig cfg, std::shared_ptr<BoundedQueue<Frame>> out);

protected:
  void run(const StopToken& global_stop,
           const std::atomic_bool& local_stop) override;

private:
  StageMetrics* metrics_;
  CameraConfig cfg_;
  std::shared_ptr<BoundedQueue<Frame>> out_;
  std::uint64_t next_id_{0}; // Simple ID used to count each frame is it comes in
};

} // namespace dcp