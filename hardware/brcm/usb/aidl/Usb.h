/*
 * Copyright (C) 2025 The Android Open Source Project
 * USB HAL AIDL Header
 */

#pragma once

#include <aidl/android/hardware/usb/BnUsb.h>

namespace aidl {
namespace android {
namespace hardware {
namespace usb {
namespace impl {
namespace rpi5 {

class Usb : public BnUsb {
  public:
    Usb();
    
    ndk::ScopedAStatus enableContaminantPresenceDetection(
            const std::string& portName, bool enable, int64_t transactionId) override;
    ndk::ScopedAStatus queryPortStatus(int64_t transactionId) override;
    ndk::ScopedAStatus setCallback(
            const std::shared_ptr<IUsbCallback>& callback) override;
    ndk::ScopedAStatus switchRole(
            const std::string& portName,
            const PortRole& role,
            int64_t transactionId) override;
    ndk::ScopedAStatus enableUsbData(
            const std::string& portName,
            bool enable,
            int64_t transactionId) override;
    ndk::ScopedAStatus enableUsbDataWhileDocked(
            const std::string& portName,
            int64_t transactionId) override;
    ndk::ScopedAStatus limitPowerTransfer(
            const std::string& portName,
            bool limit,
            int64_t transactionId) override;
    ndk::ScopedAStatus resetUsbPort(
            const std::string& portName,
            int64_t transactionId) override;

  private:
    std::shared_ptr<IUsbCallback> callback_;
};

}  // namespace rpi5
}  // namespace impl
}  // namespace usb
}  // namespace hardware
}  // namespace android
}  // namespace aidl
