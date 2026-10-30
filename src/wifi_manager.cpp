// ============================================================================
// DeskBuddy — WiFi Manager Implementation
// ============================================================================
#include "wifi_manager.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "icons.h"
#include "config.h"
#include "photo_studio_html.h"
#include "app_gallery.h"

static WebServer server(80);
static DNSServer dnsServer;
static const byte DNS_PORT = 53;

// Global pointers for web server callbacks
static Storage* g_storage = nullptr;
static bool g_shouldRestart = false;
static AppGallery* g_gallery = nullptr;
static PhotoUploadedCallback g_photoCb = nullptr;

static uint8_t hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

// --- Captive Portal HTML (PROGMEM) ---
static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<title>DeskBuddy Setup</title>
<style>
body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; background: #121212; color: #fff; display: flex; flex-direction: column; align-items: center; padding: 15px; margin: 0; }
h2 { margin: 15px 0 5px 0; color: #00E676; text-align: center; }
p.sub { font-size: 13px; color: #aaa; text-align: center; margin-bottom: 20px; }
form { background: #1e1e1e; padding: 20px; border-radius: 12px; width: 100%; max-width: 320px; box-shadow: 0 4px 12px rgba(0,0,0,0.5); box-sizing: border-box; }
label { font-size: 13px; font-weight: 600; color: #bbb; display: block; margin-top: 10px; }
input { width: 100%; padding: 10px; margin: 6px 0 10px 0; border: 1px solid #333; border-radius: 6px; background: #2a2a2a; color: #fff; box-sizing: border-box; font-size: 14px; }
input:focus { border-color: #00E676; outline: none; }
.hint { font-size: 11px; color: #777; margin-top: -6px; margin-bottom: 8px; }
input[type=submit] { background: #00E676; color: #000; font-size: 15px; font-weight: bold; border: none; padding: 12px; margin-top: 15px; border-radius: 6px; cursor: pointer; }
input[type=submit]:hover { background: #00C853; }
.note { font-size: 12px; color: #888; text-align: center; margin-top: 15px; }
</style></head><body>
<h2>🤖 DeskBuddy Setup</h2>
<p class="sub">Configure your companion's network &amp; feeds</p>
<form action="/save" method="post">
  <label>WiFi SSID *</label>
  <input type="text" name="ssid" placeholder="Home WiFi Name" required>
  <label>WiFi Password</label>
  <input type="password" name="pass" placeholder="Password (if any)">
  <label>Your City</label>
  <input type="text" name="city" placeholder="e.g. Delhi, Mumbai, London">
  <div class="hint">For local weather &amp; 3-day forecast</div>
  <label>Timezone Offset (hours)</label>
  <input type="number" step="0.5" name="tz" placeholder="5.5 for IST, -5 for EST" value="5.5">
  <label>OpenWeather API Key (Optional)</label>
  <input type="text" name="owm" placeholder="Leave empty for free Open-Meteo">
  <div class="hint">Optional: Free Open-Meteo is used by default</div>
  <label>CoinGecko API Key (Optional)</label>
  <input type="text" name="cg" placeholder="Leave empty for free public rates">
  <div class="hint">Optional: Free Binance/CoinGecko fallback is automatic</div>
  <input type="submit" value="Save &amp; Restart">
</form>
<div class="note">DeskBuddy will restart and connect automatically.</div>
</body></html>
)rawliteral";

void WifiManager::begin(DisplayManager* dm, Storage* storage) {
    _dm = dm;
    _storage = storage;
    g_storage = storage;
    _state = WifiState::IDLE;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, false);
}

void WifiManager::autoConnect() {
    String ssid = _storage ? _storage->getWifiSSID() : "";
    String pass = _storage ? _storage->getWifiPass() : "";
    
    if (ssid.length() == 0) {
        startPortal();
        return;
    }
    
    _state = WifiState::CONNECTING;
    _connectStartTime = millis();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.length() > 0 ? pass.c_str() : NULL);
    
    _drawConnectingScreen();
}

void WifiManager::startPortal() {
    _state = WifiState::PORTAL_ACTIVE;
    _startAP();
}

void WifiManager::_startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL);
    
    // Setup DNS to redirect all requests to ESP IP
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    
    _setupWebRoutes();
    server.begin();
    
    _drawPortalScreen();
}

void WifiManager::_setupWebRoutes() {
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", PORTAL_HTML);
    });
    
    server.on("/save", HTTP_POST, []() {
        String ssid = server.arg("ssid");
        String pass = server.arg("pass");
        String city = server.arg("city");
        String tz = server.arg("tz");
        String owm = server.arg("owm");
        String cg = server.arg("cg");
        
        if (ssid.length() > 0 && g_storage) {
            g_storage->setWifi(ssid, pass);
            if (tz.length() > 0) {
                float tzHours = tz.toFloat();
                g_storage->setTimezoneOffset((int32_t)(tzHours * 3600));
            }
            
            float lat = 28.6139f, lon = 77.2090f; // Default: New Delhi
            if (city.length() > 0) {
                Storage::geocodeCity(city, lat, lon);
                g_storage->setLocation(lat, lon, city);
            } else {
                g_storage->setLocation(lat, lon, "New Delhi");
            }
            
            if (owm.length() > 0) {
                g_storage->setOpenWeatherKey(owm);
            }
            if (cg.length() > 0) {
                g_storage->setCoinGeckoKey(cg);
            }
            
            g_storage->setConfigured(true);
            
            server.send(200, "text/html", "<html><body style='background:#121212;color:#fff;text-align:center;font-family:sans-serif;margin-top:50px;'><h2 style='color:#00E676;'>Settings Saved!</h2><p>DeskBuddy is restarting now...</p></body></html>");
            g_shouldRestart = true;
        } else {
            server.send(400, "text/plain", "Missing SSID");
        }
    });
    
    // Captive portal redirects
    server.onNotFound([]() {
        server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
        server.send(302, "text/plain", "");
    });
}

void WifiManager::_setupStationWebRoutes() {
    g_gallery = _gallery;
    g_photoCb = _onPhotoUploaded;
    
    // Serve Photo Studio on root and /photo
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", PHOTO_STUDIO_HTML);
    });
    server.on("/photo", HTTP_GET, []() {
        server.send_P(200, "text/html", PHOTO_STUDIO_HTML);
    });
    
    // Accept photo upload from phone/laptop browser
    server.on("/api/photo", HTTP_POST, []() {
        if (!server.hasArg("data")) {
            server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing data\"}");
            return;
        }
        String hex = server.arg("data");
        if (hex.length() < 2048) {
            server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid size\"}");
            return;
        }
        
        uint8_t buffer[1024];
        for (int i = 0; i < 1024; i++) {
            buffer[i] = (hexNibble(hex[i * 2]) << 4) | hexNibble(hex[i * 2 + 1]);
        }
        
        bool save = true;
        if (server.hasArg("save")) {
            save = (server.arg("save") == "1");
        }
        
        if (g_gallery) {
            g_gallery->setPhoto(buffer, save);
        }
        if (save && g_photoCb) {
            g_photoCb();
        }
        
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/api/photo/delete", HTTP_POST, []() {
        if (g_gallery) {
            g_gallery->clearPhoto();
        }
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    // Video upload & management endpoints
    server.on("/api/video/start", HTTP_POST, []() {
        if (!g_gallery) {
            server.send(500, "application/json", "{\"status\":\"error\",\"msg\":\"Gallery not ready\"}");
            return;
        }
        uint8_t totalFrames = server.hasArg("frames") ? (uint8_t)server.arg("frames").toInt() : 30;
        uint8_t fps = server.hasArg("fps") ? (uint8_t)server.arg("fps").toInt() : 10;
        if (totalFrames == 0 || totalFrames > 100) totalFrames = 30;
        if (fps == 0 || fps > 30) fps = 10;
        
        bool ok = g_gallery->setVideoStart(totalFrames, fps);
        if (ok) {
            server.send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            server.send(500, "application/json", "{\"status\":\"error\",\"msg\":\"Out of memory\"}");
        }
    });
    
    server.on("/api/video/frame", HTTP_POST, []() {
        if (!g_gallery || !server.hasArg("idx") || !server.hasArg("data")) {
            server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing parameters\"}");
            return;
        }
        uint8_t idx = (uint8_t)server.arg("idx").toInt();
        String hex = server.arg("data");
        if (hex.length() < 2048) {
            server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Frame size invalid\"}");
            return;
        }
        uint8_t buffer[1024];
        for (int i = 0; i < 1024; i++) {
            buffer[i] = (hexNibble(hex[i * 2]) << 4) | hexNibble(hex[i * 2 + 1]);
        }
        bool ok = g_gallery->setVideoFrame(idx, buffer);
        if (ok) {
            server.send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Failed to store frame\"}");
        }
    });
    
    server.on("/api/video/end", HTTP_POST, []() {
        if (!g_gallery) {
            server.send(500, "application/json", "{\"status\":\"error\"}");
            return;
        }
        bool save = true;
        if (server.hasArg("save")) {
            save = (server.arg("save") == "1");
        }
        g_gallery->setVideoEnd(save);
        if (g_photoCb) {
            g_photoCb(); // Instantly switch display to Gallery to watch the new video!
        }
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    server.on("/api/video/delete", HTTP_POST, []() {
        if (g_gallery) {
            g_gallery->clearVideo();
        }
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    server.on("/api/gallery/info", HTTP_GET, []() {
        if (!g_gallery) {
            server.send(500, "application/json", "{\"status\":\"error\"}");
            return;
        }
        String json = "{\"status\":\"ok\",\"hasPhoto\":" + String(g_gallery->hasPhoto() ? "true" : "false") +
                      ",\"hasVideo\":" + String(g_gallery->hasVideo() ? "true" : "false") +
                      ",\"frames\":" + String(g_gallery->getVideoFramesCount()) +
                      ",\"fps\":" + String(g_gallery->getVideoFps()) + "}";
        server.send(200, "application/json", json);
    });
    
    server.on("/api/status", HTTP_GET, []() {
        server.send(200, "application/json", "{\"status\":\"online\",\"device\":\"DeskBuddy\"}");
    });
    
    server.on("/api/location", HTTP_GET, []() {
        if (!g_storage) {
            server.send(500, "application/json", "{\"status\":\"error\"}");
            return;
        }
        String json = "{\"status\":\"ok\",\"city\":\"" + g_storage->getCityName() + 
                      "\",\"lat\":" + String(g_storage->getLatitude(), 4) + 
                      ",\"lon\":" + String(g_storage->getLongitude(), 4) + "}";
        server.send(200, "application/json", json);
    });

    server.on("/api/location", HTTP_POST, []() {
        if (!g_storage || !server.hasArg("city")) {
            server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing city\"}");
            return;
        }
        String city = server.arg("city");
        city.trim();
        if (city.length() == 0) {
            server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Empty city\"}");
            return;
        }
        float lat = 0, lon = 0;
        Storage::geocodeCity(city, lat, lon);
        g_storage->setLocation(lat, lon, city);
        server.send(200, "application/json", "{\"status\":\"ok\",\"city\":\"" + city + "\"}");
    });

    server.on("/api/server", HTTP_GET, []() {
        if (!g_storage) {
            server.send(500, "application/json", "{\"status\":\"error\"}");
            return;
        }
        String url = g_storage->loadString("server_url", SERVER_MONITOR_URL);
        String name = g_storage->loadString("server_name", SERVER_DISPLAY_NAME);
        String json = "{\"status\":\"ok\",\"url\":\"" + url + "\",\"name\":\"" + name + "\"}";
        server.send(200, "application/json", json);
    });

    server.on("/api/server", HTTP_POST, []() {
        if (!g_storage) {
            server.send(500, "application/json", "{\"status\":\"error\"}");
            return;
        }
        if (server.hasArg("url")) {
            String url = server.arg("url");
            url.trim();
            g_storage->saveString("server_url", url);
        }
        if (server.hasArg("name")) {
            String name = server.arg("name");
            name.trim();
            g_storage->saveString("server_name", name);
        }
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });
}

void WifiManager::update() {
    if (g_shouldRestart) {
        delay(1000);
        ESP.restart();
    }
    
    if (_state == WifiState::PORTAL_ACTIVE) {
        dnsServer.processNextRequest();
        server.handleClient();
        
        static unsigned long lastDraw = 0;
        if (millis() - lastDraw > 1000) {
            _drawPortalScreen();
            lastDraw = millis();
        }
    } 
    else if (_state == WifiState::CONNECTED) {
        // Handle web requests from phone/laptop
        server.handleClient();
    }
    else if (_state == WifiState::CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _state = WifiState::CONNECTED;
            _hasConnectedOnce = true;
            Serial.printf("WiFi Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            if (_gallery) {
                _gallery->setIP(WiFi.localIP().toString());
            }
            _setupStationWebRoutes();
            server.begin();
        } else if (millis() - _connectStartTime > 15000) {
            static int retryCount = 0;
            retryCount++;
            Serial.printf("WiFi timeout attempt %d/3\n", retryCount);
            
            if (retryCount >= 3) {
                // If it never connected once on boot, start portal so user isn't locked out
                if (!_hasConnectedOnce) {
                    retryCount = 0;
                    startPortal();
                    return;
                }
                // If it already connected once before, stay in station mode and silently retry
                retryCount = 0;
            }
            
            _connectStartTime = millis();
            String ssid = _storage ? _storage->getWifiSSID() : "";
            String pass = _storage ? _storage->getWifiPass() : "";
            if (ssid.length() > 0) {
                WiFi.disconnect(false, false);
                WiFi.begin(ssid.c_str(), pass.length() > 0 ? pass.c_str() : NULL);
            } else if (!_hasConnectedOnce) {
                startPortal();
            }
        } else {
            if (!_hasConnectedOnce) {
                _drawConnectingScreen();
            }
        }
    }
}

void WifiManager::disconnect() {
    WiFi.disconnect(false, false);
    _state = WifiState::IDLE;
}

String WifiManager::getIP() const {
    if (_state == WifiState::CONNECTED) {
        return WiFi.localIP().toString();
    } else if (_state == WifiState::PORTAL_ACTIVE) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

void WifiManager::_drawConnectingScreen() {
    if (!_dm) return;
    _dm->beginFrame();
    Adafruit_SSD1306& d = _dm->display();
    
    d.drawBitmap(SCREEN_W/2 - 12, 8, icon_globe_24, 24, 24, SSD1306_WHITE);
    _dm->drawCenteredText("Connecting WiFi", 38, 1);
    
    // Animated progress dots
    int dots = (millis() / 300) % 4;
    String txt = "";
    for (int i = 0; i < dots; i++) txt += ".";
    d.setCursor(SCREEN_W/2 + 42, 38);
    d.print(txt);
    
    _dm->show();
}

void WifiManager::_drawPortalScreen() {
    if (!_dm) return;
    _dm->beginFrame();
    Adafruit_SSD1306& d = _dm->display();
    
    _dm->drawCenteredText("SETUP DESKBUDDY", 2, 1);
    d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);
    
    _dm->drawCenteredText("Connect to WiFi AP:", 16, 1);
    
    d.drawRoundRect(8, 26, SCREEN_W - 16, 14, 3, SSD1306_WHITE);
    _dm->drawCenteredText(AP_SSID, 29, 1);
    
    char buf[32];
    snprintf(buf, sizeof(buf), "Pass: %s", AP_PASS);
    _dm->drawCenteredText(buf, 44, 1);
    
    _dm->drawCenteredText("Open: 192.168.4.1", 54, 1);
    
    _dm->show();
}
