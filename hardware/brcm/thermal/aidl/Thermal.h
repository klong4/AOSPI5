/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Thermal HAL AIDL Header for Raspberry Pi 5
 */

#pragma once

#include <aidl/android/hardware/thermal/BnThermal.h>
#include <mutex>
#include <vector>

namespace aidl {
namespace android {
namespace hardware {
namespace thermal {
namespace impl {
namespace rpi5 {

class Thermal : public BnThermal {
  public:
    Thermal();
    
    ndk::ScopedAStatus getTemperatures(
            std::vector<Temperature>* _aidl_return) override;
    ndk::ScopedAStatus getTemperaturesWithType(
            TemperatureType type,
            std::vector<Temperature>* _aidl_return) override;
    ndk::ScopedAStatus getCoolingDevices(
            std::vector<CoolingDevice>* _aidl_return) override;
    ndk::ScopedAStatus getCoolingDevicesWithType(
            CoolingType type,
            std::vector<CoolingDevice>* _aidl_return) override;
    ndk::ScopedAStatus getTemperatureThresholds(
            std::vector<TemperatureThreshold>* _aidl_return) override;
    ndk::ScopedAStatus getTemperatureThresholdsWithType(
            TemperatureType type,
            std::vector<TemperatureThreshold>* _aidl_return) override;
    ndk::ScopedAStatus registerThermalChangedCallback(
            const std::shared_ptr<IThermalChangedCallback>& callback) override;
    ndk::ScopedAStatus registerThermalChangedCallbackWithType(
            const std::shared_ptr<IThermalChangedCallback>& callback,
            TemperatureType type) override;
    ndk::ScopedAStatus unregisterThermalChangedCallback(
            const std::shared_ptr<IThermalChangedCallback>& callback) override;
    ndk::ScopedAStatus registerCoolingDeviceChangedCallbackWithType(
            const std::shared_ptr<ICoolingDeviceChangedCallback>& callback,
            CoolingType type) override;
    ndk::ScopedAStatus unregisterCoolingDeviceChangedCallback(
            const std::shared_ptr<ICoolingDeviceChangedCallback>& callback) override;
    ndk::ScopedAStatus forecastSkinTemperature(
            int32_t forecastSeconds, float* _aidl_return) override;

  private:
    std::mutex callback_mutex_;
    std::vector<std::shared_ptr<IThermalChangedCallback>> callbacks_;
};

}  // namespace rpi5
}  // namespace impl
}  // namespace thermal
}  // namespace hardware
}  // namespace android
}  // namespace aidl
