#pragma once

#include <cstdint>
#include <memory>

#include "core/config.hpp"
#include "core/frame.hpp"
#include "core/preprocessed_frame.hpp"
#include "infra/bounded_queue.hpp"
#include "infra/latest_store.hpp"
#include "stages/stage.hpp"

namespace dcp {

class PreprocessStage final : public Stage {
public:
  PreprocessStage(PreprocessConfig cfg, std::shared_ptr<BoundedQueue<Frame>> in, std::shared_ptr<BoundedQueue<Frame>> out, std::shared_ptr<LatestStore<PreprocessedFrame>> preprocessed_latest_store);

protected:
  void run(const StopToken& global_stop,
           const std::atomic_bool& local_stop) override;

private:
  PreprocessConfig cfg_;
  std::shared_ptr<BoundedQueue<Frame>> in_;
  std::shared_ptr<BoundedQueue<Frame>> out_;
  std::shared_ptr<LatestStore<PreprocessedFrame>> preprocessed_latest_store_;
};

} // namespace dcp