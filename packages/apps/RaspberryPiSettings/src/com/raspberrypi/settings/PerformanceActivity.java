/*
 * Copyright (C) 2024 The Android Open Source Project
 * Performance Settings Activity
 */

package com.raspberrypi.settings;

import android.os.Bundle;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

public class PerformanceActivity extends AppCompatActivity {

    private TextView cpuFreqText;
    private TextView gpuFreqText;
    private TextView tempText;
    private SeekBar cpuSeekBar;
    private SeekBar gpuSeekBar;
    private Switch overclockSwitch;

    // CPU frequencies for Pi 5 (in KHz)
    private static final int[] CPU_FREQS = {
        1500000, 1800000, 2000000, 2200000, 2400000
    };

    // GPU frequencies for Pi 5 (in MHz)
    private static final int[] GPU_FREQS = {
        500, 600, 700, 800, 910
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_performance);

        cpuFreqText = findViewById(R.id.cpu_freq_text);
        gpuFreqText = findViewById(R.id.gpu_freq_text);
        tempText = findViewById(R.id.temp_text);
        cpuSeekBar = findViewById(R.id.cpu_seekbar);
        gpuSeekBar = findViewById(R.id.gpu_seekbar);
        overclockSwitch = findViewById(R.id.overclock_switch);

        setupSeekBars();
        setupOverclockSwitch();
        updateStats();
    }

    private void setupSeekBars() {
        cpuSeekBar.setMax(CPU_FREQS.length - 1);
        cpuSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                int freq = CPU_FREQS[progress];
                cpuFreqText.setText(String.format("CPU: %d MHz", freq / 1000));
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                applyCpuFrequency(CPU_FREQS[seekBar.getProgress()]);
            }
        });

        gpuSeekBar.setMax(GPU_FREQS.length - 1);
        gpuSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                int freq = GPU_FREQS[progress];
                gpuFreqText.setText(String.format("GPU: %d MHz", freq));
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                applyGpuFrequency(GPU_FREQS[seekBar.getProgress()]);
            }
        });

        // Set initial positions
        cpuSeekBar.setProgress(4); // 2.4 GHz default
        gpuSeekBar.setProgress(4); // 910 MHz default
    }

    private void setupOverclockSwitch() {
        overclockSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                Toast.makeText(this, 
                    "Warning: Overclocking may require additional cooling",
                    Toast.LENGTH_LONG).show();
            }
        });
    }

    private void applyCpuFrequency(int freqKHz) {
        try {
            String path = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq";
            writeFile(path, String.valueOf(freqKHz));
            Toast.makeText(this, "CPU frequency updated", Toast.LENGTH_SHORT).show();
        } catch (IOException e) {
            Toast.makeText(this, "Failed to set CPU frequency", Toast.LENGTH_SHORT).show();
        }
    }

    private void applyGpuFrequency(int freqMHz) {
        // GPU frequency is typically set via config.txt
        // This is a placeholder for runtime adjustment if supported
        Toast.makeText(this, 
            "GPU frequency requires config.txt edit and reboot",
            Toast.LENGTH_SHORT).show();
    }

    private void updateStats() {
        // Update temperature
        try {
            String temp = readFile("/sys/class/thermal/thermal_zone0/temp");
            float tempC = Float.parseFloat(temp.trim()) / 1000.0f;
            tempText.setText(String.format("Temperature: %.1f°C", tempC));
        } catch (Exception e) {
            tempText.setText("Temperature: --°C");
        }

        // Update current CPU frequency
        try {
            String freq = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
            int freqKHz = Integer.parseInt(freq.trim());
            cpuFreqText.setText(String.format("CPU: %d MHz", freqKHz / 1000));
        } catch (Exception e) {
            cpuFreqText.setText("CPU: -- MHz");
        }
    }

    private String readFile(String path) throws IOException {
        BufferedReader reader = new BufferedReader(new FileReader(path));
        String line = reader.readLine();
        reader.close();
        return line;
    }

    private void writeFile(String path, String value) throws IOException {
        FileWriter writer = new FileWriter(path);
        writer.write(value);
        writer.close();
    }

    @Override
    protected void onResume() {
        super.onResume();
        updateStats();
    }
}
