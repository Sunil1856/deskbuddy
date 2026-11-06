#pragma once
// ============================================================================
// DeskBuddy — I2S Audio Driver (INMP441 Microphone & MAX98357A Speaker)
// ============================================================================
#include <Arduino.h>
#include <driver/i2s.h>
#include "config.h"

class AudioI2S {
public:
    AudioI2S();
    ~AudioI2S();

    bool begin();
    void end();

    // Microphone (INMP441 on I2S_NUM_0)
    int readMicrophone(int16_t* outBuffer, int maxSamples, float gain = 2.5f);
    float getAudioRMS(const int16_t* samples, int count);

    // Speaker / DAC (MAX98357A on I2S_NUM_1)
    size_t writeSpeaker(const uint8_t* pcmData, size_t bytes);
    void writeSpeakerSilence(int ms = 50);
    void flushSpeaker();
    void setVolume(float volume); // 0.0 to 1.0
    float getVolume() const { return _volume; }

    // Helper: Build standard 44-byte WAV header
    static void buildWavHeader(uint8_t* header, uint32_t pcmDataSize, 
                               uint32_t sampleRate = AUDIO_SAMPLE_RATE, 
                               uint16_t channels = 1, 
                               uint16_t bitsPerSample = AUDIO_BITS_PER_SAMPLE);

private:
    bool _micInitialized = false;
    bool _spkInitialized = false;
    float _volume = 0.85f;

    bool _initMicrophone();
    bool _initSpeaker();
};
