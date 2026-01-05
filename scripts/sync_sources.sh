#!/bin/bash
#
# Sync AOSP sources and local manifests for AOSPI5
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# AOSP version
AOSP_BRANCH="android-14.0.0_r1"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

install_repo() {
    if ! command -v repo &> /dev/null; then
        log_info "Installing repo tool..."
        mkdir -p ~/bin
        curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
        chmod a+x ~/bin/repo
        export PATH=~/bin:$PATH
    fi
}

init_repo() {
    log_info "Initializing AOSP repository..."
    
    cd "$PROJECT_ROOT"
    
    # Initialize repo with AOSP
    repo init -u https://android.googlesource.com/platform/manifest -b "$AOSP_BRANCH"
    
    # Create local manifests directory
    mkdir -p .repo/local_manifests
    
    # Copy our local manifest
    cp "${PROJECT_ROOT}/manifests/rpi5.xml" .repo/local_manifests/
    
    log_success "Repository initialized"
}

sync_sources() {
    log_info "Syncing sources (this may take several hours)..."
    
    cd "$PROJECT_ROOT"
    
    # Sync with optimizations
    repo sync -c -j$(nproc) --force-sync --no-tags --no-clone-bundle --optimized-fetch
    
    log_success "Sources synced"
}

apply_patches() {
    log_info "Applying patches..."
    
    # Apply kernel patches
    if [ -d "${PROJECT_ROOT}/kernel/brcm/rpi5/patches" ]; then
        cd "${PROJECT_ROOT}/kernel/brcm/rpi5"
        for patch in patches/*.patch; do
            if [ -f "$patch" ]; then
                git apply "$patch" 2>/dev/null || true
            fi
        done
    fi
    
    log_success "Patches applied"
}

main() {
    echo "=================================="
    echo "  AOSPI5 Source Sync"
    echo "=================================="
    
    install_repo
    init_repo
    sync_sources
    apply_patches
    
    log_success "Source sync completed!"
    echo ""
    echo "Next steps:"
    echo "  cd $PROJECT_ROOT"
    echo "  source build/envsetup.sh"
    echo "  lunch rpi5-userdebug"
    echo "  m -j\$(nproc)"
}

main "$@"
