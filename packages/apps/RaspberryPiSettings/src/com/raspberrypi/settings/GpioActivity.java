/*
 * Copyright (C) 2024 The Android Open Source Project
 * GPIO Configuration Activity
 */

package com.raspberrypi.settings;

import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

public class GpioActivity extends AppCompatActivity {

    private Spinner pinSpinner;
    private Spinner modeSpinner;
    private Switch valueSwitch;
    private TextView statusText;

    // GPIO pins available on Pi 5 header
    private static final int[] GPIO_PINS = {
        2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
        14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27
    };

    private static final String[] PIN_MODES = {"Input", "Output"};

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_gpio);

        pinSpinner = findViewById(R.id.pin_spinner);
        modeSpinner = findViewById(R.id.mode_spinner);
        valueSwitch = findViewById(R.id.value_switch);
        statusText = findViewById(R.id.status_text);

        setupSpinners();
        setupButtons();
    }

    private void setupSpinners() {
        String[] pinNames = new String[GPIO_PINS.length];
        for (int i = 0; i < GPIO_PINS.length; i++) {
            pinNames[i] = "GPIO " + GPIO_PINS[i];
        }

        ArrayAdapter<String> pinAdapter = new ArrayAdapter<>(
            this, android.R.layout.simple_spinner_item, pinNames);
        pinAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        pinSpinner.setAdapter(pinAdapter);

        ArrayAdapter<String> modeAdapter = new ArrayAdapter<>(
            this, android.R.layout.simple_spinner_item, PIN_MODES);
        modeAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        modeSpinner.setAdapter(modeAdapter);
    }

    private void setupButtons() {
        Button applyButton = findViewById(R.id.apply_button);
        applyButton.setOnClickListener(v -> applyGpioConfig());

        Button readButton = findViewById(R.id.read_button);
        readButton.setOnClickListener(v -> readGpioValue());

        valueSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (modeSpinner.getSelectedItemPosition() == 1) { // Output mode
                writeGpioValue(isChecked ? 1 : 0);
            }
        });
    }

    private void applyGpioConfig() {
        int pinIndex = pinSpinner.getSelectedItemPosition();
        int pin = GPIO_PINS[pinIndex];
        int mode = modeSpinner.getSelectedItemPosition();
        String direction = (mode == 0) ? "in" : "out";

        try {
            // Export GPIO
            writeFile("/sys/class/gpio/export", String.valueOf(pin));
            
            // Set direction
            writeFile("/sys/class/gpio/gpio" + pin + "/direction", direction);
            
            statusText.setText("GPIO " + pin + " configured as " + PIN_MODES[mode]);
            Toast.makeText(this, "GPIO configured", Toast.LENGTH_SHORT).show();
        } catch (IOException e) {
            statusText.setText("Error: " + e.getMessage());
            Toast.makeText(this, "Failed to configure GPIO", Toast.LENGTH_SHORT).show();
        }
    }

    private void readGpioValue() {
        int pinIndex = pinSpinner.getSelectedItemPosition();
        int pin = GPIO_PINS[pinIndex];

        try {
            String value = readFile("/sys/class/gpio/gpio" + pin + "/value");
            int val = Integer.parseInt(value.trim());
            valueSwitch.setChecked(val == 1);
            statusText.setText("GPIO " + pin + " value: " + val);
        } catch (IOException e) {
            statusText.setText("Error reading GPIO: " + e.getMessage());
        }
    }

    private void writeGpioValue(int value) {
        int pinIndex = pinSpinner.getSelectedItemPosition();
        int pin = GPIO_PINS[pinIndex];

        try {
            writeFile("/sys/class/gpio/gpio" + pin + "/value", String.valueOf(value));
            statusText.setText("GPIO " + pin + " set to " + value);
        } catch (IOException e) {
            statusText.setText("Error writing GPIO: " + e.getMessage());
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
}
