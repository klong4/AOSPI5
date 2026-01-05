/*
 * Copyright (C) 2024 The Android Open Source Project
 * GPIO Background Service
 */

package com.raspberrypi.settings;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.Handler;
import android.os.Looper;

public class GpioService extends Service {

    private static final String CHANNEL_ID = "gpio_service_channel";
    private Handler handler;
    private Runnable monitorRunnable;

    @Override
    public void onCreate() {
        super.onCreate();
        createNotificationChannel();
        handler = new Handler(Looper.getMainLooper());
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Notification notification = new Notification.Builder(this, CHANNEL_ID)
            .setContentTitle("GPIO Service")
            .setContentText("Monitoring GPIO pins")
            .setSmallIcon(android.R.drawable.ic_menu_manage)
            .build();

        startForeground(1, notification);

        // Start monitoring
        startMonitoring();

        return START_STICKY;
    }

    private void createNotificationChannel() {
        NotificationChannel channel = new NotificationChannel(
            CHANNEL_ID,
            "GPIO Service",
            NotificationManager.IMPORTANCE_LOW
        );
        channel.setDescription("Background service for GPIO monitoring");
        
        NotificationManager manager = getSystemService(NotificationManager.class);
        manager.createNotificationChannel(channel);
    }

    private void startMonitoring() {
        monitorRunnable = new Runnable() {
            @Override
            public void run() {
                // Monitor GPIO pins for changes
                // This could be enhanced to watch for specific pin changes
                handler.postDelayed(this, 1000);
            }
        };
        handler.post(monitorRunnable);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (handler != null && monitorRunnable != null) {
            handler.removeCallbacks(monitorRunnable);
        }
    }
}
