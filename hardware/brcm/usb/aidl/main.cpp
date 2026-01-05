/*
 * Copyright (C) 2025 The Android Open Source Project
 * USB HAL AIDL Service Main
 */

#define LOG_TAG "android.hardware.usb-service.rpi5"

#include "Usb.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::usb::impl::rpi5::Usb;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    
    std::shared_ptr<Usb> usb = ndk::SharedRefBase::make<Usb>();
    
    const std::string instance = std::string() + Usb::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(
            usb->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);

    LOG(INFO) << "Raspberry Pi 5 USB HAL AIDL Service started";
    
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
