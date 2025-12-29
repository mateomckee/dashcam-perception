#include <chrono>
#include <iostream>
#include <thread>

#include "core/config_loader.hpp"
#include "core/frame.hpp"

#include "infra/latest_store.hpp"


int main(int argc, char** argv) {
  const std::string cfg_path = (argc > 1) ? argv[1] : "configs/dev.yaml";

  try {
    dcp::AppConfig cfg = dcp::LoadConfigFromYamlFile(cfg_path);

  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }

  return 0;
}
