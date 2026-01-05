/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "PowerHAL_RPi5"

#include <android-base/logging.h>
#include <android/hardware/power/1.3/IPower.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <fstream>
#include <string>
#include <mutex>

namespace android {
namespace hardware {
namespace power {
namespace V1_3 {
namespace implementation {

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::power::V1_0::Feature;
using ::android::hardware::power::V1_0::PowerHint;
using ::android::hardware::power::V1_0::Status;

// CPU frequency paths for Raspberry Pi 5
constexpr char CPU_FREQ_MAX[] = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq";
constexpr char CPU_FREQ_MIN[] = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq";
constexpr char CPU_FREQ_GOV[] = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
constexpr char CPU_FREQ_CUR[] = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq";

// GPU frequency paths
constexpr char GPU_FREQ_MAX[] = "/sys/class/devfreq/gpu/max_freq";
constexpr char GPU_FREQ_MIN[] = "/sys/class/devfreq/gpu/min_freq";
constexpr char GPU_FREQ_GOV[] = "/sys/class/devfreq/gpu/governor";

// Thermal throttling
constexpr char THERMAL_ZONE[] = "/sys/class/thermal/thermal_zone0/temp";

// BCM2712 specific frequencies (in kHz for CPU, Hz for GPU)
constexpr int CPU_FREQ_POWERSAVE = 600000;    // 600 MHz
constexpr int CPU_FREQ_BALANCED = 1500000;    // 1.5 GHz
constexpr int CPU_FREQ_PERFORMANCE = 2400000; // 2.4 GHz

constexpr int GPU_FREQ_POWERSAVE = 500000000;    // 500 MHz
constexpr int GPU_FREQ_PERFORMANCE = 800000000;  // 800 MHz

class Power : public IPower {
public:
    Power();
    ~Power() = default;

    // V1_0
    Return<void> setInteractive(bool interactive) override;
    Return<void> powerHint(PowerHint hint, int32_t data) override;
    Return<void> setFeature(Feature feature, bool activate) override;
    Return<void> getPlatformLowPowerStats(getPlatformLowPowerStats_cb _hidl_cb) override;

    // V1_1
    Return<void> getSubsystemLowPowerStats(getSubsystemLowPowerStats_cb _hidl_cb) override;
    Return<void> powerHintAsync(PowerHint hint, int32_t data) override;

    // V1_2
    Return<void> powerHintAsync_1_2(PowerHint_1_2 hint, int32_t data) override;

    // V1_3
    Return<void> powerHintAsync_1_3(PowerHint_1_3 hint, int32_t data) override;

private:
    void setPerformanceMode(bool enable);
    void setPowersaveMode(bool enable);
    void setBalancedMode();
    void handleSustainedPerformance(bool enable);
    void handleVrMode(bool enable);
    void handleLaunch(int32_t duration);
    void handleInteraction(int32_t duration);
    
    bool writeFile(const char* path, const std::string& value);
    std::string readFile(const char* path);
    int readInt(const char* path);

    std::mutex mMutex;
    bool mInteractive;
    bool mSustainedPerformance;
    bool mVrMode;
    int mCurrentProfile;  // 0=powersave, 1=balanced, 2=performance
};

Power::Power() : mInteractive(true), mSustainedPerformance(false), 
                 mVrMode(false), mCurrentProfile(1) {
    LOG(INFO) << "Power HAL initialized for Raspberry Pi 5";
    setBalancedMode();
}

bool Power::writeFile(const char* path, const std::string& value) {
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG(ERROR) << "Failed to open: " << path;
        return false;
    }
    file << value;
    return file.good();
}

std::string Power::readFile(const char* path) {
    std::ifstream file(path);
    std::string value;
    if (file.is_open()) {
        file >> value;
    }
    return value;
}

int Power::readInt(const char* path) {
    std::string value = readFile(path);
    return value.empty() ? 0 : std::stoi(value);
}

void Power::setPerformanceMode(bool enable) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    if (enable) {
        writeFile(CPU_FREQ_GOV, "performance");
        writeFile(CPU_FREQ_MIN, std::to_string(CPU_FREQ_PERFORMANCE));
        writeFile(GPU_FREQ_MIN, std::to_string(GPU_FREQ_PERFORMANCE));
        mCurrentProfile = 2;
        LOG(INFO) << "Performance mode enabled";
    }
}

void Power::setPowersaveMode(bool enable) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    if (enable) {
        writeFile(CPU_FREQ_GOV, "powersave");
        writeFile(CPU_FREQ_MAX, std::to_string(CPU_FREQ_POWERSAVE));
        writeFile(GPU_FREQ_MAX, std::to_string(GPU_FREQ_POWERSAVE));
        mCurrentProfile = 0;
        LOG(INFO) << "Powersave mode enabled";
    }
}

void Power::setBalancedMode() {
    std::lock_guard<std::mutex> lock(mMutex);
    
    writeFile(CPU_FREQ_GOV, "schedutil");
    writeFile(CPU_FREQ_MIN, std::to_string(CPU_FREQ_POWERSAVE));
    writeFile(CPU_FREQ_MAX, std::to_string(CPU_FREQ_PERFORMANCE));
    writeFile(GPU_FREQ_MIN, std::to_string(GPU_FREQ_POWERSAVE));
    writeFile(GPU_FREQ_MAX, std::to_string(GPU_FREQ_PERFORMANCE));
    mCurrentProfile = 1;
    LOG(INFO) << "Balanced mode enabled";
}

void Power::handleSustainedPerformance(bool enable) {
    mSustainedPerformance = enable;
    if (enable) {
        // Use balanced frequencies to prevent thermal throttling
        writeFile(CPU_FREQ_GOV, "schedutil");
        writeFile(CPU_FREQ_MAX, std::to_string(CPU_FREQ_BALANCED));
    } else if (!mVrMode) {
        setBalancedMode();
    }
}

void Power::handleVrMode(bool enable) {
    mVrMode = enable;
    if (enable) {
        setPerformanceMode(true);
    } else if (!mSustainedPerformance) {
        setBalancedMode();
    }
}

void Power::handleLaunch(int32_t duration) {
    // Boost CPU for app launch
    if (duration > 0) {
        writeFile(CPU_FREQ_GOV, "performance");
        // Duration is ignored in this simple implementation
        // A full implementation would use a timer
    }
}

void Power::handleInteraction(int32_t duration) {
    // Brief CPU boost for UI interaction
    if (duration > 0 && mCurrentProfile != 2) {
        writeFile(CPU_FREQ_MIN, std::to_string(CPU_FREQ_BALANCED));
    }
}

Return<void> Power::setInteractive(bool interactive) {
    mInteractive = interactive;
    
    if (!interactive) {
        // Screen off - enter powersave
        if (!mSustainedPerformance && !mVrMode) {
            setPowersaveMode(true);
        }
    } else {
        // Screen on - restore balanced mode
        if (!mSustainedPerformance && !mVrMode) {
            setBalancedMode();
        }
    }
    
    return Void();
}

Return<void> Power::powerHint(PowerHint hint, int32_t data) {
    switch (hint) {
        case PowerHint::VSYNC:
            // VSync hint - not much we can do
            break;
        case PowerHint::INTERACTION:
            handleInteraction(data);
            break;
        case PowerHint::VIDEO_ENCODE:
        case PowerHint::VIDEO_DECODE:
            // Video processing - maintain balanced mode
            break;
        case PowerHint::LOW_POWER:
            setPowersaveMode(data != 0);
            break;
        case PowerHint::SUSTAINED_PERFORMANCE:
            handleSustainedPerformance(data != 0);
            break;
        case PowerHint::VR_MODE:
            handleVrMode(data != 0);
            break;
        case PowerHint::LAUNCH:
            handleLaunch(data);
            break;
        default:
            break;
    }
    return Void();
}

Return<void> Power::setFeature(Feature feature, bool activate) {
    switch (feature) {
        case Feature::POWER_FEATURE_DOUBLE_TAP_TO_WAKE:
            // Not supported on Pi 5 without additional hardware
            break;
        default:
            break;
    }
    return Void();
}

Return<void> Power::getPlatformLowPowerStats(getPlatformLowPowerStats_cb _hidl_cb) {
    hidl_vec<PowerStatePlatformSleepState> states;
    // Raspberry Pi 5 doesn't have traditional sleep states
    _hidl_cb(states, Status::SUCCESS);
    return Void();
}

Return<void> Power::getSubsystemLowPowerStats(getSubsystemLowPowerStats_cb _hidl_cb) {
    hidl_vec<PowerStateSubsystem> subsystems;
    _hidl_cb(subsystems, Status::SUCCESS);
    return Void();
}

Return<void> Power::powerHintAsync(PowerHint hint, int32_t data) {
    return powerHint(hint, data);
}

Return<void> Power::powerHintAsync_1_2(PowerHint_1_2 hint, int32_t data) {
    switch (hint) {
        case PowerHint_1_2::AUDIO_LOW_LATENCY:
            // Boost for low-latency audio
            handleInteraction(data);
            break;
        case PowerHint_1_2::AUDIO_STREAMING:
            // Maintain stable clocks for audio streaming
            break;
        case PowerHint_1_2::CAMERA_LAUNCH:
        case PowerHint_1_2::CAMERA_STREAMING:
        case PowerHint_1_2::CAMERA_SHOT:
            handleLaunch(data);
            break;
        default:
            return powerHint(static_cast<PowerHint>(hint), data);
    }
    return Void();
}

Return<void> Power::powerHintAsync_1_3(PowerHint_1_3 hint, int32_t data) {
    switch (hint) {
        case PowerHint_1_3::EXPENSIVE_RENDERING:
            if (data != 0) {
                setPerformanceMode(true);
            } else {
                setBalancedMode();
            }
            break;
        default:
            return powerHintAsync_1_2(static_cast<PowerHint_1_2>(hint), data);
    }
    return Void();
}

}  // namespace implementation
}  // namespace V1_3
}  // namespace power
}  // namespace hardware
}  // namespace android
