/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Power HAL AIDL Service Main for Raspberry Pi 5
 */

#define LOG_TAG "android.hardware.power-service.rpi5"

#include "Power.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::power::impl::rpi5::Power;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    
    std::shared_ptr<Power> power = ndk::SharedRefBase::make<Power>();
    
    const std::string instance = std::string() + Power::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(
            power->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);

    LOG(INFO) << "Raspberry Pi 5 Power HAL AIDL Service started";
    
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // Should not reach here
}
