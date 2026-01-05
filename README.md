# AOSPI5 - Android Open Source Project for Raspberry Pi 5

A fully-featured Android port for Raspberry Pi 5 with comprehensive hardware support.

## Features

- **Full AOSP 14 (Android 14)** base
- **Hardware Acceleration** via VideoCore VII GPU (V3D 7.1)
- **Complete GPIO Support** with Android HAL
- **Camera Support** (Pi Camera Module 3, USB cameras)
- **Display Support** (HDMI, DSI displays)
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
| Display | 2x micro-HDMI (4Kp60), DSI |
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
│       ├── graphics/
│       ├── gpio/
│       ├── sensors/
│       └── power/
├── kernel/                    # Linux kernel
│   └── brcm/
│       └── rpi5/
├── vendor/                    # Vendor-specific files
│   └── brcm/
│       └── rpi5/
├── bootloader/               # Bootloader configs
├── overlays/                 # Device tree overlays
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

# Initialize repo with Android 14
repo init -u https://android.googlesource.com/platform/manifest -b android-14.0.0_r1

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

## Hardware Support Status

| Component | Status | Notes |
|-----------|--------|-------|
| Display (HDMI) | ✅ Working | 4K@60Hz supported |
| Display (DSI) | 🔄 In Progress | Official 7" display |
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
| PWM | ✅ Working | - |
| SPI | ✅ Working | - |
| I2C | ✅ Working | - |
| UART | ✅ Working | - |

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
