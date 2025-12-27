#include <iostream>
#include "core/config_loader.hpp"

// live_viewer.cpp is a bring-up tool
// Runs the camera, preprocessing, and visualization stages. Great for testing the camera and overlay

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
