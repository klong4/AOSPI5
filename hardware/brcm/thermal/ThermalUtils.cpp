// Copyright (C) 2024 The Android Open Source Project
// Thermal Utilities for Raspberry Pi 5

#include "ThermalUtils.h"
#include <android-base/logging.h>
#include <android-base/file.h>
#include <android-base/strings.h>
#include <json/json.h>
#include <fstream>

namespace android {
namespace hardware {
namespace thermal {
namespace V2_0 {
namespace implementation {

static const char* THERMAL_CONFIG_PATH = "/vendor/etc/thermal_info_config.json";

ThermalUtils::ThermalUtils() {
    loadConfig();
}

bool ThermalUtils::loadConfig() {
    std::ifstream file(THERMAL_CONFIG_PATH);
    if (!file.is_open()) {
        LOG(WARNING) << "Cannot open thermal config: " << THERMAL_CONFIG_PATH;
        return false;
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;

    if (!Json::parseFromStream(builder, file, &root, &errors)) {
        LOG(ERROR) << "Failed to parse thermal config: " << errors;
        return false;
    }

    // Parse sensors
    if (root.isMember("sensors")) {
        for (const auto& sensor : root["sensors"]) {
            ThermalSensorConfig config;
            config.name = sensor.get("name", "").asString();
            config.type = sensor.get("type", "").asString();
            config.sysfsPath = sensor.get("sysfs_path", "").asString();
            config.multiplier = sensor.get("multiplier", 1.0).asFloat();
            config.hotThreshold = sensor.get("hot_threshold", 80.0).asFloat();
            config.criticalThreshold = sensor.get("critical_threshold", 95.0).asFloat();
            mSensorConfigs[config.name] = config;
        }
    }

    // Parse cooling devices
    if (root.isMember("cooling_devices")) {
        for (const auto& device : root["cooling_devices"]) {
            CoolingDeviceConfig config;
            config.name = device.get("name", "").asString();
            config.type = device.get("type", "").asString();
            config.sysfsPath = device.get("sysfs_path", "").asString();
            config.maxState = device.get("max_state", 255).asUInt();
            mCoolingConfigs[config.name] = config;
        }
    }

    LOG(INFO) << "Loaded " << mSensorConfigs.size() << " sensors, "
              << mCoolingConfigs.size() << " cooling devices";
    return true;
}

float ThermalUtils::readTemperature(const std::string& name) {
    auto it = mSensorConfigs.find(name);
    if (it == mSensorConfigs.end()) {
        LOG(WARNING) << "Unknown sensor: " << name;
        return 0.0f;
    }

    std::string temp;
    if (!android::base::ReadFileToString(it->second.sysfsPath, &temp)) {
        LOG(WARNING) << "Failed to read " << it->second.sysfsPath;
        return 0.0f;
    }

    return std::stof(android::base::Trim(temp)) * it->second.multiplier;
}

bool ThermalUtils::setCoolingLevel(const std::string& name, uint32_t level) {
    auto it = mCoolingConfigs.find(name);
    if (it == mCoolingConfigs.end()) {
        LOG(WARNING) << "Unknown cooling device: " << name;
        return false;
    }

    if (level > it->second.maxState) {
        level = it->second.maxState;
    }

    return android::base::WriteStringToFile(std::to_string(level), it->second.sysfsPath);
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace thermal
}  // namespace hardware
}  // namespace android
