/*
 * Copyright (C) 2025 The Android Open Source Project
 * Light HAL AIDL Service Main
 */

#define LOG_TAG "android.hardware.light-service.rpi5"

#include "Lights.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::light::impl::rpi5::Lights;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    
    std::shared_ptr<Lights> lights = ndk::SharedRefBase::make<Lights>();
    
    const std::string instance = std::string() + Lights::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(
            lights->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);

    LOG(INFO) << "Raspberry Pi 5 Light HAL AIDL Service started";
    
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
