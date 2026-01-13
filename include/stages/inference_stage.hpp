#pragma once

#include <cstdint>
#include <memory>

#include "core/config.hpp"
#include "infra/metrics.hpp"
#include "core/preprocessed_frame.hpp"
#include "core/detections.hpp"
#include "infra/latest_store.hpp"
#include "stages/stage.hpp"

#include "core/yolo_dnn.hpp"

namespace dcp {

class InferenceStage final : public Stage {
public:
  InferenceStage(StageMetrics* metrics, InferenceConfig cfg, std::shared_ptr<LatestStore<PreprocessedFrame>> preprocessed_latest_store, std::shared_ptr<LatestStore<Detections>> detections_latest_store);

protected:
  void run(const StopToken& global_stop,
           const std::atomic_bool& local_stop) override;

private:
  StageMetrics* metrics_;
  InferenceConfig cfg_;
  std::shared_ptr<LatestStore<PreprocessedFrame>> preprocessed_latest_store_;
  std::shared_ptr<LatestStore<Detections>> detections_latest_store_;
  std::unique_ptr<YoloDnn> yolo_;
};

} // namespace dcp