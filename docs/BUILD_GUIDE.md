# AOSPI5 Build Guide

Complete guide for building Android for Raspberry Pi 5.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Setting Up Build Environment](#setting-up-build-environment)
3. [Syncing Sources](#syncing-sources)
4. [Building Android](#building-android)
5. [Creating Bootable SD Card](#creating-bootable-sd-card)
6. [First Boot](#first-boot)
7. [Troubleshooting](#troubleshooting)

## Prerequisites

### Hardware Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| CPU | 8 cores | 16+ cores |
| RAM | 32 GB | 64 GB |
| Storage | 400 GB SSD | 1 TB NVMe SSD |
| OS | Ubuntu 22.04 LTS | Ubuntu 22.04 LTS |

### Software Requirements

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install required packages
sudo apt install -y \
    git-core gnupg flex bison build-essential zip curl \
    zlib1g-dev libc6-dev-i386 libncurses5 lib32ncurses5-dev \
    x11proto-core-dev libx11-dev lib32z1-dev libgl1-mesa-dev \
    libxml2-utils xsltproc unzip fontconfig python3 python3-pip \
    openjdk-11-jdk adb fastboot mtools dosfstools \
    device-tree-compiler bc cpio kmod libssl-dev rsync \
    libelf-dev u-boot-tools parted gdisk

# Install repo tool
mkdir -p ~/bin
curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
chmod a+x ~/bin/repo
echo 'export PATH=~/bin:$PATH' >> ~/.bashrc
source ~/.bashrc

# Configure git
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

### Increase File Descriptors

```bash
# Add to /etc/security/limits.conf
echo "* soft nofile 65536" | sudo tee -a /etc/security/limits.conf
echo "* hard nofile 65536" | sudo tee -a /etc/security/limits.conf

# Reboot or re-login
```

## Setting Up Build Environment

### 1. Create Working Directory

```bash
mkdir -p ~/aospi5
cd ~/aospi5
```

### 2. Initialize Repository

```bash
# Initialize with AOSP Android 14
repo init -u https://android.googlesource.com/platform/manifest -b android-14.0.0_r1

# Add local manifests
mkdir -p .repo/local_manifests
# Copy rpi5.xml to .repo/local_manifests/
```

### 3. Sync Sources

```bash
# Full sync (takes several hours)
repo sync -c -j$(nproc) --force-sync --no-tags --no-clone-bundle

# Or use our script
./scripts/sync_sources.sh
```

## Building Android

### 1. Setup Build Environment

```bash
source build/envsetup.sh
```

### 2. Select Build Target

```bash
# For standard Android tablet
lunch rpi5-userdebug

# For Android Automotive
lunch rpi5_car-userdebug

# For Android TV
lunch rpi5_tv-userdebug
```

### 3. Build

```bash
# Full build
m -j$(nproc)

# Or use build script
./scripts/build.sh all
```

### Build Variants

| Variant | Description | Use Case |
|---------|-------------|----------|
| `user` | Production build, limited debugging | Final deployment |
| `userdebug` | Like user, but with root access | Development/Testing |
| `eng` | Full debugging, all tools | Development |

## Creating Bootable SD Card

### Requirements

- microSD card (32GB minimum, Class 10 or better)
- SD card reader
- Linux system with root access

### Steps

```bash
# Identify SD card device
lsblk

# Create bootable SD card (replace /dev/sdX with your device)
sudo ./scripts/create_sdcard.sh /dev/sdX
```

### Manual Method

```bash
# Partition the SD card
sudo parted /dev/sdX mklabel gpt
sudo parted /dev/sdX mkpart boot fat32 1MiB 257MiB
sudo parted /dev/sdX mkpart system ext4 257MiB 2305MiB
sudo parted /dev/sdX mkpart vendor ext4 2305MiB 2817MiB
sudo parted /dev/sdX mkpart userdata ext4 2817MiB 100%

# Format partitions
sudo mkfs.vfat -F 32 -n BOOT /dev/sdX1
sudo mkfs.ext4 -L system /dev/sdX2
sudo mkfs.ext4 -L vendor /dev/sdX3
sudo mkfs.f2fs -l userdata /dev/sdX4

# Mount and copy files
sudo mount /dev/sdX1 /mnt/boot
sudo cp out/boot/* /mnt/boot/
sudo umount /mnt/boot

# Flash images
sudo dd if=out/images/system.img of=/dev/sdX2 bs=4M status=progress
sudo dd if=out/images/vendor.img of=/dev/sdX3 bs=4M status=progress
```

## First Boot

### Required Hardware

- Raspberry Pi 5 (4GB or 8GB)
- Power supply (USB-C, 5V/5A recommended)
- microSD card with Android
- HDMI display
- USB keyboard and mouse

### Boot Process

1. Insert the SD card into Pi 5
2. Connect display, keyboard, mouse
3. Connect power supply
4. Wait for Android to boot (first boot takes longer)

### Initial Setup

1. Select language
2. Connect to WiFi
3. Skip Google account (or sign in if GMS is included)
4. Complete setup wizard

## Troubleshooting

### Build Errors

#### Out of Memory

```bash
# Limit parallel jobs
m -j4

# Add swap space
sudo fallocate -l 16G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

#### Java Version Issues

```bash
# Use Java 11
sudo update-alternatives --set java /usr/lib/jvm/java-11-openjdk-amd64/bin/java
```

### Boot Issues

#### No Display Output

1. Check HDMI connection
2. Try different HDMI port
3. Edit config.txt:
   ```
   hdmi_force_hotplug=1
   hdmi_safe=1
   ```

#### Boot Loop

1. Check serial console for errors
2. Verify kernel and DTB files are present
3. Check cmdline.txt parameters

#### ADB Not Working

```bash
# Enable ADB over network
setprop service.adb.tcp.port 5555
stop adbd
start adbd

# Connect from host
adb connect <pi_ip_address>:5555
```

### Performance Issues

#### Low Frame Rate

1. Check thermal throttling
2. Ensure adequate cooling
3. Enable GPU compositor:
   ```
   setprop debug.hwui.renderer opengl
   ```

#### WiFi/Bluetooth Issues

1. Verify firmware files in /vendor/firmware
2. Check kernel modules are loaded:
   ```
   lsmod | grep brcm
   ```

## Advanced Topics

### Enabling Developer Options

1. Go to Settings > About tablet
2. Tap "Build number" 7 times
3. Developer options now available in Settings

### Custom Kernel

```bash
cd kernel/brcm/rpi5
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image modules dtbs
```

### Adding HAT Support

Create device tree overlay for your HAT and add to overlays directory.
See [overlays/README.md](../overlays/README.md) for details.

## Resources

- [AOSP Documentation](https://source.android.com/docs)
- [Raspberry Pi Documentation](https://www.raspberrypi.com/documentation/)
- [Project Issues](https://github.com/aospi5/device_brcm_rpi5/issues)
