// Copyright (C) 2024 The Android Open Source Project

#pragma once

#include <android/hardware/usb/1.3/IUsb.h>
#include <android/hardware/usb/1.3/types.h>
#include <android/hardware/usb/1.2/types.h>
#include <android/hardware/usb/1.1/types.h>
#include <android/hardware/usb/1.1/IUsbCallback.h>
#include <hidl/Status.h>
#include <pthread.h>
#include <string>

namespace android {
namespace hardware {
namespace usb {
namespace V1_3 {
namespace implementation {

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::sp;

using ::android::hardware::usb::V1_0::PortDataRole;
using ::android::hardware::usb::V1_0::PortPowerRole;
using ::android::hardware::usb::V1_0::PortRole;
using ::android::hardware::usb::V1_0::PortRoleType;
using ::android::hardware::usb::V1_0::Status;
using ::android::hardware::usb::V1_1::PortMode_1_1;
using ::android::hardware::usb::V1_2::PortStatus;

class Usb : public IUsb {
public:
    Usb();
    ~Usb();

    // V1_0 methods
    Return<void> switchRole(const hidl_string& portName,
                            const V1_0::PortRole& newRole) override;
    Return<void> setCallback(const sp<V1_0::IUsbCallback>& callback) override;
    Return<void> queryPortStatus() override;

    // V1_1 methods
    Return<void> setCallback_1_1(const sp<V1_1::IUsbCallback>& callback);

    // V1_2 methods
    Return<void> enableContaminantPresenceDetection(const hidl_string& portName,
                                                     bool enable) override;
    Return<void> enableContaminantPresenceProtection(const hidl_string& portName,
                                                      bool enable) override;

    // V1_3 methods
    Return<bool> enableUsbDataSignal(bool enable) override;

private:
    void findUsbController();
    void queryPortStatus(PortStatus* status);
    void pollLoop();
    static void* pollThreadFunc(void* arg);

    sp<V1_0::IUsbCallback> mCallback;
    std::string mUsbController;
    pthread_t mPollThread;
};

}  // namespace implementation
}  // namespace V1_3
}  // namespace usb
}  // namespace hardware
}  // namespace android
