// Copyright (C) 2024 The Android Open Source Project
// GPIO HAL Service Entry Point

#define LOG_TAG "GpioService"

#include <android-base/logging.h>
#include <android/hardware/gpio/1.0/IGpio.h>
#include <hidl/HidlTransportSupport.h>

using android::hardware::gpio::V1_0::IGpio;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;
using android::OK;

int main() {
    android::base::InitLogging(nullptr);
    LOG(INFO) << "GPIO HAL Service starting...";

    configureRpcThreadpool(1, true);

    sp<IGpio> gpio = IGpio::getService();
    if (gpio == nullptr) {
        LOG(ERROR) << "Failed to get GPIO HAL implementation";
        return 1;
    }

    android::status_t status = gpio->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Failed to register GPIO HAL service: " << status;
        return 1;
    }

    LOG(INFO) << "GPIO HAL Service started successfully";
    joinRpcThreadpool();

    LOG(ERROR) << "GPIO HAL Service exited unexpectedly";
    return 1;
}
