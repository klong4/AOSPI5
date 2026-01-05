#!/usr/bin/env python3
import re

with open("/home/klong/aosp-rpi5/packages/modules/Profiling/apex/Android.bp", "r") as f:
    content = f.read()

# Add enabled condition to sdk module
old_sdk = '''sdk {
    name: "profiling-module-sdk",
    apexes: [
        "com.android.profiling",
    ],
}'''

new_sdk = '''sdk {
    enabled: select(release_flag("RELEASE_PACKAGE_PROFILING_MODULE"), {
        true: true,
        false: false,
    }),
    name: "profiling-module-sdk",
    apexes: [
        "com.android.profiling",
    ],
}'''

content = content.replace(old_sdk, new_sdk)

with open("/home/klong/aosp-rpi5/packages/modules/Profiling/apex/Android.bp", "w") as f:
    f.write(content)

print("Fixed profiling-module-sdk")
