#pragma once

#include <atomic>
#include <string>

#include "infra/stop_token.hpp"
#include "infra/thread_runner.hpp"

namespace dcp {

class Stage {
public:
  explicit Stage(std::string name);
  virtual ~Stage();

  Stage(const Stage&) = delete;
  Stage& operator=(const Stage&) = delete;

  void start(StopToken global_stop);
  void stop();

  const std::string& name() const { return name_; }

protected:
  virtual void run(const StopToken& global_stop,
                   const std::atomic_bool& local_stop) = 0;

private:
  std::string name_;
  ThreadRunner runner_;
};

} // namespace dcp