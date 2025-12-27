#pragma once
#include <string>
#include "core/config.hpp"

namespace dcp {

// Loads YAML at 'path', applies defaults, validates, throws on error
AppConfig LoadConfigFromYamlFile(const std::string& path);

// Throws error if config is invalid
void ValidateOrThrow(const AppConfig& cfg);

}
