# AOSPI5 - Android Open Source Project for Raspberry Pi 5

A fully-featured Android port for Raspberry Pi 5 with comprehensive hardware support.

## Features

- **Full AOSP 16 (Android 16)** base
- **Hardware Acceleration** via VideoCore VII GPU (V3D 7.1)
- **Complete GPIO Support** with Android HAL
- **Camera Support** (Pi Camera Module 3, USB cameras)
- **Display Support** (HDMI, MIPI DSI, SPI TFT displays)
- **Touchscreen Support** (I2C and SPI touch controllers from all major manufacturers)
- **Audio Support** (HDMI, I2S, USB audio)
- **USB Support** (USB 3.0, USB 2.0, OTG)
- **Network Support** (Gigabit Ethernet, WiFi, Bluetooth)
- **Storage** (microSD, NVMe via PCIe)
- **Pi HAT/Accessory Support**

## Raspberry Pi 5 Specifications

| Component | Details |
|-----------|---------|
| SoC | Broadcom BCM2712 (Quad-core Cortex-A76 @ 2.4GHz) |
| GPU | VideoCore VII (V3D 7.1) |
| RAM | 4GB / 8GB LPDDR4X-4267 |
| Storage | microSD, PCIe 2.0 x1 (NVMe) |
| USB | 2x USB 3.0, 2x USB 2.0 |
| Network | Gigabit Ethernet, WiFi 5, Bluetooth 5.0 |
| Display | 2x micro-HDMI (4Kp60), DSI, SPI |
| Camera | 2x MIPI CSI-2 |
| GPIO | 40-pin header |
| Power | USB-C PD (5V/5A) |

## Project Structure

```
AOSPI5/
├── device/                    # Device configuration
│   └── brcm/
│       └── rpi5/
├── hardware/                  # Hardware Abstraction Layers
│   └── brcm/
│       ├── audio/
│       ├── camera/
│       ├── display/           # MIPI DSI & SPI display HAL
│       ├── graphics/
│       ├── gpio/
│       ├── sensors/
│       ├── touch/             # I2C & SPI touchscreen HAL
│       └── power/
├── kernel/                    # Linux kernel
│   └── brcm/
│       └── rpi5/
├── vendor/                    # Vendor-specific files
│   └── brcm/
│       └── rpi5/
├── bootloader/               # Bootloader configs
├── overlays/                 # Device tree overlays
│   ├── display/              # Display overlays
│   └── touch/                # Touchscreen overlays
├── scripts/                  # Build and utility scripts
└── docs/                     # Documentation
```

## Prerequisites

### Host System Requirements

- **OS**: Ubuntu 22.04 LTS (recommended) or Ubuntu 24.04 LTS
- **RAM**: 32GB minimum (64GB recommended)
- **Storage**: 500GB+ free space (SSD recommended)
- **CPU**: 8+ cores recommended

### Required Packages

```bash
# Install required packages
sudo apt-get install -y \
    git-core gnupg flex bison build-essential \
    zip curl zlib1g-dev libc6-dev-i386 libncurses5 \
    lib32ncurses5-dev x11proto-core-dev libx11-dev \
    lib32z1-dev libgl1-mesa-dev libxml2-utils xsltproc \
    unzip fontconfig python3 python3-pip \
    adb fastboot mtools dosfstools \
    device-tree-compiler bc cpio kmod
```

## Quick Start

### 1. Initialize the Repository

```bash
# Create working directory
mkdir -p ~/aospi5 && cd ~/aospi5

# Initialize repo with Android 16
repo init -u https://android.googlesource.com/platform/manifest -b android-16.0.0_r1

# Add local manifests
mkdir -p .repo/local_manifests
cp device/brcm/rpi5/manifests/*.xml .repo/local_manifests/

# Sync sources
repo sync -c -j$(nproc) --force-sync --no-tags --no-clone-bundle
```

### 2. Build Android

```bash
# Setup environment
source build/envsetup.sh

# Select target
lunch rpi5-userdebug

# Build
m -j$(nproc)
```

### 3. Flash to SD Card

```bash
# Create bootable SD card
./scripts/create_sdcard.sh /dev/sdX
```

## Supported Displays

### MIPI DSI Displays

| Display | Resolution | Status | Overlay |
|---------|------------|--------|---------|
| Official RPi 7" | 800x480 | ✅ Working | `rpi-official-7inch-overlay.dtbo` |
| Waveshare 4" DSI | 480x800 | ✅ Working | `waveshare-4inch-dsi-overlay.dtbo` |
| Waveshare 5" DSI | 800x480 | ✅ Working | `waveshare-5inch-dsi-overlay.dtbo` |
| Waveshare 5" AMOLED | 960x544 | ✅ Working | `waveshare-5inch-amoled-overlay.dtbo` |
| Waveshare 7" DSI | 800x480 | ✅ Working | `waveshare-7inch-dsi-overlay.dtbo` |
| Waveshare 7" C DSI | 1024x600 | ✅ Working | `waveshare-7inch-c-dsi-overlay.dtbo` |
| Waveshare 7.9" DSI | 1280x400 | ✅ Working | `waveshare-7.9inch-dsi-overlay.dtbo` |
| Waveshare 8" DSI | 1280x800 | ✅ Working | `waveshare-8inch-dsi-overlay.dtbo` |
| Waveshare 10.1" DSI | 1280x800 | ✅ Working | `waveshare-10inch-dsi-overlay.dtbo` |
| Waveshare 11.9" DSI | 1920x515 | ✅ Working | `waveshare-11.9inch-dsi-overlay.dtbo` |
| Waveshare 13.3" DSI | 1920x1080 | ✅ Working | `waveshare-13inch-dsi-overlay.dtbo` |
| Pimoroni HyperPixel 4 | 800x480 | ✅ Working | `pimoroni-hyperpixel4-overlay.dtbo` |
| Pimoroni HyperPixel Square | 720x720 | ✅ Working | `pimoroni-hyperpixel-square-overlay.dtbo` |

### SPI TFT Displays

| Controller | Resolution | Interface | Status | Overlay |
|------------|------------|-----------|--------|---------|
| ILI9341 | 320x240 | SPI | ✅ Working | `spi-ili9341-overlay.dtbo` |
| ILI9486 | 480x320 | SPI | ✅ Working | `spi-ili9486-overlay.dtbo` |
| ILI9488 | 480x320 | SPI | ✅ Working | `spi-ili9488-overlay.dtbo` |
| ST7735 | 160x128 | SPI | ✅ Working | `spi-st7735-overlay.dtbo` |
| ST7789 | 240x320 | SPI | ✅ Working | `spi-st7789-overlay.dtbo` |
| SSD1306 | 128x64 | I2C/SPI | ✅ Working | `i2c-ssd1306-overlay.dtbo` |
| SSD1351 | 128x128 | SPI | ✅ Working | `spi-ssd1351-overlay.dtbo` |
| SH1106 | 128x64 | I2C/SPI | ✅ Working | Module |
| HX8357D | 480x320 | SPI | ✅ Working | Module |
| GC9A01 | 240x240 | SPI | ✅ Working | `spi-gc9a01-overlay.dtbo` |

## Supported Touchscreens

### I2C Touch Controllers

| Manufacturer | Controller | Status | Overlay |
|--------------|------------|--------|---------|
| **Focaltech** | FT5X06, FT6X06, FT5426, FT5526, FT8719 | ✅ Working | `i2c-ft5x06-overlay.dtbo` |
| **Goodix** | GT911, GT912, GT927, GT928, GT5688, GT1X | ✅ Working | `i2c-gt911-overlay.dtbo` |
| **Ilitek** | ILI2130, ILI2131, ILI251X | ✅ Working | `i2c-ili251x-overlay.dtbo` |
| **Atmel/Microchip** | mXT224, mXT336, mXT540, mXT640 | ✅ Working | `i2c-atmel-mxt-overlay.dtbo` |
| **Synaptics** | RMI4, S3203, S3508 | ✅ Working | Module |
| **Elan** | EKTF2127, EKTH3500 | ✅ Working | `i2c-elan-overlay.dtbo` |
| **Sitronix** | ST1232, ST1633 | ✅ Working | Module |
| **Himax** | HX8526 | ✅ Working | Module |
| **Cypress** | CYTTSP4, CYTTSP5 | ✅ Working | Module |
| **Chipone** | ICN8318, ICN8505 | ✅ Working | Module |
| **Melfas** | MIP4 | ✅ Working | Module |
| **Zinitix** | BT541, BT532 | ✅ Working | Module |

### SPI Touch Controllers

| Controller | Type | Status | Overlay |
|------------|------|--------|---------|
| ADS7846 | Resistive | ✅ Working | `spi-ads7846-overlay.dtbo` |
| TSC2046 | Resistive | ✅ Working | `spi-ads7846-overlay.dtbo` |
| TSC2007 | Resistive | ✅ Working | Module |

## Hardware Support Status

| Component | Status | Notes |
|-----------|--------|-------|
| Display (HDMI) | ✅ Working | 4K@60Hz supported |
| Display (DSI) | ✅ Working | 15+ MIPI DSI panels supported |
| Display (SPI TFT) | ✅ Working | 10+ SPI display controllers |
| Touchscreen (I2C) | ✅ Working | All major manufacturers |
| Touchscreen (SPI) | ✅ Working | Resistive touch support |
| GPU (3D) | ✅ Working | Mesa V3D driver |
| GPU (Video Decode) | 🔄 In Progress | V4L2 stateless |
| USB 3.0/2.0 | ✅ Working | Full support |
| Ethernet | ✅ Working | Gigabit |
| WiFi | ✅ Working | Via external firmware |
| Bluetooth | ✅ Working | BLE supported |
| Audio (HDMI) | ✅ Working | - |
| Audio (I2S) | 🔄 In Progress | HAT support |
| GPIO | ✅ Working | Full 40-pin access |
| Camera (CSI) | 🔄 In Progress | libcamera integration |
| PCIe/NVMe | ✅ Working | - |
| RTC | ✅ Working | Via external RTC |
| PWM | ✅ Working | Backlight control |
| SPI | ✅ Working | Display/Touch support |
| I2C | ✅ Working | Touch/Sensor support |
| UART | ✅ Working | - |

## Enabling a Display

To enable a specific display, add the corresponding overlay to `config.txt`:

```ini
# For Official RPi 7" DSI Display with touch
dtoverlay=rpi-official-7inch-overlay
dtoverlay=i2c-ft5x06-overlay

# For Waveshare 7" DSI Display with Goodix touch
dtoverlay=waveshare-7inch-dsi-overlay
dtoverlay=i2c-gt911-overlay

# For SPI ILI9341 Display with resistive touch
dtoverlay=spi-ili9341-overlay
dtoverlay=spi-ads7846-overlay
```

## Contributing

See [CONTRIBUTING.md](docs/CONTRIBUTING.md) for guidelines.

## License

This project is licensed under Apache 2.0. See [LICENSE](LICENSE) for details.

## Acknowledgments

- AOSP Team
- Raspberry Pi Foundation
- Mesa3D Project
- LineageOS Team
- GloDroid Project (inspiration)
