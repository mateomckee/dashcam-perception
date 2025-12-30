#pragma once

#include <cstdint>
#include <memory>

#include "core/config.hpp"
#include "core/render_frame.hpp"
#include "infra/bounded_queue.hpp"
#include "stages/stage.hpp"

namespace dcp {

class VisualizationStage final : public Stage {
public:
  VisualizationStage(VisualizationConfig cfg, std::shared_ptr<BoundedQueue<RenderFrame>> in);

protected:
  void run(const StopToken& global_stop,
           const std::atomic_bool& local_stop) override;

private:
  VisualizationConfig cfg_;
  std::shared_ptr<BoundedQueue<RenderFrame>> in_;
};

} // namespace dcp