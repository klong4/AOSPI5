/*
 * Copyright (C) 2024 The Android Open Source Project
 * Display Settings Activity
 */

package com.raspberrypi.settings;

import android.os.Bundle;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import java.io.FileWriter;
import java.io.IOException;

public class DisplayActivity extends AppCompatActivity {

    private Spinner resolutionSpinner;
    private SeekBar brightnessSeekBar;
    private Switch hdmiBoostSwitch;
    private TextView brightnessText;

    private static final String[] RESOLUTIONS = {
        "Auto",
        "1920x1080 60Hz",
        "1920x1080 30Hz", 
        "1280x720 60Hz",
        "1280x720 50Hz",
        "720x480 60Hz",
        "3840x2160 30Hz",
        "3840x2160 60Hz"
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_display);

        resolutionSpinner = findViewById(R.id.resolution_spinner);
        brightnessSeekBar = findViewById(R.id.brightness_seekbar);
        hdmiBoostSwitch = findViewById(R.id.hdmi_boost_switch);
        brightnessText = findViewById(R.id.brightness_text);

        setupSpinner();
        setupBrightness();
        setupApplyButton();
    }

    private void setupSpinner() {
        ArrayAdapter<String> adapter = new ArrayAdapter<>(
            this, android.R.layout.simple_spinner_item, RESOLUTIONS);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        resolutionSpinner.setAdapter(adapter);
    }

    private void setupBrightness() {
        brightnessSeekBar.setMax(255);
        brightnessSeekBar.setProgress(255);
        
        brightnessSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                brightnessText.setText(String.format("Brightness: %d%%", (progress * 100) / 255));
                if (fromUser) {
                    applyBrightness(progress);
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });
    }

    private void setupApplyButton() {
        Button applyButton = findViewById(R.id.apply_button);
        applyButton.setOnClickListener(v -> applyDisplaySettings());
    }

    private void applyBrightness(int brightness) {
        try {
            // Try DSI backlight first
            writeFile("/sys/class/backlight/rpi_backlight/brightness", 
                      String.valueOf(brightness));
        } catch (IOException e) {
            // Try generic backlight
            try {
                writeFile("/sys/class/backlight/backlight/brightness", 
                          String.valueOf(brightness));
            } catch (IOException e2) {
                // Brightness control may not be available for HDMI
            }
        }
    }

    private void applyDisplaySettings() {
        int resIndex = resolutionSpinner.getSelectedItemPosition();
        boolean hdmiBoost = hdmiBoostSwitch.isChecked();

        // Display settings typically require config.txt changes and reboot
        Toast.makeText(this, 
            "Display changes require editing config.txt and rebooting",
            Toast.LENGTH_LONG).show();

        // Show instructions
        String instructions = "Add to /boot/config.txt:\n";
        if (resIndex > 0) {
            instructions += "hdmi_mode=" + getHdmiMode(resIndex) + "\n";
        }
        if (hdmiBoost) {
            instructions += "hdmi_boost=4\n";
        }
        
        Toast.makeText(this, instructions, Toast.LENGTH_LONG).show();
    }

    private int getHdmiMode(int resIndex) {
        // CEA modes for common resolutions
        switch (resIndex) {
            case 1: return 16; // 1080p 60Hz
            case 2: return 34; // 1080p 30Hz
            case 3: return 4;  // 720p 60Hz
            case 4: return 19; // 720p 50Hz
            case 5: return 3;  // 480p 60Hz
            case 6: return 95; // 4K 30Hz
            case 7: return 97; // 4K 60Hz
            default: return 0;
        }
    }

    private void writeFile(String path, String value) throws IOException {
        FileWriter writer = new FileWriter(path);
        writer.write(value);
        writer.close();
    }
}
