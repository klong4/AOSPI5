// Copyright (C) 2024 The Android Open Source Project
// Audio HAL implementation for Raspberry Pi 5

#define LOG_TAG "AudioHAL"

#include <android-base/logging.h>
#include <hardware/audio.h>
#include <hardware/hardware.h>
#include <system/audio.h>
#include <tinyalsa/asoundlib.h>
#include "Audio.h"
#include "AudioDevice.h"

namespace android {
namespace hardware {
namespace audio {

// Default audio configuration for Pi 5
static const int DEFAULT_CARD = 0;
static const int DEFAULT_DEVICE = 0;
static const int DEFAULT_SAMPLE_RATE = 48000;
static const int DEFAULT_CHANNELS = 2;
static const int DEFAULT_PERIOD_SIZE = 1024;
static const int DEFAULT_PERIOD_COUNT = 4;

AudioHAL::AudioHAL() {
    LOG(INFO) << "AudioHAL constructor";
    enumerateDevices();
}

AudioHAL::~AudioHAL() {
    LOG(INFO) << "AudioHAL destructor";
}

void AudioHAL::enumerateDevices() {
    // Check for HDMI audio
    struct mixer* hdmiMixer = mixer_open(0);
    if (hdmiMixer) {
        mOutputDevices.push_back({
            .name = "HDMI Audio",
            .card = 0,
            .device = 0,
            .type = AUDIO_DEVICE_OUT_AUX_DIGITAL,
        });
        mixer_close(hdmiMixer);
    }

    // Check for headphone jack (3.5mm audio)
    struct mixer* headphoneMixer = mixer_open(1);
    if (headphoneMixer) {
        mOutputDevices.push_back({
            .name = "Headphone Jack",
            .card = 1,
            .device = 0,
            .type = AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
        });
        mixer_close(headphoneMixer);
    }

    // USB Audio (dynamic)
    for (int card = 2; card < 8; card++) {
        struct mixer* usbMixer = mixer_open(card);
        if (usbMixer) {
            mOutputDevices.push_back({
                .name = "USB Audio",
                .card = card,
                .device = 0,
                .type = AUDIO_DEVICE_OUT_USB_DEVICE,
            });
            mInputDevices.push_back({
                .name = "USB Microphone",
                .card = card,
                .device = 0,
                .type = AUDIO_DEVICE_IN_USB_DEVICE,
            });
            mixer_close(usbMixer);
        }
    }

    LOG(INFO) << "Found " << mOutputDevices.size() << " output devices, "
              << mInputDevices.size() << " input devices";
}

int AudioHAL::openOutputStream(audio_io_handle_t handle,
                                audio_devices_t devices,
                                audio_output_flags_t flags,
                                struct audio_config* config,
                                struct audio_stream_out** streamOut) {
    // Find appropriate device
    int card = DEFAULT_CARD;
    int device = DEFAULT_DEVICE;

    for (const auto& dev : mOutputDevices) {
        if (dev.type & devices) {
            card = dev.card;
            device = dev.device;
            break;
        }
    }

    auto stream = std::make_unique<AudioStreamOut>(card, device, config);
    if (!stream->isValid()) {
        return -ENODEV;
    }

    *streamOut = stream->getStream();
    mOutputStreams[handle] = std::move(stream);

    return 0;
}

int AudioHAL::openInputStream(audio_io_handle_t handle,
                               audio_devices_t devices,
                               struct audio_config* config,
                               struct audio_stream_in** streamIn) {
    // Find appropriate device
    int card = DEFAULT_CARD;
    int device = DEFAULT_DEVICE;

    for (const auto& dev : mInputDevices) {
        if (dev.type & devices) {
            card = dev.card;
            device = dev.device;
            break;
        }
    }

    auto stream = std::make_unique<AudioStreamIn>(card, device, config);
    if (!stream->isValid()) {
        return -ENODEV;
    }

    *streamIn = stream->getStream();
    mInputStreams[handle] = std::move(stream);

    return 0;
}

void AudioHAL::closeOutputStream(audio_io_handle_t handle) {
    auto it = mOutputStreams.find(handle);
    if (it != mOutputStreams.end()) {
        mOutputStreams.erase(it);
    }
}

void AudioHAL::closeInputStream(audio_io_handle_t handle) {
    auto it = mInputStreams.find(handle);
    if (it != mInputStreams.end()) {
        mInputStreams.erase(it);
    }
}

int AudioHAL::setMasterVolume(float volume) {
    mMasterVolume = volume;
    // Apply to all active streams
    for (auto& pair : mOutputStreams) {
        pair.second->setVolume(volume, volume);
    }
    return 0;
}

int AudioHAL::getMasterVolume(float* volume) {
    *volume = mMasterVolume;
    return 0;
}

int AudioHAL::setMasterMute(bool mute) {
    mMasterMute = mute;
    return 0;
}

int AudioHAL::getMasterMute(bool* mute) {
    *mute = mMasterMute;
    return 0;
}

// AudioStreamOut implementation
AudioStreamOut::AudioStreamOut(int card, int device, struct audio_config* config) 
    : mCard(card), mDevice(device), mPcm(nullptr), mValid(false) {
    
    mConfig = {
        .channels = DEFAULT_CHANNELS,
        .rate = config->sample_rate ? config->sample_rate : DEFAULT_SAMPLE_RATE,
        .period_size = DEFAULT_PERIOD_SIZE,
        .period_count = DEFAULT_PERIOD_COUNT,
        .format = PCM_FORMAT_S16_LE,
        .start_threshold = DEFAULT_PERIOD_SIZE * 2,
        .stop_threshold = DEFAULT_PERIOD_SIZE * DEFAULT_PERIOD_COUNT,
    };

    // Update config with actual values
    config->sample_rate = mConfig.rate;
    config->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    config->format = AUDIO_FORMAT_PCM_16_BIT;

    mValid = true;
}

AudioStreamOut::~AudioStreamOut() {
    if (mPcm) {
        pcm_close(mPcm);
    }
}

int AudioStreamOut::start() {
    if (!mPcm) {
        mPcm = pcm_open(mCard, mDevice, PCM_OUT, &mConfig);
        if (!pcm_is_ready(mPcm)) {
            LOG(ERROR) << "Failed to open PCM: " << pcm_get_error(mPcm);
            pcm_close(mPcm);
            mPcm = nullptr;
            return -ENODEV;
        }
    }
    return 0;
}

ssize_t AudioStreamOut::write(const void* buffer, size_t bytes) {
    if (start() != 0) {
        return -ENODEV;
    }

    int ret = pcm_write(mPcm, buffer, bytes);
    if (ret != 0) {
        LOG(ERROR) << "PCM write error: " << pcm_get_error(mPcm);
        return -EIO;
    }

    return bytes;
}

int AudioStreamOut::setVolume(float left, float right) {
    mVolumeLeft = left;
    mVolumeRight = right;
    return 0;
}

// AudioStreamIn implementation
AudioStreamIn::AudioStreamIn(int card, int device, struct audio_config* config)
    : mCard(card), mDevice(device), mPcm(nullptr), mValid(false) {

    mConfig = {
        .channels = config->channel_mask == AUDIO_CHANNEL_IN_MONO ? 1u : 2u,
        .rate = config->sample_rate ? config->sample_rate : DEFAULT_SAMPLE_RATE,
        .period_size = DEFAULT_PERIOD_SIZE,
        .period_count = DEFAULT_PERIOD_COUNT,
        .format = PCM_FORMAT_S16_LE,
        .start_threshold = 1,
        .stop_threshold = DEFAULT_PERIOD_SIZE * DEFAULT_PERIOD_COUNT,
    };

    config->sample_rate = mConfig.rate;
    config->format = AUDIO_FORMAT_PCM_16_BIT;

    mValid = true;
}

AudioStreamIn::~AudioStreamIn() {
    if (mPcm) {
        pcm_close(mPcm);
    }
}

int AudioStreamIn::start() {
    if (!mPcm) {
        mPcm = pcm_open(mCard, mDevice, PCM_IN, &mConfig);
        if (!pcm_is_ready(mPcm)) {
            LOG(ERROR) << "Failed to open PCM for capture: " << pcm_get_error(mPcm);
            pcm_close(mPcm);
            mPcm = nullptr;
            return -ENODEV;
        }
    }
    return 0;
}

ssize_t AudioStreamIn::read(void* buffer, size_t bytes) {
    if (start() != 0) {
        return -ENODEV;
    }

    int ret = pcm_read(mPcm, buffer, bytes);
    if (ret != 0) {
        LOG(ERROR) << "PCM read error: " << pcm_get_error(mPcm);
        return -EIO;
    }

    return bytes;
}

}  // namespace audio
}  // namespace hardware
}  // namespace android
