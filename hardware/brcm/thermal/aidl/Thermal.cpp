/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Thermal HAL AIDL Implementation for Raspberry Pi 5
 * Android 16 compatible
 */

#define LOG_TAG "android.hardware.thermal-service.rpi5"

#include "Thermal.h"

#include <android-base/logging.h>
#include <android-base/file.h>
#include <android-base/strings.h>

namespace aidl {
namespace android {
namespace hardware {
namespace thermal {
namespace impl {
namespace rpi5 {

using ::aidl::android::hardware::thermal::CoolingDevice;
using ::aidl::android::hardware::thermal::Temperature;
using ::aidl::android::hardware::thermal::TemperatureType;
using ::aidl::android::hardware::thermal::ThrottlingSeverity;

static constexpr const char* kCpuTempPath = "/sys/class/thermal/thermal_zone0/temp";
static constexpr const char* kGpuTempPath = "/sys/class/thermal/thermal_zone1/temp";

static float readTemperature(const char* path) {
    std::string temp_str;
    if (::android::base::ReadFileToString(path, &temp_str)) {
        return std::stof(::android::base::Trim(temp_str)) / 1000.0f;
    }
    return 0.0f;
}

static ThrottlingSeverity getSeverity(float temp) {
    if (temp >= 85.0f) return ThrottlingSeverity::SHUTDOWN;
    if (temp >= 80.0f) return ThrottlingSeverity::CRITICAL;
    if (temp >= 75.0f) return ThrottlingSeverity::SEVERE;
    if (temp >= 70.0f) return ThrottlingSeverity::MODERATE;
    if (temp >= 65.0f) return ThrottlingSeverity::LIGHT;
    return ThrottlingSeverity::NONE;
}

Thermal::Thermal() {
    LOG(INFO) << "Raspberry Pi 5 Thermal HAL AIDL initialized";
}

ndk::ScopedAStatus Thermal::getTemperatures(std::vector<Temperature>* _aidl_return) {
    Temperature cpuTemp;
    cpuTemp.type = TemperatureType::CPU;
    cpuTemp.name = "CPU";
    cpuTemp.value = readTemperature(kCpuTempPath);
    cpuTemp.throttlingStatus = getSeverity(cpuTemp.value);
    _aidl_return->push_back(cpuTemp);

    Temperature gpuTemp;
    gpuTemp.type = TemperatureType::GPU;
    gpuTemp.name = "GPU";
    gpuTemp.value = readTemperature(kGpuTempPath);
    gpuTemp.throttlingStatus = getSeverity(gpuTemp.value);
    _aidl_return->push_back(gpuTemp);

    Temperature skinTemp;
    skinTemp.type = TemperatureType::SKIN;
    skinTemp.name = "SoC";
    skinTemp.value = readTemperature(kCpuTempPath);  // Use CPU temp as proxy
    skinTemp.throttlingStatus = getSeverity(skinTemp.value);
    _aidl_return->push_back(skinTemp);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Thermal::getTemperaturesWithType(
        TemperatureType type, std::vector<Temperature>* _aidl_return) {
    std::vector<Temperature> allTemps;
    getTemperatures(&allTemps);
    
    for (const auto& temp : allTemps) {
        if (temp.type == type) {
            _aidl_return->push_back(temp);
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Thermal::getCoolingDevices(std::vector<CoolingDevice>* _aidl_return) {
    CoolingDevice fan;
    fan.type = CoolingType::FAN;
    fan.name = "CPU Fan";
    fan.value = 0;  // Would read actual fan speed if available
    _aidl_return->push_back(fan);
    
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Thermal::getCoolingDevicesWithType(
        CoolingType type, std::vector<CoolingDevice>* _aidl_return) {
    std::vector<CoolingDevice> allDevices;
    getCoolingDevices(&allDevices);
    
    for (const auto& device : allDevices) {
        if (device.type == type) {
            _aidl_return->push_back(device);
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Thermal::getTemperatureThresholds(
        std::vector<TemperatureThreshold>* _aidl_return) {
    TemperatureThreshold cpuThreshold;
    cpuThreshold.type = TemperatureType::CPU;
    cpuThreshold.name = "CPU";
    cpuThreshold.hotThrottlingThresholds = {0.0f, 0.0f, 65.0f, 70.0f, 75.0f, 80.0f, 85.0f};
    cpuThreshold.coldThrottlingThresholds = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    _aidl_return->push_back(cpuThreshold);
    
    TemperatureThreshold gpuThreshold;
    gpuThreshold.type = TemperatureType::GPU;
    gpuThreshold.name = "GPU";
    gpuThreshold.hotThrottlingThresholds = {0.0f, 0.0f, 65.0f, 70.0f, 75.0f, 80.0f, 85.0f};
    gpuThreshold.coldThrottlingThresholds = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    _aidl_return->push_back(gpuThreshold);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Thermal::getTemperatureThresholdsWithType(
        TemperatureType type, std::vector<TemperatureThreshold>* _aidl_return) {
    std::vector<TemperatureThreshold> allThresholds;
    getTemperatureThresholds(&allThresholds);
    
    for (const auto& threshold : allThresholds) {
        if (threshold.type == type) {
            _aidl_return->push_back(threshold);
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Thermal::registerThermalChangedCallback(
        const std::shared_ptr<IThermalChangedCallback>& callback) {
    if (callback == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    
    std::lock_guard<std::mutex> lock(callback_mutex_);
    callbacks_.push_back(callback);
    
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Thermal::registerThermalChangedCallbackWithType(
        const std::shared_ptr<IThermalChangedCallback>& callback,
        TemperatureType /*type*/) {
    return registerThermalChangedCallback(callback);
}

ndk::ScopedAStatus Thermal::unregisterThermalChangedCallback(
        const std::shared_ptr<IThermalChangedCallback>& callback) {
    if (callback == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    
    std::lock_guard<std::mutex> lock(callback_mutex_);
    callbacks_.erase(
        std::remove_if(callbacks_.begin(), callbacks_.end(),
            [&](const std::shared_ptr<IThermalChangedCallback>& cb) {
                return cb->asBinder() == callback->asBinder();
            }),
        callbacks_.end());
    
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Thermal::registerCoolingDeviceChangedCallbackWithType(
        const std::shared_ptr<ICoolingDeviceChangedCallback>& /*callback*/,
        CoolingType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Thermal::unregisterCoolingDeviceChangedCallback(
        const std::shared_ptr<ICoolingDeviceChangedCallback>& /*callback*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Thermal::forecastSkinTemperature(
        int32_t /*forecastSeconds*/, float* _aidl_return) {
    *_aidl_return = readTemperature(kCpuTempPath);
    return ndk::ScopedAStatus::ok();
}

}  // namespace rpi5
}  // namespace impl
}  // namespace thermal
}  // namespace hardware
}  // namespace android
}  // namespace aidl
