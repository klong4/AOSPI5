#!/bin/bash
#
# AOSPI5 - Create Flashable Image for Raspberry Pi Imager
# 
# Creates a single .img file that can be flashed using Raspberry Pi Imager on Windows
#
# Usage: ./create_flashable_image.sh [output_name]
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/out"
OUTPUT_DIR="${PROJECT_ROOT}/release"
IMAGE_NAME="${1:-AOSPI5_Android16}"
DATE_STAMP=$(date +%Y%m%d)
FULL_IMAGE_NAME="${IMAGE_NAME}_${DATE_STAMP}.img"

# Partition sizes (in MB)
BOOT_SIZE=512
SYSTEM_SIZE=4096
VENDOR_SIZE=1024
PRODUCT_SIZE=512
USERDATA_SIZE=4096

# Total image size (boot + system + vendor + product + userdata + misc)
TOTAL_SIZE=$((BOOT_SIZE + SYSTEM_SIZE + VENDOR_SIZE + PRODUCT_SIZE + USERDATA_SIZE + 128))

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

log_step() {
    echo -e "${CYAN}[STEP]${NC} $1"
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root (for loop device operations)"
        log_info "Please run: sudo $0"
        exit 1
    fi
}

check_prerequisites() {
    log_step "Checking prerequisites..."
    
    local required_tools=("parted" "mkfs.vfat" "mkfs.ext4" "losetup" "dd" "rsync")
    local missing_tools=()
    
    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            missing_tools+=("$tool")
        fi
    done
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        log_error "Missing required tools: ${missing_tools[*]}"
        log_info "Install with: apt-get install parted dosfstools e2fsprogs"
        exit 1
    fi
    
    # Check if build output exists
    local product_dir="${BUILD_DIR}/target/product/rpi5"
    if [ ! -d "$product_dir" ]; then
        log_error "Build output not found at $product_dir"
        log_info "Please run ./build.sh first"
        exit 1
    fi
    
    log_success "Prerequisites check passed"
}

create_empty_image() {
    log_step "Creating ${TOTAL_SIZE}MB empty image..."
    
    mkdir -p "$OUTPUT_DIR"
    
    # Create sparse file
    dd if=/dev/zero of="${OUTPUT_DIR}/${FULL_IMAGE_NAME}" bs=1M count=0 seek=${TOTAL_SIZE} status=progress
    
    log_success "Empty image created"
}

create_partitions() {
    log_step "Creating partition table..."
    
    local image_file="${OUTPUT_DIR}/${FULL_IMAGE_NAME}"
    
    # Create GPT partition table
    parted -s "$image_file" mklabel gpt
    
    # Calculate partition boundaries (in MB)
    local boot_start=1
    local boot_end=$((boot_start + BOOT_SIZE))
    local system_start=$boot_end
    local system_end=$((system_start + SYSTEM_SIZE))
    local vendor_start=$system_end
    local vendor_end=$((vendor_start + VENDOR_SIZE))
    local product_start=$vendor_end
    local product_end=$((product_start + PRODUCT_SIZE))
    local userdata_start=$product_end
    local userdata_end=$((userdata_start + USERDATA_SIZE))
    
    # Create partitions
    parted -s "$image_file" mkpart boot fat32 ${boot_start}MiB ${boot_end}MiB
    parted -s "$image_file" mkpart system ext4 ${system_start}MiB ${system_end}MiB
    parted -s "$image_file" mkpart vendor ext4 ${vendor_start}MiB ${vendor_end}MiB
    parted -s "$image_file" mkpart product ext4 ${product_start}MiB ${product_end}MiB
    parted -s "$image_file" mkpart userdata ext4 ${userdata_start}MiB ${userdata_end}MiB
    
    # Set boot partition as bootable
    parted -s "$image_file" set 1 boot on
    
    log_success "Partition table created"
    parted -s "$image_file" print
}

setup_loop_device() {
    log_step "Setting up loop device..."
    
    local image_file="${OUTPUT_DIR}/${FULL_IMAGE_NAME}"
    
    # Setup loop device with partition scanning
    LOOP_DEV=$(losetup -f --show -P "$image_file")
    
    if [ -z "$LOOP_DEV" ]; then
        log_error "Failed to setup loop device"
        exit 1
    fi
    
    # Wait for partition devices to appear
    sleep 2
    partprobe "$LOOP_DEV" 2>/dev/null || true
    sleep 1
    
    log_success "Loop device: $LOOP_DEV"
}

format_partitions() {
    log_step "Formatting partitions..."
    
    # Format boot partition as FAT32
    mkfs.vfat -F 32 -n "boot" "${LOOP_DEV}p1"
    
    # Format system partition as ext4
    mkfs.ext4 -L "system" -q "${LOOP_DEV}p2"
    
    # Format vendor partition as ext4
    mkfs.ext4 -L "vendor" -q "${LOOP_DEV}p3"
    
    # Format product partition as ext4
    mkfs.ext4 -L "product" -q "${LOOP_DEV}p4"
    
    # Format userdata partition as ext4
    mkfs.ext4 -L "userdata" -q "${LOOP_DEV}p5"
    
    log_success "Partitions formatted"
}

mount_partitions() {
    log_step "Mounting partitions..."
    
    MOUNT_DIR=$(mktemp -d)
    
    mkdir -p "${MOUNT_DIR}/boot"
    mkdir -p "${MOUNT_DIR}/system"
    mkdir -p "${MOUNT_DIR}/vendor"
    mkdir -p "${MOUNT_DIR}/product"
    mkdir -p "${MOUNT_DIR}/userdata"
    
    mount "${LOOP_DEV}p1" "${MOUNT_DIR}/boot"
    mount "${LOOP_DEV}p2" "${MOUNT_DIR}/system"
    mount "${LOOP_DEV}p3" "${MOUNT_DIR}/vendor"
    mount "${LOOP_DEV}p4" "${MOUNT_DIR}/product"
    mount "${LOOP_DEV}p5" "${MOUNT_DIR}/userdata"
    
    log_success "Partitions mounted at $MOUNT_DIR"
}

populate_boot_partition() {
    log_step "Populating boot partition..."
    
    local product_dir="${BUILD_DIR}/target/product/rpi5"
    local kernel_dir="${BUILD_DIR}/kernel/arch/arm64/boot"
    local boot_mount="${MOUNT_DIR}/boot"
    
    # Copy kernel
    if [ -f "${kernel_dir}/Image" ]; then
        cp "${kernel_dir}/Image" "${boot_mount}/"
    elif [ -f "${product_dir}/kernel" ]; then
        cp "${product_dir}/kernel" "${boot_mount}/Image"
    fi
    
    # Copy device tree blobs
    mkdir -p "${boot_mount}/overlays"
    if [ -d "${kernel_dir}/dts/broadcom" ]; then
        cp "${kernel_dir}/dts/broadcom/bcm2712"*.dtb "${boot_mount}/" 2>/dev/null || true
    fi
    
    # Copy device tree overlays
    if [ -d "${PROJECT_ROOT}/overlays" ]; then
        for overlay_dir in display camera touch spi; do
            if [ -d "${PROJECT_ROOT}/overlays/${overlay_dir}" ]; then
                cp "${PROJECT_ROOT}/overlays/${overlay_dir}/"*.dtbo "${boot_mount}/overlays/" 2>/dev/null || true
            fi
        done
    fi
    
    # Create config.txt
    cat > "${boot_mount}/config.txt" << 'EOF'
# AOSPI5 - Android 16 for Raspberry Pi 5
# Raspberry Pi Imager Compatible Configuration

# Kernel
kernel=Image
arm_64bit=1
arm_boost=1

# Memory
gpu_mem=256
total_mem=8192

# Display - Auto-detect
display_auto_detect=1
max_framebuffers=2

# HDMI fallback
hdmi_force_hotplug=1
hdmi_group=2
hdmi_mode=82
disable_overscan=1

# Enable DRM VC4 V3D driver (RPi5)
dtoverlay=vc4-kms-v3d-pi5,cma-512

# Boot parameters
cmdline=cmdline.txt

# Enable peripherals
dtparam=audio=on
dtparam=i2c_arm=on
dtparam=spi=on
enable_uart=1

# Fan control
dtparam=fan_temp0=45000
dtparam=fan_temp1=50000
dtparam=fan_temp2=55000
dtparam=fan_temp3=60000

# PCIe for NVMe/NPU
dtparam=pciex1
dtparam=pciex1_gen=3

# USB power
usb_max_current_enable=1

# Camera auto-detect (MIPI CSI)
camera_auto_detect=1

# ===== DISPLAY OVERLAYS =====
# Uncomment ONE display overlay based on your hardware

# == MIPI DSI Displays ==
#dtoverlay=vc4-kms-dsi-7inch
#dtoverlay=vc4-kms-dsi-waveshare-panel,4_0_inch
#dtoverlay=waveshare-dsi-5inch

# == SPI Displays (2.8") ==
#dtoverlay=waveshare-28-tft-touch
#dtoverlay=adafruit-pitft28-resistive

# == SPI Displays (3.2") ==
#dtoverlay=waveshare-32-spi-touch

# == SPI Displays (3.5") ==
#dtoverlay=waveshare-35-a
#dtoverlay=waveshare-35-b
#dtoverlay=waveshare-35-c
#dtoverlay=adafruit-pitft35
#dtoverlay=elecrow-35
#dtoverlay=kedei-35
#dtoverlay=spi-ili9488-35

# == SPI Displays (4.0"+) ==
#dtoverlay=waveshare-4inch-rpi
#dtoverlay=waveshare-5inch-hdmi

# ===== CAMERA OVERLAYS =====
# Uncomment ONE camera overlay based on your hardware

#dtoverlay=ov5647           # Pi Camera v1 (5MP)
#dtoverlay=imx219           # Pi Camera v2 (8MP)
#dtoverlay=imx708           # Pi Camera v3 (12MP with AF)
#dtoverlay=imx477           # Pi HQ Camera (12.3MP)
#dtoverlay=imx296           # Pi Global Shutter Camera
#dtoverlay=arducam-64mp     # Arducam 64MP (16MP AF mode)
#dtoverlay=ov9281           # OV9281 Global Shutter

# ===== TOUCH OVERLAYS =====
# Uncomment for I2C touch controllers
#dtoverlay=goodix
#dtoverlay=ft5x06
#dtoverlay=gt911

[all]
EOF

    # Create cmdline.txt
    cat > "${boot_mount}/cmdline.txt" << 'EOF'
coherent_pool=1M 8250.nr_uarts=1 cma=512M video=HDMI-A-1:1920x1080@60 androidboot.hardware=rpi5 androidboot.selinux=permissive init=/init firmware_class.path=/vendor/firmware printk.devkmsg=on root=/dev/mmcblk0p2 rootfstype=ext4 rootwait
EOF

    # Copy ramdisk if exists
    if [ -f "${product_dir}/ramdisk.img" ]; then
        cp "${product_dir}/ramdisk.img" "${boot_mount}/"
    fi
    
    log_success "Boot partition populated"
}

populate_system_partition() {
    log_step "Populating system partition..."
    
    local product_dir="${BUILD_DIR}/target/product/rpi5"
    local system_mount="${MOUNT_DIR}/system"
    
    # Extract system image if it exists
    if [ -f "${product_dir}/system.img" ]; then
        # If system.img is sparse, convert to raw first
        if file "${product_dir}/system.img" | grep -q "sparse"; then
            log_info "Converting sparse system.img..."
            simg2img "${product_dir}/system.img" /tmp/system_raw.img
            
            local sys_loop=$(losetup -f --show /tmp/system_raw.img)
            mount -o ro "$sys_loop" /tmp/system_mnt
            rsync -aAX /tmp/system_mnt/ "${system_mount}/"
            umount /tmp/system_mnt
            losetup -d "$sys_loop"
            rm /tmp/system_raw.img
        else
            local sys_loop=$(losetup -f --show "${product_dir}/system.img")
            mount -o ro "$sys_loop" /tmp/system_mnt
            rsync -aAX /tmp/system_mnt/ "${system_mount}/"
            umount /tmp/system_mnt
            losetup -d "$sys_loop"
        fi
    elif [ -d "${product_dir}/system" ]; then
        rsync -aAX "${product_dir}/system/" "${system_mount}/"
    fi
    
    log_success "System partition populated"
}

populate_vendor_partition() {
    log_step "Populating vendor partition..."
    
    local product_dir="${BUILD_DIR}/target/product/rpi5"
    local vendor_mount="${MOUNT_DIR}/vendor"
    
    # Extract vendor image if it exists
    if [ -f "${product_dir}/vendor.img" ]; then
        if file "${product_dir}/vendor.img" | grep -q "sparse"; then
            log_info "Converting sparse vendor.img..."
            simg2img "${product_dir}/vendor.img" /tmp/vendor_raw.img
            
            local vnd_loop=$(losetup -f --show /tmp/vendor_raw.img)
            mkdir -p /tmp/vendor_mnt
            mount -o ro "$vnd_loop" /tmp/vendor_mnt
            rsync -aAX /tmp/vendor_mnt/ "${vendor_mount}/"
            umount /tmp/vendor_mnt
            losetup -d "$vnd_loop"
            rm /tmp/vendor_raw.img
        else
            local vnd_loop=$(losetup -f --show "${product_dir}/vendor.img")
            mkdir -p /tmp/vendor_mnt
            mount -o ro "$vnd_loop" /tmp/vendor_mnt
            rsync -aAX /tmp/vendor_mnt/ "${vendor_mount}/"
            umount /tmp/vendor_mnt
            losetup -d "$vnd_loop"
        fi
    elif [ -d "${product_dir}/vendor" ]; then
        rsync -aAX "${product_dir}/vendor/" "${vendor_mount}/"
    fi
    
    log_success "Vendor partition populated"
}

populate_product_partition() {
    log_step "Populating product partition..."
    
    local product_dir="${BUILD_DIR}/target/product/rpi5"
    local product_mount="${MOUNT_DIR}/product"
    
    # Extract product image if it exists
    if [ -f "${product_dir}/product.img" ]; then
        if file "${product_dir}/product.img" | grep -q "sparse"; then
            simg2img "${product_dir}/product.img" /tmp/product_raw.img
            
            local prd_loop=$(losetup -f --show /tmp/product_raw.img)
            mkdir -p /tmp/product_mnt
            mount -o ro "$prd_loop" /tmp/product_mnt
            rsync -aAX /tmp/product_mnt/ "${product_mount}/"
            umount /tmp/product_mnt
            losetup -d "$prd_loop"
            rm /tmp/product_raw.img
        else
            local prd_loop=$(losetup -f --show "${product_dir}/product.img")
            mkdir -p /tmp/product_mnt
            mount -o ro "$prd_loop" /tmp/product_mnt
            rsync -aAX /tmp/product_mnt/ "${product_mount}/"
            umount /tmp/product_mnt
            losetup -d "$prd_loop"
        fi
    elif [ -d "${product_dir}/product" ]; then
        rsync -aAX "${product_dir}/product/" "${product_mount}/"
    fi
    
    log_success "Product partition populated"
}

cleanup() {
    log_step "Cleaning up..."
    
    # Sync filesystems
    sync
    
    # Unmount partitions
    if [ -n "$MOUNT_DIR" ] && [ -d "$MOUNT_DIR" ]; then
        umount "${MOUNT_DIR}/boot" 2>/dev/null || true
        umount "${MOUNT_DIR}/system" 2>/dev/null || true
        umount "${MOUNT_DIR}/vendor" 2>/dev/null || true
        umount "${MOUNT_DIR}/product" 2>/dev/null || true
        umount "${MOUNT_DIR}/userdata" 2>/dev/null || true
        rm -rf "$MOUNT_DIR"
    fi
    
    # Detach loop device
    if [ -n "$LOOP_DEV" ]; then
        losetup -d "$LOOP_DEV" 2>/dev/null || true
    fi
    
    # Cleanup temp mounts
    umount /tmp/system_mnt 2>/dev/null || true
    umount /tmp/vendor_mnt 2>/dev/null || true
    umount /tmp/product_mnt 2>/dev/null || true
    
    log_success "Cleanup completed"
}

compress_image() {
    log_step "Compressing image for distribution..."
    
    local image_file="${OUTPUT_DIR}/${FULL_IMAGE_NAME}"
    
    # Create xz compressed version (best for Raspberry Pi Imager)
    xz -9 -k -v "${image_file}"
    
    # Also create zip for Windows users
    cd "$OUTPUT_DIR"
    zip -9 "${FULL_IMAGE_NAME%.img}.zip" "${FULL_IMAGE_NAME}"
    cd "$PROJECT_ROOT"
    
    log_success "Compressed images created"
}

create_checksums() {
    log_step "Creating checksums..."
    
    local image_file="${OUTPUT_DIR}/${FULL_IMAGE_NAME}"
    
    cd "$OUTPUT_DIR"
    
    # SHA256 checksum
    sha256sum "${FULL_IMAGE_NAME}" > "${FULL_IMAGE_NAME}.sha256"
    sha256sum "${FULL_IMAGE_NAME}.xz" >> "${FULL_IMAGE_NAME}.sha256" 2>/dev/null || true
    sha256sum "${FULL_IMAGE_NAME%.img}.zip" >> "${FULL_IMAGE_NAME}.sha256" 2>/dev/null || true
    
    # MD5 checksum
    md5sum "${FULL_IMAGE_NAME}" > "${FULL_IMAGE_NAME}.md5"
    
    cd "$PROJECT_ROOT"
    
    log_success "Checksums created"
}

print_summary() {
    echo ""
    echo -e "${GREEN}============================================${NC}"
    echo -e "${GREEN}       AOSPI5 Image Creation Complete       ${NC}"
    echo -e "${GREEN}============================================${NC}"
    echo ""
    echo -e "Output files in: ${CYAN}${OUTPUT_DIR}${NC}"
    echo ""
    echo -e "  ${BLUE}•${NC} ${FULL_IMAGE_NAME}"
    echo -e "  ${BLUE}•${NC} ${FULL_IMAGE_NAME}.xz (for Raspberry Pi Imager)"
    echo -e "  ${BLUE}•${NC} ${FULL_IMAGE_NAME%.img}.zip (for Windows)"
    echo -e "  ${BLUE}•${NC} ${FULL_IMAGE_NAME}.sha256"
    echo ""
    echo -e "${YELLOW}Flashing Instructions:${NC}"
    echo ""
    echo "1. Download Raspberry Pi Imager from:"
    echo "   https://www.raspberrypi.com/software/"
    echo ""
    echo "2. Open Raspberry Pi Imager"
    echo "3. Click 'Choose OS' → 'Use custom' → Select .img.xz or .img file"
    echo "4. Click 'Choose Storage' → Select your SD card"
    echo "5. Click 'Write'"
    echo ""
    echo -e "${YELLOW}First Boot:${NC}"
    echo "- Initial boot may take 2-3 minutes"
    echo "- Default display: HDMI (1080p)"
    echo "- Edit config.txt on boot partition to enable SPI/DSI displays"
    echo ""
}

# Trap for cleanup on exit
trap cleanup EXIT

main() {
    echo ""
    echo -e "${CYAN}======================================${NC}"
    echo -e "${CYAN}  AOSPI5 Flashable Image Creator     ${NC}"
    echo -e "${CYAN}  Android 16 for Raspberry Pi 5      ${NC}"
    echo -e "${CYAN}======================================${NC}"
    echo ""
    
    check_root
    check_prerequisites
    create_empty_image
    create_partitions
    setup_loop_device
    format_partitions
    mount_partitions
    populate_boot_partition
    populate_system_partition
    populate_vendor_partition
    populate_product_partition
    cleanup
    compress_image
    create_checksums
    print_summary
}

main "$@"
