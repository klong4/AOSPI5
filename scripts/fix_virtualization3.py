#!/usr/bin/env python3
"""Fix virtualization module by disabling problematic modules that need kernel prebuilts"""

import re

filepath = "/home/klong/aosp-rpi5/packages/modules/Virtualization/build/microdroid/Android.bp"

with open(filepath, "r") as f:
    content = f.read()

# These modules need kernel prebuilts that don't exist
problematic_modules = [
    "microdroid_kernel_signed",
    "microdroid_kernel_16k_signed", 
    "microdroid_kernel",
    "microdroid_kernel_16k",
    "microdroid_gki-android15-6.6_kernel_signed",
    "microdroid_gki-android15-6.6_kernel_signed_supports_uefi_boot",
    "microdroid_gki-android15-6.6_kernel_signed-lz4",
    "microdroid_gki-android15-6.6_kernel",
    "microdroid_gki-android16-6.12_kernel_signed",
    "microdroid_gki-android16-6.12_kernel_signed_supports_uefi_boot",
    "microdroid_gki-android16-6.12_kernel_signed-lz4",
    "microdroid_gki-android16-6.12_kernel",
]

for module in problematic_modules:
    # Look for the module definition
    # Pattern matches: name: "module_name",\n and captures what follows
    pattern = rf'(name: "{re.escape(module)}",\n)([ \t]+)'
    match = re.search(pattern, content)
    
    if match:
        full_match = match.group(0)
        indent = match.group(2)
        
        # Check if already has enabled property right after name
        check_after = content[match.end():match.end()+50]
        if not check_after.strip().startswith('enabled:'):
            # Add enabled: false after the name
            replacement = f'{match.group(1)}{indent}enabled: false,\n{indent}'
            content = content.replace(full_match, replacement, 1)
            print(f"Disabled: {module}")
        else:
            print(f"Already has enabled: {module}")
    else:
        print(f"Not found: {module}")

with open(filepath, "w") as f:
    f.write(content)

print("Done!")
