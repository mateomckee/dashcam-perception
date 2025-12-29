#include "stages/stage.hpp"
#include <iostream>

#include <utility>

namespace dcp {

Stage::Stage(std::string name)
    : name_(std::move(name)), runner_(name_) {}

Stage::~Stage() = default;

void Stage::start(StopToken global_stop) {
  std::cout << name_ << " started" << std::endl;
  
  runner_.start(global_stop, [this](const StopToken& g, const std::atomic_bool& l) {
    run(g, l);
  });
}

void Stage::stop() {
  std::cout << name_ << " stopped" << std::endl;

  runner_.request_stop();
  runner_.join();
}

} // namespace dcp