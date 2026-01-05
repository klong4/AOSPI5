# Contributing to AOSPI5

Thank you for your interest in contributing to AOSPI5! This document provides guidelines for contributing to the project.

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow

## How to Contribute

### Reporting Issues

1. Check existing issues before creating a new one
2. Use the issue template
3. Provide detailed information:
   - Hardware configuration
   - Android version
   - Steps to reproduce
   - Expected vs actual behavior
   - Logs (logcat, dmesg, etc.)

### Submitting Changes

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make your changes
4. Test thoroughly
5. Commit with clear messages
6. Push to your fork
7. Create a Pull Request

### Commit Message Format

```
[component] Brief description

Detailed explanation of changes.

Fixes: #issue_number
```

Components:
- `kernel` - Kernel changes
- `hal` - Hardware Abstraction Layer
- `device` - Device configuration
- `overlay` - Device tree overlays
- `scripts` - Build/utility scripts
- `docs` - Documentation

### Code Style

#### C/C++
- Follow Android AOSP code style
- Use clang-format with Android style
- Comment complex logic

#### Python
- Follow PEP 8
- Use type hints where appropriate

#### Shell Scripts
- Use bash
- Include error handling (`set -e`)
- Comment non-obvious commands

### Testing Requirements

Before submitting:
1. Build completes without errors
2. Boot test on real hardware
3. Test affected features
4. Run relevant unit tests

## Development Setup

### Prerequisites

- Ubuntu 22.04 LTS
- 64GB+ RAM recommended
- 500GB+ SSD storage
- Raspberry Pi 5 for testing

### Getting Started

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/aospi5.git

# Add upstream remote
git remote add upstream https://github.com/aospi5/aospi5.git

# Keep your fork updated
git fetch upstream
git rebase upstream/main
```

## Areas for Contribution

### High Priority

- [ ] Camera HAL improvements
- [ ] Video codec support
- [ ] WiFi stability
- [ ] Bluetooth audio quality
- [ ] Power management

### Medium Priority

- [ ] Additional HAT support
- [ ] Performance optimizations
- [ ] Documentation improvements
- [ ] Test coverage

### Low Priority

- [ ] UI/UX improvements
- [ ] Additional apps
- [ ] Translations

## Hardware Abstraction Layers (HALs)

### Creating a New HAL

1. Define HIDL interface in `hardware/brcm/<hal>/1.0/`
2. Implement in C++ following existing HALs
3. Add Android.bp build file
4. Register in device manifest
5. Add SELinux policies
6. Test thoroughly

### HAL Testing

```bash
# Build HAL
mmma hardware/brcm/<hal>

# Test with VTS
vts-tradefed run commandAndExit vts -m <hal>VtsHalTest
```

## Kernel Development

### Adding a Driver

1. Port driver to Pi 5 kernel version
2. Create defconfig entry
3. Test functionality
4. Document in HARDWARE_SUPPORT.md

### Creating Device Tree Overlay

1. Create .dts file in `overlays/`
2. Follow existing overlay patterns
3. Test overlay loading
4. Document usage

## Release Process

1. Feature freeze
2. Testing phase
3. Bug fixes only
4. Release candidate
5. Final testing
6. Release

## Getting Help

- GitHub Issues for bugs
- GitHub Discussions for questions
- IRC: #aospi5 on Libera.Chat (if available)

## License

By contributing, you agree that your contributions will be licensed under the Apache 2.0 License.
