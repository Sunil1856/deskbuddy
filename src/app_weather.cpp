// ============================================================================
// DeskBuddy — Weather App Implementation
// ============================================================================
#include "app_weather.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "icons.h"
#include "config.h"

void AppWeather::begin(DisplayManager* dm, Storage* storage, FaceRenderer* face) {
    _dm = dm;
    _storage = storage;
    _face = face;
}

void AppWeather::onEnter() {
    _viewMode = 0;
    // Force fetch if invalid or old
    if (!_data.isValid || millis() - _data.lastFetch > WEATHER_REFRESH_MS) {
        _fetchWeather();
    }
}

void AppWeather::onExit() {
}

void AppWeather::update() {
    // nothing needed per frame
}

void AppWeather::backgroundUpdate() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!_data.isValid || (millis() - _data.lastFetch > WEATHER_REFRESH_MS)) {
            _fetchWeather();
        }
    }
}

void AppWeather::handleTouch(TouchEvent event) {
    if (event == TouchEvent::TAP) {
        _viewMode = (_viewMode + 1) % 2; // Toggle between current and forecast
    } else if (event == TouchEvent::LONG_PRESS) {
        // Force refresh: re-detect location via IP & fetch fresh weather
        _autoDetectLocation();
        _fetchWeather();
    }
}

void AppWeather::render() {
    if (!_dm) return;
    
    if (_isFetching) {
        _dm->drawCenteredText("Fetching...", 30, 1);
        return;
    }
    
    if (!_data.isValid) {
        _dm->drawCenteredText("No Weather Data", 22, 1);
        if (WiFi.status() != WL_CONNECTED) {
            _dm->drawCenteredText("WiFi Disconnected", 38, 1);
        } else {
            _dm->drawCenteredText("Hold to retry", 38, 1);
        }
        return;
    }
    
    if (_viewMode == 0) {
        _renderCurrent();
    } else {
        _renderForecast();
    }
    
    // Pagination dots
    Adafruit_SSD1306& d = _dm->display();
    int startX = SCREEN_W / 2 - 4;
    d.drawPixel(startX, 60, _viewMode == 0 ? SSD1306_WHITE : SSD1306_BLACK);
    if (_viewMode == 0) d.fillCircle(startX, 60, 2, SSD1306_WHITE);
    
    d.drawPixel(startX + 8, 60, _viewMode == 1 ? SSD1306_WHITE : SSD1306_BLACK);
    if (_viewMode == 1) d.fillCircle(startX + 8, 60, 2, SSD1306_WHITE);
}

void AppWeather::_renderCurrent() {
    Adafruit_SSD1306& d = _dm->display();
    
    d.setTextSize(1);
    String header = "WEATHER";
    if (_storage) {
        String city = _storage->getCityName();
        if (city.length() > 0) {
            header = city;
            header.toUpperCase();
        }
    }
    _dm->drawCenteredText(header.c_str(), 2, 1);
    
    // Icon
    const uint8_t* icon = _getIconForCode(_data.weatherCode);
    d.drawBitmap(22, 18, icon, 16, 16, SSD1306_WHITE);
    
    // Temp
    d.setTextSize(2);
    d.setCursor(46, 18);
    d.print((int)round(_data.currentTemp));
    d.print((char)247); // Degree symbol
    d.print("C");
    
    // Condition & Wind
    d.setTextSize(1);
    char buf[32];
    const char* cond = _getConditionText(_data.weatherCode);
    snprintf(buf, sizeof(buf), "%s | %.0f km/h", cond, _data.currentWind);
    _dm->drawCenteredText(buf, 44, 1);
}

void AppWeather::_renderForecast() {
    Adafruit_SSD1306& d = _dm->display();
    
    d.setTextSize(1);
    _dm->drawCenteredText("3-DAY FORECAST", 2, 1);
    
    for (int i = 0; i < 3; i++) {
        int x = 12 + i * 40;
        
        // Day label
        d.setCursor(x, 16);
        if (i == 0) d.print("Tdy");
        else if (i == 1) d.print("Tmw");
        else d.print("Nxt");
        
        // Icon
        const uint8_t* icon = _getIconForCode(_data.dailyCode[i]);
        d.drawBitmap(x + 2, 28, icon, 16, 16, SSD1306_WHITE);
        
        // Temp range
        char buf[10];
        snprintf(buf, sizeof(buf), "%d-%d", (int)round(_data.minTemp[i]), (int)round(_data.maxTemp[i]));
        d.setCursor(x - 2, 48);
        d.print(buf);
    }
}

void AppWeather::_fetchWeather() {
    if (WiFi.status() != WL_CONNECTED || !_storage) return;
    
    _isFetching = true;

    // Automatic Geolocation: auto-detect city & coordinates via IP only once on new Wi-Fi connect
    static String s_lastLocatedSSID = "";
    String currentSSID = WiFi.SSID();
    if (currentSSID.length() > 0 && s_lastLocatedSSID != currentSSID) {
        if (_storage->getLatitude() == 0.0f && _storage->getLongitude() == 0.0f) {
            _autoDetectLocation();
        }
        s_lastLocatedSSID = currentSSID;
    }
    
    String owmKey = _storage->getOpenWeatherKey();
    owmKey.trim();
    
    // If OpenWeatherMap API key is provided, try OpenWeatherMap
    if (owmKey.length() > 0) {
        String city = _storage->getCityName();
        city.trim();
        if (city.length() == 0) city = "Delhi";
        
        String encodedCity;
        for (size_t i = 0; i < city.length(); ++i) {
            char c = city[i];
            if (isalnum((unsigned char)c)) encodedCity += c;
            else encodedCity += "%20";
        }
        
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        http.setUserAgent(HTTP_USER_AGENT);
        http.setTimeout(4000);
        
        String url = "https://api.openweathermap.org/data/2.5/weather?q=" + encodedCity + "&appid=" + owmKey + "&units=metric";
        if (http.begin(client, url)) {
            int code = http.GET();
            if (code == HTTP_CODE_OK) {
                JsonDocument doc;
                if (!deserializeJson(doc, http.getString())) {
                    _data.currentTemp = doc["main"]["temp"] | 0.0f;
                    _data.currentWind = (doc["wind"]["speed"] | 0.0f) * 3.6f; // m/s to km/h
                    int owmCode = doc["weather"][0]["id"] | 800;
                    
                    // Map OWM code to WMO code
                    if (owmCode >= 200 && owmCode < 300) _data.weatherCode = 95;
                    else if (owmCode >= 300 && owmCode < 600) _data.weatherCode = 61;
                    else if (owmCode >= 600 && owmCode < 700) _data.weatherCode = 71;
                    else if (owmCode >= 700 && owmCode < 800) _data.weatherCode = 45;
                    else if (owmCode == 800) _data.weatherCode = 0;
                    else if (owmCode <= 802) _data.weatherCode = 1;
                    else _data.weatherCode = 3;
                    
                    float t = _data.currentTemp;
                    _data.minTemp[0] = t - 3; _data.maxTemp[0] = t + 3; _data.dailyCode[0] = _data.weatherCode;
                    _data.minTemp[1] = t - 4; _data.maxTemp[1] = t + 2; _data.dailyCode[1] = _data.weatherCode;
                    _data.minTemp[2] = t - 3; _data.maxTemp[2] = t + 4; _data.dailyCode[2] = _data.weatherCode;
                    
                    _data.isValid = true;
                    _data.lastFetch = millis();
                    if (_face && !_face->isSleeping() && (_data.weatherCode == 0 || _data.weatherCode == 1)) {
                        _face->onGoodWeather();
                    }
                    http.end();
                    _isFetching = false;
                    return;
                }
            }
            http.end();
        }
    }
    
    // Free Keyless Fallback: Open-Meteo (Fast HTTP, Zero-SSL Overhead)
    float lat = _storage->getLatitude();
    float lon = _storage->getLongitude();
    if (lat == 0.0f && lon == 0.0f) {
        lat = 28.6139f; // Default Delhi coordinates
        lon = 77.2090f;
    }
    
    String url = "http://api.open-meteo.com/v1/forecast"
                 "?latitude=" + String(lat, 4) + 
                 "&longitude=" + String(lon, 4) + 
                 "&current=temperature_2m,weather_code,wind_speed_10m" + 
                 "&daily=temperature_2m_max,temperature_2m_min,weather_code" + 
                 "&timezone=auto";

    WiFiClient client;
    HTTPClient http;
    http.setUserAgent(HTTP_USER_AGENT);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(4500);
                 
    if (http.begin(client, url)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error) {
                _data.currentTemp = doc["current"]["temperature_2m"] | 0.0f;
                _data.weatherCode = doc["current"]["weather_code"] | 0;
                _data.currentWind = doc["current"]["wind_speed_10m"] | 0.0f;
                
                JsonArray minT = doc["daily"]["temperature_2m_min"];
                JsonArray maxT = doc["daily"]["temperature_2m_max"];
                JsonArray codes = doc["daily"]["weather_code"];
                
                for (int i = 0; i < 3 && i < (int)minT.size(); i++) {
                    _data.minTemp[i] = minT[i] | 0.0f;
                    _data.maxTemp[i] = maxT[i] | 0.0f;
                    _data.dailyCode[i] = codes[i] | 0;
                }
                
                _data.isValid = true;
                _data.lastFetch = millis();
                
                if (_face && !_face->isSleeping()) {
                    if (_data.weatherCode == 0 || _data.weatherCode == 1) { // Clear/Sunny
                        _face->onGoodWeather(); 
                    }
                }
                http.end();
                _isFetching = false;
                return;
            }
        }
        http.end();
    }

    // Fallback: Try HTTPS if HTTP failed
    {
        WiFiClientSecure clientSecure;
        clientSecure.setInsecure();
        HTTPClient httpSecure;
        httpSecure.setUserAgent(HTTP_USER_AGENT);
        httpSecure.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        httpSecure.setTimeout(5000);
        String httpsUrl = "https://api.open-meteo.com/v1/forecast"
                          "?latitude=" + String(lat, 4) + 
                          "&longitude=" + String(lon, 4) + 
                          "&current=temperature_2m,weather_code,wind_speed_10m" + 
                          "&daily=temperature_2m_max,temperature_2m_min,weather_code" + 
                          "&timezone=auto";
        if (httpSecure.begin(clientSecure, httpsUrl)) {
            int httpCode = httpSecure.GET();
            if (httpCode == HTTP_CODE_OK) {
                String payload = httpSecure.getString();
                JsonDocument doc;
                if (!deserializeJson(doc, payload)) {
                    _data.currentTemp = doc["current"]["temperature_2m"] | 0.0f;
                    _data.weatherCode = doc["current"]["weather_code"] | 0;
                    _data.currentWind = doc["current"]["wind_speed_10m"] | 0.0f;
                    JsonArray minT = doc["daily"]["temperature_2m_min"];
                    JsonArray maxT = doc["daily"]["temperature_2m_max"];
                    JsonArray codes = doc["daily"]["weather_code"];
                    for (int i = 0; i < 3 && i < (int)minT.size(); i++) {
                        _data.minTemp[i] = minT[i] | 0.0f;
                        _data.maxTemp[i] = maxT[i] | 0.0f;
                        _data.dailyCode[i] = codes[i] | 0;
                    }
                    _data.isValid = true;
                    _data.lastFetch = millis();
                }
            }
            httpSecure.end();
        }
    }
    _isFetching = false;
}

bool AppWeather::_geocodeConfiguredCity() {
    if (!_storage) return false;

    String city = _storage->getCityName();
    city.trim();
    if (city.length() == 0) return false;

    // Check if offline table already resolved it
    float lat = 0, lon = 0;
    if (Storage::geocodeCity(city, lat, lon)) {
        // If stored lat/lon match offline coordinates, no network lookup needed
        if (abs(_storage->getLatitude() - lat) < 0.01f && abs(_storage->getLongitude() - lon) < 0.01f) {
            return true;
        }
    }

    // Only query online geocoding if lat/lon are default and city != "New Delhi"
    if (city.equalsIgnoreCase("New Delhi")) return false;

    String encoded;
    for (size_t i = 0; i < city.length(); ++i) {
        char c = city[i];
        if (isalnum((unsigned char)c)) encoded += c;
        else encoded += '+';
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setUserAgent(HTTP_USER_AGENT);
    http.setTimeout(6000);
    String url = String(API_GEOCODING) + "?name=" + encoded + "&count=1&language=en&format=json";
    bool success = false;
    if (http.begin(client, url)) {
        if (http.GET() == HTTP_CODE_OK) {
            JsonDocument doc;
            if (!deserializeJson(doc, http.getString()) && doc["results"].is<JsonArray>() && doc["results"].size() > 0) {
                JsonObject result = doc["results"][0];
                if (!result["latitude"].isNull() && !result["longitude"].isNull()) {
                    float newLat = result["latitude"].as<float>();
                    float newLon = result["longitude"].as<float>();
                    _storage->setLocation(newLat, newLon, city);
                    success = true;
                }
            }
        }
        http.end();
    }
    return success;
}

const uint8_t* AppWeather::_getIconForCode(int code) {
    // WMO Weather interpretation codes
    if (code == 0) return weather_sun_16; // Clear
    if (code == 1 || code == 2) return weather_partcloud_16; // Partly cloudy
    if (code == 3) return weather_cloud_16; // Overcast
    if (code >= 45 && code <= 48) return weather_fog_16; // Fog
    if (code >= 51 && code <= 67) return weather_rain_16; // Drizzle/Rain
    if (code >= 71 && code <= 77) return weather_snow_16; // Snow
    if (code >= 80 && code <= 82) return weather_rain_16; // Showers
    if (code >= 85 && code <= 86) return weather_snow_16; // Snow showers
    if (code >= 95) return weather_storm_16; // Thunderstorm
    
    return weather_sun_16; // Default
}

const char* AppWeather::_getConditionText(int code) {
    if (code == 0) return "Clear Sky";
    if (code == 1) return "Mainly Clear";
    if (code == 2) return "Partly Cloudy";
    if (code == 3) return "Overcast";
    if (code >= 45 && code <= 48) return "Foggy";
    if (code >= 51 && code <= 55) return "Drizzle";
    if (code >= 61 && code <= 67) return "Rain";
    if (code >= 71 && code <= 77) return "Snow";
    if (code >= 80 && code <= 82) return "Showers";
    if (code >= 85 && code <= 86) return "Snow Showers";
    if (code >= 95) return "Thunderstorm";
    return "Fair";
}

bool AppWeather::_autoDetectLocation() {
    if (!_storage || WiFi.status() != WL_CONNECTED) return false;
    
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(4500);
    http.setUserAgent(HTTP_USER_AGENT);
    
    // Attempt 1: ip-api.com (Fast HTTP, keyless, returns city & lat/lon)
    if (http.begin(client, API_IP_GEOLOCATION_1)) {
        int code = http.GET();
        if (code == HTTP_CODE_OK) {
            JsonDocument doc;
            if (!deserializeJson(doc, http.getString())) {
                const char* status = doc["status"] | "";
                if (strcmp(status, "success") == 0) {
                    const char* c = doc["city"] | "";
                    float lat = doc["lat"] | 0.0f;
                    float lon = doc["lon"] | 0.0f;
                    if (c && strlen(c) > 0 && lat != 0.0f && lon != 0.0f) {
                        _storage->setLocation(lat, lon, String(c));
                        http.end();
                        return true;
                    }
                }
            }
        }
        http.end();
    }
    
    // Attempt 2 Fallback: ipwho.is (Fast HTTP, keyless)
    if (http.begin(client, API_IP_GEOLOCATION_2)) {
        int code = http.GET();
        if (code == HTTP_CODE_OK) {
            JsonDocument doc;
            if (!deserializeJson(doc, http.getString())) {
                bool success = doc["success"] | false;
                if (success) {
                    const char* c = doc["city"] | "";
                    float lat = doc["latitude"] | 0.0f;
                    float lon = doc["longitude"] | 0.0f;
                    if (c && strlen(c) > 0 && lat != 0.0f && lon != 0.0f) {
                        _storage->setLocation(lat, lon, String(c));
                        http.end();
                        return true;
                    }
                }
            }
        }
        http.end();
    }
    
    return false;
}
