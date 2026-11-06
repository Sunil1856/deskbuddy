// ============================================================================
// DeskBuddy — Real-Time Voice AI Assistant Implementation
// ============================================================================
#include "app_voice.h"
#include "icons.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

AppVoice::AppVoice() {
    _state = VoiceState::IDLE;
    _recBuffer = nullptr;
    _maxSamples = 16000 * 3.5; // ~3.5 seconds
    _recordedSamples = 0;
    _isSpeechDetected = false;
    _currentRms = 0.0f;
    _smoothedRms = 0.0f;
    _animFrame = 0;
    _lastAnimTime = 0;
    _stateStartTime = 0;
    memset(_waveHistory, 0, sizeof(_waveHistory));
}

AppVoice::~AppVoice() {
    onExit();
}

void AppVoice::begin(DisplayManager* dm, Storage* storage, FaceRenderer* face, AudioI2S* audio) {
    _dm = dm;
    _storage = storage;
    _face = face;
    _audio = audio;
    memset(_waveHistory, 0, sizeof(_waveHistory));
}

void AppVoice::onEnter() {
    Serial.println("[VOICE] Entering Voice AI Assistant Mode");
    
    // Allocate recording buffer on demand (~112 KB for 3.5s @ 16kHz 16-bit)
    if (!_recBuffer) {
        _recBuffer = (int16_t*)malloc(_maxSamples * sizeof(int16_t));
        if (!_recBuffer) {
            // Fallback to 2.5s if heap is tight
            _maxSamples = 16000 * 2.5;
            _recBuffer = (int16_t*)malloc(_maxSamples * sizeof(int16_t));
        }
    }

    if (_face) {
        _face->setExpression(Expression::HAPPY, 200);
    }

    startListening();
}

void AppVoice::onExit() {
    Serial.println("[VOICE] Exiting Voice AI Assistant Mode");
    _state = VoiceState::IDLE;
    
    if (_audio) {
        _audio->flushSpeaker();
    }

    if (_recBuffer) {
        free(_recBuffer);
        _recBuffer = nullptr;
    }
}

void AppVoice::startListening() {
    _state = VoiceState::LISTENING;
    _recordedSamples = 0;
    _isSpeechDetected = false;
    _speechStartTime = 0;
    _lastSpeechTime = 0;
    _listeningStartTime = millis();
    _stateStartTime = millis();
    _currentRms = 0.0f;
    _smoothedRms = 0.0f;
    memset(_waveHistory, 0, sizeof(_waveHistory));

    if (_face) {
        _face->setExpression(Expression::SKEPTICAL, 200);
    }
    Serial.println("[VOICE] State: LISTENING for speech...");
}

void AppVoice::cancel() {
    _state = VoiceState::IDLE;
    if (_audio) {
        _audio->flushSpeaker();
    }
}

void AppVoice::handleTouch(TouchEvent event) {
    if (event == TouchEvent::TAP) {
        if (_state == VoiceState::LISTENING) {
            if (_isSpeechDetected && _recordedSamples > 4000) {
                // Manual tap to stop recording and send immediately
                stopListeningAndSend();
            } else {
                // Tap to force start speech recording
                _isSpeechDetected = true;
                _lastSpeechTime = millis();
            }
        } else if (_state == VoiceState::SPEAKING) {
            // Tap to skip audio and start listening again
            if (_audio) _audio->flushSpeaker();
            startListening();
        }
    }
}

void AppVoice::update() {
    if (millis() - _lastAnimTime > 70) {
        _animFrame = (_animFrame + 1) % 60;
        _lastAnimTime = millis();
    }

    switch (_state) {
        case VoiceState::LISTENING:
            _processListening();
            break;
        case VoiceState::THINKING:
            // Handled synchronously or in background
            break;
        case VoiceState::SPEAKING:
            // Audio streaming is driven in _playAudioStream
            break;
        default:
            break;
    }
}

void AppVoice::_processListening() {
    if (!_audio || !_recBuffer) return;

    static int16_t chunk[256];
    int samplesRead = _audio->readMicrophone(chunk, 256, 2.5f);
    if (samplesRead <= 0) return;

    // Calculate audio RMS energy for Voice Activity Detection (VAD)
    float rms = _audio->getAudioRMS(chunk, samplesRead);
    _currentRms = rms;
    _smoothedRms = (_smoothedRms * 0.7f) + (rms * 0.3f);

    // Update Waveform Visualizer
    uint8_t waveHeight = (uint8_t)constrain((int)(_smoothedRms / 30.0f), 1, 24);
    for (int i = 0; i < 15; i++) {
        _waveHistory[i] = _waveHistory[i + 1];
    }
    _waveHistory[15] = waveHeight;

    // VAD Speech Threshold (typical background noise is ~80-180, speech is > 350)
    const float SPEECH_THRESHOLD = 320.0f;
    unsigned long now = millis();

    if (rms > SPEECH_THRESHOLD) {
        if (!_isSpeechDetected) {
            _isSpeechDetected = true;
            _speechStartTime = now;
            Serial.println("[VOICE] Speech Detected! Recording...");
        }
        _lastSpeechTime = now;
    }

    // If speech has started, accumulate audio samples into buffer
    if (_isSpeechDetected) {
        if (_recordedSamples + samplesRead < _maxSamples) {
            memcpy(_recBuffer + _recordedSamples, chunk, samplesRead * sizeof(int16_t));
            _recordedSamples += samplesRead;
        } else {
            // Buffer full: automatically finalize and send
            Serial.println("[VOICE] Buffer full, finalizing...");
            stopListeningAndSend();
            return;
        }

        // Voice Activity Detection: Stop if silence detected for > 700ms after speech
        if (now - _lastSpeechTime > 700 && _recordedSamples >= 8000) { // min 0.5s speech
            Serial.printf("[VOICE] Silence detected! Total samples: %d\n", _recordedSamples);
            stopListeningAndSend();
            return;
        }
    } else {
        // Auto-exit if no speech detected for 14 seconds in continuous mode
        if (now - _listeningStartTime > 14000) {
            Serial.println("[VOICE] Inactivity timeout, returning to Idle");
            _state = VoiceState::IDLE;
        }
    }
}

void AppVoice::stopListeningAndSend() {
    if (_recordedSamples < 4000) { // Less than 0.25s of audio
        startListening();
        return;
    }

    _state = VoiceState::THINKING;
    _stateStartTime = millis();

    if (_face) {
        _face->setExpression(Expression::SKEPTICAL, 300);
    }

    _sendAudioToServer();
}

String AppVoice::_getServerUrl() {
    String url = VOICE_SERVER_URL;
    if (_storage) {
        String custom = _storage->loadString("voice_server_url", "");
        if (custom.length() > 0 && custom.startsWith("http")) {
            url = custom;
        } else {
            // Also check server_url from Pi/Laptop monitor if set
            String srv = _storage->loadString("server_url", "");
            if (srv.length() > 0 && srv.startsWith("http")) {
                int portIdx = srv.lastIndexOf(":");
                if (portIdx > 6) {
                    url = srv.substring(0, portIdx) + ":8000";
                }
            }
        }
    }
    return url;
}

void AppVoice::_sendAudioToServer() {
    if (WiFi.status() != WL_CONNECTED) {
        _lastReply = "WiFi Disconnected!";
        _lastEmotion = "ANNOYED";
        delay(1500);
        startListening();
        return;
    }

    String serverBase = _getServerUrl();
    String uploadUrl = serverBase + "/api/voice/chat";

    Serial.printf("[VOICE] Uploading %d bytes of WAV audio to: %s\n", 
                  (int)(_recordedSamples * 2 + 44), uploadUrl.c_str());

    HTTPClient http;
    http.setTimeout(12000); // 12s timeout for speech transcription + LLM + TTS
    http.begin(uploadUrl);

    // Build standard 44-byte WAV header in front of recording buffer
    uint32_t pcmBytes = _recordedSamples * sizeof(int16_t);
    uint8_t wavHeader[44];
    AudioI2S::buildWavHeader(wavHeader, pcmBytes, AUDIO_SAMPLE_RATE, 1, AUDIO_BITS_PER_SAMPLE);

    // Prepare multipart boundaries
    String boundary = "----DeskBuddyAudioBoundary7MA4YWxk";
    String head = "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"audio\"; filename=\"deskbuddy_voice.wav\"\r\n" +
                  "Content-Type: audio/wav\r\n\r\n";
    String tail = "\r\n--" + boundary + "--\r\n";

    size_t totalPayloadSize = head.length() + sizeof(wavHeader) + pcmBytes + tail.length();

    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    http.addHeader("Content-Length", String(totalPayloadSize));

    // Standard raw payload upload: Combine header + pcmBytes
    size_t fullWavSize = sizeof(wavHeader) + pcmBytes;
    uint8_t* fullPayload = (uint8_t*)malloc(head.length() + fullWavSize + tail.length());
    if (fullPayload) {
        memcpy(fullPayload, head.c_str(), head.length());
        memcpy(fullPayload + head.length(), wavHeader, sizeof(wavHeader));
        memcpy(fullPayload + head.length() + sizeof(wavHeader), _recBuffer, pcmBytes);
        memcpy(fullPayload + head.length() + fullWavSize, tail.c_str(), tail.length());

        int code = http.POST(fullPayload, head.length() + fullWavSize + tail.length());
        free(fullPayload);

        Serial.printf("[VOICE] Server HTTP Code: %d\n", code);

        if (code == HTTP_CODE_OK) {
            String response = http.getString();
            Serial.printf("[VOICE] Server Response: %s\n", response.c_str());

            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, response);
            if (!err && doc["status"] == "ok") {
                _lastReply = doc["reply"].is<String>() ? doc["reply"].as<String>() : (doc["reply_text"] | "Hello!");
                _lastEmotion = doc["emotion"] | "HAPPY";
                String audioPath = doc["audio_url"] | "";

                if (_face) {
                    if (_lastEmotion == "HAPPY") _face->setExpression(Expression::HAPPY, 300);
                    else if (_lastEmotion == "EXCITED") _face->setExpression(Expression::EXCITED, 300);
                    else if (_lastEmotion == "LOVE") _face->setExpression(Expression::LOVE, 300);
                    else if (_lastEmotion == "ANNOYED") _face->setExpression(Expression::ANGRY, 300);
                    else _face->setExpression(Expression::HAPPY, 300);
                }

                http.end();

                // If audio URL is returned, stream and play on MAX98357A speaker!
                if (audioPath.length() > 0) {
                    String fullAudioUrl = audioPath.startsWith("http") ? audioPath : (serverBase + audioPath);
                    _playAudioStream(fullAudioUrl);
                } else {
                    // No audio, show text for 3 seconds
                    delay(3000);
                    startListening();
                }
                return;
            } else {
                _lastReply = doc["message"] | "AI Error occurred";
            }
        } else {
            _lastReply = "Server Unreachable!";
            _lastEmotion = "ANNOYED";
        }
    } else {
        _lastReply = "Out of memory!";
    }

    http.end();
    delay(2000);
    startListening();
}

void AppVoice::_playAudioStream(const String& fullAudioUrl) {
    if (!_audio) return;
    _state = VoiceState::SPEAKING;
    _stateStartTime = millis();

    Serial.printf("[VOICE] Streaming audio from: %s\n", fullAudioUrl.c_str());

    HTTPClient http;
    http.setTimeout(10000);
    http.begin(fullAudioUrl);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        int totalLen = http.getSize();
        WiFiClient* stream = http.getStreamPtr();

        // Skip 44-byte WAV header
        uint8_t header[44];
        if (stream->available() >= 44) {
            stream->readBytes(header, 44);
        }

        uint8_t buffer[512];
        while (http.connected() && (totalLen > 0 || totalLen == -1)) {
            size_t availableBytes = stream->available();
            if (availableBytes > 0) {
                int bytesToRead = availableBytes > sizeof(buffer) ? sizeof(buffer) : availableBytes;
                int bytesRead = stream->readBytes(buffer, bytesToRead);
                if (bytesRead > 0) {
                    _audio->writeSpeaker(buffer, bytesRead);
                    if (totalLen > 0) totalLen -= bytesRead;
                }
            } else {
                delay(2);
            }

            // Quick render while playing so OLED animates in sync with speech!
            render();
            if (_dm) _dm->display().display();
        }

        _audio->writeSpeakerSilence(50);
        Serial.println("[VOICE] Audio playback finished!");
    } else {
        Serial.printf("[VOICE] Audio download failed, code: %d\n", httpCode);
    }
    http.end();

    // REAL-TIME CONTINUOUS MODE: Automatically listen for follow-up question!
    delay(400);
    startListening();
}

void AppVoice::render() {
    if (!_dm) return;

    switch (_state) {
        case VoiceState::LISTENING:
            _renderListening();
            break;
        case VoiceState::THINKING:
            _renderThinking();
            break;
        case VoiceState::SPEAKING:
            _renderSpeaking();
            break;
        case VoiceState::IDLE:
        default:
            _renderIdle();
            break;
    }
}

void AppVoice::_renderListening() {
    Adafruit_SSD1306& d = _dm->display();

    // Header Bar
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE);
    _dm->drawCenteredText("🎙️ LISTENING... [LIVE]", 2, 1);
    d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);

    // Live Animated Equalizer Waveform in Center
    int centerX = SCREEN_W / 2;
    int baseY = 36;
    int numBars = 16;
    int barW = 4;
    int spacing = 3;
    int startX = centerX - ((numBars * (barW + spacing)) / 2);

    for (int i = 0; i < numBars; i++) {
        int x = startX + i * (barW + spacing);
        int h = _waveHistory[i];
        if (h < 2) h = 2;
        if (h > 20) h = 20;

        d.fillRect(x, baseY - (h / 2), barW, h, SSD1306_WHITE);
    }

    // Status Subtitle
    if (_isSpeechDetected) {
        d.setTextSize(1);
        _dm->drawCenteredText("Speaking... (Tap to Send)", 52, 1);
    } else {
        d.setTextSize(1);
        _dm->drawCenteredText("Say anything to DeskBuddy", 52, 1);
    }
}

void AppVoice::_renderThinking() {
    Adafruit_SSD1306& d = _dm->display();

    // Header Bar
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE);
    _dm->drawCenteredText("🧠 THINKING...", 2, 1);
    d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);

    // Animated Pulsing Brain / Orbiting Dots
    int cx = 64;
    int cy = 32;
    int r = 10;
    float angle = (_animFrame * 12.0f) * 3.14159f / 180.0f;
    int dotX = cx + (int)(cos(angle) * r);
    int dotY = cy + (int)(sin(angle) * r);

    d.drawCircle(cx, cy, 6, SSD1306_WHITE);
    d.fillCircle(dotX, dotY, 3, SSD1306_WHITE);

    // Processing status text
    _dm->drawCenteredText("Connecting to Phone AI...", 48, 1);
}

void AppVoice::_renderSpeaking() {
    Adafruit_SSD1306& d = _dm->display();

    // Header Bar with Emotion
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE);
    String header = "🔊 BUDDY [" + _lastEmotion.substring(0, 5) + "]";
    d.setCursor(2, 2);
    d.print(header);

    // Animated mouth wave on top right
    int mouthH = 2 + (_animFrame % 6);
    d.drawRoundRect(106, 2, 18, mouthH + 4, 2, SSD1306_WHITE);
    d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);

    // Display AI response text (up to 4 lines with clean margin)
    int y = 16;
    d.setTextSize(1);
    int len = _lastReply.length();
    int startIdx = 0;

    for (int line = 0; line < 4 && startIdx < len; line++) {
        String lineStr = _lastReply.substring(startIdx, min(startIdx + 21, len));
        d.setCursor(2, y);
        d.print(lineStr);
        y += 10;
        startIdx += 21;
    }

    // Footer prompt
    d.drawLine(0, 55, SCREEN_W, 55, SSD1306_WHITE);
    d.setTextSize(1);
    _dm->drawCenteredText("Tap: Skip | 2x: Exit", 56, 1);
}

void AppVoice::_renderIdle() {
    Adafruit_SSD1306& d = _dm->display();
    _dm->drawCenteredText("VOICE AI READY", 2, 1);
    d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);
    _dm->drawCenteredText("Tap to Talk", 30, 2);
    _dm->drawCenteredText("Double-tap to Exit", 52, 1);
}
