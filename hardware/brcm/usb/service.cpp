// Copyright (C) 2024 The Android Open Source Project

#define LOG_TAG "UsbService"

#include <android-base/logging.h>
#include <android/hardware/usb/1.3/IUsb.h>
#include <hidl/HidlTransportSupport.h>
#include "Usb.h"

using android::hardware::usb::V1_3::IUsb;
using android::hardware::usb::V1_3::implementation::Usb;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;
using android::OK;

int main() {
    android::base::InitLogging(nullptr);
    LOG(INFO) << "USB HAL Service starting...";

    configureRpcThreadpool(1, true);

    sp<IUsb> usb = new Usb();
    android::status_t status = usb->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Failed to register USB HAL service: " << status;
        return 1;
    }

    LOG(INFO) << "USB HAL Service started successfully";
    joinRpcThreadpool();

    LOG(ERROR) << "USB HAL Service exited unexpectedly";
    return 1;
}
