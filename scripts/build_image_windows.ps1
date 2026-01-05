# AOSPI5 - Windows Image Builder
# Creates a flashable image for Raspberry Pi Imager
# Requires: WSL2 with Ubuntu or running in a Linux VM
#
# Usage: .\build_image_windows.ps1 [-OutputName <name>] [-UseWSL]
#

param(
    [string]$OutputName = "AOSPI5_Android16",
    [switch]$UseWSL,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

# Colors
$Colors = @{
    Info = "Cyan"
    Success = "Green"
    Warning = "Yellow"
    Error = "Red"
    Step = "Magenta"
}

function Write-Log {
    param(
        [string]$Message,
        [string]$Level = "Info"
    )
    
    $prefix = switch ($Level) {
        "Info" { "[INFO]" }
        "Success" { "[SUCCESS]" }
        "Warning" { "[WARNING]" }
        "Error" { "[ERROR]" }
        "Step" { "[STEP]" }
    }
    
    Write-Host "$prefix $Message" -ForegroundColor $Colors[$Level]
}

function Show-Help {
    Write-Host @"

AOSPI5 Windows Image Builder
============================

This script creates a flashable Android image for Raspberry Pi 5
that can be written using Raspberry Pi Imager.

USAGE:
    .\build_image_windows.ps1 [options]

OPTIONS:
    -OutputName <name>    Base name for output image (default: AOSPI5_Android16)
    -UseWSL              Use WSL2 to build the image (recommended)
    -Help                Show this help message

REQUIREMENTS:
    - WSL2 with Ubuntu installed
    - At least 20GB free disk space
    - Android build completed first

EXAMPLES:
    # Build with default settings using WSL
    .\build_image_windows.ps1 -UseWSL

    # Build with custom name
    .\build_image_windows.ps1 -OutputName "MyAndroidPi5" -UseWSL

"@
}

function Test-Prerequisites {
    Write-Log "Checking prerequisites..." -Level "Step"
    
    # Check if WSL is available
    if ($UseWSL) {
        try {
            $wslVersion = wsl --version 2>&1
            if ($LASTEXITCODE -ne 0) {
                throw "WSL not installed"
            }
            Write-Log "WSL detected" -Level "Success"
        }
        catch {
            Write-Log "WSL2 is required but not installed" -Level "Error"
            Write-Host "Install WSL2 with: wsl --install"
            exit 1
        }
        
        # Check for Ubuntu distribution
        $distros = wsl --list --quiet 2>&1
        if ($distros -notmatch "Ubuntu") {
            Write-Log "Ubuntu WSL distribution not found" -Level "Warning"
            Write-Host "Install with: wsl --install -d Ubuntu"
        }
    }
    
    # Check for build output
    $buildOutput = Join-Path $PSScriptRoot "..\out\target\product\rpi5"
    if (-not (Test-Path $buildOutput)) {
        Write-Log "Build output not found at: $buildOutput" -Level "Error"
        Write-Host "Please run the Android build first with ./build.sh"
        exit 1
    }
    
    Write-Log "Prerequisites check passed" -Level "Success"
}

function Convert-ToWSLPath {
    param([string]$WindowsPath)
    
    $path = $WindowsPath -replace '\\', '/'
    if ($path -match '^([A-Za-z]):(.*)$') {
        $drive = $Matches[1].ToLower()
        $rest = $Matches[2]
        return "/mnt/$drive$rest"
    }
    return $path
}

function Build-ImageWithWSL {
    Write-Log "Building image using WSL..." -Level "Step"
    
    $projectRoot = (Get-Item $PSScriptRoot).Parent.FullName
    $wslPath = Convert-ToWSLPath $projectRoot
    $scriptPath = "$wslPath/scripts/create_flashable_image.sh"
    
    Write-Log "Project root (WSL): $wslPath" -Level "Info"
    
    # Make script executable and run it
    $commands = @(
        "cd '$wslPath'",
        "chmod +x '$scriptPath'",
        "sudo '$scriptPath' '$OutputName'"
    )
    
    $fullCommand = $commands -join " && "
    
    Write-Log "Executing build script in WSL..." -Level "Info"
    wsl -d Ubuntu -e bash -c $fullCommand
    
    if ($LASTEXITCODE -eq 0) {
        Write-Log "Image build completed successfully" -Level "Success"
    }
    else {
        Write-Log "Image build failed with exit code: $LASTEXITCODE" -Level "Error"
        exit 1
    }
}

function Build-ImageNative {
    Write-Log "Native Windows build not yet implemented" -Level "Warning"
    Write-Log "Please use the -UseWSL option to build using WSL2" -Level "Info"
    Write-Host ""
    Write-Host "Alternative: You can also build in a Linux VM or use Docker"
    exit 1
}

function Copy-OutputFiles {
    Write-Log "Copying output files..." -Level "Step"
    
    $projectRoot = (Get-Item $PSScriptRoot).Parent.FullName
    $releaseDir = Join-Path $projectRoot "release"
    $outputDir = Join-Path $env:USERPROFILE "AOSPI5_Images"
    
    if (-not (Test-Path $outputDir)) {
        New-Item -ItemType Directory -Path $outputDir | Out-Null
    }
    
    # If using WSL, the files are already in Windows path
    # Just display location
    
    Write-Log "Output files available at:" -Level "Success"
    Write-Host ""
    Write-Host "  $releaseDir"
    Write-Host ""
}

function Show-FlashingInstructions {
    Write-Host ""
    Write-Host "============================================" -ForegroundColor Green
    Write-Host "       AOSPI5 Image Build Complete         " -ForegroundColor Green
    Write-Host "============================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "FLASHING INSTRUCTIONS:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "1. Download Raspberry Pi Imager from:"
    Write-Host "   https://www.raspberrypi.com/software/" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "2. Open Raspberry Pi Imager"
    Write-Host ""
    Write-Host "3. Click 'Choose OS' -> 'Use custom'"
    Write-Host "   Select the .img.xz or .img file"
    Write-Host ""
    Write-Host "4. Click 'Choose Storage'"
    Write-Host "   Select your SD card (16GB+ recommended)"
    Write-Host ""
    Write-Host "5. Click 'Write'"
    Write-Host ""
    Write-Host "CONFIGURATION:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "After flashing, you can customize config.txt on the boot partition:"
    Write-Host ""
    Write-Host "  DISPLAYS:" -ForegroundColor Cyan
    Write-Host "  - HDMI: Enabled by default"
    Write-Host "  - DSI:  Uncomment appropriate dtoverlay="
    Write-Host "  - SPI:  Uncomment appropriate dtoverlay="
    Write-Host ""
    Write-Host "  CAMERAS:" -ForegroundColor Cyan
    Write-Host "  - Auto-detect enabled by default"
    Write-Host "  - Or uncomment specific camera overlay"
    Write-Host ""
    Write-Host "  NPU/AI ACCELERATORS:" -ForegroundColor Cyan
    Write-Host "  - Coral TPU: Auto-detected on PCIe"
    Write-Host "  - Hailo: Auto-detected on PCIe"
    Write-Host "  - Intel NCS2: Auto-detected on USB"
    Write-Host ""
}

# Main
if ($Help) {
    Show-Help
    exit 0
}

Write-Host ""
Write-Host "======================================" -ForegroundColor Cyan
Write-Host "  AOSPI5 Windows Image Builder       " -ForegroundColor Cyan
Write-Host "  Android 16 for Raspberry Pi 5      " -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

Test-Prerequisites

if ($UseWSL) {
    Build-ImageWithWSL
}
else {
    Write-Log "WSL mode recommended for building images" -Level "Warning"
    Write-Host "Use: .\build_image_windows.ps1 -UseWSL"
    Write-Host ""
    Build-ImageNative
}

Copy-OutputFiles
Show-FlashingInstructions
