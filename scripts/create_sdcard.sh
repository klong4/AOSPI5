#!/bin/bash
#
# Create bootable SD card for AOSPI5
#
# Usage: ./create_sdcard.sh /dev/sdX
#
# WARNING: This script will ERASE ALL DATA on the specified device!
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/out"
IMAGES_DIR="${BUILD_DIR}/images"
BOOT_DIR="${BUILD_DIR}/boot"

# Partition sizes in MB
BOOT_SIZE=256
SYSTEM_SIZE=2048
VENDOR_SIZE=512
PRODUCT_SIZE=512
USERDATA_SIZE=4096

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        log_error "This script must be run as root"
        exit 1
    fi
}

check_device() {
    local device="$1"
    
    if [ -z "$device" ]; then
        log_error "No device specified"
        echo "Usage: $0 /dev/sdX"
        exit 1
    fi
    
    if [ ! -b "$device" ]; then
        log_error "Device $device is not a block device"
        exit 1
    fi
    
    # Check if device is mounted
    if mount | grep -q "$device"; then
        log_error "Device $device is mounted. Please unmount it first."
        exit 1
    fi
    
    # Get device size
    local device_size=$(blockdev --getsize64 "$device")
    local min_size=$((16 * 1024 * 1024 * 1024))  # 16GB minimum
    
    if [ "$device_size" -lt "$min_size" ]; then
        log_error "Device is too small. Minimum 16GB required."
        exit 1
    fi
    
    log_info "Device: $device ($(numfmt --to=iec $device_size))"
}

confirm_operation() {
    local device="$1"
    
    echo ""
    log_warning "THIS WILL ERASE ALL DATA ON $device"
    echo ""
    read -p "Are you sure you want to continue? (yes/no): " confirm
    
    if [ "$confirm" != "yes" ]; then
        log_info "Operation cancelled"
        exit 0
    fi
}

create_partitions() {
    local device="$1"
    
    log_info "Creating partition table..."
    
    # Clear existing partitions
    wipefs -a "$device" 2>/dev/null || true
    
    # Create GPT partition table and partitions using parted
    parted -s "$device" mklabel gpt
    
    local start=1
    local end
    
    # Boot partition (FAT32)
    end=$((start + BOOT_SIZE))
    parted -s "$device" mkpart boot fat32 ${start}MiB ${end}MiB
    parted -s "$device" set 1 boot on
    start=$end
    
    # System partition (ext4)
    end=$((start + SYSTEM_SIZE))
    parted -s "$device" mkpart system ext4 ${start}MiB ${end}MiB
    start=$end
    
    # Vendor partition (ext4)
    end=$((start + VENDOR_SIZE))
    parted -s "$device" mkpart vendor ext4 ${start}MiB ${end}MiB
    start=$end
    
    # Product partition (ext4)
    end=$((start + PRODUCT_SIZE))
    parted -s "$device" mkpart product ext4 ${start}MiB ${end}MiB
    start=$end
    
    # Userdata partition (f2fs) - use remaining space
    parted -s "$device" mkpart userdata ext4 ${start}MiB 100%
    
    # Wait for partitions to appear
    partprobe "$device"
    sleep 2
    
    log_success "Partitions created"
}

format_partitions() {
    local device="$1"
    local part_prefix=""
    
    # Handle different device naming conventions
    if [[ "$device" == *"nvme"* ]] || [[ "$device" == *"mmcblk"* ]]; then
        part_prefix="${device}p"
    else
        part_prefix="${device}"
    fi
    
    log_info "Formatting partitions..."
    
    # Format boot partition
    mkfs.vfat -F 32 -n BOOT "${part_prefix}1"
    
    # Format system partition
    mkfs.ext4 -L system "${part_prefix}2"
    
    # Format vendor partition
    mkfs.ext4 -L vendor "${part_prefix}3"
    
    # Format product partition
    mkfs.ext4 -L product "${part_prefix}4"
    
    # Format userdata partition with f2fs if available, else ext4
    if command -v mkfs.f2fs &> /dev/null; then
        mkfs.f2fs -l userdata "${part_prefix}5"
    else
        mkfs.ext4 -L userdata "${part_prefix}5"
    fi
    
    log_success "Partitions formatted"
}

copy_boot_files() {
    local device="$1"
    local part_prefix=""
    
    if [[ "$device" == *"nvme"* ]] || [[ "$device" == *"mmcblk"* ]]; then
        part_prefix="${device}p"
    else
        part_prefix="${device}"
    fi
    
    local mount_point="/tmp/aospi5_boot"
    
    log_info "Copying boot files..."
    
    mkdir -p "$mount_point"
    mount "${part_prefix}1" "$mount_point"
    
    # Copy kernel and DTBs
    if [ -d "$BOOT_DIR" ]; then
        cp -v "$BOOT_DIR"/* "$mount_point/"
    fi
    
    # Copy firmware files
    if [ -d "${PROJECT_ROOT}/firmware" ]; then
        cp -rv "${PROJECT_ROOT}/firmware"/* "$mount_point/"
    fi
    
    # Copy overlays
    mkdir -p "$mount_point/overlays"
    if [ -d "${PROJECT_ROOT}/overlays" ]; then
        for dts in "${PROJECT_ROOT}/overlays"/*.dts; do
            if [ -f "$dts" ]; then
                dtc -I dts -O dtb -o "$mount_point/overlays/$(basename "${dts%.dts}.dtbo")" "$dts" 2>/dev/null || true
            fi
        done
    fi
    
    sync
    umount "$mount_point"
    rmdir "$mount_point"
    
    log_success "Boot files copied"
}

flash_images() {
    local device="$1"
    local part_prefix=""
    
    if [[ "$device" == *"nvme"* ]] || [[ "$device" == *"mmcblk"* ]]; then
        part_prefix="${device}p"
    else
        part_prefix="${device}"
    fi
    
    log_info "Flashing Android images..."
    
    # Flash system image
    if [ -f "${IMAGES_DIR}/system.img" ]; then
        log_info "Flashing system.img..."
        dd if="${IMAGES_DIR}/system.img" of="${part_prefix}2" bs=4M status=progress
    fi
    
    # Flash vendor image
    if [ -f "${IMAGES_DIR}/vendor.img" ]; then
        log_info "Flashing vendor.img..."
        dd if="${IMAGES_DIR}/vendor.img" of="${part_prefix}3" bs=4M status=progress
    fi
    
    # Flash product image
    if [ -f "${IMAGES_DIR}/product.img" ]; then
        log_info "Flashing product.img..."
        dd if="${IMAGES_DIR}/product.img" of="${part_prefix}4" bs=4M status=progress
    fi
    
    sync
    
    log_success "Images flashed"
}

main() {
    local device="$1"
    
    echo "=================================="
    echo "  AOSPI5 SD Card Creator"
    echo "=================================="
    echo ""
    
    check_root
    check_device "$device"
    confirm_operation "$device"
    
    create_partitions "$device"
    format_partitions "$device"
    copy_boot_files "$device"
    flash_images "$device"
    
    echo ""
    log_success "SD card created successfully!"
    echo ""
    echo "You can now:"
    echo "1. Insert the SD card into your Raspberry Pi 5"
    echo "2. Connect a display and power supply"
    echo "3. Boot into Android"
    echo ""
}

main "$@"
