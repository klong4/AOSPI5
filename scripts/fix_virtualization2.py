#!/usr/bin/env python3
"""Fix virtualization module build issues by disabling all microdroid modules"""

import re

filepath = "/home/klong/aosp-rpi5/packages/modules/Virtualization/build/microdroid/Android.bp"

with open(filepath, "r") as f:
    content = f.read()

# Find all module names that start with microdroid
pattern = r'name: "(microdroid[^"]+)",'
matches = re.findall(pattern, content)
print(f"Found {len(matches)} microdroid modules")

disabled_count = 0
for module in matches:
    # Check if this module is not already disabled
    check_pattern = rf'name: "{re.escape(module)}",\s*\n\s*enabled: false'
    if not re.search(check_pattern, content):
        # Add enabled: false after the name line
        old = f'name: "{module}",'
        new = f'name: "{module}",\n    enabled: false,'
        if old in content:
            content = content.replace(old, new, 1)
            disabled_count += 1
            print(f"Disabled: {module}")

with open(filepath, "w") as f:
    f.write(content)

print(f"Total disabled: {disabled_count}")
