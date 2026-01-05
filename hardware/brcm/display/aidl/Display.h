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

#include <aidl/android/hardware/graphics/composer3/BnComposerClient.h>
#include <vector>
#include <map>
#include <mutex>
#include <string>

namespace aidl {
namespace android {
namespace hardware {
namespace graphics {
namespace composer3 {
namespace implementation {

// Display types supported
enum class DisplayType {
    HDMI,
    MIPI_DSI,
    SPI_TFT,
};

// MIPI DSI Panel information
struct MipiPanelInfo {
    std::string name;
    uint32_t width;
    uint32_t height;
    uint32_t lanes;
    uint32_t format;  // MIPI_DSI_FMT_*
    uint32_t mode;    // MIPI_DSI_MODE_*
    uint32_t hsync;
    uint32_t vsync;
    uint32_t hbp;
    uint32_t hfp;
    uint32_t vbp;
    uint32_t vfp;
    uint32_t refreshRate;
};

// SPI Display information
struct SpiDisplayInfo {
    std::string name;
    std::string controller;  // ILI9341, ST7789, etc.
    uint32_t width;
    uint32_t height;
    uint32_t busNum;
    uint32_t chipSelect;
    uint32_t speedHz;
    uint32_t dcGpio;
    uint32_t resetGpio;
    uint32_t backlightGpio;
    uint8_t rotation;
};

// Supported MIPI DSI panels
static const std::vector<MipiPanelInfo> kSupportedMipiPanels = {
    // Official Raspberry Pi displays
    {"rpi_official_7inch", 800, 480, 2, 0, 0, 2, 2, 44, 44, 19, 21, 60},
    {"rpi_official_touch_2", 720, 1280, 4, 0, 0, 5, 5, 30, 30, 20, 20, 60},
    
    // Waveshare displays
    {"waveshare_4inch", 480, 800, 2, 0, 0, 10, 10, 20, 20, 10, 10, 60},
    {"waveshare_5inch", 800, 480, 2, 0, 0, 48, 2, 40, 40, 13, 31, 60},
    {"waveshare_5inch_amoled", 960, 544, 4, 0, 0, 40, 10, 40, 40, 10, 10, 60},
    {"waveshare_7inch", 800, 480, 2, 0, 0, 48, 2, 40, 40, 13, 31, 60},
    {"waveshare_7inch_c", 1024, 600, 4, 0, 0, 100, 2, 100, 100, 10, 10, 60},
    {"waveshare_8inch", 1280, 800, 4, 0, 0, 20, 10, 20, 20, 5, 5, 60},
    {"waveshare_10inch", 1280, 800, 4, 0, 0, 20, 10, 40, 40, 10, 10, 60},
    {"waveshare_11inch", 1560, 1440, 4, 0, 0, 40, 20, 40, 40, 20, 20, 60},
    {"waveshare_13inch", 1920, 1080, 4, 0, 0, 44, 5, 148, 88, 36, 4, 60},
    
    // Pimoroni displays
    {"pimoroni_hyperpixel4", 800, 480, 2, 0, 0, 48, 2, 40, 40, 13, 31, 60},
    {"pimoroni_hyperpixel4_square", 720, 720, 4, 0, 0, 20, 20, 20, 20, 20, 20, 60},
    
    // Adafruit displays
    {"adafruit_2_8inch", 320, 240, 1, 0, 0, 10, 10, 10, 10, 10, 10, 60},
    
    // Generic panels
    {"generic_480x800", 480, 800, 2, 0, 0, 10, 10, 20, 20, 10, 10, 60},
    {"generic_800x480", 800, 480, 2, 0, 0, 48, 2, 40, 40, 13, 31, 60},
    {"generic_1024x600", 1024, 600, 4, 0, 0, 100, 2, 100, 100, 10, 10, 60},
    {"generic_1280x720", 1280, 720, 4, 0, 0, 40, 5, 220, 110, 20, 5, 60},
    {"generic_1280x800", 1280, 800, 4, 0, 0, 20, 10, 40, 40, 10, 10, 60},
    {"generic_1920x1080", 1920, 1080, 4, 0, 0, 44, 5, 148, 88, 36, 4, 60},
};

// Supported SPI display controllers
static const std::vector<SpiDisplayInfo> kSupportedSpiDisplays = {
    // ILI9341 based displays (240x320)
    {"ili9341_240x320", "ILI9341", 240, 320, 0, 0, 32000000, 25, 24, 18, 0},
    {"ili9341_adafruit_2_8", "ILI9341", 240, 320, 0, 0, 32000000, 25, 24, 18, 0},
    
    // ILI9486 based displays (320x480)
    {"ili9486_320x480", "ILI9486", 320, 480, 0, 0, 16000000, 25, 24, 18, 0},
    {"ili9486_waveshare_3_5", "ILI9486", 320, 480, 0, 0, 16000000, 25, 24, 18, 0},
    
    // ILI9488 based displays (320x480)
    {"ili9488_320x480", "ILI9488", 320, 480, 0, 0, 32000000, 25, 24, 18, 0},
    
    // ST7735 based displays (128x160, 128x128)
    {"st7735_128x160", "ST7735", 128, 160, 0, 0, 32000000, 25, 24, 18, 0},
    {"st7735_128x128", "ST7735", 128, 128, 0, 0, 32000000, 25, 24, 18, 0},
    {"st7735_adafruit_1_8", "ST7735", 128, 160, 0, 0, 32000000, 25, 24, 18, 0},
    
    // ST7789 based displays (240x240, 240x320, 135x240)
    {"st7789_240x240", "ST7789", 240, 240, 0, 0, 62500000, 25, 24, 18, 0},
    {"st7789_240x320", "ST7789", 240, 320, 0, 0, 62500000, 25, 24, 18, 0},
    {"st7789_135x240", "ST7789", 135, 240, 0, 0, 62500000, 25, 24, 18, 0},
    {"st7789_pimoroni_1_3", "ST7789", 240, 240, 0, 0, 62500000, 25, 24, 18, 0},
    {"st7789_waveshare_1_3", "ST7789", 240, 240, 0, 0, 62500000, 25, 24, 18, 0},
    {"st7789_waveshare_1_54", "ST7789", 240, 240, 0, 0, 62500000, 25, 24, 18, 0},
    {"st7789_waveshare_2", "ST7789", 240, 320, 0, 0, 62500000, 25, 24, 18, 0},
    
    // SSD1306 OLED displays (128x64, 128x32)
    {"ssd1306_128x64", "SSD1306", 128, 64, 0, 0, 8000000, 25, 24, -1, 0},
    {"ssd1306_128x32", "SSD1306", 128, 32, 0, 0, 8000000, 25, 24, -1, 0},
    
    // SSD1351 OLED displays (128x128)
    {"ssd1351_128x128", "SSD1351", 128, 128, 0, 0, 20000000, 25, 24, -1, 0},
    
    // SH1106 OLED displays (128x64)
    {"sh1106_128x64", "SH1106", 128, 64, 0, 0, 8000000, 25, 24, -1, 0},
    
    // HX8357 based displays (320x480)
    {"hx8357_320x480", "HX8357", 320, 480, 0, 0, 32000000, 25, 24, 18, 0},
    {"hx8357_adafruit_3_5", "HX8357", 320, 480, 0, 0, 32000000, 25, 24, 18, 0},
    
    // GC9A01 round displays (240x240)
    {"gc9a01_240x240", "GC9A01", 240, 240, 0, 0, 62500000, 25, 24, 18, 0},
    {"gc9a01_waveshare_1_28", "GC9A01", 240, 240, 0, 0, 62500000, 25, 24, 18, 0},
};

class DisplayManager {
public:
    static DisplayManager& getInstance();
    
    // MIPI DSI functions
    bool initMipiDisplay(const std::string& panelName);
    bool configureMipiTiming(const MipiPanelInfo& panel);
    bool enableMipiDisplay(bool enable);
    std::vector<std::string> getSupportedMipiPanels();
    
    // SPI display functions
    bool initSpiDisplay(const std::string& displayName);
    bool configureSpiDisplay(const SpiDisplayInfo& display);
    bool enableSpiDisplay(bool enable);
    std::vector<std::string> getSupportedSpiDisplays();
    
    // Common functions
    bool setBacklight(uint32_t brightness);  // 0-255
    bool setRotation(uint8_t rotation);       // 0, 90, 180, 270
    bool setPowerMode(bool on);
    
private:
    DisplayManager();
    ~DisplayManager();
    
    std::mutex mLock;
    DisplayType mActiveDisplayType;
    std::string mActivePanelName;
    bool mDisplayEnabled;
    uint32_t mBacklightLevel;
    
    // MIPI DSI specific
    int mDsiFd;
    bool configureDsiController(const MipiPanelInfo& panel);
    bool sendDsiCommand(uint8_t cmd, const std::vector<uint8_t>& data);
    
    // SPI specific
    int mSpiFd;
    int mDcGpioFd;
    int mResetGpioFd;
    bool configureSpiController(const SpiDisplayInfo& display);
    bool sendSpiCommand(uint8_t cmd);
    bool sendSpiData(const std::vector<uint8_t>& data);
};

}  // namespace implementation
}  // namespace composer3
}  // namespace graphics
}  // namespace hardware
}  // namespace android
}  // namespace aidl
