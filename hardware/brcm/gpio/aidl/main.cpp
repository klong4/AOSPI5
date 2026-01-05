/*
 * Copyright (C) 2025 The Android Open Source Project
 * GPIO HAL AIDL Service Main
 */

#define LOG_TAG "android.hardware.gpio-service.rpi5"

#include "Gpio.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::gpio::impl::rpi5::Gpio;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    
    std::shared_ptr<Gpio> gpio = ndk::SharedRefBase::make<Gpio>();
    
    const std::string instance = std::string() + Gpio::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(
            gpio->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);

    LOG(INFO) << "Raspberry Pi 5 GPIO HAL AIDL Service started";
    
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
