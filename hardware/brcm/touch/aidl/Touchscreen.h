/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <aidl/android/hardware/input/processor/BnInputProcessor.h>
#include <linux/input.h>
#include <vector>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <atomic>

namespace aidl {
namespace android {
namespace hardware {
namespace input {
namespace implementation {

// Touchscreen controller types
enum class TouchController {
    // Focaltech controllers
    FT5X06,      // FT5206, FT5306, FT5406, FT5506
    FT6X06,      // FT6206, FT6236, FT6336
    FT5426,
    FT5526,
    FT8719,
    
    // Goodix controllers
    GT911,
    GT912,
    GT927,
    GT928,
    GT5688,
    GT9XX,       // Generic GT9xx family
    GT1X,        // GT1151, GT1158
    
    // Ilitek controllers
    ILI2130,
    ILI2131,
    ILI2132,
    ILI251X,
    
    // Atmel/Microchip controllers
    MXT224,
    MXT336,
    MXT540,
    ATMXT,       // Generic Atmel mXT
    
    // Synaptics controllers
    RMI4,
    S3203,
    S3508,
    S3706,
    
    // Elan controllers
    EKTF2127,
    EKTH3500,
    
    // Sitronix controllers
    ST1232,
    ST1633,
    
    // Himax controllers
    HX8526,
    HX8527,
    
    // Cypress controllers
    CYTTSP4,
    CYTTSP5,
    
    // ADS7846 resistive (SPI)
    ADS7846,
    TSC2007,
    
    // Waveshare specific
    WS_GT911,
    
    // Generic
    GENERIC_I2C,
    GENERIC_SPI,
};

// Touch event structure
struct TouchEvent {
    int32_t id;
    int32_t x;
    int32_t y;
    int32_t pressure;
    int32_t touchMajor;
    int32_t touchMinor;
    bool active;
};

// Touchscreen device info
struct TouchDeviceInfo {
    std::string name;
    TouchController controller;
    uint8_t i2cBus;
    uint8_t i2cAddr;
    int32_t maxX;
    int32_t maxY;
    int32_t maxTouches;
    int32_t irqGpio;
    int32_t resetGpio;
    bool invertX;
    bool invertY;
    bool swapXY;
};

// Supported touchscreen controllers with I2C addresses
static const std::vector<TouchDeviceInfo> kSupportedTouchControllers = {
    // Focaltech FT5x06 family (common on official RPi display)
    {"ft5206", TouchController::FT5X06, 1, 0x38, 800, 480, 5, 4, -1, false, false, false},
    {"ft5306", TouchController::FT5X06, 1, 0x38, 800, 480, 5, 4, -1, false, false, false},
    {"ft5406", TouchController::FT5X06, 1, 0x38, 800, 480, 10, 4, -1, false, false, false},
    {"ft5426", TouchController::FT5426, 1, 0x38, 1024, 600, 5, 4, -1, false, false, false},
    
    // Focaltech FT6x06 family
    {"ft6206", TouchController::FT6X06, 1, 0x38, 320, 240, 2, 4, -1, false, false, false},
    {"ft6236", TouchController::FT6X06, 1, 0x38, 480, 320, 2, 4, -1, false, false, false},
    {"ft6336", TouchController::FT6X06, 1, 0x38, 480, 320, 2, 4, -1, false, false, false},
    
    // Goodix GT911 (very common)
    {"gt911", TouchController::GT911, 1, 0x5D, 1024, 600, 5, 4, 17, false, false, false},
    {"gt911_alt", TouchController::GT911, 1, 0x14, 1024, 600, 5, 4, 17, false, false, false},
    
    // Goodix GT912
    {"gt912", TouchController::GT912, 1, 0x5D, 1280, 800, 5, 4, 17, false, false, false},
    
    // Goodix GT927
    {"gt927", TouchController::GT927, 1, 0x14, 1920, 1080, 10, 4, 17, false, false, false},
    
    // Goodix GT928  
    {"gt928", TouchController::GT928, 1, 0x5D, 1920, 1200, 10, 4, 17, false, false, false},
    
    // Goodix GT5688
    {"gt5688", TouchController::GT5688, 1, 0x14, 1080, 1920, 10, 4, 17, false, false, false},
    
    // Goodix GT1151
    {"gt1151", TouchController::GT1X, 1, 0x14, 720, 1280, 10, 4, 17, false, false, false},
    
    // Ilitek controllers
    {"ili2130", TouchController::ILI2130, 1, 0x41, 800, 480, 2, 4, -1, false, false, false},
    {"ili2131", TouchController::ILI2131, 1, 0x41, 1024, 600, 2, 4, -1, false, false, false},
    {"ili251x", TouchController::ILI251X, 1, 0x41, 1280, 800, 10, 4, -1, false, false, false},
    
    // Atmel mXT controllers
    {"mxt224", TouchController::MXT224, 1, 0x4A, 1024, 768, 10, 4, -1, false, false, false},
    {"mxt336", TouchController::MXT336, 1, 0x4A, 1280, 800, 10, 4, -1, false, false, false},
    {"mxt540", TouchController::MXT540, 1, 0x4B, 1920, 1080, 10, 4, -1, false, false, false},
    
    // Synaptics controllers
    {"s3203", TouchController::S3203, 1, 0x20, 1080, 1920, 10, 4, -1, false, false, false},
    {"s3508", TouchController::S3508, 1, 0x20, 1080, 2160, 10, 4, -1, false, false, false},
    
    // Elan controllers
    {"ektf2127", TouchController::EKTF2127, 1, 0x10, 800, 480, 5, 4, -1, false, false, false},
    {"ekth3500", TouchController::EKTH3500, 1, 0x10, 1024, 600, 10, 4, -1, false, false, false},
    
    // Sitronix controllers
    {"st1232", TouchController::ST1232, 1, 0x55, 800, 480, 2, 4, -1, false, false, false},
    {"st1633", TouchController::ST1633, 1, 0x55, 1024, 768, 5, 4, -1, false, false, false},
    
    // Himax controllers
    {"hx8526", TouchController::HX8526, 1, 0x48, 1080, 1920, 10, 4, -1, false, false, false},
    
    // Cypress controllers
    {"cyttsp4", TouchController::CYTTSP4, 1, 0x24, 800, 480, 5, 4, -1, false, false, false},
    {"cyttsp5", TouchController::CYTTSP5, 1, 0x24, 1280, 800, 10, 4, -1, false, false, false},
    
    // Waveshare displays with GT911
    {"waveshare_4inch", TouchController::WS_GT911, 1, 0x14, 480, 800, 5, 4, 17, false, false, true},
    {"waveshare_5inch", TouchController::WS_GT911, 1, 0x14, 800, 480, 5, 4, 17, false, false, false},
    {"waveshare_7inch", TouchController::WS_GT911, 1, 0x14, 800, 480, 5, 4, 17, false, false, false},
    {"waveshare_7inch_c", TouchController::WS_GT911, 1, 0x14, 1024, 600, 5, 4, 17, false, false, false},
    {"waveshare_10inch", TouchController::WS_GT911, 1, 0x14, 1280, 800, 5, 4, 17, false, false, false},
    
    // Pimoroni HyperPixel
    {"hyperpixel4", TouchController::GT911, 1, 0x5D, 800, 480, 5, 27, -1, false, false, false},
    {"hyperpixel4_square", TouchController::GT911, 1, 0x5D, 720, 720, 5, 27, -1, false, false, false},
    
    // Adafruit displays
    {"adafruit_ft6206", TouchController::FT6X06, 1, 0x38, 320, 240, 2, 4, -1, false, false, false},
    
    // Generic fallbacks
    {"generic_i2c", TouchController::GENERIC_I2C, 1, 0x38, 800, 480, 5, 4, -1, false, false, false},
};

class TouchscreenManager {
public:
    static TouchscreenManager& getInstance();
    
    // Initialization
    bool initialize();
    bool detectTouchController();
    bool initController(const std::string& controllerName);
    bool initController(const TouchDeviceInfo& device);
    
    // Touch input handling
    void startInputThread();
    void stopInputThread();
    bool pollTouchEvents(std::vector<TouchEvent>& events);
    
    // Configuration
    bool setCalibration(int32_t minX, int32_t maxX, int32_t minY, int32_t maxY);
    bool setOrientation(bool invertX, bool invertY, bool swapXY);
    bool setTouchThreshold(uint8_t threshold);
    
    // Controller-specific
    bool resetController();
    bool sendCommand(uint8_t cmd, const std::vector<uint8_t>& data);
    bool readRegisters(uint8_t reg, std::vector<uint8_t>& data, size_t len);
    
    // Info
    std::vector<std::string> getSupportedControllers();
    std::string getActiveController();
    bool getTouchInfo(int32_t& maxX, int32_t& maxY, int32_t& maxTouches);
    
private:
    TouchscreenManager();
    ~TouchscreenManager();
    
    void inputThreadFunc();
    
    // I2C communication
    bool i2cOpen(uint8_t bus, uint8_t addr);
    void i2cClose();
    bool i2cWrite(const std::vector<uint8_t>& data);
    bool i2cRead(std::vector<uint8_t>& data, size_t len);
    bool i2cWriteRead(const std::vector<uint8_t>& writeData, 
                      std::vector<uint8_t>& readData, size_t readLen);
    
    // Controller-specific init
    bool initFT5X06();
    bool initFT6X06();
    bool initGT911();
    bool initGT9XX();
    bool initILI251X();
    bool initMXT();
    bool initElan();
    bool initSitronix();
    
    // Controller-specific read
    bool readFT5X06(std::vector<TouchEvent>& events);
    bool readFT6X06(std::vector<TouchEvent>& events);
    bool readGT911(std::vector<TouchEvent>& events);
    bool readGT9XX(std::vector<TouchEvent>& events);
    bool readILI251X(std::vector<TouchEvent>& events);
    bool readMXT(std::vector<TouchEvent>& events);
    bool readElan(std::vector<TouchEvent>& events);
    bool readSitronix(std::vector<TouchEvent>& events);
    
    std::mutex mLock;
    int mI2cFd;
    TouchDeviceInfo mActiveDevice;
    bool mInitialized;
    
    std::thread mInputThread;
    std::atomic<bool> mRunning;
    
    // Calibration
    int32_t mCalMinX, mCalMaxX;
    int32_t mCalMinY, mCalMaxY;
    bool mInvertX, mInvertY, mSwapXY;
    
    // Input event device
    int mInputFd;
    std::string mInputDevicePath;
};

}  // namespace implementation
}  // namespace input
}  // namespace hardware
}  // namespace android
}  // namespace aidl
