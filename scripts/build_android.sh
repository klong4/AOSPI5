#!/bin/bash
# AOSPI5 Android 16 Build Script
# This script builds Android and creates a flashable image

set -e

BUILD_DIR="$HOME/aosp-rpi5"
DEVICE_DIR="/mnt/d/AOSPI5"
JOBS=$(nproc)
OUTPUT_DIR="$DEVICE_DIR/out"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}======================================${NC}"
echo -e "${GREEN}  AOSPI5 Android Build${NC}"
echo -e "${GREEN}  Building Android 16 for RPi 5${NC}"
echo -e "${GREEN}======================================${NC}"

# Check if build environment exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}[ERROR]${NC} Build environment not found!"
    echo -e "Please run ${YELLOW}setup_build_env.sh${NC} first"
    exit 1
fi

cd "$BUILD_DIR"

# Setup ccache for faster rebuilds
export USE_CCACHE=1
export CCACHE_DIR="$HOME/.ccache"
export CCACHE_EXEC=/usr/bin/ccache
ccache -M 50G

echo -e "${GREEN}[STEP 1]${NC} Setting up build environment..."
source build/envsetup.sh
lunch aosp_rpi5-userdebug

echo -e "${GREEN}[STEP 2]${NC} Building Android (this will take 2-6 hours)..."
echo -e "${YELLOW}[INFO]${NC} Using $JOBS parallel jobs"
echo -e "${YELLOW}[INFO]${NC} Build started at $(date)"

# Build Android
make -j$JOBS

echo -e "${GREEN}[STEP 3]${NC} Build completed at $(date)"

# Check for output files
OUT_DIR="$BUILD_DIR/out/target/product/rpi5"
if [ -d "$OUT_DIR" ]; then
    echo -e "${GREEN}[STEP 4]${NC} Copying output files..."
    mkdir -p "$OUTPUT_DIR/target/product/rpi5"
    
    # Copy important images
    for img in boot.img system.img vendor.img product.img userdata.img; do
        if [ -f "$OUT_DIR/$img" ]; then
            echo "  Copying $img..."
            cp "$OUT_DIR/$img" "$OUTPUT_DIR/target/product/rpi5/"
        fi
    done
    
    # Copy sparse images if they exist
    for img in system.img vendor.img product.img userdata.img; do
        sparse="$OUT_DIR/${img%.img}_sparse.img"
        if [ -f "$sparse" ]; then
            cp "$sparse" "$OUTPUT_DIR/target/product/rpi5/"
        fi
    done
    
    echo -e "${GREEN}[STEP 5]${NC} Creating flashable image..."
    cd "$DEVICE_DIR"
    chmod +x scripts/create_flashable_image.sh
    ./scripts/create_flashable_image.sh
    
    echo -e "${GREEN}======================================${NC}"
    echo -e "${GREEN}  Build Complete!${NC}"
    echo -e "${GREEN}======================================${NC}"
    echo ""
    echo -e "Flashable images are in: ${YELLOW}$OUTPUT_DIR${NC}"
    echo ""
    echo "Files:"
    ls -lh "$OUTPUT_DIR/"*.img* 2>/dev/null || true
    ls -lh "$OUTPUT_DIR/"*.xz 2>/dev/null || true
    ls -lh "$OUTPUT_DIR/"*.zip 2>/dev/null || true
else
    echo -e "${RED}[ERROR]${NC} Build output not found at $OUT_DIR"
    exit 1
fi
