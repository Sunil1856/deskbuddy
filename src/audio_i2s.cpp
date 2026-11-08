// ============================================================================
// DeskBuddy — I2S Audio Driver Implementation
// ============================================================================
#include "audio_i2s.h"
#include <math.h>

AudioI2S::AudioI2S() {
    _micInitialized = false;
    _spkInitialized = false;
    _volume = 0.85f;
}

AudioI2S::~AudioI2S() {
    end();
}

bool AudioI2S::begin() {
    bool okMic = _initMicrophone();
    bool okSpk = _initSpeaker();
    Serial.printf("[I2S] Mic Init: %s | Speaker Init: %s\n", 
                  okMic ? "OK (GPIO 18,19,23)" : "FAILED", 
                  okSpk ? "OK (GPIO 25,26,33)" : "FAILED");
    return okMic || okSpk;
}

void AudioI2S::end() {
    if (_micInitialized) {
        i2s_driver_uninstall(I2S_MIC_PORT);
        _micInitialized = false;
    }
    if (_spkInitialized) {
        i2s_driver_uninstall(I2S_SPK_PORT);
        _spkInitialized = false;
    }
}

bool AudioI2S::_initMicrophone() {
    if (_micInitialized) return true;

    // INMP441 I2S Configuration (Master RX, 16kHz, 32-bit slots, Stereo to capture L/R GND or VDD)
    i2s_config_t mic_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // Stereo: auto-detects Left OR Right channel!
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t mic_pins = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = PIN_I2S_MIC_SCK,
        .ws_io_num = PIN_I2S_MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = PIN_I2S_MIC_SD
    };

    esp_err_t err = i2s_driver_install(I2S_MIC_PORT, &mic_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[I2S] Mic driver install failed: 0x%x\n", err);
        return false;
    }

    err = i2s_set_pin(I2S_MIC_PORT, &mic_pins);
    if (err != ESP_OK) {
        Serial.printf("[I2S] Mic pin config failed: 0x%x\n", err);
        i2s_driver_uninstall(I2S_MIC_PORT);
        return false;
    }

    _micInitialized = true;
    return true;
}

bool AudioI2S::_initSpeaker() {
    if (_spkInitialized) return true;

    // MAX98357A I2S Configuration (Master TX, 16kHz, 16-bit)
    i2s_config_t spk_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 6,
        .dma_buf_len = 512,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t spk_pins = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = PIN_I2S_SPK_BCLK,
        .ws_io_num = PIN_I2S_SPK_LRC,
        .data_out_num = PIN_I2S_SPK_DIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_SPK_PORT, &spk_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[I2S] Speaker driver install failed: 0x%x\n", err);
        return false;
    }

    err = i2s_set_pin(I2S_SPK_PORT, &spk_pins);
    if (err != ESP_OK) {
        Serial.printf("[I2S] Speaker pin config failed: 0x%x\n", err);
        i2s_driver_uninstall(I2S_SPK_PORT);
        return false;
    }

    _spkInitialized = true;
    return true;
}

int AudioI2S::readMicrophone(int16_t* outBuffer, int maxSamples, float gain) {
    if (!_micInitialized || !outBuffer || maxSamples <= 0) return 0;

    // Read 32-bit stereo slots from INMP441 DMA
    static int32_t rawSamples[512];
    int samplesToRead = maxSamples > 256 ? 256 : maxSamples;
    size_t bytesRead = 0;

    esp_err_t err = i2s_read(I2S_MIC_PORT, rawSamples, samplesToRead * 2 * sizeof(int32_t), &bytesRead, pdMS_TO_TICKS(100));
    if (err != ESP_OK || bytesRead == 0) return 0;

    int stereoPairs = bytesRead / (2 * sizeof(int32_t));

    // DC high-pass filter state (~15 Hz high-pass to remove MEMS DC bias)
    static float xPrev = 0.0f;
    static float yPrev = 0.0f;
    const float alpha = 0.995f;

    for (int i = 0; i < stereoPairs; i++) {
        int32_t left = rawSamples[i * 2] >> 14;
        int32_t right = rawSamples[i * 2 + 1] >> 14;

        // Auto-select active channel (whether L/R pin is connected to GND, 3.3V, or floating)
        int32_t rawSample = (abs(left) >= abs(right)) ? left : right;

        // Apply DC blocker filter
        float x = (float)rawSample;
        float y = x - xPrev + (alpha * yPrev);
        xPrev = x;
        yPrev = y;

        // Apply gain & clamp to 16-bit signed range [-32768, 32767]
        int32_t sample32 = (int32_t)(y * gain);
        if (sample32 > 32767) sample32 = 32767;
        else if (sample32 < -32768) sample32 = -32768;

        outBuffer[i] = (int16_t)sample32;
    }

    return stereoPairs;
}

float AudioI2S::getAudioRMS(const int16_t* samples, int count) {
    if (!samples || count <= 0) return 0.0f;
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += ((double)samples[i] * (double)samples[i]);
    }
    return (float)sqrt(sum / count);
}

size_t AudioI2S::writeSpeaker(const uint8_t* pcmData, size_t bytes) {
    if (!_spkInitialized || !pcmData || bytes == 0) return 0;
    size_t bytesWritten = 0;

    if (_volume >= 0.99f) {
        i2s_write(I2S_SPK_PORT, pcmData, bytes, &bytesWritten, pdMS_TO_TICKS(200));
    } else {
        // Apply digital volume scaling to 16-bit samples
        int16_t scaledBuf[256];
        const int16_t* inSamples = (const int16_t*)pcmData;
        size_t samplesTotal = bytes / sizeof(int16_t);
        size_t totalWritten = 0;

        for (size_t offset = 0; offset < samplesTotal; offset += 256) {
            size_t chunk = (samplesTotal - offset > 256) ? 256 : (samplesTotal - offset);
            for (size_t i = 0; i < chunk; i++) {
                scaledBuf[i] = (int16_t)(inSamples[offset + i] * _volume);
            }
            size_t written = 0;
            i2s_write(I2S_SPK_PORT, (const uint8_t*)scaledBuf, chunk * sizeof(int16_t), &written, pdMS_TO_TICKS(150));
            totalWritten += written;
        }
        bytesWritten = totalWritten;
    }

    return bytesWritten;
}

void AudioI2S::writeSpeakerSilence(int ms) {
    if (!_spkInitialized || ms <= 0) return;
    int samples = (AUDIO_SAMPLE_RATE * ms) / 1000;
    static int16_t silence[256] = {0};
    while (samples > 0) {
        int chunk = samples > 256 ? 256 : samples;
        size_t written = 0;
        i2s_write(I2S_SPK_PORT, (const uint8_t*)silence, chunk * sizeof(int16_t), &written, pdMS_TO_TICKS(50));
        samples -= chunk;
    }
}

void AudioI2S::flushSpeaker() {
    if (_spkInitialized) {
        i2s_zero_dma_buffer(I2S_SPK_PORT);
    }
}

void AudioI2S::setVolume(float volume) {
    if (volume < 0.0f) _volume = 0.0f;
    else if (volume > 1.0f) _volume = 1.0f;
    else _volume = volume;
}

void AudioI2S::buildWavHeader(uint8_t* header, uint32_t pcmDataSize, 
                              uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample) {
    uint32_t totalDataLen = pcmDataSize + 36;
    uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
    uint16_t blockAlign = channels * (bitsPerSample / 8);

    // RIFF chunk descriptor
    header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
    header[4] = (uint8_t)(totalDataLen & 0xFF);
    header[5] = (uint8_t)((totalDataLen >> 8) & 0xFF);
    header[6] = (uint8_t)((totalDataLen >> 16) & 0xFF);
    header[7] = (uint8_t)((totalDataLen >> 24) & 0xFF);
    header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';

    // "fmt " sub-chunk
    header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
    header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0; // 16 for PCM
    header[20] = 1; header[21] = 0; // Linear PCM
    header[22] = (uint8_t)channels; header[23] = 0;
    header[24] = (uint8_t)(sampleRate & 0xFF);
    header[25] = (uint8_t)((sampleRate >> 8) & 0xFF);
    header[26] = (uint8_t)((sampleRate >> 16) & 0xFF);
    header[27] = (uint8_t)((sampleRate >> 24) & 0xFF);
    header[28] = (uint8_t)(byteRate & 0xFF);
    header[29] = (uint8_t)((byteRate >> 8) & 0xFF);
    header[30] = (uint8_t)((byteRate >> 16) & 0xFF);
    header[31] = (uint8_t)((byteRate >> 24) & 0xFF);
    header[32] = (uint8_t)blockAlign; header[33] = 0;
    header[34] = (uint8_t)bitsPerSample; header[35] = 0;

    // "data" sub-chunk
    header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
    header[40] = (uint8_t)(pcmDataSize & 0xFF);
    header[41] = (uint8_t)((pcmDataSize >> 8) & 0xFF);
    header[42] = (uint8_t)((pcmDataSize >> 16) & 0xFF);
    header[43] = (uint8_t)((pcmDataSize >> 24) & 0xFF);
}
