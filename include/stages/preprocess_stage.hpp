#pragma once

#include <cstdint>
#include <memory>

#include <opencv2/imgproc.hpp>

#include "core/config.hpp"
#include "core/frame.hpp"
#include "core/preprocessed_frame.hpp"
#include "infra/bounded_queue.hpp"
#include "stages/stage.hpp"

namespace dcp {

class PreprocessStage final : public Stage {
public:
  PreprocessStage(PreprocessConfig cfg, std::shared_ptr<BoundedQueue<Frame>> in, std::shared_ptr<BoundedQueue<PreprocessedFrame>> out);

protected:
  void run(const StopToken& global_stop,
           const std::atomic_bool& local_stop) override;

private:
  PreprocessConfig cfg_;
  std::shared_ptr<BoundedQueue<Frame>> in_;
  std::shared_ptr<BoundedQueue<PreprocessedFrame>> out_;
};

} // namespace dcp