/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Power HAL AIDL Implementation for Raspberry Pi 5
 * Android 16 compatible
 */

#define LOG_TAG "android.hardware.power-service.rpi5"

#include "Power.h"

#include <android-base/logging.h>
#include <android-base/file.h>
#include <android-base/strings.h>

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace impl {
namespace rpi5 {

using ::aidl::android::hardware::power::Mode;
using ::aidl::android::hardware::power::Boost;

static constexpr const char* kCpuGovernorPath = 
    "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
static constexpr const char* kCpuMaxFreqPath = 
    "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq";
static constexpr const char* kCpuMinFreqPath = 
    "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq";

Power::Power() {
    LOG(INFO) << "Raspberry Pi 5 Power HAL AIDL initialized";
}

ndk::ScopedAStatus Power::setMode(Mode type, bool enabled) {
    LOG(DEBUG) << "setMode: " << static_cast<int>(type) << " enabled: " << enabled;
    
    switch (type) {
        case Mode::LOW_POWER:
            if (enabled) {
                ::android::base::WriteStringToFile("powersave", kCpuGovernorPath);
                ::android::base::WriteStringToFile("1500000", kCpuMaxFreqPath);
            } else {
                ::android::base::WriteStringToFile("schedutil", kCpuGovernorPath);
                ::android::base::WriteStringToFile("2400000", kCpuMaxFreqPath);
            }
            break;
            
        case Mode::SUSTAINED_PERFORMANCE:
            if (enabled) {
                ::android::base::WriteStringToFile("performance", kCpuGovernorPath);
                ::android::base::WriteStringToFile("2000000", kCpuMaxFreqPath);
                ::android::base::WriteStringToFile("2000000", kCpuMinFreqPath);
            } else {
                ::android::base::WriteStringToFile("schedutil", kCpuGovernorPath);
                ::android::base::WriteStringToFile("1500000", kCpuMinFreqPath);
            }
            break;
            
        case Mode::LAUNCH:
        case Mode::INTERACTIVE:
            if (enabled) {
                ::android::base::WriteStringToFile("schedutil", kCpuGovernorPath);
                ::android::base::WriteStringToFile("2400000", kCpuMaxFreqPath);
            }
            break;
            
        case Mode::DEVICE_IDLE:
            if (enabled) {
                ::android::base::WriteStringToFile("powersave", kCpuGovernorPath);
                ::android::base::WriteStringToFile("1000000", kCpuMaxFreqPath);
            } else {
                ::android::base::WriteStringToFile("schedutil", kCpuGovernorPath);
                ::android::base::WriteStringToFile("2400000", kCpuMaxFreqPath);
            }
            break;
            
        case Mode::DISPLAY_INACTIVE:
        case Mode::AUDIO_STREAMING_LOW_LATENCY:
        case Mode::CAMERA_STREAMING_SECURE:
        case Mode::CAMERA_STREAMING_LOW:
        case Mode::CAMERA_STREAMING_MID:
        case Mode::CAMERA_STREAMING_HIGH:
        case Mode::VR:
        case Mode::EXPENSIVE_RENDERING:
        case Mode::FIXED_PERFORMANCE:
        case Mode::GAME:
        case Mode::GAME_LOADING:
            // Handle other modes as needed
            break;
            
        default:
            break;
    }
    
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isModeSupported(Mode type, bool* _aidl_return) {
    switch (type) {
        case Mode::LOW_POWER:
        case Mode::SUSTAINED_PERFORMANCE:
        case Mode::LAUNCH:
        case Mode::INTERACTIVE:
        case Mode::DEVICE_IDLE:
            *_aidl_return = true;
            break;
        default:
            *_aidl_return = false;
            break;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::setBoost(Boost type, int32_t durationMs) {
    LOG(DEBUG) << "setBoost: " << static_cast<int>(type) << " duration: " << durationMs;
    
    switch (type) {
        case Boost::INTERACTION:
            // Short burst to max frequency
            ::android::base::WriteStringToFile("performance", kCpuGovernorPath);
            // Would need a timer to restore - simplified for now
            break;
            
        case Boost::DISPLAY_UPDATE_IMMINENT:
        case Boost::ML_ACC:
        case Boost::AUDIO_LAUNCH:
        case Boost::CAMERA_LAUNCH:
        case Boost::CAMERA_SHOT:
            // Handle other boosts as needed
            break;
            
        default:
            break;
    }
    
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isBoostSupported(Boost type, bool* _aidl_return) {
    switch (type) {
        case Boost::INTERACTION:
            *_aidl_return = true;
            break;
        default:
            *_aidl_return = false;
            break;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::createHintSession(
        int32_t /*tgid*/, int32_t /*uid*/,
        const std::vector<int32_t>& /*threadIds*/,
        int64_t /*durationNanos*/,
        std::shared_ptr<IPowerHintSession>* _aidl_return) {
    *_aidl_return = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::getHintSessionPreferredRate(int64_t* outNanoseconds) {
    *outNanoseconds = -1;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::createHintSessionWithConfig(
        int32_t /*tgid*/, int32_t /*uid*/,
        const std::vector<int32_t>& /*threadIds*/,
        int64_t /*durationNanos*/,
        SessionTag /*tag*/,
        SessionConfig* /*config*/,
        std::shared_ptr<IPowerHintSession>* _aidl_return) {
    *_aidl_return = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::getSessionChannel(
        int32_t /*tgid*/, int32_t /*uid*/,
        ChannelConfig* /*config*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::closeSessionChannel(int32_t /*tgid*/, int32_t /*uid*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace rpi5
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl
