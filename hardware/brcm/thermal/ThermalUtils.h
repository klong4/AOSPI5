// Copyright (C) 2024 The Android Open Source Project

#pragma once

#include <string>
#include <map>
#include <cstdint>

namespace android {
namespace hardware {
namespace thermal {
namespace V2_0 {
namespace implementation {

struct ThermalSensorConfig {
    std::string name;
    std::string type;
    std::string sysfsPath;
    float multiplier;
    float hotThreshold;
    float criticalThreshold;
};

struct CoolingDeviceConfig {
    std::string name;
    std::string type;
    std::string sysfsPath;
    uint32_t maxState;
};

class ThermalUtils {
public:
    ThermalUtils();

    bool loadConfig();
    float readTemperature(const std::string& name);
    bool setCoolingLevel(const std::string& name, uint32_t level);

    const std::map<std::string, ThermalSensorConfig>& getSensorConfigs() const {
        return mSensorConfigs;
    }

    const std::map<std::string, CoolingDeviceConfig>& getCoolingConfigs() const {
        return mCoolingConfigs;
    }

private:
    std::map<std::string, ThermalSensorConfig> mSensorConfigs;
    std::map<std::string, CoolingDeviceConfig> mCoolingConfigs;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace thermal
}  // namespace hardware
}  // namespace android
