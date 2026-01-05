/*
 * Copyright (C) 2024 The Android Open Source Project
 * GPIO Controller Main Activity
 */

package com.raspberrypi.gpio;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    private RecyclerView pinGrid;
    private GpioAdapter adapter;
    private Handler handler;
    private Runnable updateRunnable;
    
    // All GPIO pins on Pi 5 40-pin header
    private static final int[][] PIN_LAYOUT = {
        {-1, 1, 2, -1},   // 3.3V, 5V
        {2, 3, 4, -1},    // GPIO2 (SDA), 5V
        {3, 5, 6, -1},    // GPIO3 (SCL), GND
        {4, 7, 8, 14},    // GPIO4, GPIO14 (TXD)
        {-1, 9, 10, 15},  // GND, GPIO15 (RXD)
        {17, 11, 12, 18}, // GPIO17, GPIO18
        {27, 13, 14, -1}, // GPIO27, GND
        {22, 15, 16, 23}, // GPIO22, GPIO23
        {-1, 17, 18, 24}, // 3.3V, GPIO24
        {10, 19, 20, -1}, // GPIO10 (MOSI), GND
        {9, 21, 22, 25},  // GPIO9 (MISO), GPIO25
        {11, 23, 24, 8},  // GPIO11 (SCLK), GPIO8 (CE0)
        {-1, 25, 26, 7},  // GND, GPIO7 (CE1)
        {0, 27, 28, 1},   // ID_SD, ID_SC
        {5, 29, 30, -1},  // GPIO5, GND
        {6, 31, 32, 12},  // GPIO6, GPIO12
        {13, 33, 34, -1}, // GPIO13, GND
        {19, 35, 36, 16}, // GPIO19, GPIO16
        {26, 37, 38, 20}, // GPIO26, GPIO20
        {-1, 39, 40, 21}  // GND, GPIO21
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        pinGrid = findViewById(R.id.pin_grid);
        pinGrid.setLayoutManager(new GridLayoutManager(this, 4));
        
        adapter = new GpioAdapter();
        pinGrid.setAdapter(adapter);
        
        handler = new Handler(Looper.getMainLooper());
        
        setupButtons();
        startPolling();
    }

    private void setupButtons() {
        Button exportAllBtn = findViewById(R.id.export_all_btn);
        exportAllBtn.setOnClickListener(v -> exportAllPins());
        
        Button unexportAllBtn = findViewById(R.id.unexport_all_btn);
        unexportAllBtn.setOnClickListener(v -> unexportAllPins());
    }

    private void startPolling() {
        updateRunnable = new Runnable() {
            @Override
            public void run() {
                adapter.updatePinStates();
                handler.postDelayed(this, 500);
            }
        };
        handler.post(updateRunnable);
    }

    private void exportAllPins() {
        for (int i = 0; i < 28; i++) {
            try {
                writeFile("/sys/class/gpio/export", String.valueOf(i));
            } catch (IOException e) {
                // Pin may already be exported
            }
        }
        Toast.makeText(this, "All GPIO pins exported", Toast.LENGTH_SHORT).show();
    }

    private void unexportAllPins() {
        for (int i = 0; i < 28; i++) {
            try {
                writeFile("/sys/class/gpio/unexport", String.valueOf(i));
            } catch (IOException e) {
                // Pin may not be exported
            }
        }
        Toast.makeText(this, "All GPIO pins unexported", Toast.LENGTH_SHORT).show();
    }

    private void writeFile(String path, String value) throws IOException {
        FileWriter writer = new FileWriter(path);
        writer.write(value);
        writer.close();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (handler != null && updateRunnable != null) {
            handler.removeCallbacks(updateRunnable);
        }
    }

    class GpioAdapter extends RecyclerView.Adapter<GpioAdapter.PinViewHolder> {
        
        private List<GpioPin> pins = new ArrayList<>();
        
        GpioAdapter() {
            // Create flat list of pins from layout
            for (int i = 0; i < 28; i++) {
                pins.add(new GpioPin(i));
            }
        }

        @Override
        public PinViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_gpio_pin, parent, false);
            return new PinViewHolder(view);
        }

        @Override
        public void onBindViewHolder(PinViewHolder holder, int position) {
            GpioPin pin = pins.get(position);
            holder.bind(pin);
        }

        @Override
        public int getItemCount() {
            return pins.size();
        }

        void updatePinStates() {
            for (GpioPin pin : pins) {
                pin.readState();
            }
            notifyDataSetChanged();
        }

        class PinViewHolder extends RecyclerView.ViewHolder {
            TextView pinNumber;
            TextView pinState;
            ImageView pinIndicator;
            View itemView;

            PinViewHolder(View view) {
                super(view);
                this.itemView = view;
                pinNumber = view.findViewById(R.id.pin_number);
                pinState = view.findViewById(R.id.pin_state);
                pinIndicator = view.findViewById(R.id.pin_indicator);
            }

            void bind(GpioPin pin) {
                pinNumber.setText("GPIO" + pin.number);
                pinState.setText(pin.isExported ? 
                    (pin.isOutput ? "OUT" : "IN") + ":" + pin.value : "---");
                
                if (pin.isExported) {
                    pinIndicator.setBackgroundColor(pin.value == 1 ? 
                        0xFF4CAF50 : 0xFFF44336);
                } else {
                    pinIndicator.setBackgroundColor(0xFF9E9E9E);
                }

                itemView.setOnClickListener(v -> togglePin(pin));
            }

            void togglePin(GpioPin pin) {
                if (!pin.isExported) {
                    pin.export();
                } else if (pin.isOutput) {
                    pin.setValue(pin.value == 0 ? 1 : 0);
                } else {
                    pin.setOutput(true);
                }
                pin.readState();
                notifyItemChanged(getAdapterPosition());
            }
        }
    }

    class GpioPin {
        int number;
        boolean isExported;
        boolean isOutput;
        int value;

        GpioPin(int number) {
            this.number = number;
            readState();
        }

        void readState() {
            String basePath = "/sys/class/gpio/gpio" + number;
            try {
                // Check if exported
                java.io.File dir = new java.io.File(basePath);
                isExported = dir.exists();
                
                if (isExported) {
                    String direction = readFile(basePath + "/direction");
                    isOutput = "out".equals(direction.trim());
                    
                    String val = readFile(basePath + "/value");
                    value = Integer.parseInt(val.trim());
                }
            } catch (Exception e) {
                isExported = false;
            }
        }

        void export() {
            try {
                writeFile("/sys/class/gpio/export", String.valueOf(number));
                isExported = true;
            } catch (IOException e) {
                // Already exported
            }
        }

        void setOutput(boolean output) {
            if (!isExported) return;
            try {
                writeFile("/sys/class/gpio/gpio" + number + "/direction",
                         output ? "out" : "in");
                isOutput = output;
            } catch (IOException e) {
                // Failed
            }
        }

        void setValue(int val) {
            if (!isExported || !isOutput) return;
            try {
                writeFile("/sys/class/gpio/gpio" + number + "/value",
                         String.valueOf(val));
                value = val;
            } catch (IOException e) {
                // Failed
            }
        }

        private String readFile(String path) throws IOException {
            BufferedReader reader = new BufferedReader(new FileReader(path));
            String line = reader.readLine();
            reader.close();
            return line;
        }

        private void writeFile(String path, String val) throws IOException {
            FileWriter writer = new FileWriter(path);
            writer.write(val);
            writer.close();
        }
    }
}
