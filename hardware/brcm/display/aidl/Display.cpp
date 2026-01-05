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

#define LOG_TAG "DisplayHAL"

#include "Display.h"

#include <android-base/logging.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstring>

namespace aidl {
namespace android {
namespace hardware {
namespace graphics {
namespace composer3 {
namespace implementation {

// MIPI DSI sysfs paths
static const char* kMipiDsiPath = "/sys/class/drm/card0-DSI-1";
static const char* kMipiBacklightPath = "/sys/class/backlight/";

// SPI device paths
static const char* kSpiDevPath = "/dev/spidev";

// GPIO base path
static const char* kGpioBasePath = "/sys/class/gpio";

DisplayManager& DisplayManager::getInstance() {
    static DisplayManager instance;
    return instance;
}

DisplayManager::DisplayManager()
    : mActiveDisplayType(DisplayType::HDMI),
      mDisplayEnabled(false),
      mBacklightLevel(255),
      mDsiFd(-1),
      mSpiFd(-1),
      mDcGpioFd(-1),
      mResetGpioFd(-1) {
    LOG(INFO) << "DisplayManager initialized";
}

DisplayManager::~DisplayManager() {
    if (mDsiFd >= 0) close(mDsiFd);
    if (mSpiFd >= 0) close(mSpiFd);
    if (mDcGpioFd >= 0) close(mDcGpioFd);
    if (mResetGpioFd >= 0) close(mResetGpioFd);
}

// ============================================================================
// MIPI DSI Display Functions
// ============================================================================

bool DisplayManager::initMipiDisplay(const std::string& panelName) {
    std::lock_guard<std::mutex> lock(mLock);
    
    LOG(INFO) << "Initializing MIPI display: " << panelName;
    
    // Find panel configuration
    const MipiPanelInfo* panel = nullptr;
    for (const auto& p : kSupportedMipiPanels) {
        if (p.name == panelName) {
            panel = &p;
            break;
        }
    }
    
    if (!panel) {
        LOG(ERROR) << "Unknown MIPI panel: " << panelName;
        return false;
    }
    
    // Configure DSI controller
    if (!configureDsiController(*panel)) {
        LOG(ERROR) << "Failed to configure DSI controller";
        return false;
    }
    
    // Configure timing
    if (!configureMipiTiming(*panel)) {
        LOG(ERROR) << "Failed to configure MIPI timing";
        return false;
    }
    
    mActiveDisplayType = DisplayType::MIPI_DSI;
    mActivePanelName = panelName;
    mDisplayEnabled = true;
    
    LOG(INFO) << "MIPI display initialized: " << panel->width << "x" << panel->height;
    return true;
}

bool DisplayManager::configureDsiController(const MipiPanelInfo& panel) {
    // Open DSI device
    std::string dsiPath = std::string(kMipiDsiPath) + "/enabled";
    int fd = open(dsiPath.c_str(), O_WRONLY);
    if (fd < 0) {
        LOG(WARNING) << "Could not open DSI control: " << dsiPath;
        // Try alternative path for RPi5
        dsiPath = "/sys/class/drm/card1-DSI-1/enabled";
        fd = open(dsiPath.c_str(), O_WRONLY);
        if (fd < 0) {
            LOG(ERROR) << "No DSI controller found";
            return false;
        }
    }
    
    // Enable DSI output
    write(fd, "on", 2);
    close(fd);
    
    // Configure lane count via device tree overlay (handled at boot)
    // Configure format via device tree overlay (handled at boot)
    
    return true;
}

bool DisplayManager::configureMipiTiming(const MipiPanelInfo& panel) {
    // MIPI timing is typically configured via device tree
    // This function can send panel-specific initialization commands
    
    LOG(INFO) << "Configuring MIPI timing for " << panel.name;
    LOG(INFO) << "  Resolution: " << panel.width << "x" << panel.height;
    LOG(INFO) << "  Lanes: " << panel.lanes;
    LOG(INFO) << "  Refresh: " << panel.refreshRate << "Hz";
    LOG(INFO) << "  H-timing: " << panel.hsync << "/" << panel.hbp << "/" << panel.hfp;
    LOG(INFO) << "  V-timing: " << panel.vsync << "/" << panel.vbp << "/" << panel.vfp;
    
    return true;
}

bool DisplayManager::enableMipiDisplay(bool enable) {
    std::lock_guard<std::mutex> lock(mLock);
    
    std::string enablePath = std::string(kMipiDsiPath) + "/enabled";
    int fd = open(enablePath.c_str(), O_WRONLY);
    if (fd < 0) {
        enablePath = "/sys/class/drm/card1-DSI-1/enabled";
        fd = open(enablePath.c_str(), O_WRONLY);
        if (fd < 0) {
            LOG(ERROR) << "Cannot control DSI display";
            return false;
        }
    }
    
    const char* value = enable ? "on" : "off";
    write(fd, value, strlen(value));
    close(fd);
    
    mDisplayEnabled = enable;
    return true;
}

std::vector<std::string> DisplayManager::getSupportedMipiPanels() {
    std::vector<std::string> panels;
    for (const auto& p : kSupportedMipiPanels) {
        panels.push_back(p.name);
    }
    return panels;
}

bool DisplayManager::sendDsiCommand(uint8_t cmd, const std::vector<uint8_t>& data) {
    // DSI commands are sent via DRM subsystem
    // This is a placeholder for panel-specific init sequences
    return true;
}

// ============================================================================
// SPI Display Functions
// ============================================================================

bool DisplayManager::initSpiDisplay(const std::string& displayName) {
    std::lock_guard<std::mutex> lock(mLock);
    
    LOG(INFO) << "Initializing SPI display: " << displayName;
    
    // Find display configuration
    const SpiDisplayInfo* display = nullptr;
    for (const auto& d : kSupportedSpiDisplays) {
        if (d.name == displayName) {
            display = &d;
            break;
        }
    }
    
    if (!display) {
        LOG(ERROR) << "Unknown SPI display: " << displayName;
        return false;
    }
    
    // Configure SPI controller
    if (!configureSpiController(*display)) {
        LOG(ERROR) << "Failed to configure SPI controller";
        return false;
    }
    
    // Initialize display
    if (!configureSpiDisplay(*display)) {
        LOG(ERROR) << "Failed to initialize display";
        return false;
    }
    
    mActiveDisplayType = DisplayType::SPI_TFT;
    mActivePanelName = displayName;
    mDisplayEnabled = true;
    
    LOG(INFO) << "SPI display initialized: " << display->controller 
              << " " << display->width << "x" << display->height;
    return true;
}

bool DisplayManager::configureSpiController(const SpiDisplayInfo& display) {
    // Open SPI device
    char spiPath[64];
    snprintf(spiPath, sizeof(spiPath), "%s%u.%u", 
             kSpiDevPath, display.busNum, display.chipSelect);
    
    mSpiFd = open(spiPath, O_RDWR);
    if (mSpiFd < 0) {
        LOG(ERROR) << "Cannot open SPI device: " << spiPath;
        return false;
    }
    
    // Configure SPI mode
    uint8_t mode = SPI_MODE_0;
    if (ioctl(mSpiFd, SPI_IOC_WR_MODE, &mode) < 0) {
        LOG(ERROR) << "Cannot set SPI mode";
        return false;
    }
    
    // Configure bits per word
    uint8_t bits = 8;
    if (ioctl(mSpiFd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        LOG(ERROR) << "Cannot set SPI bits";
        return false;
    }
    
    // Configure speed
    uint32_t speed = display.speedHz;
    if (ioctl(mSpiFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        LOG(ERROR) << "Cannot set SPI speed";
        return false;
    }
    
    LOG(INFO) << "SPI configured: " << spiPath << " @ " << speed << "Hz";
    
    // Export and configure DC GPIO
    if (display.dcGpio > 0) {
        char gpioPath[128];
        snprintf(gpioPath, sizeof(gpioPath), "%s/gpio%u/direction", 
                 kGpioBasePath, display.dcGpio);
        int fd = open(gpioPath, O_WRONLY);
        if (fd >= 0) {
            write(fd, "out", 3);
            close(fd);
        }
        
        snprintf(gpioPath, sizeof(gpioPath), "%s/gpio%u/value", 
                 kGpioBasePath, display.dcGpio);
        mDcGpioFd = open(gpioPath, O_WRONLY);
    }
    
    // Export and configure Reset GPIO
    if (display.resetGpio > 0) {
        char gpioPath[128];
        snprintf(gpioPath, sizeof(gpioPath), "%s/gpio%u/direction", 
                 kGpioBasePath, display.resetGpio);
        int fd = open(gpioPath, O_WRONLY);
        if (fd >= 0) {
            write(fd, "out", 3);
            close(fd);
        }
        
        snprintf(gpioPath, sizeof(gpioPath), "%s/gpio%u/value", 
                 kGpioBasePath, display.resetGpio);
        mResetGpioFd = open(gpioPath, O_WRONLY);
    }
    
    return true;
}

bool DisplayManager::configureSpiDisplay(const SpiDisplayInfo& display) {
    // Hardware reset
    if (mResetGpioFd >= 0) {
        write(mResetGpioFd, "0", 1);
        usleep(10000);  // 10ms
        write(mResetGpioFd, "1", 1);
        usleep(120000); // 120ms
    }
    
    // Send controller-specific initialization
    if (display.controller == "ILI9341") {
        // ILI9341 init sequence
        sendSpiCommand(0x01);  // Software reset
        usleep(5000);
        sendSpiCommand(0x28);  // Display off
        sendSpiCommand(0xCF);  // Power control B
        sendSpiData({0x00, 0xC1, 0x30});
        sendSpiCommand(0xED);  // Power on sequence control
        sendSpiData({0x64, 0x03, 0x12, 0x81});
        sendSpiCommand(0xE8);  // Driver timing control A
        sendSpiData({0x85, 0x00, 0x78});
        sendSpiCommand(0xCB);  // Power control A
        sendSpiData({0x39, 0x2C, 0x00, 0x34, 0x02});
        sendSpiCommand(0xF7);  // Pump ratio control
        sendSpiData({0x20});
        sendSpiCommand(0xEA);  // Driver timing control B
        sendSpiData({0x00, 0x00});
        sendSpiCommand(0xC0);  // Power control 1
        sendSpiData({0x23});
        sendSpiCommand(0xC1);  // Power control 2
        sendSpiData({0x10});
        sendSpiCommand(0xC5);  // VCOM control 1
        sendSpiData({0x3E, 0x28});
        sendSpiCommand(0xC7);  // VCOM control 2
        sendSpiData({0x86});
        sendSpiCommand(0x36);  // Memory access control
        sendSpiData({0x48});
        sendSpiCommand(0x3A);  // Pixel format
        sendSpiData({0x55});   // 16-bit
        sendSpiCommand(0xB1);  // Frame rate control
        sendSpiData({0x00, 0x18});
        sendSpiCommand(0xB6);  // Display function control
        sendSpiData({0x08, 0x82, 0x27});
        sendSpiCommand(0xF2);  // 3Gamma function disable
        sendSpiData({0x00});
        sendSpiCommand(0x26);  // Gamma curve selected
        sendSpiData({0x01});
        sendSpiCommand(0x11);  // Sleep out
        usleep(120000);
        sendSpiCommand(0x29);  // Display on
    } else if (display.controller == "ST7789") {
        // ST7789 init sequence
        sendSpiCommand(0x01);  // Software reset
        usleep(150000);
        sendSpiCommand(0x11);  // Sleep out
        usleep(500000);
        sendSpiCommand(0x3A);  // Interface pixel format
        sendSpiData({0x55});   // 16-bit
        sendSpiCommand(0x36);  // Memory data access control
        sendSpiData({0x00});
        sendSpiCommand(0xB2);  // Porch setting
        sendSpiData({0x0C, 0x0C, 0x00, 0x33, 0x33});
        sendSpiCommand(0xB7);  // Gate control
        sendSpiData({0x35});
        sendSpiCommand(0xBB);  // VCOM setting
        sendSpiData({0x19});
        sendSpiCommand(0xC0);  // LCM control
        sendSpiData({0x2C});
        sendSpiCommand(0xC2);  // VDV and VRH command enable
        sendSpiData({0x01});
        sendSpiCommand(0xC3);  // VRH set
        sendSpiData({0x12});
        sendSpiCommand(0xC4);  // VDV set
        sendSpiData({0x20});
        sendSpiCommand(0xC6);  // Frame rate control
        sendSpiData({0x0F});
        sendSpiCommand(0xD0);  // Power control 1
        sendSpiData({0xA4, 0xA1});
        sendSpiCommand(0x21);  // Display inversion on
        sendSpiCommand(0x29);  // Display on
    } else if (display.controller == "ST7735") {
        // ST7735 init sequence
        sendSpiCommand(0x01);  // Software reset
        usleep(150000);
        sendSpiCommand(0x11);  // Sleep out
        usleep(500000);
        sendSpiCommand(0xB1);  // Frame rate control
        sendSpiData({0x01, 0x2C, 0x2D});
        sendSpiCommand(0xB2);
        sendSpiData({0x01, 0x2C, 0x2D});
        sendSpiCommand(0xB3);
        sendSpiData({0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D});
        sendSpiCommand(0xB4);  // Display inversion
        sendSpiData({0x07});
        sendSpiCommand(0xC0);  // Power control
        sendSpiData({0xA2, 0x02, 0x84});
        sendSpiCommand(0xC1);
        sendSpiData({0xC5});
        sendSpiCommand(0xC2);
        sendSpiData({0x0A, 0x00});
        sendSpiCommand(0xC3);
        sendSpiData({0x8A, 0x2A});
        sendSpiCommand(0xC4);
        sendSpiData({0x8A, 0xEE});
        sendSpiCommand(0xC5);  // VCOM
        sendSpiData({0x0E});
        sendSpiCommand(0x36);  // MX, MY, RGB mode
        sendSpiData({0xC8});
        sendSpiCommand(0x3A);  // Color mode
        sendSpiData({0x05});   // 16-bit
        sendSpiCommand(0x29);  // Display on
    } else if (display.controller == "SSD1306") {
        // SSD1306 OLED init sequence
        sendSpiCommand(0xAE);  // Display off
        sendSpiCommand(0xD5);  // Set display clock
        sendSpiCommand(0x80);
        sendSpiCommand(0xA8);  // Set multiplex
        sendSpiCommand(0x3F);
        sendSpiCommand(0xD3);  // Set display offset
        sendSpiCommand(0x00);
        sendSpiCommand(0x40);  // Set start line
        sendSpiCommand(0x8D);  // Charge pump
        sendSpiCommand(0x14);
        sendSpiCommand(0x20);  // Memory mode
        sendSpiCommand(0x00);
        sendSpiCommand(0xA1);  // Seg remap
        sendSpiCommand(0xC8);  // COM scan direction
        sendSpiCommand(0xDA);  // COM pins
        sendSpiCommand(0x12);
        sendSpiCommand(0x81);  // Contrast
        sendSpiCommand(0xCF);
        sendSpiCommand(0xD9);  // Pre-charge
        sendSpiCommand(0xF1);
        sendSpiCommand(0xDB);  // VCOM detect
        sendSpiCommand(0x40);
        sendSpiCommand(0xA4);  // Resume RAM
        sendSpiCommand(0xA6);  // Normal display
        sendSpiCommand(0xAF);  // Display on
    } else if (display.controller == "GC9A01") {
        // GC9A01 round display init
        sendSpiCommand(0xEF);
        sendSpiCommand(0xEB); sendSpiData({0x14});
        sendSpiCommand(0xFE);
        sendSpiCommand(0xEF);
        sendSpiCommand(0xEB); sendSpiData({0x14});
        sendSpiCommand(0x84); sendSpiData({0x40});
        sendSpiCommand(0x85); sendSpiData({0xFF});
        sendSpiCommand(0x86); sendSpiData({0xFF});
        sendSpiCommand(0x87); sendSpiData({0xFF});
        sendSpiCommand(0x88); sendSpiData({0x0A});
        sendSpiCommand(0x89); sendSpiData({0x21});
        sendSpiCommand(0x8A); sendSpiData({0x00});
        sendSpiCommand(0x8B); sendSpiData({0x80});
        sendSpiCommand(0x8C); sendSpiData({0x01});
        sendSpiCommand(0x8D); sendSpiData({0x01});
        sendSpiCommand(0x8E); sendSpiData({0xFF});
        sendSpiCommand(0x8F); sendSpiData({0xFF});
        sendSpiCommand(0x3A); sendSpiData({0x55});
        sendSpiCommand(0x11);
        usleep(120000);
        sendSpiCommand(0x29);
    }
    // Add more controllers as needed...
    
    LOG(INFO) << "Display controller " << display.controller << " initialized";
    return true;
}

bool DisplayManager::enableSpiDisplay(bool enable) {
    std::lock_guard<std::mutex> lock(mLock);
    
    if (mSpiFd < 0) {
        LOG(ERROR) << "SPI display not initialized";
        return false;
    }
    
    if (enable) {
        sendSpiCommand(0x29);  // Display on
    } else {
        sendSpiCommand(0x28);  // Display off
    }
    
    mDisplayEnabled = enable;
    return true;
}

std::vector<std::string> DisplayManager::getSupportedSpiDisplays() {
    std::vector<std::string> displays;
    for (const auto& d : kSupportedSpiDisplays) {
        displays.push_back(d.name);
    }
    return displays;
}

bool DisplayManager::sendSpiCommand(uint8_t cmd) {
    if (mSpiFd < 0) return false;
    
    // Set DC low for command
    if (mDcGpioFd >= 0) {
        write(mDcGpioFd, "0", 1);
    }
    
    write(mSpiFd, &cmd, 1);
    return true;
}

bool DisplayManager::sendSpiData(const std::vector<uint8_t>& data) {
    if (mSpiFd < 0) return false;
    
    // Set DC high for data
    if (mDcGpioFd >= 0) {
        write(mDcGpioFd, "1", 1);
    }
    
    write(mSpiFd, data.data(), data.size());
    return true;
}

// ============================================================================
// Common Display Functions
// ============================================================================

bool DisplayManager::setBacklight(uint32_t brightness) {
    std::lock_guard<std::mutex> lock(mLock);
    
    if (brightness > 255) brightness = 255;
    mBacklightLevel = brightness;
    
    // Try various backlight interfaces
    const char* backlightPaths[] = {
        "/sys/class/backlight/rpi_backlight/brightness",
        "/sys/class/backlight/10-0045/brightness",
        "/sys/class/backlight/backlight/brightness",
        "/sys/class/leds/lcd-backlight/brightness",
    };
    
    for (const auto& path : backlightPaths) {
        int fd = open(path, O_WRONLY);
        if (fd >= 0) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%u", brightness);
            write(fd, buf, strlen(buf));
            close(fd);
            LOG(INFO) << "Backlight set to " << brightness << " via " << path;
            return true;
        }
    }
    
    LOG(WARNING) << "No backlight control found";
    return false;
}

bool DisplayManager::setRotation(uint8_t rotation) {
    std::lock_guard<std::mutex> lock(mLock);
    
    // Rotation is typically handled by the display controller
    uint8_t madctl = 0;
    switch (rotation) {
        case 0:   madctl = 0x00; break;
        case 90:  madctl = 0x60; break;
        case 180: madctl = 0xC0; break;
        case 270: madctl = 0xA0; break;
        default:  return false;
    }
    
    if (mActiveDisplayType == DisplayType::SPI_TFT && mSpiFd >= 0) {
        sendSpiCommand(0x36);  // MADCTL
        sendSpiData({madctl});
        return true;
    }
    
    return false;
}

bool DisplayManager::setPowerMode(bool on) {
    if (mActiveDisplayType == DisplayType::MIPI_DSI) {
        return enableMipiDisplay(on);
    } else if (mActiveDisplayType == DisplayType::SPI_TFT) {
        return enableSpiDisplay(on);
    }
    return false;
}

}  // namespace implementation
}  // namespace composer3
}  // namespace graphics
}  // namespace hardware
}  // namespace android
}  // namespace aidl
