#!/bin/bash
#
# AOSPI5 Build Script
# Builds Android for Raspberry Pi 5
#
# Usage: ./build.sh [target] [variant]
#   target: rpi5 (default), rpi5_car, rpi5_tv
#   variant: userdebug (default), eng, user
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/out"
JOBS=$(nproc)

# Default values
TARGET="${1:-rpi5}"
VARIANT="${2:-userdebug}"

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

check_prerequisites() {
    log_info "Checking prerequisites..."
    
    # Check for required tools
    local required_tools=("repo" "git" "make" "python3" "java" "adb" "fastboot")
    local missing_tools=()
    
    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            missing_tools+=("$tool")
        fi
    done
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        log_error "Missing required tools: ${missing_tools[*]}"
        echo "Please install them and try again."
        exit 1
    fi
    
    # Check disk space (minimum 250GB)
    local available_space=$(df -BG "$PROJECT_ROOT" | tail -1 | awk '{print $4}' | tr -d 'G')
    if [ "$available_space" -lt 250 ]; then
        log_warning "Low disk space: ${available_space}GB available, 250GB+ recommended"
    fi
    
    # Check RAM (minimum 16GB)
    local total_ram=$(free -g | awk '/^Mem:/{print $2}')
    if [ "$total_ram" -lt 16 ]; then
        log_warning "Low RAM: ${total_ram}GB available, 16GB+ recommended"
    fi
    
    log_success "Prerequisites check passed"
}

setup_environment() {
    log_info "Setting up build environment..."
    
    # Source Android build environment
    if [ -f "${PROJECT_ROOT}/build/envsetup.sh" ]; then
        source "${PROJECT_ROOT}/build/envsetup.sh"
    else
        log_error "AOSP not found. Please sync the sources first."
        exit 1
    fi
    
    # Set build target
    lunch "${TARGET}-${VARIANT}"
    
    log_success "Build environment configured for ${TARGET}-${VARIANT}"
}

build_kernel() {
    log_info "Building kernel..."
    
    local kernel_dir="${PROJECT_ROOT}/kernel/brcm/rpi5"
    local kernel_out="${BUILD_DIR}/kernel"
    
    mkdir -p "$kernel_out"
    
    cd "$kernel_dir"
    
    # Configure kernel
    make O="$kernel_out" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- rpi5_android_defconfig
    
    # Build kernel
    make O="$kernel_out" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j"$JOBS" Image modules dtbs
    
    log_success "Kernel build completed"
    cd "$PROJECT_ROOT"
}

build_android() {
    log_info "Building Android (this may take several hours)..."
    
    # Full Android build
    m -j"$JOBS"
    
    log_success "Android build completed"
}

build_bootloader() {
    log_info "Preparing boot files..."
    
    local boot_dir="${BUILD_DIR}/boot"
    mkdir -p "$boot_dir"
    
    # Copy kernel
    cp "${BUILD_DIR}/kernel/arch/arm64/boot/Image" "$boot_dir/"
    
    # Copy DTBs
    cp "${BUILD_DIR}/kernel/arch/arm64/boot/dts/broadcom/bcm2712*.dtb" "$boot_dir/"
    
    # Create config.txt for Raspberry Pi
    cat > "$boot_dir/config.txt" << 'EOF'
# AOSPI5 Boot Configuration

# Kernel
kernel=Image
arm_64bit=1
arm_boost=1

# Memory
gpu_mem=256
total_mem=8192

# Display
hdmi_force_hotplug=1
hdmi_group=2
hdmi_mode=82
disable_overscan=1
max_framebuffers=2

# Enable DRM VC4 V3D driver
dtoverlay=vc4-kms-v3d-pi5,cma-512

# Android boot parameters
cmdline=cmdline.txt

# Enable audio
dtparam=audio=on

# Enable I2C
dtparam=i2c_arm=on

# Enable SPI
dtparam=spi=on

# Enable UART
enable_uart=1

# Fan control
dtparam=fan_temp0=45000
dtparam=fan_temp1=50000
dtparam=fan_temp2=55000
dtparam=fan_temp3=60000

# PCIe for NVMe
dtparam=pciex1
dtparam=pciex1_gen=3

# USB power
usb_max_current_enable=1

# Camera
camera_auto_detect=1

# Display auto-detect
display_auto_detect=1

[all]
EOF

    # Create cmdline.txt
    cat > "$boot_dir/cmdline.txt" << 'EOF'
coherent_pool=1M 8250.nr_uarts=1 cma=512M video=HDMI-A-1:1920x1080@60 androidboot.hardware=rpi5 androidboot.selinux=permissive init=/init firmware_class.path=/vendor/firmware printk.devkmsg=on
EOF

    log_success "Boot files prepared"
}

create_images() {
    log_info "Creating flashable images..."
    
    local images_dir="${BUILD_DIR}/images"
    mkdir -p "$images_dir"
    
    # Create system image
    if [ -f "${BUILD_DIR}/target/product/${TARGET}/system.img" ]; then
        cp "${BUILD_DIR}/target/product/${TARGET}/system.img" "$images_dir/"
    fi
    
    # Create vendor image
    if [ -f "${BUILD_DIR}/target/product/${TARGET}/vendor.img" ]; then
        cp "${BUILD_DIR}/target/product/${TARGET}/vendor.img" "$images_dir/"
    fi
    
    # Create boot image
    if [ -f "${BUILD_DIR}/target/product/${TARGET}/boot.img" ]; then
        cp "${BUILD_DIR}/target/product/${TARGET}/boot.img" "$images_dir/"
    fi
    
    # Copy other partition images
    for img in product system_ext odm userdata cache; do
        if [ -f "${BUILD_DIR}/target/product/${TARGET}/${img}.img" ]; then
            cp "${BUILD_DIR}/target/product/${TARGET}/${img}.img" "$images_dir/"
        fi
    done
    
    log_success "Images created in $images_dir"
}

print_usage() {
    echo "AOSPI5 Build Script"
    echo ""
    echo "Usage: $0 [command] [options]"
    echo ""
    echo "Commands:"
    echo "  all         Build everything (default)"
    echo "  kernel      Build only the kernel"
    echo "  android     Build only Android"
    echo "  bootloader  Prepare boot files"
    echo "  images      Create flashable images"
    echo "  clean       Clean build directory"
    echo "  help        Show this help"
    echo ""
    echo "Options:"
    echo "  -t, --target    Build target (rpi5, rpi5_car, rpi5_tv)"
    echo "  -v, --variant   Build variant (userdebug, eng, user)"
    echo "  -j, --jobs      Number of parallel jobs"
    echo ""
}

clean_build() {
    log_info "Cleaning build directory..."
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    
    # Clean kernel
    if [ -d "${PROJECT_ROOT}/kernel/brcm/rpi5" ]; then
        cd "${PROJECT_ROOT}/kernel/brcm/rpi5"
        make clean
        cd "$PROJECT_ROOT"
    fi
    
    log_success "Clean completed"
}

main() {
    local command="${1:-all}"
    
    case "$command" in
        all)
            check_prerequisites
            setup_environment
            build_kernel
            build_android
            build_bootloader
            create_images
            log_success "Full build completed successfully!"
            ;;
        kernel)
            check_prerequisites
            setup_environment
            build_kernel
            ;;
        android)
            check_prerequisites
            setup_environment
            build_android
            ;;
        bootloader)
            build_bootloader
            ;;
        images)
            create_images
            ;;
        clean)
            clean_build
            ;;
        help|--help|-h)
            print_usage
            ;;
        *)
            log_error "Unknown command: $command"
            print_usage
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
