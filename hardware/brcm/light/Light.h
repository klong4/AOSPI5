// Copyright (C) 2024 The Android Open Source Project

#pragma once

#include <android/hardware/light/2.0/ILight.h>
#include <hidl/Status.h>
#include <string>

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::light::V2_0::ILight;
using ::android::hardware::light::V2_0::LightState;
using ::android::hardware::light::V2_0::Status;
using ::android::hardware::light::V2_0::Type;

class Light : public ILight {
public:
    Light();
    ~Light();

    Return<Status> setLight(Type type, const LightState& state) override;
    Return<void> getSupportedTypes(getSupportedTypes_cb _hidl_cb) override;

private:
    void findBacklightDevice();
    Status setBacklight(const LightState& state);
    Status setNotificationLight(const LightState& state);
    Status setAttentionLight(const LightState& state);
    Status setBatteryLight(const LightState& state);

    std::string mBacklightPath;
    int mMaxBacklight = 255;
    bool mHasActivityLed = false;
    bool mHasPowerLed = false;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android
