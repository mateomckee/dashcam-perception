#include <iostream>
#include "core/config_loader.hpp"

// live_pipeline.cpp is my full system MVP
// Starting point for running the whole pipeline

int main(int argc, char** argv) {
  const std::string cfg_path = (argc > 1) ? argv[1] : "configs/dev.yaml";

  try {
    dcp::AppConfig cfg = dcp::LoadConfigFromYamlFile(cfg_path);
    std::cout << "Loaded config OK: " << cfg_path << "\n";

    // Actual pipeline logic starts here

    // Begin by creating all resources (queues/lateststores/metrics/etc.) needed
    // Create the stages and pass references of resources to appropriate stages
    // Start each stage. The stage will then handle its own looping/thread logic

    // Start consumer stages before producer stages so queues don't fill immediately
    // Visualization, Tracking, Inference, Preprocessing, Camera

    // OnStop logic:
    // If a stage requests a global stop, handle it here. Stop all stages and cleanup
    // Stop producers first
    // 

  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }

  return 0;
}
