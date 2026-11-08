// ============================================================================
// DeskBuddy — NVS Storage Implementation
// ============================================================================
#include "storage.h"
#include <Preferences.h>

static Preferences prefs;

bool Storage::begin() {
    return prefs.begin(NVS_NAMESPACE, false);
}

bool Storage::saveString(const char* key, const String& value) {
    return prefs.putString(key, value) > 0;
}

String Storage::loadString(const char* key, const String& defaultVal) {
    return prefs.getString(key, defaultVal);
}

bool Storage::saveInt(const char* key, int32_t value) {
    return prefs.putInt(key, value) > 0;
}

int32_t Storage::loadInt(const char* key, int32_t defaultVal) {
    return prefs.getInt(key, defaultVal);
}

bool Storage::saveFloat(const char* key, float value) {
    return prefs.putFloat(key, value) > 0;
}

float Storage::loadFloat(const char* key, float defaultVal) {
    return prefs.getFloat(key, defaultVal);
}

bool Storage::saveBool(const char* key, bool value) {
    return prefs.putBool(key, value);
}

bool Storage::loadBool(const char* key, bool defaultVal) {
    return prefs.getBool(key, defaultVal);
}

// --- Convenience Accessors ---

bool Storage::isConfigured() {
    return loadBool(NVS_KEY_CONFIGURED, false);
}

void Storage::setConfigured(bool val) {
    saveBool(NVS_KEY_CONFIGURED, val);
}

String Storage::getWifiSSID() {
    return loadString(NVS_KEY_WIFI_SSID);
}

String Storage::getWifiPass() {
    return loadString(NVS_KEY_WIFI_PASS);
}

void Storage::setWifi(const String& ssid, const String& pass) {
    saveString(NVS_KEY_WIFI_SSID, ssid);
    saveString(NVS_KEY_WIFI_PASS, pass);
}

float Storage::getLatitude() {
    return loadFloat(NVS_KEY_LATITUDE, 28.6139f); // Default: New Delhi
}

float Storage::getLongitude() {
    return loadFloat(NVS_KEY_LONGITUDE, 77.2090f);
}

String Storage::getCityName() {
    return loadString(NVS_KEY_CITY_NAME, "New Delhi");
}

void Storage::setLocation(float lat, float lon, const String& cityName) {
    saveFloat(NVS_KEY_LATITUDE, lat);
    saveFloat(NVS_KEY_LONGITUDE, lon);
    if (cityName.length() > 0) {
        saveString(NVS_KEY_CITY_NAME, cityName);
    }
}

bool Storage::geocodeCity(const String& cityName, float& lat, float& lon) {
    if (cityName.length() == 0) return false;
    
    String c = cityName;
    c.toLowerCase();
    c.trim();
    
    // Indian Cities
    if (c.indexOf("delhi") >= 0 || c.indexOf("noida") >= 0 || c.indexOf("gurgaon") >= 0 || c.indexOf("gurugram") >= 0) { lat = 28.6139f; lon = 77.2090f; return true; }
    if (c.indexOf("mumbai") >= 0 || c.indexOf("bombay") >= 0 || c.indexOf("thane") >= 0) { lat = 19.0760f; lon = 72.8777f; return true; }
    if (c.indexOf("bangalore") >= 0 || c.indexOf("bengaluru") >= 0) { lat = 12.9716f; lon = 77.5946f; return true; }
    if (c.indexOf("kolkata") >= 0 || c.indexOf("calcutta") >= 0) { lat = 22.5726f; lon = 88.3639f; return true; }
    if (c.indexOf("chennai") >= 0 || c.indexOf("madras") >= 0) { lat = 13.0827f; lon = 80.2707f; return true; }
    if (c.indexOf("hyderabad") >= 0 || c.indexOf("secunderabad") >= 0) { lat = 17.3850f; lon = 78.4867f; return true; }
    if (c.indexOf("pune") >= 0) { lat = 18.5204f; lon = 73.8567f; return true; }
    if (c.indexOf("ahmedabad") >= 0) { lat = 23.0225f; lon = 72.5714f; return true; }
    if (c.indexOf("surat") >= 0) { lat = 21.1702f; lon = 72.8311f; return true; }
    if (c.indexOf("jaipur") >= 0) { lat = 26.9124f; lon = 75.7873f; return true; }
    if (c.indexOf("lucknow") >= 0) { lat = 26.8467f; lon = 80.9462f; return true; }
    if (c.indexOf("kanpur") >= 0) { lat = 26.4499f; lon = 80.3319f; return true; }
    if (c.indexOf("nagpur") >= 0) { lat = 21.1458f; lon = 79.0882f; return true; }
    if (c.indexOf("indore") >= 0) { lat = 22.7196f; lon = 75.8577f; return true; }
    if (c.indexOf("bhopal") >= 0) { lat = 23.2599f; lon = 77.4126f; return true; }
    if (c.indexOf("patna") >= 0) { lat = 25.5941f; lon = 85.1376f; return true; }
    if (c.indexOf("vadodara") >= 0 || c.indexOf("baroda") >= 0) { lat = 22.3072f; lon = 73.1812f; return true; }
    if (c.indexOf("chandigarh") >= 0) { lat = 30.7333f; lon = 76.7794f; return true; }
    if (c.indexOf("agra") >= 0) { lat = 27.1767f; lon = 78.0081f; return true; }
    if (c.indexOf("varanasi") >= 0 || c.indexOf("banaras") >= 0 || c.indexOf("kashi") >= 0) { lat = 25.3176f; lon = 82.9739f; return true; }
    if (c.indexOf("amritsar") >= 0) { lat = 31.6340f; lon = 74.8723f; return true; }
    if (c.indexOf("coimbatore") >= 0) { lat = 11.0168f; lon = 76.9558f; return true; }
    if (c.indexOf("kochi") >= 0 || c.indexOf("cochin") >= 0) { lat = 9.9312f; lon = 76.2673f; return true; }
    if (c.indexOf("visakhapatnam") >= 0 || c.indexOf("vizag") >= 0) { lat = 17.6868f; lon = 83.2185f; return true; }
    
    // Global Cities
    if (c.indexOf("london") >= 0) { lat = 51.5074f; lon = -0.1278f; return true; }
    if (c.indexOf("new york") >= 0 || c.indexOf("nyc") >= 0) { lat = 40.7128f; lon = -74.0060f; return true; }
    if (c.indexOf("tokyo") >= 0) { lat = 35.6762f; lon = 139.6503f; return true; }
    if (c.indexOf("paris") >= 0) { lat = 48.8566f; lon = 2.3522f; return true; }
    if (c.indexOf("dubai") >= 0) { lat = 25.2048f; lon = 55.2708f; return true; }
    if (c.indexOf("singapore") >= 0) { lat = 1.3521f; lon = 103.8198f; return true; }
    if (c.indexOf("sydney") >= 0) { lat = -33.8688f; lon = 151.2093f; return true; }
    if (c.indexOf("toronto") >= 0) { lat = 43.6532f; lon = -79.3832f; return true; }
    if (c.indexOf("berlin") >= 0) { lat = 52.5200f; lon = 13.4050f; return true; }
    
    return false;
}

uint8_t Storage::getBrightness() {
    uint8_t b = (uint8_t)loadInt(NVS_KEY_BRIGHTNESS, 3);
    if (b == 0 || b > 3) b = 3; // Default: Max brightness (3)
    return b;
}

void Storage::setBrightness(uint8_t level) {
    saveInt(NVS_KEY_BRIGHTNESS, level);
}

bool Storage::getTimeFormat24() {
    return loadBool(NVS_KEY_TIME_FMT, true); // Default: 24h
}

void Storage::setTimeFormat24(bool is24h) {
    saveBool(NVS_KEY_TIME_FMT, is24h);
}

bool Storage::getDisplayFlip() {
    return loadBool(NVS_KEY_DISPLAY_FLIP, false);
}

void Storage::setDisplayFlip(bool flipped) {
    saveBool(NVS_KEY_DISPLAY_FLIP, flipped);
}

int32_t Storage::getTimezoneOffset() {
    return loadInt(NVS_KEY_TIMEZONE, 19800); // Default: IST (+5:30)
}

void Storage::setTimezoneOffset(int32_t offsetSec) {
    saveInt(NVS_KEY_TIMEZONE, offsetSec);
}

uint32_t Storage::getSleepTimeout() {
    return (uint32_t)loadInt(NVS_KEY_SLEEP_TIMEOUT, 600000); // Default: 10 min
}

void Storage::setSleepTimeout(uint32_t ms) {
    saveInt(NVS_KEY_SLEEP_TIMEOUT, (int32_t)ms);
}

uint8_t Storage::getClockMode() {
    return (uint8_t)loadInt(NVS_KEY_CLOCK_MODE, 0); // Default: MODE_FULL_DIGITAL (0)
}

void Storage::setClockMode(uint8_t mode) {
    saveInt(NVS_KEY_CLOCK_MODE, (int32_t)mode);
}

uint8_t Storage::getEyeStyle() {
    return (uint8_t)loadInt(NVS_KEY_EYE_STYLE, 0); // Default: CLASSIC (0)
}

void Storage::setEyeStyle(uint8_t style) {
    saveInt(NVS_KEY_EYE_STYLE, (int32_t)style);
}

uint8_t Storage::getAutoCycle() {
    return (uint8_t)loadInt(NVS_KEY_AUTOCYCLE, 3); // Default: 3 min (0=OFF, 1=1m, 2=2m, 3=3m, 4=5m)
}

void Storage::setAutoCycle(uint8_t mode) {
    saveInt(NVS_KEY_AUTOCYCLE, (int32_t)mode);
}

uint8_t Storage::getHappiness() {
    return (uint8_t)loadInt(NVS_KEY_HAPPINESS, 75);
}

void Storage::setHappiness(uint8_t happiness) {
    saveInt(NVS_KEY_HAPPINESS, (int32_t)happiness);
}

String Storage::getOpenWeatherKey() {
    String key = loadString(NVS_KEY_OWM_KEY, "");
    if (key.length() == 0 && String(OPENWEATHER_API_KEY).length() > 0) {
        return String(OPENWEATHER_API_KEY);
    }
    return key;
}

void Storage::setOpenWeatherKey(const String& key) {
    saveString(NVS_KEY_OWM_KEY, key);
}

String Storage::getCoinGeckoKey() {
    String key = loadString(NVS_KEY_CG_KEY, "");
    if (key.length() == 0 && String(COINGECKO_API_KEY).length() > 0) {
        return String(COINGECKO_API_KEY);
    }
    return key;
}

void Storage::setCoinGeckoKey(const String& key) {
    saveString(NVS_KEY_CG_KEY, key);
}

int32_t Storage::getDinoHighScore() {
    return loadInt(NVS_KEY_DINO_HISCORE, 0);
}

void Storage::setDinoHighScore(int32_t score) {
    saveInt(NVS_KEY_DINO_HISCORE, score);
}

int32_t Storage::getFlappyHighScore() {
    return loadInt(NVS_KEY_FLAPPY_HISCORE, 0);
}

void Storage::setFlappyHighScore(int32_t score) {
    saveInt(NVS_KEY_FLAPPY_HISCORE, score);
}

int32_t Storage::getStackHighScore() {
    return loadInt(NVS_KEY_STACK_HISCORE, 0);
}

void Storage::setStackHighScore(int32_t score) {
    saveInt(NVS_KEY_STACK_HISCORE, score);
}

int32_t Storage::getReactionBest() {
    return loadInt(NVS_KEY_REACT_BEST, 9999);
}

void Storage::setReactionBest(int32_t ms) {
    saveInt(NVS_KEY_REACT_BEST, ms);
}

bool Storage::hasCustomPhoto() {
    return loadBool(NVS_KEY_PHOTO_HAS, false);
}

bool Storage::saveCustomPhoto(const uint8_t* bitmap) {
    if (!bitmap) return false;
    prefs.putBytes(NVS_KEY_PHOTO_DATA, bitmap, 1024);
    saveBool(NVS_KEY_PHOTO_HAS, true);
    return true;
}

bool Storage::loadCustomPhoto(uint8_t* bitmap) {
    if (!bitmap) return false;
    if (!hasCustomPhoto()) return false;
    size_t readLen = prefs.getBytes(NVS_KEY_PHOTO_DATA, bitmap, 1024);
    return readLen == 1024;
}

void Storage::deleteCustomPhoto() {
    saveBool(NVS_KEY_PHOTO_HAS, false);
    prefs.remove(NVS_KEY_PHOTO_DATA);
}

bool Storage::getPhotoInverted() {
    return loadBool(NVS_KEY_PHOTO_INV, false);
}

void Storage::setPhotoInverted(bool inv) {
    saveBool(NVS_KEY_PHOTO_INV, inv);
}

void Storage::eraseAll() {
    prefs.clear();
}
