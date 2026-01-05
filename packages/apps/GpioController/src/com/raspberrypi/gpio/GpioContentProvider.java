/*
 * Copyright (C) 2024 The Android Open Source Project
 * GPIO Content Provider for inter-app communication
 */

package com.raspberrypi.gpio;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

public class GpioContentProvider extends ContentProvider {

    public static final String AUTHORITY = "com.raspberrypi.gpio.provider";
    public static final Uri CONTENT_URI = Uri.parse("content://" + AUTHORITY);

    private static final int GPIO_ALL = 1;
    private static final int GPIO_PIN = 2;

    private static final UriMatcher uriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
    static {
        uriMatcher.addURI(AUTHORITY, "gpio", GPIO_ALL);
        uriMatcher.addURI(AUTHORITY, "gpio/#", GPIO_PIN);
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
                       String[] selectionArgs, String sortOrder) {
        MatrixCursor cursor = new MatrixCursor(new String[]{
            "pin", "exported", "direction", "value"
        });

        int match = uriMatcher.match(uri);
        if (match == GPIO_ALL) {
            for (int i = 0; i < 28; i++) {
                addPinToCursor(cursor, i);
            }
        } else if (match == GPIO_PIN) {
            int pin = Integer.parseInt(uri.getLastPathSegment());
            addPinToCursor(cursor, pin);
        }

        return cursor;
    }

    private void addPinToCursor(MatrixCursor cursor, int pin) {
        String basePath = "/sys/class/gpio/gpio" + pin;
        java.io.File dir = new java.io.File(basePath);
        boolean exported = dir.exists();
        String direction = "none";
        int value = 0;

        if (exported) {
            try {
                direction = readFile(basePath + "/direction").trim();
                value = Integer.parseInt(readFile(basePath + "/value").trim());
            } catch (Exception e) {
                // Ignore
            }
        }

        cursor.addRow(new Object[]{pin, exported ? 1 : 0, direction, value});
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        // Export a pin
        int pin = values.getAsInteger("pin");
        try {
            writeFile("/sys/class/gpio/export", String.valueOf(pin));
            
            if (values.containsKey("direction")) {
                writeFile("/sys/class/gpio/gpio" + pin + "/direction",
                         values.getAsString("direction"));
            }
        } catch (IOException e) {
            // Ignore
        }
        return Uri.withAppendedPath(CONTENT_URI, "gpio/" + pin);
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        int match = uriMatcher.match(uri);
        if (match != GPIO_PIN) return 0;

        int pin = Integer.parseInt(uri.getLastPathSegment());
        String basePath = "/sys/class/gpio/gpio" + pin;

        try {
            if (values.containsKey("direction")) {
                writeFile(basePath + "/direction", values.getAsString("direction"));
            }
            if (values.containsKey("value")) {
                writeFile(basePath + "/value", values.getAsString("value"));
            }
            return 1;
        } catch (IOException e) {
            return 0;
        }
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        // Unexport a pin
        int match = uriMatcher.match(uri);
        if (match != GPIO_PIN) return 0;

        int pin = Integer.parseInt(uri.getLastPathSegment());
        try {
            writeFile("/sys/class/gpio/unexport", String.valueOf(pin));
            return 1;
        } catch (IOException e) {
            return 0;
        }
    }

    @Override
    public String getType(Uri uri) {
        return "vnd.android.cursor.dir/vnd.raspberrypi.gpio";
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
