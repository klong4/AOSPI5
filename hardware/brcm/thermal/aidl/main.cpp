/*
 * Copyright (C) 2025 The Android Open Source Project
 * Thermal HAL AIDL Service Main
 */

#define LOG_TAG "android.hardware.thermal-service.rpi5"

#include "Thermal.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::thermal::impl::rpi5::Thermal;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    
    std::shared_ptr<Thermal> thermal = ndk::SharedRefBase::make<Thermal>();
    
    const std::string instance = std::string() + Thermal::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(
            thermal->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);

    LOG(INFO) << "Raspberry Pi 5 Thermal HAL AIDL Service started";
    
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
