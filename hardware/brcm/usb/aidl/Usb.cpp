/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * USB HAL AIDL Implementation for Raspberry Pi 5
 * Android 16 compatible
 */

#define LOG_TAG "android.hardware.usb-service.rpi5"

#include "Usb.h"

#include <android-base/logging.h>
#include <android-base/file.h>
#include <android-base/properties.h>

namespace aidl {
namespace android {
namespace hardware {
namespace usb {
namespace impl {
namespace rpi5 {

static constexpr const char* kUdcPath = "/sys/class/udc";

Usb::Usb() {
    LOG(INFO) << "Raspberry Pi 5 USB HAL AIDL initialized";
}

ndk::ScopedAStatus Usb::enableContaminantPresenceDetection(
        const std::string& /*portName*/, bool /*enable*/, int64_t /*transactionId*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Usb::queryPortStatus(int64_t transactionId) {
    std::vector<PortStatus> ports;
    
    PortStatus status;
    status.portName = "usb0";
    status.currentDataRole = PortDataRole::HOST;
    status.currentPowerRole = PortPowerRole::SOURCE;
    status.currentMode = PortMode::DFP;
    status.canChangeMode = false;
    status.canChangeDataRole = false;
    status.canChangePowerRole = false;
    status.supportedModes = PortMode::DFP;
    status.usbDataStatus = UsbDataStatus::ENABLED;
    status.powerTransferLimited = false;
    status.powerBrickStatus = PowerBrickStatus::NOT_CONNECTED;
    
    ports.push_back(status);
    
    if (callback_) {
        callback_->notifyPortStatusChange(ports, Status::SUCCESS);
    }
    
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Usb::setCallback(
        const std::shared_ptr<IUsbCallback>& callback) {
    callback_ = callback;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Usb::switchRole(
        const std::string& /*portName*/,
        const PortRole& /*role*/,
        int64_t /*transactionId*/) {
    // Role switching not supported on Pi 5
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Usb::enableUsbData(
        const std::string& /*portName*/,
        bool /*enable*/,
        int64_t /*transactionId*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Usb::enableUsbDataWhileDocked(
        const std::string& /*portName*/,
        int64_t /*transactionId*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Usb::limitPowerTransfer(
        const std::string& /*portName*/,
        bool /*limit*/,
        int64_t /*transactionId*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Usb::resetUsbPort(
        const std::string& /*portName*/,
        int64_t /*transactionId*/) {
    return ndk::ScopedAStatus::ok();
}

}  // namespace rpi5
}  // namespace impl
}  // namespace usb
}  // namespace hardware
}  // namespace android
}  // namespace aidl
