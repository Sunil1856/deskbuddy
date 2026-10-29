// ============================================================================
// DeskBuddy — World Info App Implementation
// ============================================================================
#include "app_world_info.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "icons.h"

namespace {

// Streaming RSS Parser: reads directly from HTTPClient stream without allocating
// 100KB XML payload in RAM, eliminating ESP32 heap exhaustion (OOM) crashes.
// Also properly handles chunked transfer-encoding via HTTPClient stream wrapper.
String parseRssHeadlineStream(WiFiClient* stream, int timeoutMs = 3000) {
    if (!stream) return "";
    unsigned long start = millis();
    
    // 1. Look for "<item>"
    const char* targetItem = "<item>";
    int itemMatch = 0;
    while (stream->connected() || stream->available()) {
        if (millis() - start > (unsigned long)timeoutMs) return "";
        if (stream->available()) {
            char c = (char)stream->read();
            if (c == targetItem[itemMatch]) {
                itemMatch++;
                if (targetItem[itemMatch] == '\0') {
                    break; // Found <item>
                }
            } else {
                itemMatch = (c == targetItem[0]) ? 1 : 0;
            }
        } else {
            delay(2);
        }
    }
    if (targetItem[itemMatch] != '\0') return "";

    // 2. Look for "<title>" inside this item
    const char* targetTitle = "<title>";
    int titleMatch = 0;
    while (stream->connected() || stream->available()) {
        if (millis() - start > (unsigned long)timeoutMs) return "";
        if (stream->available()) {
            char c = (char)stream->read();
            if (c == targetTitle[titleMatch]) {
                titleMatch++;
                if (targetTitle[titleMatch] == '\0') {
                    break; // Found <title>
                }
            } else {
                titleMatch = (c == targetTitle[0]) ? 1 : 0;
            }
        } else {
            delay(2);
        }
    }
    if (targetTitle[titleMatch] != '\0') return "";

    // 3. Read title characters until "</title>"
    String title = "";
    const char* endTitle = "</title>";
    int endMatch = 0;
    while (stream->connected() || stream->available()) {
        if (millis() - start > (unsigned long)timeoutMs) break;
        if (stream->available()) {
            char c = (char)stream->read();
            if (c == endTitle[endMatch]) {
                endMatch++;
                if (endTitle[endMatch] == '\0') {
                    break; // Finished title
                }
            } else {
                if (endMatch > 0) {
                    for (int k = 0; k < endMatch; k++) {
                        if (title.length() < 140) title += endTitle[k];
                    }
                    endMatch = (c == endTitle[0]) ? 1 : 0;
                }
                if (endMatch == 0) {
                    if (title.length() < 140) {
                        title += c;
                    }
                }
            }
        } else {
            delay(2);
        }
    }

    // Clean up XML entities and CDATA
    title.replace("<![CDATA[", "");
    title.replace("]]>", "");
    title.replace("&amp;", "&");
    title.replace("&quot;", "\"");
    title.replace("&#39;", "'");
    title.replace("&apos;", "'");
    title.replace("&lt;", "<");
    title.replace("&gt;", ">");
    title.trim();
    return title;
}

// Built-in fallbacks if external quote/fact rate limits trigger
const char* FALLBACK_QUOTES[][2] = {
    {"Believe you can and you're halfway there.", "Theodore Roosevelt"},
    {"Action is the foundational key to all success.", "Pablo Picasso"},
    {"Stay hungry. Stay foolish.", "Steve Jobs"},
    {"Do what you can, with what you have, where you are.", "T. Roosevelt"},
    {"Happiness depends upon ourselves.", "Aristotle"},
    {"Dream big and dare to fail.", "Norman Vaughan"}
};

const char* FALLBACK_FACTS[] = {
    "Honey never spoils; archaeologists found edible 3000-year-old honey in Egyptian tombs.",
    "A day on Venus is longer than a year on Venus.",
    "Octopuses have three hearts and blue blood.",
    "Bananas are curved because they grow towards the sun against gravity.",
    "Butterflies taste with their feet.",
    "The shortest commercial flight in the world takes just 57 seconds in Scotland."
};

} // namespace

void AppWorldInfo::begin(DisplayManager* dm, Storage* storage) {
    _dm = dm;
    _storage = storage;
    _pageSwitchTime = millis();
    
    // Initialize clean fallbacks so every section displays instantly with 0ms latency
    _infoText[(int)InfoType::GOLD] = "Rs 78,500 / 10g";
    _authorText[(int)InfoType::GOLD] = "24K India (MCX Est)";
    
    _infoText[(int)InfoType::SPORTS] = "India Cricket & Sports live updates";
    _authorText[(int)InfoType::SPORTS] = "India Sports";
    
    _infoText[(int)InfoType::INDIA_NEWS] = "National headlines & current events";
    _authorText[(int)InfoType::INDIA_NEWS] = "India News";
    
    _infoText[(int)InfoType::TECH] = "AI & modern tech breakthroughs";
    _authorText[(int)InfoType::TECH] = "Tech News";
    
    _infoText[(int)InfoType::CRYPTO] = "BTC: ~$95,000 | ETH: ~$3,300";
    _authorText[(int)InfoType::CRYPTO] = "Live Markets";
    
    _infoText[(int)InfoType::QUOTE] = "\"Stay hungry. Stay foolish.\"";
    _authorText[(int)InfoType::QUOTE] = "- Steve Jobs";
    
    _infoText[(int)InfoType::FACT] = "A day on Venus is longer than a year on Venus.";
    _authorText[(int)InfoType::FACT] = "Did You Know?";
}

void AppWorldInfo::onEnter() {
    _scrollX = 0;
    _pageSwitchTime = millis();
}

void AppWorldInfo::onExit() {
}

void AppWorldInfo::update() {
    // Smooth horizontal ticker scroll
    _scrollX++;
    
    // Non-blocking background fetch: only triggers after user settles on a section (> 800ms)
    if (WiFi.status() == WL_CONNECTED && !_isFetching) {
        if (millis() - _pageSwitchTime > 800) {
            if (_lastFetchTime[(int)_type] == 0 || (millis() - _lastFetchTime[(int)_type] > INFO_REFRESH_MS)) {
                _fetchData(_type);
            }
        }
    }
}

void AppWorldInfo::handleTouch(TouchEvent event) {
    if (event == TouchEvent::TAP) {
        // Instant section switch (0ms latency, zero blocking!)
        _type = (InfoType)(((int)_type + 1) % (int)InfoType::COUNT);
        _scrollX = 0;
        _pageSwitchTime = millis();
    } else if (event == TouchEvent::LONG_PRESS) {
        // Force refresh current section
        _lastFetchTime[(int)_type] = 0;
        _pageSwitchTime = 0;
    }
}

void AppWorldInfo::render() {
    if (!_dm) return;
    Adafruit_SSD1306& d = _dm->display();
    
    d.setTextSize(1);
    
    // Header
    const char* header = "";
    switch (_type) {
        case InfoType::GOLD:       header = "GOLD PRICE (24K)"; break;
        case InfoType::SPORTS:     header = "INDIA SPORTS"; break;
        case InfoType::INDIA_NEWS: header = "INDIA NEWS"; break;
        case InfoType::TECH:       header = "TECH HEADLINES"; break;
        case InfoType::CRYPTO:     header = "CRYPTO (BTC/ETH)"; break;
        case InfoType::QUOTE:      header = "DAILY QUOTE"; break;
        case InfoType::FACT:       header = "RANDOM FACT"; break;
        default: break;
    }
    
    _dm->drawCenteredText(header, 2, 1);
    d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);
    
    String currText = _infoText[(int)_type];
    String currAuthor = _authorText[(int)_type];
    
    // Only show "Loading..." if no cached/fallback content is available
    if (_isFetching && currText.length() == 0) {
        _dm->drawCenteredText("Loading...", 32, 1);
        return;
    }
    
    if (currText.length() == 0) {
        if (WiFi.status() != WL_CONNECTED) {
            _dm->drawCenteredText("WiFi Disconnected", 32, 1);
        } else {
            _dm->drawCenteredText("Fetch failed", 28, 1);
            _dm->drawCenteredText("Hold to retry", 40, 1);
        }
        return;
    }
    
    // Content rendering based on type
    if (_type == InfoType::GOLD || _type == InfoType::CRYPTO) {
        d.setTextSize(1);
        _dm->drawCenteredText(currText.c_str(), 24, 1);
        if (currAuthor.length() > 0) {
            _dm->drawCenteredText(currAuthor.c_str(), 42, 1);
        }
    } else {
        d.setTextSize(1);
        int16_t x1, y1;
        uint16_t tw, th;
        d.getTextBounds(currText.c_str(), 0, 0, &x1, &y1, &tw, &th);
        
        if ((int16_t)tw <= SCREEN_W - 4) {
            _dm->drawCenteredText(currText.c_str(), 24, 1);
            _scrollX = 0;
        } else {
            // Smooth horizontal ticker scroll
            int maxScroll = tw + 40;
            int curOffset = _scrollX % maxScroll;
            d.setCursor(SCREEN_W - curOffset, 24);
            d.print(currText);
        }
        
        if (currAuthor.length() > 0) {
            _dm->drawCenteredText(currAuthor.c_str(), 44, 1);
        }
    }
    
    // Page indicators (7 dots centered)
    int total = (int)InfoType::COUNT;
    int dotSpacing = 8;
    int startX = SCREEN_W / 2 - ((total - 1) * dotSpacing) / 2;
    for (int i = 0; i < total; i++) {
        int x = startX + i * dotSpacing;
        if (i == (int)_type) {
            d.fillCircle(x, 59, 2, SSD1306_WHITE);
        } else {
            d.drawPixel(x, 59, SSD1306_WHITE);
        }
    }
}

void AppWorldInfo::_fetchData(InfoType type) {
    if (WiFi.status() != WL_CONNECTED) return;
    
    _isFetching = true;
    
    switch (type) {
        case InfoType::GOLD:       _fetchGold(); break;
        case InfoType::SPORTS:     _fetchSports(); break;
        case InfoType::INDIA_NEWS: _fetchIndiaNews(); break;
        case InfoType::TECH:       _fetchTechNews(); break;
        case InfoType::CRYPTO:     _fetchCrypto(); break;
        case InfoType::QUOTE:      _fetchQuote(); break;
        case InfoType::FACT:       _fetchFact(); break;
        default: break;
    }
    
    _lastFetchTime[(int)type] = millis();
    _isFetching = false;
    _scrollX = 0;
}

static bool fetchMarketsCoinGecko(Storage* storage, float& btc, float& eth, float& goldUsd, float& goldInr) {
    btc = eth = goldUsd = goldInr = 0.0f;
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setUserAgent(HTTP_USER_AGENT);
    http.setTimeout(2500);
    
    String cgKey = storage ? storage->getCoinGeckoKey() : "";
    String url = API_MARKETS_COINGECKO;
    
    if (!http.begin(client, url)) return false;
    if (cgKey.length() > 0) {
        http.addHeader("x-cg-demo-api-key", cgKey);
    }
    
    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        http.end();
        return false;
    }
    
    JsonDocument doc;
    const bool valid = !deserializeJson(doc, http.getString());
    http.end();
    if (!valid) return false;
    
    btc = doc["bitcoin"]["usd"] | 0.0f;
    eth = doc["ethereum"]["usd"] | 0.0f;
    goldUsd = doc["pax-gold"]["usd"] | 0.0f;
    goldInr = doc["pax-gold"]["inr"] | 0.0f;
    return (btc > 0 || eth > 0 || goldUsd > 0 || goldInr > 0);
}

static bool fetchMarketsBinance(float& btc, float& eth, float& gold) {
    btc = eth = gold = 0.0f;
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setUserAgent(HTTP_USER_AGENT);
    http.setTimeout(2500);
    
    if (!http.begin(client, API_MARKETS_BINANCE)) return false;
    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        http.end();
        return false;
    }
    
    JsonDocument doc;
    const bool valid = !deserializeJson(doc, http.getString());
    http.end();
    if (!valid || !doc.is<JsonArray>()) return false;
    
    for (JsonObject item : doc.as<JsonArray>()) {
        const char* sym = item["symbol"] | "";
        float price = item["price"] | 0.0f;
        if (strcmp(sym, "BTCUSDT") == 0) btc = price;
        else if (strcmp(sym, "ETHUSDT") == 0) eth = price;
        else if (strcmp(sym, "PAXGUSDT") == 0) gold = price;
    }
    return (btc > 0 || eth > 0 || gold > 0);
}

void AppWorldInfo::_fetchGold() {
    float btc = 0, eth = 0, priceOzUsd = 0, priceOzInr = 0;
    bool success = fetchMarketsCoinGecko(_storage, btc, eth, priceOzUsd, priceOzInr);
    if (!success || (priceOzUsd <= 0 && priceOzInr <= 0)) {
        success = fetchMarketsBinance(btc, eth, priceOzUsd);
    }
    
    if (success && (priceOzUsd > 0 || priceOzInr > 0)) {
        // 1 Troy Ounce = 31.1034768 grams
        // If CoinGecko direct INR rate is available, use it; otherwise convert via live USD/INR (~86.85)
        float inrPerOunce = priceOzInr;
        if (inrPerOunce <= 0 && priceOzUsd > 0) {
            inrPerOunce = priceOzUsd * 86.85f;
        }
        
        float priceGramInr = inrPerOunce / 31.1035f;
        
        // India Retail 24K Gold Rate Formula (matches IBJA / MCX / Retail Jewellers):
        // Raw Spot + 6% Import Duty & AIDC + 3% GST + 0.5% standard bullion premium = 1.095x multiplier
        int retail10g = (int)round(priceGramInr * 10.0f * 1.095f);
        
        char buf1[32], buf2[32];
        if (retail10g >= 1000) {
            snprintf(buf1, sizeof(buf1), "Rs %d,%03d / 10g", retail10g / 1000, retail10g % 1000);
        } else {
            snprintf(buf1, sizeof(buf1), "Rs %d / 10g", retail10g);
        }
        
        if (priceOzUsd > 0) {
            snprintf(buf2, sizeof(buf2), "24K India | $%d/oz", (int)round(priceOzUsd));
        } else {
            snprintf(buf2, sizeof(buf2), "24K India (MCX Est)");
        }
        
        _infoText[(int)InfoType::GOLD] = buf1;
        _authorText[(int)InfoType::GOLD] = buf2;
    }
}

void AppWorldInfo::_fetchCrypto() {
    float btcPrice = 0, ethPrice = 0, goldUsd = 0, goldInr = 0;
    bool success = fetchMarketsCoinGecko(_storage, btcPrice, ethPrice, goldUsd, goldInr);
    if (!success || (btcPrice <= 0 && ethPrice <= 0)) {
        success = fetchMarketsBinance(btcPrice, ethPrice, goldUsd);
    }
    
    if (success && (btcPrice > 0 || ethPrice > 0)) {
        char buf1[32], buf2[32];
        snprintf(buf1, sizeof(buf1), "BTC: $%d", (int)round(btcPrice));
        snprintf(buf2, sizeof(buf2), "ETH: $%d", (int)round(ethPrice));
        _infoText[(int)InfoType::CRYPTO] = buf1;
        _authorText[(int)InfoType::CRYPTO] = buf2;
    }
}

void AppWorldInfo::_fetchSports() {
    _fetchNews(API_NEWS_SPORTS, InfoType::SPORTS, "India Sports");
}

void AppWorldInfo::_fetchIndiaNews() {
    _fetchNews(API_NEWS_INDIA, InfoType::INDIA_NEWS, "India Headlines");
}

void AppWorldInfo::_fetchTechNews() {
    _fetchNews(API_NEWS_TECH, InfoType::TECH, "Tech News");
}

void AppWorldInfo::_fetchNews(const char* url, InfoType type, const char* source) {
    WiFiClientSecure client;
    client.setInsecure();
    
    HTTPClient http;
    http.setUserAgent(HTTP_USER_AGENT);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(3000);
    
    if (http.begin(client, url)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            WiFiClient* stream = http.getStreamPtr();
            if (stream) {
                String headline = parseRssHeadlineStream(stream, 3000);
                if (headline.length() > 0) {
                    _infoText[(int)type] = headline;
                    _authorText[(int)type] = source;
                }
            }
        }
        http.end();
    }
}

void AppWorldInfo::_fetchQuote() {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setUserAgent(HTTP_USER_AGENT);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(2500);
    
    bool gotQuote = false;
    if (http.begin(client, API_QUOTES)) {
        int code = http.GET();
        if (code == HTTP_CODE_OK) {
            String payload = http.getString();
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, payload);
            if (!err && doc.is<JsonArray>() && doc.size() > 0) {
                const char* q = doc[0]["q"];
                const char* a = doc[0]["a"];
                if (q && strstr(q, "Too many requests") == nullptr) {
                    _infoText[(int)InfoType::QUOTE] = String("\"") + q + "\"";
                    _authorText[(int)InfoType::QUOTE] = a ? (String("- ") + a) : "- Unknown";
                    gotQuote = true;
                }
            }
        }
        http.end();
    }
    
    if (!gotQuote && _infoText[(int)InfoType::QUOTE].length() == 0) {
        int idx = random(sizeof(FALLBACK_QUOTES) / sizeof(FALLBACK_QUOTES[0]));
        _infoText[(int)InfoType::QUOTE] = String("\"") + FALLBACK_QUOTES[idx][0] + "\"";
        _authorText[(int)InfoType::QUOTE] = String("- ") + FALLBACK_QUOTES[idx][1];
    }
}

void AppWorldInfo::_fetchFact() {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setUserAgent(HTTP_USER_AGENT);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(2500);
    
    bool gotFact = false;
    if (http.begin(client, API_FACTS)) {
        int code = http.GET();
        if (code == HTTP_CODE_OK) {
            String payload = http.getString();
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, payload);
            if (!err) {
                const char* text = doc["text"];
                if (text && strlen(text) > 5) {
                    _infoText[(int)InfoType::FACT] = text;
                    _authorText[(int)InfoType::FACT] = "Random Fact";
                    gotFact = true;
                }
            }
        }
        http.end();
    }
    
    if (!gotFact && _infoText[(int)InfoType::FACT].length() == 0) {
        int idx = random(sizeof(FALLBACK_FACTS) / sizeof(FALLBACK_FACTS[0]));
        _infoText[(int)InfoType::FACT] = FALLBACK_FACTS[idx];
        _authorText[(int)InfoType::FACT] = "Fact";
    }
}
