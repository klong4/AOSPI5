#!/usr/bin/env python3
"""Fix virtualization module build issues by disabling problematic modules"""

import re

filepath = "/home/klong/aosp-rpi5/packages/modules/Virtualization/build/microdroid/Android.bp"

with open(filepath, "r") as f:
    content = f.read()

# Modules to disable (they depend on missing kernel prebuilts)
modules_to_disable = [
    "microdroid_kernel_signed",
    "microdroid_kernel_16k_signed", 
    "microdroid_gki_kernel_signed",
    "microdroid_gki_kernel_16k_signed",
    "microdroid_kernel",
    "microdroid_kernel_16k",
    "microdroid_gki_kernel",
    "microdroid_gki_kernel_16k",
]

for module in modules_to_disable:
    # Pattern to find module definition and add enabled: false after name
    pattern = rf'(name: "{module}",)\n'
    replacement = rf'\1\n    enabled: false,'
    
    # Only add if not already disabled
    if f'name: "{module}",' in content and f'name: "{module}",\n    enabled: false' not in content:
        content = re.sub(pattern, replacement, content)
        print(f"Disabled: {module}")

with open(filepath, "w") as f:
    f.write(content)

print("Done!")
