/*
 * Copyright (C) 2025 The Android Open Source Project
 * GPIO HAL AIDL Header
 */

#pragma once

#include <aidl/android/hardware/gpio/BnGpio.h>
#include <gpiod.h>
#include <mutex>
#include <map>

namespace aidl {
namespace android {
namespace hardware {
namespace gpio {
namespace impl {
namespace rpi5 {

class Gpio : public BnGpio {
  public:
    Gpio();
    ~Gpio();
    
    ndk::ScopedAStatus getPinCount(int32_t* _aidl_return) override;
    ndk::ScopedAStatus exportPin(int32_t pin) override;
    ndk::ScopedAStatus unexportPin(int32_t pin) override;
    ndk::ScopedAStatus setDirection(int32_t pin, GpioDirection direction) override;
    ndk::ScopedAStatus getDirection(int32_t pin, GpioDirection* _aidl_return) override;
    ndk::ScopedAStatus setValue(int32_t pin, int32_t value) override;
    ndk::ScopedAStatus getValue(int32_t pin, int32_t* _aidl_return) override;
    ndk::ScopedAStatus setEdge(int32_t pin, GpioEdge edge) override;
    ndk::ScopedAStatus getEdge(int32_t pin, GpioEdge* _aidl_return) override;

  private:
    struct gpiod_chip* chip_;
    std::mutex lines_mutex_;
    std::map<int32_t, struct gpiod_line*> lines_;
};

}  // namespace rpi5
}  // namespace impl
}  // namespace gpio
}  // namespace hardware
}  // namespace android
}  // namespace aidl
