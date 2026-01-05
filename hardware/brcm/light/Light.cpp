// Copyright (C) 2024 The Android Open Source Project
// Light HAL implementation for Raspberry Pi 5

#define LOG_TAG "LightHAL"

#include <android-base/logging.h>
#include <android-base/file.h>
#include <android/hardware/light/2.0/ILight.h>
#include <hidl/Status.h>
#include <fstream>
#include "Light.h"

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

// LED paths on Raspberry Pi 5
static const char* LED_ACT_PATH = "/sys/class/leds/ACT/brightness";
static const char* LED_PWR_PATH = "/sys/class/leds/PWR/brightness";
static const char* BACKLIGHT_PATH = "/sys/class/backlight/*/brightness";
static const char* BACKLIGHT_MAX_PATH = "/sys/class/backlight/*/max_brightness";

Light::Light() {
    LOG(INFO) << "Light HAL initialized";

    // Check available LEDs
    mHasActivityLed = access(LED_ACT_PATH, W_OK) == 0;
    mHasPowerLed = access(LED_PWR_PATH, W_OK) == 0;

    // Find backlight device
    findBacklightDevice();
}

Light::~Light() {
    LOG(INFO) << "Light HAL destroyed";
}

void Light::findBacklightDevice() {
    // Look for DSI/HDMI backlight
    const char* backlightDirs[] = {
        "/sys/class/backlight/rpi_backlight",
        "/sys/class/backlight/backlight",
        "/sys/class/backlight/10-0045",
    };

    for (const char* dir : backlightDirs) {
        std::string brightnessPath = std::string(dir) + "/brightness";
        std::string maxBrightnessPath = std::string(dir) + "/max_brightness";

        if (access(brightnessPath.c_str(), W_OK) == 0) {
            mBacklightPath = brightnessPath;
            
            std::string maxStr;
            if (android::base::ReadFileToString(maxBrightnessPath, &maxStr)) {
                mMaxBacklight = std::stoi(maxStr);
            }
            
            LOG(INFO) << "Found backlight at " << dir << ", max=" << mMaxBacklight;
            break;
        }
    }
}

Return<Status> Light::setLight(Type type, const LightState& state) {
    switch (type) {
        case Type::BACKLIGHT:
            return setBacklight(state);
        case Type::NOTIFICATIONS:
            return setNotificationLight(state);
        case Type::ATTENTION:
            return setAttentionLight(state);
        case Type::BATTERY:
            return setBatteryLight(state);
        default:
            return Status::LIGHT_NOT_SUPPORTED;
    }
}

Return<void> Light::getSupportedTypes(getSupportedTypes_cb _hidl_cb) {
    std::vector<Type> types;

    if (!mBacklightPath.empty()) {
        types.push_back(Type::BACKLIGHT);
    }

    if (mHasActivityLed) {
        types.push_back(Type::NOTIFICATIONS);
        types.push_back(Type::ATTENTION);
    }

    if (mHasPowerLed) {
        types.push_back(Type::BATTERY);
    }

    _hidl_cb(types);
    return Void();
}

Status Light::setBacklight(const LightState& state) {
    if (mBacklightPath.empty()) {
        return Status::LIGHT_NOT_SUPPORTED;
    }

    // Extract brightness from ARGB color (use max of RGB)
    uint8_t brightness = ((state.color >> 16) & 0xFF);
    brightness = std::max(brightness, (uint8_t)((state.color >> 8) & 0xFF));
    brightness = std::max(brightness, (uint8_t)(state.color & 0xFF));

    // Scale to max backlight
    int scaledBrightness = (brightness * mMaxBacklight) / 255;

    if (!android::base::WriteStringToFile(std::to_string(scaledBrightness), mBacklightPath)) {
        LOG(ERROR) << "Failed to write backlight brightness";
        return Status::UNKNOWN;
    }

    return Status::SUCCESS;
}

Status Light::setNotificationLight(const LightState& state) {
    if (!mHasActivityLed) {
        return Status::LIGHT_NOT_SUPPORTED;
    }

    // Use activity LED for notifications
    bool on = (state.color & 0x00FFFFFF) != 0;

    // Handle flash mode
    if (state.flashMode == Flash::TIMED && on) {
        // For blinking, we'd need a separate thread or use LED trigger
        // For now, just turn on
    }

    if (!android::base::WriteStringToFile(on ? "1" : "0", LED_ACT_PATH)) {
        LOG(ERROR) << "Failed to write notification LED";
        return Status::UNKNOWN;
    }

    return Status::SUCCESS;
}

Status Light::setAttentionLight(const LightState& state) {
    // Use same as notification
    return setNotificationLight(state);
}

Status Light::setBatteryLight(const LightState& state) {
    if (!mHasPowerLed) {
        return Status::LIGHT_NOT_SUPPORTED;
    }

    // Power LED for battery status
    bool on = (state.color & 0x00FFFFFF) != 0;

    if (!android::base::WriteStringToFile(on ? "1" : "0", LED_PWR_PATH)) {
        LOG(ERROR) << "Failed to write battery LED";
        return Status::UNKNOWN;
    }

    return Status::SUCCESS;
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android
