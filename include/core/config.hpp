#pragma once
#include <cstddef>
#include <string>

namespace dcp {

enum class DropPolicy {
  DropOldest,
  DropNewest
};

struct RoiConfig {
  bool enabled = false;
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

struct QueueConfig {
  std::size_t capacity = 4;
  DropPolicy drop_policy = DropPolicy::DropOldest;
};

struct CameraConfig {
  std::string backend = "opencv";
  int device_index = 0;

  int width = 1280;
  int height = 720;
  int fps = 30;

  bool flip_vertical = false;
  bool flip_horizontal = false;
};

struct PreprocessConfig {
  int resize_width = 640;
  int resize_height = 360;
  RoiConfig crop_roi{};
};

struct LatestStoresConfig {
  bool inference_frame = true;
  bool inference_detections = true;
  bool world_state = true;
};

struct QueuesConfig {
  QueueConfig camera_to_preprocess{};
  QueueConfig preprocess_to_tracking{};
};

struct BufferingConfig {
  QueuesConfig queues{};
  LatestStoresConfig latest_stores{};
};

struct ModelConfig {
  std::string path = "";
  int input_width = 640;
  int input_height = 360;
};

struct InferenceConfig {
  bool enabled = true;
  std::string backend = "dummy"; // dummy | onnx | tensorrt (later)
  int target_fps = 10;
  float confidence_threshold = 0.5f;
  ModelConfig model{};
};

struct TrackingConfig {
  std::string backend = "iou"; // iou | kalman (later)
  float iou_threshold = 0.3f;
  int max_missed_frames = 5;
  int min_confirmed_frames = 3;
};

struct RecordingConfig {
  bool enabled = false;
  std::string output_path = "output/run.mp4";
  int fps = 30;
};

struct VisualizationConfig {
  bool enabled = true;
  std::string window_name = "Dashcam Perception";

  bool show_boxes = true;
  bool show_track_ids = true;
  bool show_confidence = true;

  bool show_hud = true;
  bool show_fps = true;
  bool show_latency = true;

  RecordingConfig recording{};
};

struct CsvMetricsConfig {
  bool enabled = false;
  std::string output_path = "logs/metrics.csv";
};

struct MetricsConfig {
  bool enable_console_log = true;
  int log_interval_ms = 1000;
  CsvMetricsConfig record_csv{};
};

struct AppConfig {
  CameraConfig camera{};
  PreprocessConfig preprocess{};
  BufferingConfig buffering{};
  InferenceConfig inference{};
  TrackingConfig tracking{};
  VisualizationConfig visualization{};
  MetricsConfig metrics{};
};

}
