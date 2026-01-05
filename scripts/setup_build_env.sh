#!/bin/bash
# AOSPI5 Android 16 Build Environment Setup Script
# This script sets up and builds Android 16 for Raspberry Pi 5

set -e

# Configuration
ANDROID_VERSION="android-16.0.0_r1"
BUILD_DIR="$HOME/aosp-rpi5"
DEVICE_DIR="/mnt/d/AOSPI5"
JOBS=$(nproc)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}======================================${NC}"
echo -e "${GREEN}  AOSPI5 Build Environment Setup${NC}"
echo -e "${GREEN}  Android 16 for Raspberry Pi 5${NC}"
echo -e "${GREEN}======================================${NC}"

# Check if running in WSL
if grep -qi microsoft /proc/version; then
    echo -e "${GREEN}[INFO]${NC} Running in WSL2"
else
    echo -e "${YELLOW}[WARN]${NC} Not running in WSL - some paths may differ"
fi

# Setup git config if not already set
if ! git config --global user.email > /dev/null 2>&1; then
    echo -e "${YELLOW}[SETUP]${NC} Configuring git..."
    git config --global user.email "builder@aospi5.local"
    git config --global user.name "AOSPI5 Builder"
fi

# Enable color UI for repo
git config --global color.ui auto

# Create build directory
echo -e "${GREEN}[STEP 1]${NC} Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Check if repo is already initialized
if [ -d ".repo" ]; then
    echo -e "${GREEN}[INFO]${NC} Repo already initialized, syncing..."
else
    echo -e "${GREEN}[STEP 2]${NC} Initializing AOSP repository..."
    echo -e "${YELLOW}[INFO]${NC} This will download ~100GB of source code"
    
    # Initialize repo with Android 16
    ~/bin/repo init -u https://android.googlesource.com/platform/manifest -b $ANDROID_VERSION --depth=1
    
    # Create local_manifests for RPi5 device
    mkdir -p .repo/local_manifests
    cat > .repo/local_manifests/rpi5.xml << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<manifest>
  <!-- Raspberry Pi 5 Android Device Tree -->
  <remote name="aospi5" fetch="https://github.com/klong4" />
  
  <!-- Device configuration -->
  <project path="device/brcm/rpi5" name="AOSPI5" remote="aospi5" revision="main" />
  
  <!-- Raspberry Pi kernel -->
  <project path="kernel/brcm/rpi5" name="AOSPI5" remote="aospi5" revision="main" clone-depth="1" />
  
  <!-- Hardware HALs -->
  <project path="hardware/brcm/camera" name="AOSPI5" remote="aospi5" revision="main" />
  <project path="hardware/brcm/display" name="AOSPI5" remote="aospi5" revision="main" />
  <project path="hardware/brcm/npu" name="AOSPI5" remote="aospi5" revision="main" />
  
</manifest>
EOF
fi

echo -e "${GREEN}[STEP 3]${NC} Syncing AOSP source (this may take several hours)..."
~/bin/repo sync -c -j$JOBS --force-sync --no-tags --no-clone-bundle

# Copy our device tree files from Windows
echo -e "${GREEN}[STEP 4]${NC} Copying device configuration from $DEVICE_DIR..."
if [ -d "$DEVICE_DIR/device" ]; then
    cp -rv "$DEVICE_DIR/device/"* device/ 2>/dev/null || true
fi
if [ -d "$DEVICE_DIR/hardware" ]; then
    cp -rv "$DEVICE_DIR/hardware/"* hardware/ 2>/dev/null || true
fi
if [ -d "$DEVICE_DIR/kernel" ]; then
    cp -rv "$DEVICE_DIR/kernel/"* kernel/ 2>/dev/null || true
fi
if [ -d "$DEVICE_DIR/overlays" ]; then
    mkdir -p device/brcm/rpi5/overlays
    cp -rv "$DEVICE_DIR/overlays/"* device/brcm/rpi5/overlays/ 2>/dev/null || true
fi

echo -e "${GREEN}[STEP 5]${NC} Setting up build environment..."
source build/envsetup.sh

echo -e "${GREEN}[STEP 6]${NC} Selecting build target..."
lunch aosp_rpi5-userdebug || lunch rpi5-userdebug || {
    echo -e "${YELLOW}[INFO]${NC} Custom lunch target not found, creating..."
    # If lunch target doesn't exist, we'll need to add it
    cat > device/brcm/rpi5/AndroidProducts.mk << 'PRODUCTS_EOF'
PRODUCT_MAKEFILES := \
    $(LOCAL_DIR)/aosp_rpi5.mk

COMMON_LUNCH_CHOICES := \
    aosp_rpi5-userdebug \
    aosp_rpi5-eng
PRODUCTS_EOF
    
    cat > device/brcm/rpi5/aosp_rpi5.mk << 'PRODUCT_EOF'
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)

PRODUCT_NAME := aosp_rpi5
PRODUCT_DEVICE := rpi5
PRODUCT_BRAND := Raspberry
PRODUCT_MODEL := Pi 5
PRODUCT_MANUFACTURER := Raspberry Pi Foundation

PRODUCT_CHARACTERISTICS := tablet

# Include device-specific configurations
$(call inherit-product, device/brcm/rpi5/device.mk)
PRODUCT_EOF
    
    # Re-source and lunch
    source build/envsetup.sh
    lunch aosp_rpi5-userdebug
}

echo -e "${GREEN}======================================${NC}"
echo -e "${GREEN}  Build environment is ready!${NC}"
echo -e "${GREEN}======================================${NC}"
echo ""
echo -e "To build Android, run:"
echo -e "  ${YELLOW}cd $BUILD_DIR${NC}"
echo -e "  ${YELLOW}source build/envsetup.sh${NC}"
echo -e "  ${YELLOW}lunch aosp_rpi5-userdebug${NC}"
echo -e "  ${YELLOW}make -j$JOBS${NC}"
echo ""
echo -e "Or to start the build now, run:"
echo -e "  ${YELLOW}$DEVICE_DIR/scripts/build_android.sh${NC}"
