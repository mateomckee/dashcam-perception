#include "core/config_loader.hpp"

#include <yaml-cpp/yaml.h>

#include <sstream>
#include <stdexcept>

namespace dcp {

static std::string PathJoin(const std::string& a, const std::string& b) {
  if (a.empty()) return b;
  if (!a.empty() && a.back() == '.') return a + b;
  return a + "." + b;
}

static std::runtime_error ConfigError(const std::string& key_path, const std::string& msg) {
  std::ostringstream oss;
  oss << "Config error at '" << key_path << "': " << msg;
  return std::runtime_error(oss.str());
}

static YAML::Node Child(const YAML::Node& parent, const char* key) {
  if (!parent || !parent.IsMap()) return YAML::Node();
  return parent[key];
}

template <typename T>
static T GetOrKey(const YAML::Node& parent, const char* key, const std::string& key_path, const T& fallback) {
  const YAML::Node n = Child(parent, key);
  if (!n) return fallback;
  try {
    return n.as<T>(fallback);
  } catch (const YAML::Exception& e) {
    throw ConfigError(key_path, e.what());
  }
}

static DropPolicy ParseDropPolicyKey(const YAML::Node& parent, const char* key, const std::string& key_path, DropPolicy fallback) {
  const YAML::Node n = Child(parent, key);
  if (!n) return fallback;
  const std::string s = GetOrKey<std::string>(parent, key, key_path, "");
  if (s == "drop_oldest") return DropPolicy::DropOldest;
  if (s == "drop_newest") return DropPolicy::DropNewest;
  throw ConfigError(key_path, "unknown drop_policy '" + s + "'. Use: drop_oldest | drop_newest");
}

static void LoadQueueConfig(const YAML::Node& qnode, const std::string& key_path, QueueConfig& out) {
  if (!qnode) return;
  out.capacity = GetOrKey<std::size_t>(qnode, "capacity", PathJoin(key_path, "capacity"), out.capacity);
  out.drop_policy = ParseDropPolicyKey(qnode, "drop_policy", PathJoin(key_path, "drop_policy"), out.drop_policy);
}

static void LoadCamera(const YAML::Node& root, CameraConfig& cfg) {
  const YAML::Node cam = root["camera"];
  if (!cam) return;
  const std::string p = "camera";

  cfg.backend = GetOrKey<std::string>(cam, "backend", PathJoin(p, "backend"), cfg.backend);
  cfg.device_index = GetOrKey<int>(cam, "device_index", PathJoin(p, "device_index"), cfg.device_index);
  cfg.width = GetOrKey<int>(cam, "width", PathJoin(p, "width"), cfg.width);
  cfg.height = GetOrKey<int>(cam, "height", PathJoin(p, "height"), cfg.height);
  cfg.fps = GetOrKey<int>(cam, "fps", PathJoin(p, "fps"), cfg.fps);
  cfg.flip_vertical = GetOrKey<bool>(cam, "flip_vertical", PathJoin(p, "flip_vertical"), cfg.flip_vertical);
  cfg.flip_horizontal = GetOrKey<bool>(cam, "flip_horizontal", PathJoin(p, "flip_horizontal"), cfg.flip_horizontal);
}

static void LoadPreprocess(const YAML::Node& root, PreprocessConfig& cfg) {
  const YAML::Node pre = root["preprocess"];
  if (!pre) return;
  const std::string p = "preprocess";

  cfg.resize_width = GetOrKey<int>(pre, "resize_width", PathJoin(p, "resize_width"), cfg.resize_width);
  cfg.resize_height = GetOrKey<int>(pre, "resize_height", PathJoin(p, "resize_height"), cfg.resize_height);

  const YAML::Node roi = pre["crop_roi"];
  const std::string rp = PathJoin(p, "crop_roi");
  if (roi) {
    cfg.crop_roi.enabled = GetOrKey<bool>(roi, "enabled", PathJoin(rp, "enabled"), cfg.crop_roi.enabled);
    cfg.crop_roi.x = GetOrKey<int>(roi, "x", PathJoin(rp, "x"), cfg.crop_roi.x);
    cfg.crop_roi.y = GetOrKey<int>(roi, "y", PathJoin(rp, "y"), cfg.crop_roi.y);
    cfg.crop_roi.width = GetOrKey<int>(roi, "width", PathJoin(rp, "width"), cfg.crop_roi.width);
    cfg.crop_roi.height = GetOrKey<int>(roi, "height", PathJoin(rp, "height"), cfg.crop_roi.height);
  }
}

static void LoadBuffering(const YAML::Node& root, BufferingConfig& cfg) {
  const YAML::Node buf = root["buffering"];
  if (!buf) return;
  const std::string p = "buffering";

  const YAML::Node qs = buf["queues"];
  if (qs) {
    LoadQueueConfig(qs["camera_to_preprocess"],
                    PathJoin(PathJoin(p, "queues"), "camera_to_preprocess"),
                    cfg.queues.camera_to_preprocess);

    LoadQueueConfig(qs["preprocess_to_tracking"],
                    PathJoin(PathJoin(p, "queues"), "preprocess_to_tracking"),
                    cfg.queues.preprocess_to_tracking);
  }

  const YAML::Node ls = buf["latest_stores"];
  if (ls) {
    const std::string lp = PathJoin(p, "latest_stores");
    cfg.latest_stores.inference_frame = GetOrKey<bool>(ls, "inference_frame", PathJoin(lp, "inference_frame"), cfg.latest_stores.inference_frame);
    cfg.latest_stores.inference_detections = GetOrKey<bool>(ls, "inference_detections", PathJoin(lp, "inference_detections"), cfg.latest_stores.inference_detections);
    cfg.latest_stores.world_state = GetOrKey<bool>(ls, "world_state", PathJoin(lp, "world_state"), cfg.latest_stores.world_state);
  }
}

static void LoadInference(const YAML::Node& root, InferenceConfig& cfg) {
  const YAML::Node inf = root["inference"];
  if (!inf) return;
  const std::string p = "inference";

  cfg.enabled = GetOrKey<bool>(inf, "enabled", PathJoin(p, "enabled"), cfg.enabled);
  cfg.backend = GetOrKey<std::string>(inf, "backend", PathJoin(p, "backend"), cfg.backend);
  cfg.target_fps = GetOrKey<int>(inf, "target_fps", PathJoin(p, "target_fps"), cfg.target_fps);
  cfg.confidence_threshold = GetOrKey<float>(inf, "confidence_threshold", PathJoin(p, "confidence_threshold"), cfg.confidence_threshold);

  const YAML::Node model = inf["model"];
  const std::string mp = PathJoin(p, "model");
  if (model) {
    cfg.model.path = GetOrKey<std::string>(model, "path", PathJoin(mp, "path"), cfg.model.path);
    cfg.model.input_width = GetOrKey<int>(model, "input_width", PathJoin(mp, "input_width"), cfg.model.input_width);
    cfg.model.input_height = GetOrKey<int>(model, "input_height", PathJoin(mp, "input_height"), cfg.model.input_height);
  }
}

static void LoadTracking(const YAML::Node& root, TrackingConfig& cfg) {
  const YAML::Node tr = root["tracking"];
  if (!tr) return;
  const std::string p = "tracking";

  cfg.backend = GetOrKey<std::string>(tr, "backend", PathJoin(p, "backend"), cfg.backend);
  cfg.iou_threshold = GetOrKey<float>(tr, "iou_threshold", PathJoin(p, "iou_threshold"), cfg.iou_threshold);
  cfg.max_missed_frames = GetOrKey<int>(tr, "max_missed_frames", PathJoin(p, "max_missed_frames"), cfg.max_missed_frames);
  cfg.min_confirmed_frames = GetOrKey<int>(tr, "min_confirmed_frames", PathJoin(p, "min_confirmed_frames"), cfg.min_confirmed_frames);
}

static void LoadVisualization(const YAML::Node& root, VisualizationConfig& cfg) {
  const YAML::Node viz = root["visualization"];
  if (!viz) return;
  const std::string p = "visualization";

  cfg.enabled = GetOrKey<bool>(viz, "enabled", PathJoin(p, "enabled"), cfg.enabled);
  cfg.window_name = GetOrKey<std::string>(viz, "window_name", PathJoin(p, "window_name"), cfg.window_name);

  cfg.show_boxes = GetOrKey<bool>(viz, "show_boxes", PathJoin(p, "show_boxes"), cfg.show_boxes);
  cfg.show_track_ids = GetOrKey<bool>(viz, "show_track_ids", PathJoin(p, "show_track_ids"), cfg.show_track_ids);
  cfg.show_confidence = GetOrKey<bool>(viz, "show_confidence", PathJoin(p, "show_confidence"), cfg.show_confidence);

  cfg.show_hud = GetOrKey<bool>(viz, "show_hud", PathJoin(p, "show_hud"), cfg.show_hud);
  cfg.show_fps = GetOrKey<bool>(viz, "show_fps", PathJoin(p, "show_fps"), cfg.show_fps);
  cfg.show_latency = GetOrKey<bool>(viz, "show_latency", PathJoin(p, "show_latency"), cfg.show_latency);

  const YAML::Node rec = viz["recording"];
  const std::string rp = PathJoin(p, "recording");
  if (rec) {
    cfg.recording.enabled = GetOrKey<bool>(rec, "enabled", PathJoin(rp, "enabled"), cfg.recording.enabled);
    cfg.recording.output_path = GetOrKey<std::string>(rec, "output_path", PathJoin(rp, "output_path"), cfg.recording.output_path);
    cfg.recording.fps = GetOrKey<int>(rec, "fps", PathJoin(rp, "fps"), cfg.recording.fps);
  }
}

static void LoadMetrics(const YAML::Node& root, MetricsConfig& cfg) {
  const YAML::Node m = root["metrics"];
  if (!m) return;
  const std::string p = "metrics";

  cfg.enable_console_log = GetOrKey<bool>(m, "enable_console_log", PathJoin(p, "enable_console_log"), cfg.enable_console_log);
  cfg.log_interval_ms = GetOrKey<int>(m, "log_interval_ms", PathJoin(p, "log_interval_ms"), cfg.log_interval_ms);

  const YAML::Node csv = m["record_csv"];
  const std::string cp = PathJoin(p, "record_csv");
  if (csv) {
    cfg.record_csv.enabled = GetOrKey<bool>(csv, "enabled", PathJoin(cp, "enabled"), cfg.record_csv.enabled);
    cfg.record_csv.output_path = GetOrKey<std::string>(csv, "output_path", PathJoin(cp, "output_path"), cfg.record_csv.output_path);
  }
}

void ValidateOrThrow(const AppConfig& cfg) {
  if (cfg.camera.width <= 0 || cfg.camera.height <= 0) throw ConfigError("camera", "width/height must be > 0");
  if (cfg.camera.fps <= 0) throw ConfigError("camera.fps", "must be > 0");

  if (cfg.preprocess.resize_width <= 0 || cfg.preprocess.resize_height <= 0)
    throw ConfigError("preprocess", "resize_width/resize_height must be > 0");

  if (cfg.preprocess.crop_roi.enabled) {
    const auto& r = cfg.preprocess.crop_roi;
    if (r.width <= 0 || r.height <= 0) throw ConfigError("preprocess.crop_roi", "width/height must be > 0 when enabled");
    if (r.x < 0 || r.y < 0) throw ConfigError("preprocess.crop_roi", "x/y must be >= 0");
  }

  if (cfg.buffering.queues.camera_to_preprocess.capacity < 1)
    throw ConfigError("buffering.queues.camera_to_preprocess.capacity", "must be >= 1");
  if (cfg.buffering.queues.preprocess_to_tracking.capacity < 1)
    throw ConfigError("buffering.queues.preprocess_to_tracking.capacity", "must be >= 1");

  if (cfg.inference.enabled) {
    if (cfg.inference.target_fps <= 0)
      throw ConfigError("inference.target_fps", "must be > 0 when inference.enabled=true");
    if (cfg.inference.confidence_threshold < 0.f || cfg.inference.confidence_threshold > 1.f)
      throw ConfigError("inference.confidence_threshold", "must be in [0, 1]");
    if (cfg.inference.backend != "dummy" && cfg.inference.model.path.empty())
      throw ConfigError("inference.model.path", "required when inference.backend != 'dummy'");
  }

  if (cfg.tracking.iou_threshold < 0.f || cfg.tracking.iou_threshold > 1.f)
    throw ConfigError("tracking.iou_threshold", "must be in [0, 1]");
  if (cfg.tracking.max_missed_frames < 0) throw ConfigError("tracking.max_missed_frames", "must be >= 0");
  if (cfg.tracking.min_confirmed_frames < 1) throw ConfigError("tracking.min_confirmed_frames", "must be >= 1");

  if (cfg.visualization.recording.enabled && cfg.visualization.recording.fps <= 0)
    throw ConfigError("visualization.recording.fps", "must be > 0 when recording enabled");

  if (cfg.metrics.log_interval_ms <= 0) throw ConfigError("metrics.log_interval_ms", "must be > 0");
}

AppConfig LoadConfigFromYamlFile(const std::string& path) {
  AppConfig cfg;
  YAML::Node root;

  try {
    root = YAML::LoadFile(path);
  } catch (const YAML::Exception& e) {
    throw std::runtime_error(std::string("Failed to load YAML file '") + path + "': " + e.what());
  }

  LoadCamera(root, cfg.camera);
  LoadPreprocess(root, cfg.preprocess);
  LoadBuffering(root, cfg.buffering);
  LoadInference(root, cfg.inference);
  LoadTracking(root, cfg.tracking);
  LoadVisualization(root, cfg.visualization);
  LoadMetrics(root, cfg.metrics);

  ValidateOrThrow(cfg);
  return cfg;
}

} // namespace dcp
