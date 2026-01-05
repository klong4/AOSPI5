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

#define LOG_TAG "TouchscreenHAL"

#include "Touchscreen.h"

#include <android-base/logging.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <cstring>
#include <dirent.h>

namespace aidl {
namespace android {
namespace hardware {
namespace input {
namespace implementation {

// I2C device path format
static const char* kI2cDevPath = "/dev/i2c-";

// Input device path
static const char* kInputDevPath = "/dev/input/";

// GPIO sysfs path
static const char* kGpioPath = "/sys/class/gpio";

// FT5X06 registers
static const uint8_t FT5X06_REG_DEVICE_MODE = 0x00;
static const uint8_t FT5X06_REG_TD_STATUS = 0x02;
static const uint8_t FT5X06_REG_TOUCH_START = 0x03;
static const uint8_t FT5X06_REG_ID_G_THGROUP = 0x80;
static const uint8_t FT5X06_REG_ID_G_PERIODACTIVE = 0x88;
static const uint8_t FT5X06_REG_ID_G_LIB_VERSION = 0xA1;
static const uint8_t FT5X06_REG_ID_G_CIPHER = 0xA3;
static const uint8_t FT5X06_REG_ID_G_FWVERSION = 0xA6;

// GT911 registers
static const uint16_t GT911_REG_CONFIG = 0x8047;
static const uint16_t GT911_REG_CONFIG_CHKSUM = 0x80FF;
static const uint16_t GT911_REG_CONFIG_FRESH = 0x8100;
static const uint16_t GT911_REG_COOR_ADDR = 0x814E;
static const uint16_t GT911_REG_STATUS = 0x814E;
static const uint16_t GT911_REG_POINT_INFO = 0x814F;
static const uint16_t GT911_REG_COMMAND = 0x8040;

TouchscreenManager& TouchscreenManager::getInstance() {
    static TouchscreenManager instance;
    return instance;
}

TouchscreenManager::TouchscreenManager()
    : mI2cFd(-1),
      mInitialized(false),
      mRunning(false),
      mCalMinX(0), mCalMaxX(0),
      mCalMinY(0), mCalMaxY(0),
      mInvertX(false), mInvertY(false), mSwapXY(false),
      mInputFd(-1) {
    LOG(INFO) << "TouchscreenManager initialized";
}

TouchscreenManager::~TouchscreenManager() {
    stopInputThread();
    i2cClose();
    if (mInputFd >= 0) {
        close(mInputFd);
    }
}

bool TouchscreenManager::initialize() {
    std::lock_guard<std::mutex> lock(mLock);
    
    if (mInitialized) {
        return true;
    }
    
    // Try to auto-detect touch controller
    if (detectTouchController()) {
        mInitialized = true;
        return true;
    }
    
    LOG(WARNING) << "No touch controller detected";
    return false;
}

bool TouchscreenManager::detectTouchController() {
    LOG(INFO) << "Auto-detecting touch controller...";
    
    // Try each known controller
    for (const auto& device : kSupportedTouchControllers) {
        if (i2cOpen(device.i2cBus, device.i2cAddr)) {
            // Try to read device ID
            std::vector<uint8_t> data;
            
            switch (device.controller) {
                case TouchController::FT5X06:
                case TouchController::FT6X06:
                case TouchController::FT5426:
                case TouchController::FT5526: {
                    // Read FT chip ID
                    std::vector<uint8_t> regAddr = {FT5X06_REG_ID_G_CIPHER};
                    if (i2cWriteRead(regAddr, data, 1) && data.size() > 0) {
                        uint8_t chipId = data[0];
                        if (chipId == 0x55 || chipId == 0x06 || chipId == 0x36 || 
                            chipId == 0x64 || chipId == 0x26) {
                            LOG(INFO) << "Detected Focaltech controller: " << device.name 
                                      << " (chip ID: 0x" << std::hex << (int)chipId << ")";
                            mActiveDevice = device;
                            return initController(device);
                        }
                    }
                    break;
                }
                
                case TouchController::GT911:
                case TouchController::GT912:
                case TouchController::GT927:
                case TouchController::GT928:
                case TouchController::GT5688:
                case TouchController::GT1X:
                case TouchController::WS_GT911: {
                    // Read GT911 product ID
                    std::vector<uint8_t> regAddr = {0x81, 0x40};  // Product ID register
                    if (i2cWriteRead(regAddr, data, 4) && data.size() >= 4) {
                        char productId[5] = {0};
                        memcpy(productId, data.data(), 4);
                        if (strncmp(productId, "911", 3) == 0 ||
                            strncmp(productId, "912", 3) == 0 ||
                            strncmp(productId, "927", 3) == 0 ||
                            strncmp(productId, "928", 3) == 0 ||
                            strncmp(productId, "568", 3) == 0 ||
                            strncmp(productId, "115", 3) == 0) {
                            LOG(INFO) << "Detected Goodix controller: " << device.name 
                                      << " (product ID: " << productId << ")";
                            mActiveDevice = device;
                            return initController(device);
                        }
                    }
                    break;
                }
                
                case TouchController::ILI2130:
                case TouchController::ILI2131:
                case TouchController::ILI2132:
                case TouchController::ILI251X: {
                    // Read Ilitek firmware version
                    std::vector<uint8_t> cmd = {0x40};  // Get firmware version
                    if (i2cWriteRead(cmd, data, 4) && data.size() >= 4) {
                        LOG(INFO) << "Detected Ilitek controller: " << device.name;
                        mActiveDevice = device;
                        return initController(device);
                    }
                    break;
                }
                
                case TouchController::MXT224:
                case TouchController::MXT336:
                case TouchController::MXT540:
                case TouchController::ATMXT: {
                    // Read Atmel info block
                    std::vector<uint8_t> regAddr = {0x00, 0x00};
                    if (i2cWriteRead(regAddr, data, 7) && data.size() >= 7) {
                        uint8_t familyId = data[0];
                        if (familyId == 0x81 || familyId == 0x82 || familyId == 0xA2) {
                            LOG(INFO) << "Detected Atmel mXT controller: " << device.name;
                            mActiveDevice = device;
                            return initController(device);
                        }
                    }
                    break;
                }
                
                case TouchController::EKTF2127:
                case TouchController::EKTH3500: {
                    // Read Elan hello packet
                    std::vector<uint8_t> cmd = {0x55, 0x55, 0x55, 0x55};
                    if (i2cWrite(cmd)) {
                        usleep(10000);
                        if (i2cRead(data, 4) && data.size() >= 4) {
                            LOG(INFO) << "Detected Elan controller: " << device.name;
                            mActiveDevice = device;
                            return initController(device);
                        }
                    }
                    break;
                }
                
                case TouchController::ST1232:
                case TouchController::ST1633: {
                    // Read Sitronix status
                    std::vector<uint8_t> regAddr = {0x00};
                    if (i2cWriteRead(regAddr, data, 8) && data.size() >= 8) {
                        LOG(INFO) << "Detected Sitronix controller: " << device.name;
                        mActiveDevice = device;
                        return initController(device);
                    }
                    break;
                }
                
                default:
                    break;
            }
            
            i2cClose();
        }
    }
    
    return false;
}

bool TouchscreenManager::initController(const std::string& controllerName) {
    for (const auto& device : kSupportedTouchControllers) {
        if (device.name == controllerName) {
            return initController(device);
        }
    }
    LOG(ERROR) << "Unknown controller: " << controllerName;
    return false;
}

bool TouchscreenManager::initController(const TouchDeviceInfo& device) {
    std::lock_guard<std::mutex> lock(mLock);
    
    mActiveDevice = device;
    
    // Open I2C
    if (!i2cOpen(device.i2cBus, device.i2cAddr)) {
        LOG(ERROR) << "Failed to open I2C for " << device.name;
        return false;
    }
    
    // Reset if GPIO available
    if (device.resetGpio > 0) {
        resetController();
    }
    
    // Controller-specific initialization
    bool result = false;
    switch (device.controller) {
        case TouchController::FT5X06:
        case TouchController::FT5426:
        case TouchController::FT5526:
            result = initFT5X06();
            break;
            
        case TouchController::FT6X06:
            result = initFT6X06();
            break;
            
        case TouchController::GT911:
        case TouchController::GT912:
        case TouchController::GT927:
        case TouchController::GT928:
        case TouchController::GT5688:
        case TouchController::GT1X:
        case TouchController::WS_GT911:
            result = initGT911();
            break;
            
        case TouchController::ILI2130:
        case TouchController::ILI2131:
        case TouchController::ILI2132:
        case TouchController::ILI251X:
            result = initILI251X();
            break;
            
        case TouchController::MXT224:
        case TouchController::MXT336:
        case TouchController::MXT540:
        case TouchController::ATMXT:
            result = initMXT();
            break;
            
        case TouchController::EKTF2127:
        case TouchController::EKTH3500:
            result = initElan();
            break;
            
        case TouchController::ST1232:
        case TouchController::ST1633:
            result = initSitronix();
            break;
            
        default:
            LOG(WARNING) << "No specific init for controller, using generic";
            result = true;
            break;
    }
    
    if (result) {
        // Set calibration defaults
        mCalMinX = 0;
        mCalMaxX = device.maxX;
        mCalMinY = 0;
        mCalMaxY = device.maxY;
        mInvertX = device.invertX;
        mInvertY = device.invertY;
        mSwapXY = device.swapXY;
        
        LOG(INFO) << "Touch controller " << device.name << " initialized";
        LOG(INFO) << "  Resolution: " << device.maxX << "x" << device.maxY;
        LOG(INFO) << "  Max touches: " << device.maxTouches;
        
        mInitialized = true;
    }
    
    return result;
}

// ============================================================================
// Controller-specific initialization
// ============================================================================

bool TouchscreenManager::initFT5X06() {
    LOG(INFO) << "Initializing FT5X06 touch controller";
    
    // Read chip ID
    std::vector<uint8_t> regAddr = {FT5X06_REG_ID_G_CIPHER};
    std::vector<uint8_t> data;
    if (i2cWriteRead(regAddr, data, 1) && data.size() > 0) {
        LOG(INFO) << "FT5X06 chip ID: 0x" << std::hex << (int)data[0];
    }
    
    // Read firmware version
    regAddr = {FT5X06_REG_ID_G_FWVERSION};
    if (i2cWriteRead(regAddr, data, 1) && data.size() > 0) {
        LOG(INFO) << "FT5X06 firmware version: 0x" << std::hex << (int)data[0];
    }
    
    // Set device mode to normal
    std::vector<uint8_t> cmd = {FT5X06_REG_DEVICE_MODE, 0x00};
    i2cWrite(cmd);
    
    // Set touch threshold
    cmd = {FT5X06_REG_ID_G_THGROUP, 0x16};  // Threshold = 22
    i2cWrite(cmd);
    
    // Set report rate
    cmd = {FT5X06_REG_ID_G_PERIODACTIVE, 0x06};  // 6 = 100Hz
    i2cWrite(cmd);
    
    return true;
}

bool TouchscreenManager::initFT6X06() {
    LOG(INFO) << "Initializing FT6X06 touch controller";
    
    // Similar to FT5X06 but optimized for 2-touch
    std::vector<uint8_t> cmd = {FT5X06_REG_DEVICE_MODE, 0x00};
    i2cWrite(cmd);
    
    return true;
}

bool TouchscreenManager::initGT911() {
    LOG(INFO) << "Initializing GT911 touch controller";
    
    // Read product ID
    std::vector<uint8_t> regAddr = {0x81, 0x40};
    std::vector<uint8_t> data;
    if (i2cWriteRead(regAddr, data, 4) && data.size() >= 4) {
        char productId[5] = {0};
        memcpy(productId, data.data(), 4);
        LOG(INFO) << "GT911 product ID: " << productId;
    }
    
    // Read firmware version
    regAddr = {0x81, 0x44};
    if (i2cWriteRead(regAddr, data, 2) && data.size() >= 2) {
        uint16_t fwVersion = (data[1] << 8) | data[0];
        LOG(INFO) << "GT911 firmware version: 0x" << std::hex << fwVersion;
    }
    
    // Soft reset
    std::vector<uint8_t> cmd = {0x80, 0x40, 0x02};  // Command register, soft reset
    i2cWrite(cmd);
    usleep(50000);
    
    // Read config
    regAddr = {0x80, 0x47};
    if (i2cWriteRead(regAddr, data, 1) && data.size() > 0) {
        LOG(INFO) << "GT911 config version: 0x" << std::hex << (int)data[0];
    }
    
    return true;
}

bool TouchscreenManager::initGT9XX() {
    return initGT911();  // Same protocol
}

bool TouchscreenManager::initILI251X() {
    LOG(INFO) << "Initializing ILI251X touch controller";
    
    // Get firmware version
    std::vector<uint8_t> cmd = {0x40};
    std::vector<uint8_t> data;
    if (i2cWriteRead(cmd, data, 4) && data.size() >= 4) {
        LOG(INFO) << "ILI251X firmware: " << (int)data[0] << "." 
                  << (int)data[1] << "." << (int)data[2] << "." << (int)data[3];
    }
    
    // Get protocol version
    cmd = {0x42};
    if (i2cWriteRead(cmd, data, 2) && data.size() >= 2) {
        LOG(INFO) << "ILI251X protocol: " << (int)data[0] << "." << (int)data[1];
    }
    
    return true;
}

bool TouchscreenManager::initMXT() {
    LOG(INFO) << "Initializing Atmel mXT touch controller";
    
    // Read info block
    std::vector<uint8_t> regAddr = {0x00, 0x00};
    std::vector<uint8_t> data;
    if (i2cWriteRead(regAddr, data, 7) && data.size() >= 7) {
        LOG(INFO) << "mXT family ID: 0x" << std::hex << (int)data[0];
        LOG(INFO) << "mXT variant ID: 0x" << std::hex << (int)data[1];
        LOG(INFO) << "mXT version: " << (int)data[2] << "." << (int)data[3];
    }
    
    return true;
}

bool TouchscreenManager::initElan() {
    LOG(INFO) << "Initializing Elan touch controller";
    
    // Send hello command
    std::vector<uint8_t> cmd = {0x55, 0x55, 0x55, 0x55};
    i2cWrite(cmd);
    usleep(10000);
    
    // Read hello response
    std::vector<uint8_t> data;
    if (i2cRead(data, 4) && data.size() >= 4) {
        LOG(INFO) << "Elan hello response received";
    }
    
    return true;
}

bool TouchscreenManager::initSitronix() {
    LOG(INFO) << "Initializing Sitronix touch controller";
    
    // Read status
    std::vector<uint8_t> regAddr = {0x00};
    std::vector<uint8_t> data;
    if (i2cWriteRead(regAddr, data, 8) && data.size() >= 8) {
        LOG(INFO) << "Sitronix status: " << (int)data[0];
    }
    
    return true;
}

// ============================================================================
// Touch event reading
// ============================================================================

bool TouchscreenManager::pollTouchEvents(std::vector<TouchEvent>& events) {
    std::lock_guard<std::mutex> lock(mLock);
    
    if (!mInitialized || mI2cFd < 0) {
        return false;
    }
    
    events.clear();
    
    bool result = false;
    switch (mActiveDevice.controller) {
        case TouchController::FT5X06:
        case TouchController::FT5426:
        case TouchController::FT5526:
            result = readFT5X06(events);
            break;
            
        case TouchController::FT6X06:
            result = readFT6X06(events);
            break;
            
        case TouchController::GT911:
        case TouchController::GT912:
        case TouchController::GT927:
        case TouchController::GT928:
        case TouchController::GT5688:
        case TouchController::GT1X:
        case TouchController::WS_GT911:
            result = readGT911(events);
            break;
            
        case TouchController::ILI2130:
        case TouchController::ILI2131:
        case TouchController::ILI2132:
        case TouchController::ILI251X:
            result = readILI251X(events);
            break;
            
        case TouchController::MXT224:
        case TouchController::MXT336:
        case TouchController::MXT540:
        case TouchController::ATMXT:
            result = readMXT(events);
            break;
            
        case TouchController::EKTF2127:
        case TouchController::EKTH3500:
            result = readElan(events);
            break;
            
        case TouchController::ST1232:
        case TouchController::ST1633:
            result = readSitronix(events);
            break;
            
        default:
            break;
    }
    
    // Apply calibration and orientation
    for (auto& event : events) {
        // Apply calibration
        event.x = (event.x - mCalMinX) * mActiveDevice.maxX / (mCalMaxX - mCalMinX);
        event.y = (event.y - mCalMinY) * mActiveDevice.maxY / (mCalMaxY - mCalMinY);
        
        // Clamp
        if (event.x < 0) event.x = 0;
        if (event.x > mActiveDevice.maxX) event.x = mActiveDevice.maxX;
        if (event.y < 0) event.y = 0;
        if (event.y > mActiveDevice.maxY) event.y = mActiveDevice.maxY;
        
        // Apply orientation
        if (mSwapXY) {
            std::swap(event.x, event.y);
        }
        if (mInvertX) {
            event.x = mActiveDevice.maxX - event.x;
        }
        if (mInvertY) {
            event.y = mActiveDevice.maxY - event.y;
        }
    }
    
    return result;
}

bool TouchscreenManager::readFT5X06(std::vector<TouchEvent>& events) {
    // Read touch count
    std::vector<uint8_t> regAddr = {FT5X06_REG_TD_STATUS};
    std::vector<uint8_t> data;
    
    if (!i2cWriteRead(regAddr, data, 1) || data.size() < 1) {
        return false;
    }
    
    int touchCount = data[0] & 0x0F;
    if (touchCount > 10) touchCount = 10;  // Sanity check
    
    if (touchCount == 0) {
        return true;  // No touches, but successful read
    }
    
    // Read touch data
    regAddr = {FT5X06_REG_TOUCH_START};
    if (!i2cWriteRead(regAddr, data, touchCount * 6) || data.size() < (size_t)(touchCount * 6)) {
        return false;
    }
    
    // Parse touch points
    for (int i = 0; i < touchCount; i++) {
        int offset = i * 6;
        
        TouchEvent event;
        event.id = (data[offset + 2] >> 4) & 0x0F;
        event.x = ((data[offset] & 0x0F) << 8) | data[offset + 1];
        event.y = ((data[offset + 2] & 0x0F) << 8) | data[offset + 3];
        event.pressure = data[offset + 4];
        event.touchMajor = data[offset + 5];
        event.touchMinor = data[offset + 5];
        event.active = ((data[offset] >> 6) & 0x03) != 0x01;  // Not lift-up
        
        events.push_back(event);
    }
    
    return true;
}

bool TouchscreenManager::readFT6X06(std::vector<TouchEvent>& events) {
    // FT6x06 uses same protocol but max 2 touches
    return readFT5X06(events);
}

bool TouchscreenManager::readGT911(std::vector<TouchEvent>& events) {
    // Read status register
    std::vector<uint8_t> regAddr = {0x81, 0x4E};  // Status register
    std::vector<uint8_t> data;
    
    if (!i2cWriteRead(regAddr, data, 1) || data.size() < 1) {
        return false;
    }
    
    uint8_t status = data[0];
    if (!(status & 0x80)) {
        return true;  // No data ready
    }
    
    int touchCount = status & 0x0F;
    if (touchCount > 10) touchCount = 10;
    
    // Read touch points
    if (touchCount > 0) {
        regAddr = {0x81, 0x4F};  // First touch point
        if (!i2cWriteRead(regAddr, data, touchCount * 8) || data.size() < (size_t)(touchCount * 8)) {
            return false;
        }
        
        for (int i = 0; i < touchCount; i++) {
            int offset = i * 8;
            
            TouchEvent event;
            event.id = data[offset];
            event.x = data[offset + 1] | (data[offset + 2] << 8);
            event.y = data[offset + 3] | (data[offset + 4] << 8);
            event.touchMajor = data[offset + 5] | (data[offset + 6] << 8);
            event.touchMinor = event.touchMajor;
            event.pressure = 50;  // GT911 doesn't report pressure
            event.active = true;
            
            events.push_back(event);
        }
    }
    
    // Clear status
    std::vector<uint8_t> clear = {0x81, 0x4E, 0x00};
    i2cWrite(clear);
    
    return true;
}

bool TouchscreenManager::readGT9XX(std::vector<TouchEvent>& events) {
    return readGT911(events);  // Same protocol
}

bool TouchscreenManager::readILI251X(std::vector<TouchEvent>& events) {
    // Read touch status
    std::vector<uint8_t> cmd = {0x10};  // Read touch info
    std::vector<uint8_t> data;
    
    if (!i2cWriteRead(cmd, data, 1) || data.size() < 1) {
        return false;
    }
    
    int touchCount = data[0];
    if (touchCount == 0 || touchCount > 10) {
        return true;
    }
    
    // Read touch data
    cmd = {0x10};
    if (!i2cWriteRead(cmd, data, 1 + touchCount * 5) || data.size() < (size_t)(1 + touchCount * 5)) {
        return false;
    }
    
    for (int i = 0; i < touchCount; i++) {
        int offset = 1 + i * 5;
        
        TouchEvent event;
        event.id = (data[offset] & 0x3F) >> 2;
        event.x = ((data[offset] & 0x03) << 8) | data[offset + 1];
        event.y = ((data[offset + 2] & 0x03) << 8) | data[offset + 3];
        event.pressure = data[offset + 4];
        event.touchMajor = 20;
        event.touchMinor = 20;
        event.active = true;
        
        events.push_back(event);
    }
    
    return true;
}

bool TouchscreenManager::readMXT(std::vector<TouchEvent>& events) {
    // mXT uses message-based protocol
    // This is simplified; real implementation would track object table
    return true;
}

bool TouchscreenManager::readElan(std::vector<TouchEvent>& events) {
    std::vector<uint8_t> data;
    
    if (!i2cRead(data, 34) || data.size() < 34) {
        return false;
    }
    
    if (data[0] != 0x55 || data[1] != 0x55) {
        return false;  // Invalid report
    }
    
    int touchCount = data[2] & 0x0F;
    
    for (int i = 0; i < touchCount && i < 5; i++) {
        int offset = 3 + i * 6;
        
        TouchEvent event;
        event.id = i;
        event.x = (data[offset] << 4) | ((data[offset + 2] & 0xF0) >> 4);
        event.y = (data[offset + 1] << 4) | (data[offset + 2] & 0x0F);
        event.pressure = data[offset + 3];
        event.touchMajor = data[offset + 4];
        event.touchMinor = data[offset + 5];
        event.active = true;
        
        events.push_back(event);
    }
    
    return true;
}

bool TouchscreenManager::readSitronix(std::vector<TouchEvent>& events) {
    std::vector<uint8_t> regAddr = {0x00};
    std::vector<uint8_t> data;
    
    if (!i2cWriteRead(regAddr, data, 16) || data.size() < 16) {
        return false;
    }
    
    int touchCount = data[0] & 0x0F;
    
    for (int i = 0; i < touchCount && i < 2; i++) {
        int offset = 2 + i * 4;
        
        TouchEvent event;
        event.id = i;
        event.x = ((data[offset] & 0x70) << 4) | data[offset + 1];
        event.y = ((data[offset] & 0x07) << 8) | data[offset + 2];
        event.pressure = data[offset + 3];
        event.touchMajor = 20;
        event.touchMinor = 20;
        event.active = (data[offset] & 0x80) != 0;
        
        events.push_back(event);
    }
    
    return true;
}

// ============================================================================
// I2C Communication
// ============================================================================

bool TouchscreenManager::i2cOpen(uint8_t bus, uint8_t addr) {
    char path[32];
    snprintf(path, sizeof(path), "%s%d", kI2cDevPath, bus);
    
    mI2cFd = open(path, O_RDWR);
    if (mI2cFd < 0) {
        LOG(WARNING) << "Cannot open I2C bus: " << path;
        return false;
    }
    
    if (ioctl(mI2cFd, I2C_SLAVE, addr) < 0) {
        LOG(WARNING) << "Cannot set I2C address: 0x" << std::hex << (int)addr;
        close(mI2cFd);
        mI2cFd = -1;
        return false;
    }
    
    return true;
}

void TouchscreenManager::i2cClose() {
    if (mI2cFd >= 0) {
        close(mI2cFd);
        mI2cFd = -1;
    }
}

bool TouchscreenManager::i2cWrite(const std::vector<uint8_t>& data) {
    if (mI2cFd < 0 || data.empty()) return false;
    return write(mI2cFd, data.data(), data.size()) == (ssize_t)data.size();
}

bool TouchscreenManager::i2cRead(std::vector<uint8_t>& data, size_t len) {
    if (mI2cFd < 0 || len == 0) return false;
    data.resize(len);
    ssize_t ret = read(mI2cFd, data.data(), len);
    if (ret < 0) {
        data.clear();
        return false;
    }
    data.resize(ret);
    return true;
}

bool TouchscreenManager::i2cWriteRead(const std::vector<uint8_t>& writeData,
                                       std::vector<uint8_t>& readData, size_t readLen) {
    if (!i2cWrite(writeData)) return false;
    return i2cRead(readData, readLen);
}

// ============================================================================
// Configuration
// ============================================================================

bool TouchscreenManager::resetController() {
    if (mActiveDevice.resetGpio <= 0) {
        return false;
    }
    
    char gpioPath[128];
    
    // Export GPIO
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd >= 0) {
        char gpio[8];
        snprintf(gpio, sizeof(gpio), "%d", mActiveDevice.resetGpio);
        write(fd, gpio, strlen(gpio));
        close(fd);
    }
    
    // Set direction
    snprintf(gpioPath, sizeof(gpioPath), "%s/gpio%d/direction", kGpioPath, mActiveDevice.resetGpio);
    fd = open(gpioPath, O_WRONLY);
    if (fd >= 0) {
        write(fd, "out", 3);
        close(fd);
    }
    
    // Toggle reset
    snprintf(gpioPath, sizeof(gpioPath), "%s/gpio%d/value", kGpioPath, mActiveDevice.resetGpio);
    fd = open(gpioPath, O_WRONLY);
    if (fd >= 0) {
        write(fd, "0", 1);
        usleep(10000);  // 10ms low
        write(fd, "1", 1);
        usleep(50000);  // 50ms high
        close(fd);
    }
    
    LOG(INFO) << "Touch controller reset via GPIO " << mActiveDevice.resetGpio;
    return true;
}

bool TouchscreenManager::setCalibration(int32_t minX, int32_t maxX, int32_t minY, int32_t maxY) {
    std::lock_guard<std::mutex> lock(mLock);
    mCalMinX = minX;
    mCalMaxX = maxX;
    mCalMinY = minY;
    mCalMaxY = maxY;
    return true;
}

bool TouchscreenManager::setOrientation(bool invertX, bool invertY, bool swapXY) {
    std::lock_guard<std::mutex> lock(mLock);
    mInvertX = invertX;
    mInvertY = invertY;
    mSwapXY = swapXY;
    return true;
}

bool TouchscreenManager::setTouchThreshold(uint8_t threshold) {
    std::lock_guard<std::mutex> lock(mLock);
    
    switch (mActiveDevice.controller) {
        case TouchController::FT5X06:
        case TouchController::FT6X06: {
            std::vector<uint8_t> cmd = {FT5X06_REG_ID_G_THGROUP, threshold};
            return i2cWrite(cmd);
        }
        default:
            return false;
    }
}

std::vector<std::string> TouchscreenManager::getSupportedControllers() {
    std::vector<std::string> controllers;
    for (const auto& device : kSupportedTouchControllers) {
        controllers.push_back(device.name);
    }
    return controllers;
}

std::string TouchscreenManager::getActiveController() {
    return mInitialized ? mActiveDevice.name : "";
}

bool TouchscreenManager::getTouchInfo(int32_t& maxX, int32_t& maxY, int32_t& maxTouches) {
    if (!mInitialized) return false;
    maxX = mActiveDevice.maxX;
    maxY = mActiveDevice.maxY;
    maxTouches = mActiveDevice.maxTouches;
    return true;
}

void TouchscreenManager::startInputThread() {
    if (mRunning.load()) return;
    mRunning.store(true);
    mInputThread = std::thread(&TouchscreenManager::inputThreadFunc, this);
}

void TouchscreenManager::stopInputThread() {
    mRunning.store(false);
    if (mInputThread.joinable()) {
        mInputThread.join();
    }
}

void TouchscreenManager::inputThreadFunc() {
    LOG(INFO) << "Touch input thread started";
    
    while (mRunning.load()) {
        std::vector<TouchEvent> events;
        if (pollTouchEvents(events)) {
            // Events would be sent to input subsystem
            // In practice, this is handled by the kernel driver
        }
        usleep(10000);  // 100Hz polling
    }
    
    LOG(INFO) << "Touch input thread stopped";
}

}  // namespace implementation
}  // namespace input
}  // namespace hardware
}  // namespace android
}  // namespace aidl
