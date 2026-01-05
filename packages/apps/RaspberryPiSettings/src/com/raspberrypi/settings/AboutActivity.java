/*
 * Copyright (C) 2024 The Android Open Source Project
 * About Device Activity
 */

package com.raspberrypi.settings;

import android.os.Build;
import android.os.Bundle;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class AboutActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_about);

        populateDeviceInfo();
    }

    private void populateDeviceInfo() {
        setText(R.id.device_name, "Raspberry Pi 5");
        setText(R.id.android_version, "Android " + Build.VERSION.RELEASE);
        setText(R.id.build_number, Build.DISPLAY);
        setText(R.id.kernel_version, getKernelVersion());
        setText(R.id.cpu_info, getCpuInfo());
        setText(R.id.memory_info, getMemoryInfo());
        setText(R.id.serial_number, getSerialNumber());
    }

    private void setText(int viewId, String text) {
        TextView view = findViewById(viewId);
        if (view != null) {
            view.setText(text);
        }
    }

    private String getKernelVersion() {
        try {
            Process process = Runtime.getRuntime().exec("uname -r");
            BufferedReader reader = new BufferedReader(
                new InputStreamReader(process.getInputStream()));
            String line = reader.readLine();
            reader.close();
            return line != null ? line : "Unknown";
        } catch (IOException e) {
            return "Unknown";
        }
    }

    private String getCpuInfo() {
        try {
            BufferedReader reader = new BufferedReader(
                new FileReader("/proc/cpuinfo"));
            String line;
            String model = "BCM2712";
            int cores = 0;
            
            while ((line = reader.readLine()) != null) {
                if (line.startsWith("model name")) {
                    model = line.split(":")[1].trim();
                } else if (line.startsWith("processor")) {
                    cores++;
                }
            }
            reader.close();
            
            return String.format("%s (%d cores)", model, cores);
        } catch (IOException e) {
            return "Quad-core Cortex-A76 @ 2.4GHz";
        }
    }

    private String getMemoryInfo() {
        try {
            BufferedReader reader = new BufferedReader(
                new FileReader("/proc/meminfo"));
            String line;
            long totalKb = 0;
            
            while ((line = reader.readLine()) != null) {
                if (line.startsWith("MemTotal:")) {
                    String[] parts = line.split("\\s+");
                    totalKb = Long.parseLong(parts[1]);
                    break;
                }
            }
            reader.close();
            
            long totalMb = totalKb / 1024;
            long totalGb = totalMb / 1024;
            
            if (totalGb >= 1) {
                return String.format("%d GB RAM", totalGb);
            } else {
                return String.format("%d MB RAM", totalMb);
            }
        } catch (IOException e) {
            return "Unknown";
        }
    }

    private String getSerialNumber() {
        try {
            BufferedReader reader = new BufferedReader(
                new FileReader("/proc/cpuinfo"));
            String line;
            
            while ((line = reader.readLine()) != null) {
                if (line.startsWith("Serial")) {
                    reader.close();
                    return line.split(":")[1].trim();
                }
            }
            reader.close();
            return "Unknown";
        } catch (IOException e) {
            return Build.SERIAL;
        }
    }
}
