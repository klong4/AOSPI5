/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Light HAL AIDL Implementation for Raspberry Pi 5
 * Android 16 compatible
 */

#define LOG_TAG "android.hardware.light-service.rpi5"

#include "Lights.h"

#include <android-base/logging.h>
#include <android-base/file.h>

namespace aidl {
namespace android {
namespace hardware {
namespace light {
namespace impl {
namespace rpi5 {

static constexpr const char* kPwrLedPath = "/sys/class/leds/PWR/brightness";
static constexpr const char* kActLedPath = "/sys/class/leds/ACT/brightness";
static constexpr const char* kBacklightPath = "/sys/class/backlight/rpi_backlight/brightness";

Lights::Lights() {
    LOG(INFO) << "Raspberry Pi 5 Light HAL AIDL initialized";
}

ndk::ScopedAStatus Lights::setLightState(int32_t id, const HwLightState& state) {
    uint8_t brightness = static_cast<uint8_t>(
        ((state.color >> 16) & 0xFF) * 0.299f +
        ((state.color >> 8) & 0xFF) * 0.587f +
        (state.color & 0xFF) * 0.114f
    );
    
    switch (id) {
        case static_cast<int32_t>(LightType::BACKLIGHT):
            ::android::base::WriteStringToFile(
                std::to_string(brightness), kBacklightPath);
            break;
            
        case static_cast<int32_t>(LightType::NOTIFICATIONS):
            ::android::base::WriteStringToFile(
                brightness > 0 ? "255" : "0", kActLedPath);
            break;
            
        case static_cast<int32_t>(LightType::BATTERY):
            ::android::base::WriteStringToFile(
                brightness > 0 ? "255" : "0", kPwrLedPath);
            break;
            
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* _aidl_return) {
    HwLight backlight;
    backlight.id = static_cast<int32_t>(LightType::BACKLIGHT);
    backlight.type = LightType::BACKLIGHT;
    backlight.ordinal = 0;
    _aidl_return->push_back(backlight);
    
    HwLight notification;
    notification.id = static_cast<int32_t>(LightType::NOTIFICATIONS);
    notification.type = LightType::NOTIFICATIONS;
    notification.ordinal = 0;
    _aidl_return->push_back(notification);
    
    HwLight battery;
    battery.id = static_cast<int32_t>(LightType::BATTERY);
    battery.type = LightType::BATTERY;
    battery.ordinal = 0;
    _aidl_return->push_back(battery);
    
    return ndk::ScopedAStatus::ok();
}

}  // namespace rpi5
}  // namespace impl
}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
