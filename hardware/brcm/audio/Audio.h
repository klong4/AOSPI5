// Copyright (C) 2024 The Android Open Source Project

#pragma once

#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>
#include <memory>
#include <map>
#include <vector>
#include <string>

namespace android {
namespace hardware {
namespace audio {

struct AudioDeviceInfo {
    std::string name;
    int card;
    int device;
    audio_devices_t type;
};

class AudioStreamOut;
class AudioStreamIn;

class AudioHAL {
public:
    AudioHAL();
    ~AudioHAL();

    int openOutputStream(audio_io_handle_t handle,
                         audio_devices_t devices,
                         audio_output_flags_t flags,
                         struct audio_config* config,
                         struct audio_stream_out** streamOut);

    int openInputStream(audio_io_handle_t handle,
                        audio_devices_t devices,
                        struct audio_config* config,
                        struct audio_stream_in** streamIn);

    void closeOutputStream(audio_io_handle_t handle);
    void closeInputStream(audio_io_handle_t handle);

    int setMasterVolume(float volume);
    int getMasterVolume(float* volume);
    int setMasterMute(bool mute);
    int getMasterMute(bool* mute);

    const std::vector<AudioDeviceInfo>& getOutputDevices() const { return mOutputDevices; }
    const std::vector<AudioDeviceInfo>& getInputDevices() const { return mInputDevices; }

private:
    void enumerateDevices();

    std::vector<AudioDeviceInfo> mOutputDevices;
    std::vector<AudioDeviceInfo> mInputDevices;
    std::map<audio_io_handle_t, std::unique_ptr<AudioStreamOut>> mOutputStreams;
    std::map<audio_io_handle_t, std::unique_ptr<AudioStreamIn>> mInputStreams;

    float mMasterVolume = 1.0f;
    bool mMasterMute = false;
};

class AudioStreamOut {
public:
    AudioStreamOut(int card, int device, struct audio_config* config);
    ~AudioStreamOut();

    bool isValid() const { return mValid; }
    struct audio_stream_out* getStream() { return &mStream; }

    int start();
    ssize_t write(const void* buffer, size_t bytes);
    int setVolume(float left, float right);

private:
    int mCard;
    int mDevice;
    struct pcm* mPcm;
    struct pcm_config mConfig;
    struct audio_stream_out mStream;
    bool mValid;
    float mVolumeLeft = 1.0f;
    float mVolumeRight = 1.0f;
};

class AudioStreamIn {
public:
    AudioStreamIn(int card, int device, struct audio_config* config);
    ~AudioStreamIn();

    bool isValid() const { return mValid; }
    struct audio_stream_in* getStream() { return &mStream; }

    int start();
    ssize_t read(void* buffer, size_t bytes);

private:
    int mCard;
    int mDevice;
    struct pcm* mPcm;
    struct pcm_config mConfig;
    struct audio_stream_in mStream;
    bool mValid;
};

}  // namespace audio
}  // namespace hardware
}  // namespace android
