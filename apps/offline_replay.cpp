#include <iostream>
#include "core/config_loader.hpp"

// offline_replay.cpp is a debugging tool
// Reads pre-recorded videos and runs the pipeline on it

int main(int argc, char** argv) {
  const std::string cfg_path = (argc > 1) ? argv[1] : "configs/dev.yaml";

  try {
    dcp::AppConfig cfg = dcp::LoadConfigFromYamlFile(cfg_path);
    std::cout << "Loaded config OK: " << cfg_path << "\n";

    // Actual pipeline logic starts here

  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }

  return 0;
}
