/*
=============================================================================
 Smart Token Management System - Counter Controller Unit (Stage 3)
 
 Hardware Description (NodeMCU ESP8266):
   - TM1637 4-Digit Display: CLK -> D7, DIO -> D6
   - 4 Push Buttons connected between GND and:
       - BTN_NEXT  -> D2 (Increments Current Serving Token)
       - BTN_PREV  -> D3 (Decrements Current Serving Token)
       - BTN_SKIP  -> D4 (Skips current token - increments)
       - BTN_RESET -> D5 (Resets Current Serving Token to 0)
   - WiFi connection to local router
   - Direct HTTPS REST API patch to Supabase settings table
=============================================================================
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TM1637Display.h>

// ======================== HARDWARE PINS ========================
#define PIN_TM1637_CLK D7
#define PIN_TM1637_DIO D6

#define PIN_BTN_NEXT   D2
#define PIN_BTN_PREV   D3
#define PIN_BTN_SKIP   D4
#define PIN_BTN_RESET  D5

// ======================= CREDENTIALS CONFIG ====================
const char* WIFI_SSID = "GSV_Electrical_Enterprises";
const char* WIFI_PASS = "@26Nov1996#"; 

// Pre-populated with your project's active Supabase configurations
const char* SUPABASE_URL = "https://lziwnwdiyfdgyznngcma.supabase.co";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imx6aXdud2RpeWZkZ3l6bm5nY21hIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODQ3MDQ2OTYsImV4cCI6MjEwMDI4MDY5Nn0.Gpe-dZfVIjLhFT_VP__uawVjwPMbDciUfmFzWNz5hpc";


// ==================== MODULAR CLASS DEFINITIONS ================

/**
 * Encapsulates a debounced hardware push button using edge-detection.
 * Configured as INPUT_PULLUP (active LOW state when pressed).
 */
class ButtonManager {
private:
    uint8_t _pin;
    unsigned long _debounceDelay;
    unsigned long _lastPressTime;
    bool _wasHigh;

public:
    ButtonManager(uint8_t pin, unsigned long debounceDelay = 250) {
        _pin = pin;
        _debounceDelay = debounceDelay;
        _lastPressTime = 0;
        _wasHigh = true;
    }

    void begin() {
        pinMode(_pin, INPUT_PULLUP);
    }

    /**
     * Checks if the button transitions from HIGH to LOW with debouncing.
     * Returns true only on the initial falling edge.
     */
    bool isPressed() {
        bool currentVal = (digitalRead(_pin) == HIGH);
        unsigned long now = millis();

        if (_wasHigh && !currentVal) { // Falling edge detected
            if (now - _lastPressTime > _debounceDelay) {
                _lastPressTime = now;
                _wasHigh = currentVal;
                return true;
            }
        }
        _wasHigh = currentVal;
        return false;
    }
};

/**
 * Wraps the TM1637 Display library to provide simple number representation
 * and aesthetic display events (clearing, flashing).
 */
class DisplayManager {
private:
    TM1637Display _display;

public:
    DisplayManager(uint8_t clkPin, uint8_t dioPin) : _display(clkPin, dioPin) {}

    void begin() {
        _display.setBrightness(7);
        _display.showNumberDec(0, true);
    }

    /**
     * Display a number with leading zeros (e.g. 0042)
     */
    void showNumber(int num) {
        _display.showNumberDec(num, true); // true = include leading zeroes
    }

    /**
     * Visual animation for counter reset
     */
    void showResetFlash() {
        _display.showNumberDec(0, true);
        delay(150);
        _display.clear();
        delay(150);
        _display.showNumberDec(0, true);
    }
};

/**
 * Manages Wi-Fi connections, status logs, and background reconnection.
 */
class WiFiConnectionManager {
private:
    const char* _ssid;
    const char* _password;
    unsigned long _lastCheckTime;
    const unsigned long _checkInterval = 10000; // Check status every 10 seconds

public:
    WiFiConnectionManager(const char* ssid, const char* password) {
        _ssid = ssid;
        _password = password;
        _lastCheckTime = 0;
    }

    void begin() {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);
        connect();
    }

    void connect() {
        Serial.print("\n[WiFi] Connecting to: ");
        Serial.println(_ssid);
        
        WiFi.begin(_ssid, _password);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[WiFi] Connected successfully!");
            Serial.print("[WiFi] IP Address: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("\n[WiFi] Connection timed out. Will retry in background...");
        }
    }

    bool isConnected() {
        return (WiFi.status() == WL_CONNECTED);
    }

    /**
     * Monitors status and automatically triggers reconnection if Wi-Fi drops.
     */
    void keepAlive() {
        unsigned long now = millis();
        if (now - _lastCheckTime > _checkInterval) {
            _lastCheckTime = now;
            if (!isConnected()) {
                Serial.println("[WiFi] Connection lost. Reconnecting...");
                WiFi.disconnect();
                WiFi.begin(_ssid, _password);
            }
        }
    }
};

/**
 * Manages direct HTTPS communication with the Supabase PostgreSQL REST API.
 * Uses WiFiClientSecure to bypass certificate verification, allowing lightweight SSL.
 */
class SupabaseManager {
private:
    String _url;
    String _apiKey;

public:
    SupabaseManager(const char* url, const char* apiKey) {
        _url = String(url);
        _apiKey = String(apiKey);
    }

    /**
     * Patches the 'Current Serving Token' record in the settings database table.
     */
    bool updateServingToken(int currentToken) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[Supabase] Error: WiFi not connected. Patch aborted.");
            return false;
        }

        WiFiClientSecure client;
        client.setInsecure(); // Bypasses SSL handshake root cert checks for memory efficiency
        
        HTTPClient http;
        
        String endpoint = _url + "/rest/v1/settings?key=eq.Current%20Serving%20Token";
        
        Serial.printf("[Supabase] Updating Token #%d...\n", currentToken);
        
        http.begin(client, endpoint);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("apikey", _apiKey);
        http.addHeader("Authorization", "Bearer " + _apiKey);
        
        String patchBody = "{\"value\":\"" + String(currentToken) + "\"}";
        
        int httpResponseCode = http.PATCH(patchBody);
        bool success = false;
        
        if (httpResponseCode == 200 || httpResponseCode == 204) {
            Serial.println("[Supabase] Success: Token updated in database.");
            success = true;
        } else {
            Serial.printf("[Supabase] PATCH Failed. HTTP Code: %d\n", httpResponseCode);
            Serial.println(http.getString());
        }
        
        http.end();
        return success;
    }
};

// ==================== INSTANCE DECLARATIONS ====================
WiFiConnectionManager wifi(WIFI_SSID, WIFI_PASS);
DisplayManager display(PIN_TM1637_CLK, PIN_TM1637_DIO);
SupabaseManager supabase(SUPABASE_URL, SUPABASE_KEY);

ButtonManager btnNext(PIN_BTN_NEXT);
ButtonManager btnPrev(PIN_BTN_PREV);
ButtonManager btnSkip(PIN_BTN_SKIP);
ButtonManager btnReset(PIN_BTN_RESET);

int currentServingToken = 0;

// ======================== SYSTEM FUNCTIONS ====================

void nextToken() {
    currentServingToken++;
    Serial.printf("[Counter] NEXT -> Token: %d\n", currentServingToken);
    display.showNumber(currentServingToken);
    supabase.updateServingToken(currentServingToken);
}

void previousToken() {
    if (currentServingToken > 0) {
        currentServingToken--;
    }
    Serial.printf("[Counter] PREV -> Token: %d\n", currentServingToken);
    display.showNumber(currentServingToken);
    supabase.updateServingToken(currentServingToken);
}

void skipToken() {
    currentServingToken++;
    Serial.printf("[Counter] SKIP -> Token: %d\n", currentServingToken);
    display.showNumber(currentServingToken);
    supabase.updateServingToken(currentServingToken);
}

void resetCounter() {
    currentServingToken = 0;
    Serial.println("[Counter] RESET -> Token: 0");
    display.showResetFlash();
    supabase.updateServingToken(currentServingToken);
}

// ======================== MAIN ROUTINES =======================

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[System] Counter Controller Unit starting...");

    // Initialize buttons
    btnNext.begin();
    btnPrev.begin();
    btnSkip.begin();
    btnReset.begin();

    // Initialize display — shows 0000
    display.begin();

    // Connect to WiFi
    wifi.begin();
}

void loop() {
    // Keep Wi-Fi connection alive
    wifi.keepAlive();

    // Scan for button events
    if (btnNext.isPressed()) {
        nextToken();
    }
    else if (btnPrev.isPressed()) {
        previousToken();
    }
    else if (btnSkip.isPressed()) {
        skipToken();
    }
    else if (btnReset.isPressed()) {
        resetCounter();
    }
    
    delay(10);
}
