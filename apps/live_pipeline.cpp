#include <iostream>

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

// Utilities
#include "core/config_loader.hpp"
#include "core/frame.hpp"
#include "core/preprocessed_frame.hpp"
#include "core/detections.hpp"
#include "core/render_frame.hpp"

#include "infra/stop_token.hpp"

// Resources
#include "infra/bounded_queue.hpp"
#include "infra/latest_store.hpp"

// Stages
#include "stages/camera_stage.hpp"
#include "stages/preprocess_stage.hpp"
#include "stages/inference_stage.hpp"
#include "stages/tracking_stage.hpp"

#include <opencv2/highgui.hpp>  // cv::namedWindow, cv::imshow, cv::waitKey

static std::atomic_bool g_sigint{false};

static void HandleSigint(int) {
  g_sigint.store(true, std::memory_order_relaxed);
}

// live_pipeline.cpp is my full system MVP
// Starting point for running the whole pipeline

int main(int argc, char** argv) {
  const std::string cfg_path = (argc > 1) ? argv[1] : "configs/dev.yaml";

  try {
    dcp::AppConfig cfg = dcp::LoadConfigFromYamlFile(cfg_path);
    std::cout << "Loaded config OK: " << cfg_path << "\n";

    std::signal(SIGINT, HandleSigint);
    const auto start = std::chrono::steady_clock::now();
    const auto max_runtime = std::chrono::seconds(500); // test value

    dcp::StopSource global_stop;

    // Actual pipeline logic starts here

    // Begin by creating all resources (queues/lateststores/metrics/etc.) needed
    auto camera_to_preprocess_queue = std::make_shared<dcp::BoundedQueue<dcp::Frame>>(cfg.buffering.queues.camera_to_preprocess.capacity, cfg.buffering.queues.camera_to_preprocess.drop_policy);
    auto preprocess_to_tracking_queue = std::make_shared<dcp::BoundedQueue<dcp::Frame>>(cfg.buffering.queues.preprocess_to_tracking.capacity, cfg.buffering.queues.preprocess_to_tracking.drop_policy);
    auto preprocessed_latest_store = std::make_shared<dcp::LatestStore<dcp::PreprocessedFrame>>();
    auto detections_latest_store = std::make_shared<dcp::LatestStore<dcp::Detections>>();
    auto tracking_to_visualization_queue = std::make_shared<dcp::BoundedQueue<dcp::RenderFrame>>(cfg.buffering.queues.tracking_to_visualization.capacity, cfg.buffering.queues.tracking_to_visualization.drop_policy);

    // Create the stages and pass references of resources to appropriate stages
    dcp::CameraStage camera_stage(cfg.camera, camera_to_preprocess_queue);
    dcp::PreprocessStage preprocess_stage(cfg.preprocess, camera_to_preprocess_queue, preprocess_to_tracking_queue, preprocessed_latest_store);
    dcp::InferenceStage inference_stage(cfg.inference, preprocessed_latest_store, detections_latest_store);
    dcp::TrackingStage tracking_stage(cfg.tracking, preprocess_to_tracking_queue, detections_latest_store, tracking_to_visualization_queue);

    // Start each stage, consumers first. The stage will then handle its own looping/thread logic
    tracking_stage.start(global_stop.token());
    inference_stage.start(global_stop.token());
    preprocess_stage.start(global_stop.token());
    camera_stage.start(global_stop.token());

    //UI (must be on main thread on MacOS)
    cv::namedWindow(cfg.visualization.window_name, cv::WINDOW_AUTOSIZE);
    dcp::RenderFrame latest;
    bool have_latest = false;

    // Run pipeline, exit on command or time limit
    while (!global_stop.stop_requested()) {
      if (g_sigint.load(std::memory_order_relaxed)) {
        std::cout << "\nShutting down pipeline..." << std::endl;
        global_stop.request_stop();
        break;
      }

      auto now = std::chrono::steady_clock::now();
      if (now - start >= max_runtime) {
        std::cout << "Pipeline time limit exceeded. Shutting down pipeline..." << std::endl;
        global_stop.request_stop();
        break;
      }

      dcp::RenderFrame rf;
      if (tracking_to_visualization_queue->try_pop_for(rf, std::chrono::milliseconds(5))) {
        if (!rf.frame.image.empty()) {
          latest = std::move(rf);
          have_latest = true;
        }
      }

      if (have_latest) {
        cv::imshow(cfg.visualization.window_name, latest.frame.image);
      }

      int key = cv::waitKey(1);
      if (key == 'q' || key == 27) {
        std::cout << "User exited. Shutting down pipeline..." << std::endl;
        global_stop.request_stop();
        break;
      }
    }

    // Close UI window
    cv::destroyWindow(cfg.visualization.window_name);

    // Stop all stages, producers first
    camera_stage.stop();
    preprocess_stage.stop();
    inference_stage.stop();
    tracking_stage.stop();

  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }

  return 0;
}
