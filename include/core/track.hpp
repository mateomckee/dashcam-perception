#pragma once

#include <cstdint>

namespace dcp {

struct BBoxF {
  float x{0.f};
  float y{0.f};
  float w{0.f};
  float h{0.f};
};

struct Track {
  std::uint64_t id{0};
  BBoxF bbox;
  int class_id{-1};
  float confidence{0.f};

  std::uint64_t last_update_frame_id{0};
  int age_frames{0};
  int missed_frames{0};
  bool confirmed{false};
};

} // namespace dcp