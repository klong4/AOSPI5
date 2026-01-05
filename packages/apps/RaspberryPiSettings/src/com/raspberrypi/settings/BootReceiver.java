/*
 * Copyright (C) 2024 The Android Open Source Project
 * Boot Receiver for Raspberry Pi Settings
 */

package com.raspberrypi.settings;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class BootReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
            // Start GPIO service
            Intent serviceIntent = new Intent(context, GpioService.class);
            context.startForegroundService(serviceIntent);
        }
    }
}
