/*
 * Copyright (C) 2025 The Android Open Source Project
 * Light HAL AIDL Header
 */

#pragma once

#include <aidl/android/hardware/light/BnLights.h>

namespace aidl {
namespace android {
namespace hardware {
namespace light {
namespace impl {
namespace rpi5 {

class Lights : public BnLights {
  public:
    Lights();
    
    ndk::ScopedAStatus setLightState(int32_t id, const HwLightState& state) override;
    ndk::ScopedAStatus getLights(std::vector<HwLight>* _aidl_return) override;
};

}  // namespace rpi5
}  // namespace impl
}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
