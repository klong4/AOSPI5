/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * GPIO HAL AIDL Implementation for Raspberry Pi 5
 * Android 16 compatible
 */

#define LOG_TAG "android.hardware.gpio-service.rpi5"

#include "Gpio.h"

#include <android-base/logging.h>
#include <gpiod.h>
#include <unistd.h>

namespace aidl {
namespace android {
namespace hardware {
namespace gpio {
namespace impl {
namespace rpi5 {

static constexpr const char* kGpioChipPath = "/dev/gpiochip4";

Gpio::Gpio() : chip_(nullptr) {
    chip_ = gpiod_chip_open(kGpioChipPath);
    if (!chip_) {
        LOG(ERROR) << "Failed to open GPIO chip: " << kGpioChipPath;
    } else {
        LOG(INFO) << "Raspberry Pi 5 GPIO HAL AIDL initialized";
    }
}

Gpio::~Gpio() {
    std::lock_guard<std::mutex> lock(lines_mutex_);
    for (auto& [pin, line] : lines_) {
        if (line) {
            gpiod_line_release(line);
        }
    }
    lines_.clear();
    
    if (chip_) {
        gpiod_chip_close(chip_);
    }
}

ndk::ScopedAStatus Gpio::getPinCount(int32_t* _aidl_return) {
    if (!chip_) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    *_aidl_return = gpiod_chip_num_lines(chip_);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gpio::exportPin(int32_t pin) {
    if (!chip_) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    
    std::lock_guard<std::mutex> lock(lines_mutex_);
    
    if (lines_.find(pin) != lines_.end()) {
        return ndk::ScopedAStatus::ok();  // Already exported
    }
    
    struct gpiod_line* line = gpiod_chip_get_line(chip_, pin);
    if (!line) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    
    lines_[pin] = line;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gpio::unexportPin(int32_t pin) {
    std::lock_guard<std::mutex> lock(lines_mutex_);
    
    auto it = lines_.find(pin);
    if (it != lines_.end()) {
        gpiod_line_release(it->second);
        lines_.erase(it);
    }
    
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gpio::setDirection(int32_t pin, GpioDirection direction) {
    std::lock_guard<std::mutex> lock(lines_mutex_);
    
    auto it = lines_.find(pin);
    if (it == lines_.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    
    int ret;
    if (direction == GpioDirection::OUTPUT) {
        ret = gpiod_line_request_output(it->second, "android-gpio", 0);
    } else {
        ret = gpiod_line_request_input(it->second, "android-gpio");
    }
    
    if (ret < 0) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    }
    
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gpio::getDirection(int32_t pin, GpioDirection* _aidl_return) {
    std::lock_guard<std::mutex> lock(lines_mutex_);
    
    auto it = lines_.find(pin);
    if (it == lines_.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    
    int dir = gpiod_line_direction(it->second);
    *_aidl_return = (dir == GPIOD_LINE_DIRECTION_OUTPUT) ? 
                   GpioDirection::OUTPUT : GpioDirection::INPUT;
    
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gpio::setValue(int32_t pin, int32_t value) {
    std::lock_guard<std::mutex> lock(lines_mutex_);
    
    auto it = lines_.find(pin);
    if (it == lines_.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    
    int ret = gpiod_line_set_value(it->second, value ? 1 : 0);
    if (ret < 0) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    }
    
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gpio::getValue(int32_t pin, int32_t* _aidl_return) {
    std::lock_guard<std::mutex> lock(lines_mutex_);
    
    auto it = lines_.find(pin);
    if (it == lines_.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    
    int value = gpiod_line_get_value(it->second);
    if (value < 0) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    }
    
    *_aidl_return = value;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gpio::setEdge(int32_t pin, GpioEdge edge) {
    // Edge detection would require event monitoring
    // Simplified implementation
    (void)pin;
    (void)edge;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gpio::getEdge(int32_t pin, GpioEdge* _aidl_return) {
    (void)pin;
    *_aidl_return = GpioEdge::NONE;
    return ndk::ScopedAStatus::ok();
}

}  // namespace rpi5
}  // namespace impl
}  // namespace gpio
}  // namespace hardware
}  // namespace android
}  // namespace aidl
