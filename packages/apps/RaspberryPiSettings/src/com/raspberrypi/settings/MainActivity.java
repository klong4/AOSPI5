/*
 * Copyright (C) 2024 The Android Open Source Project
 * Raspberry Pi Settings Main Activity
 */

package com.raspberrypi.settings;

import android.os.Bundle;
import android.content.Intent;
import android.view.View;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.cardview.widget.CardView;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        setupCards();
        updateDeviceInfo();
    }

    private void setupCards() {
        CardView gpioCard = findViewById(R.id.gpio_card);
        gpioCard.setOnClickListener(v -> {
            startActivity(new Intent(this, GpioActivity.class));
        });

        CardView displayCard = findViewById(R.id.display_card);
        displayCard.setOnClickListener(v -> {
            startActivity(new Intent(this, DisplayActivity.class));
        });

        CardView performanceCard = findViewById(R.id.performance_card);
        performanceCard.setOnClickListener(v -> {
            startActivity(new Intent(this, PerformanceActivity.class));
        });

        CardView aboutCard = findViewById(R.id.about_card);
        aboutCard.setOnClickListener(v -> {
            startActivity(new Intent(this, AboutActivity.class));
        });
    }

    private void updateDeviceInfo() {
        TextView tempView = findViewById(R.id.cpu_temp);
        TextView freqView = findViewById(R.id.cpu_freq);

        // Read CPU temperature
        try {
            String temp = readFile("/sys/class/thermal/thermal_zone0/temp");
            float tempC = Float.parseFloat(temp.trim()) / 1000.0f;
            tempView.setText(String.format("CPU: %.1f°C", tempC));
        } catch (Exception e) {
            tempView.setText("CPU: --°C");
        }

        // Read CPU frequency
        try {
            String freq = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
            float freqMHz = Float.parseFloat(freq.trim()) / 1000.0f;
            freqView.setText(String.format("Freq: %.0f MHz", freqMHz));
        } catch (Exception e) {
            freqView.setText("Freq: -- MHz");
        }
    }

    private String readFile(String path) throws IOException {
        BufferedReader reader = new BufferedReader(new FileReader(path));
        String line = reader.readLine();
        reader.close();
        return line;
    }

    @Override
    protected void onResume() {
        super.onResume();
        updateDeviceInfo();
    }
}
