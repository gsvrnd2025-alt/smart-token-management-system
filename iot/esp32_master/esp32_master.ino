/**
 * Smart Token Management System - Master ESP32 Ticket Dispenser with LCD
 * 
 * Hardware Description:
 * - ESP32 Development Board
 * - Walk-in Push-Button connected between GPIO 4 and GND (active LOW)
 * - LED Indicator connected to GPIO 2 (onboard LED)
 * - I2C LCD Display (16x2 or 20x4) connected to:
 *     - SDA -> GPIO 21
 *     - SCL -> GPIO 22
 *     - VCC -> 5V (or 3.3V)
 *     - GND -> GND
 * - Secondary Serial Port (Serial2 on Pins RX2=16, TX2=17) for Wired Thermal Printer
 * 
 * Features:
 * - Automatically scans I2C bus to find the LCD address (0x27, 0x3F, etc.)
 * - Attempts to connect to stored WiFi credentials and displays status on LCD
 * - Captive Portal WiFi configuration with pre-scanned WiFi networks list
 * - Redirection to the GitHub Pages Dashboard on success
 * - Wireless OTA updates via GitHub HTTP Auto-Updates over the internet
 * - Direct HTTPS integration with Supabase database for generating walk-in tokens
 * - BLE CAT Fun Printer support (bitmap rendering, proprietary 0x51 0x78 protocol)
 * - Wired ESC/POS Thermal Printer support via Serial2
 * - Time Synchronization via NTP (UTC+5:30) for ticket printing
 * - Remote BLE scan requests triggered from the dashboard
 * - Remote test print requests triggered from the dashboard
 */

// ─────────────────── Includes ────────────────────────────────────────────────
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLEAdvertisedDevice.h>
#include <HTTPUpdate.h>
#include <time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ─────────────────── OTA / Version ───────────────────────────────────────────
const int CURRENT_VERSION = 2;
const char* VERSION_URL  = "https://raw.githubusercontent.com/gsvrnd2025-alt/smart-token-management-system/master/bin/master_version.txt";
const char* FIRMWARE_URL = "https://raw.githubusercontent.com/gsvrnd2025-alt/smart-token-management-system/master/bin/esp32_master.bin";

unsigned long lastUpdateCheckTime   = 0;
const unsigned long updateCheckInterval = 300000; // 5 minutes

// ─────────────────── Supabase ────────────────────────────────────────────────
const char* SUPABASE_URL = "https://lziwnwdiyfdgyznngcma.supabase.co";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imx6aXdud2RpeWZkZ3l6bm5nY21hIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODQ3MDQ2OTYsImV4cCI6MjEwMDI4MDY5Nn0.Gpe-dZfVIjLhFT_VP__uawVjwPMbDciUfmFzWNz5hpc";
const char* DASHBOARD_REDIRECT_URL = "https://gsvrnd2025-alt.github.io/smart-token-management-system/";

// ─────────────────── WiFi ────────────────────────────────────────────────────
const char* WIFI_SSID = "GSV_Electrical_Enterprises";
const char* WIFI_PASS = "@26Nov1996#";

// ─────────────────── Hardware pins ───────────────────────────────────────────
const int BUTTON_PIN = 4;
const int LED_PIN    = 2;

// ─────────────────── Servers ─────────────────────────────────────────────────
WebServer server(80);
DNSServer dnsServer;

// ─────────────────── BLE: CAT Fun Printer ────────────────────────────────────
// CAT Fun (GT01/GB01/MX series) BLE service & characteristic UUIDs.
// If printing fails, try the alternate service UUID in catConnect().
static BLEUUID CAT_SERVICE_UUID("0000ae30-0000-1000-8000-00805f9b34fb");
static BLEUUID CAT_WRITE_UUID  ("0000ae01-0000-1000-8000-00805f9b34fb");

BLEClient*              catClient    = nullptr;
BLERemoteCharacteristic* catWriteChar = nullptr;

// 48-byte canvas = one 384-pixel wide paper row
static uint8_t canvas[48];

// ─────────────────── 5×8 bitmap font (ASCII 32–127, PROGMEM) ─────────────────
// Each character: 5 column-bytes. Column byte bit-0 = top pixel row.
static const uint8_t PROGMEM font5x8[][5] = {
  {0x00,0x00,0x00,0x00,0x00}, // 32 ' '
  {0x00,0x00,0x5F,0x00,0x00}, // 33 '!'
  {0x00,0x07,0x00,0x07,0x00}, // 34 '"'
  {0x14,0x7F,0x14,0x7F,0x14}, // 35 '#'
  {0x24,0x2A,0x7F,0x2A,0x12}, // 36 '$'
  {0x23,0x13,0x08,0x64,0x62}, // 37 '%'
  {0x36,0x49,0x55,0x22,0x50}, // 38 '&'
  {0x00,0x05,0x03,0x00,0x00}, // 39 '''
  {0x00,0x1C,0x22,0x41,0x00}, // 40 '('
  {0x00,0x41,0x22,0x1C,0x00}, // 41 ')'
  {0x14,0x08,0x3E,0x08,0x14}, // 42 '*'
  {0x08,0x08,0x3E,0x08,0x08}, // 43 '+'
  {0x00,0x50,0x30,0x00,0x00}, // 44 ','
  {0x08,0x08,0x08,0x08,0x08}, // 45 '-'
  {0x00,0x60,0x60,0x00,0x00}, // 46 '.'
  {0x20,0x10,0x08,0x04,0x02}, // 47 '/'
  {0x3E,0x51,0x49,0x45,0x3E}, // 48 '0'
  {0x00,0x42,0x7F,0x40,0x00}, // 49 '1'
  {0x42,0x61,0x51,0x49,0x46}, // 50 '2'
  {0x21,0x41,0x45,0x4B,0x31}, // 51 '3'
  {0x18,0x14,0x12,0x7F,0x10}, // 52 '4'
  {0x27,0x45,0x45,0x45,0x39}, // 53 '5'
  {0x3C,0x4A,0x49,0x49,0x30}, // 54 '6'
  {0x01,0x71,0x09,0x05,0x03}, // 55 '7'
  {0x36,0x49,0x49,0x49,0x36}, // 56 '8'
  {0x06,0x49,0x49,0x29,0x1E}, // 57 '9'
  {0x00,0x36,0x36,0x00,0x00}, // 58 ':'
  {0x00,0x56,0x36,0x00,0x00}, // 59 ';'
  {0x08,0x14,0x22,0x41,0x00}, // 60 '<'
  {0x14,0x14,0x14,0x14,0x14}, // 61 '='
  {0x00,0x41,0x22,0x14,0x08}, // 62 '>'
  {0x02,0x01,0x51,0x09,0x06}, // 63 '?'
  {0x32,0x49,0x79,0x41,0x3E}, // 64 '@'
  {0x7E,0x11,0x11,0x11,0x7E}, // 65 'A'
  {0x7F,0x49,0x49,0x49,0x36}, // 66 'B'
  {0x3E,0x41,0x41,0x41,0x22}, // 67 'C'
  {0x7F,0x41,0x41,0x22,0x1C}, // 68 'D'
  {0x7F,0x49,0x49,0x49,0x41}, // 69 'E'
  {0x7F,0x09,0x09,0x09,0x01}, // 70 'F'
  {0x3E,0x41,0x49,0x49,0x7A}, // 71 'G'
  {0x7F,0x08,0x08,0x08,0x7F}, // 72 'H'
  {0x00,0x41,0x7F,0x41,0x00}, // 73 'I'
  {0x20,0x40,0x41,0x3F,0x01}, // 74 'J'
  {0x7F,0x08,0x14,0x22,0x41}, // 75 'K'
  {0x7F,0x40,0x40,0x40,0x40}, // 76 'L'
  {0x7F,0x02,0x0C,0x02,0x7F}, // 77 'M'
  {0x7F,0x04,0x08,0x10,0x7F}, // 78 'N'
  {0x3E,0x41,0x41,0x41,0x3E}, // 79 'O'
  {0x7F,0x09,0x09,0x09,0x06}, // 80 'P'
  {0x3E,0x41,0x51,0x21,0x5E}, // 81 'Q'
  {0x7F,0x09,0x19,0x29,0x46}, // 82 'R'
  {0x46,0x49,0x49,0x49,0x31}, // 83 'S'
  {0x01,0x01,0x7F,0x01,0x01}, // 84 'T'
  {0x3F,0x40,0x40,0x40,0x3F}, // 85 'U'
  {0x1F,0x20,0x40,0x20,0x1F}, // 86 'V'
  {0x3F,0x40,0x38,0x40,0x3F}, // 87 'W'
  {0x63,0x14,0x08,0x14,0x63}, // 88 'X'
  {0x07,0x08,0x70,0x08,0x07}, // 89 'Y'
  {0x61,0x51,0x49,0x45,0x43}, // 90 'Z'
  {0x00,0x7F,0x41,0x41,0x00}, // 91 '['
  {0x02,0x04,0x08,0x10,0x20}, // 92 '\'
  {0x00,0x41,0x41,0x7F,0x00}, // 93 ']'
  {0x04,0x02,0x01,0x02,0x04}, // 94 '^'
  {0x40,0x40,0x40,0x40,0x40}, // 95 '_'
  {0x00,0x01,0x02,0x04,0x00}, // 96 '`'
  {0x20,0x54,0x54,0x54,0x78}, // 97 'a'
  {0x7F,0x48,0x44,0x44,0x38}, // 98 'b'
  {0x38,0x44,0x44,0x44,0x20}, // 99 'c'
  {0x38,0x44,0x44,0x48,0x7F}, // 100 'd'
  {0x38,0x54,0x54,0x54,0x18}, // 101 'e'
  {0x08,0x7E,0x09,0x01,0x02}, // 102 'f'
  {0x0C,0x52,0x52,0x52,0x3E}, // 103 'g'
  {0x7F,0x08,0x04,0x04,0x78}, // 104 'h'
  {0x00,0x44,0x7D,0x40,0x00}, // 105 'i'
  {0x20,0x40,0x44,0x3D,0x00}, // 106 'j'
  {0x7F,0x10,0x28,0x44,0x00}, // 107 'k'
  {0x00,0x41,0x7F,0x40,0x00}, // 108 'l'
  {0x7C,0x04,0x18,0x04,0x78}, // 109 'm'
  {0x7C,0x08,0x04,0x04,0x78}, // 110 'n'
  {0x38,0x44,0x44,0x44,0x38}, // 111 'o'
  {0x7C,0x14,0x14,0x14,0x08}, // 112 'p'
  {0x08,0x14,0x14,0x18,0x7C}, // 113 'q'
  {0x7C,0x08,0x04,0x04,0x08}, // 114 'r'
  {0x48,0x54,0x54,0x54,0x20}, // 115 's'
  {0x04,0x3F,0x44,0x40,0x20}, // 116 't'
  {0x3C,0x40,0x40,0x20,0x7C}, // 117 'u'
  {0x1C,0x20,0x40,0x20,0x1C}, // 118 'v'
  {0x3C,0x40,0x30,0x40,0x3C}, // 119 'w'
  {0x44,0x28,0x10,0x28,0x44}, // 120 'x'
  {0x0C,0x50,0x50,0x50,0x3C}, // 121 'y'
  {0x44,0x64,0x54,0x4C,0x44}, // 122 'z'
  {0x00,0x08,0x36,0x41,0x00}, // 123 '{'
  {0x00,0x00,0x7F,0x00,0x00}, // 124 '|'
  {0x00,0x41,0x36,0x08,0x00}, // 125 '}'
  {0x10,0x08,0x08,0x10,0x08}, // 126 '~'
  {0x00,0x00,0x00,0x00,0x00}  // 127
};

// ─────────────────── WiFi / LCD / Timing globals ──────────────────────────────
Preferences preferences;
String ssidListHTML = "";

LiquidCrystal_I2C* lcd = NULL;
uint8_t lcdAddr = 0;
bool portalActive = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 400;

unsigned long lastScanCheckTime   = 0;
const unsigned long scanCheckInterval = 7000;

unsigned long lastStatusUpdateTime   = 0;
const unsigned long statusUpdateInterval = 10000;

// Printer config (synced from Supabase)
String printerConnectionMode = "wire";
String printerDeviceAddress  = "";
String printerPaperWidth     = "58mm";
String printerHeader         = "Welcome to our Clinic";
String lastScannedDevicesJson = "[]";

// ─────────────────── Function declarations ────────────────────────────────────
uint8_t scanI2CBus(int &outSda, int &outScl);
void    blinkLED(int count, int delayMs);
void    startCaptivePortal();
void    handleRootPortal();
void    handleEmbeddedDashboard();
void    handleSaveWiFi();
void    checkForUpdates();
void    generateSupabaseToken();
void    checkForScanRequest();
void    checkForTestPrintRequest();
void    scanBluetoothDevices();
void    updateSupabaseSetting(String key, String value);
void    fetchPrinterSettings();
void    printTicket(int tokenNum, String customerName, String serviceType, int waitTime);
String  getLocalDateTime();
// BLE / CAT Fun printer helpers
uint8_t crc8cat(const uint8_t* data, size_t len);
void    catSendCmd(uint8_t cmd, const uint8_t* data, uint16_t dataLen);
bool    catConnect(const String& macAddr);
void    catDisconnect();
void    canvasClear();
void    canvasSetPixel(int x);
void    catSendRow();
void    catPrintText(const char* text, int scale, bool center);
void    catPrintSeparator(int thickness);
void    catPrintBlank(int rows);

// ═════════════════════════════════════════════════════════════════════════════
//  I2C Bus Scanner
// ═════════════════════════════════════════════════════════════════════════════
/**
 * Scans the I2C bus for the LCD display address (commonly 0x27 or 0x3F).
 */
uint8_t scanI2CBus(int &outSda, int &outScl) {
  int sdaPins[] = {21, 21, 21};
  int sclPins[] = {22, 22, 22};
  int numCombinations = 1;

  for (int c = 0; c < numCombinations; c++) {
    int sda = sdaPins[c];
    int scl = sclPins[c];
    Wire.begin(sda, scl);
    Wire.setTimeOut(10);
    Wire.setClock(100000);
    for (uint8_t addr = 0x08; addr < 0x7F; addr++) {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0) {
        Serial.printf("[I2C Scanner] Found device at 0x%02X (SDA=%d, SCL=%d)\n", addr, sda, scl);
        if (addr == 0x27 || addr == 0x3F || addr == 0x20 || addr == 0x38) {
          outSda = sda; outScl = scl;
          return addr;
        }
      }
    }
    Wire.end();
  }
  return 0;
}

// ═════════════════════════════════════════════════════════════════════════════
//  setup()
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[System] Starting ESP32 Master Ticket Dispenser (BLE edition)...");

  // BLE init — must happen before WiFi to allow coexistence stack to start
  BLEDevice::init("Smart-Token-Dispenser");
  BLEDevice::setPower(ESP_PWR_LVL_P3); // Stable BLE TX power

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(CAT_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("[BLE] BLE stack & Advertising initialized. ESP32 Bluetooth visible as 'Smart-Token-Dispenser'.");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 1. Initialize LCD
  int sdaPin = 21, sclPin = 22;
  lcdAddr = scanI2CBus(sdaPin, sclPin);
  if (lcdAddr != 0) {
    Wire.end(); Wire.begin(sdaPin, sclPin);
    lcd = new LiquidCrystal_I2C(lcdAddr, 16, 2);
    lcd->init();
    Wire.begin(sdaPin, sclPin);
    lcd->backlight();
    lcd->setCursor(0,0); lcd->print("ESP32 Starting..");
    Serial.printf("[LCD] Initialized at 0x%02X\n", lcdAddr);
  } else {
    Serial.println("[Error] No I2C LCD found!");
  }

  // 2. Load stored WiFi credentials
  preferences.begin("wifi", false);

  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("[Setup] Button held on boot — clearing WiFi credentials...");
    if (lcd) { lcd->clear(); lcd->print("Resetting WiFi"); lcd->setCursor(0,1); lcd->print("Release Button"); }
    preferences.clear();
    blinkLED(10, 100);
  }

  String storedSSID = "", storedPASS = "";
  if (String(WIFI_SSID) != "" && String(WIFI_SSID) != " YOUR WIFI") {
    storedSSID = WIFI_SSID; storedPASS = WIFI_PASS;
    Serial.println("[Setup] Using hardcoded WiFi credentials.");
    preferences.putString("ssid", storedSSID);
    preferences.putString("pass", storedPASS);
  } else {
    storedSSID = preferences.getString("ssid", "");
    storedPASS = preferences.getString("pass", "");
  }

  if (storedSSID == "") {
    Serial.println("[Setup] No credentials — launching captive portal...");
    startCaptivePortal();
  } else {
    Serial.printf("[Setup] Connecting to: %s\n", storedSSID.c_str());
    if (lcd) { lcd->clear(); lcd->print("Connecting WiFi"); lcd->setCursor(0,1); lcd->print(storedSSID.substring(0,16).c_str()); }

    // Connect directly in AP_STA hybrid mode without turning off the radio
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);
    WiFi.setHostname("vijaybt");
    WiFi.softAP("Smart-Token-Dispenser");
    IPAddress apIP(192,168,4,1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
    dnsServer.start(53, "*", apIP);

    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(storedSSID.c_str(), storedPASS.c_str());
    if (lcd) { lcd->clear(); lcd->print("Connecting WiFi"); lcd->setCursor(0,1); lcd->print(storedSSID.substring(0,16).c_str()); }

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(500); Serial.print(".");
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      if (lcd) { lcd->setCursor(attempts % 16, 1); lcd->print("."); }
      attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      int ipWait = 0;
      while (WiFi.localIP().toString() == "0.0.0.0" && ipWait < 20) { delay(250); Serial.print("+"); ipWait++; }
      Serial.println();

      digitalWrite(LED_PIN, HIGH);
      Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
      if (lcd) { lcd->clear(); lcd->print("WiFi Connected!"); lcd->setCursor(0,1); lcd->print(WiFi.localIP().toString().c_str()); }

      // NTP time sync
      configTime(19800, 0, "pool.ntp.org");
      Serial.println("[Time] NTP configured (UTC+5:30).");

      // mDNS setup for vijaybt.local
      if (MDNS.begin("vijaybt")) {
        Serial.println("[mDNS] Responder started: http://vijaybt.local");
        MDNS.addService("http", "tcp", 80);
      } else {
        Serial.println("[mDNS] Error starting mDNS responder.");
      }

      // BLE is already initialized — no Classic BT radio conflict here
      Serial.println("[BLE] Ready. WiFi+BLE coexisting.");

      // Schedule initial checks
      lastUpdateCheckTime = millis() - updateCheckInterval + 15000;
      lastScanCheckTime   = millis() - scanCheckInterval   + 5000;

      // Concurrent AP for portal
      ssidListHTML = "<option value=\"" + storedSSID + "\">Connected to: " + storedSSID + "</option>";
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP("Smart-Token-Dispenser");
      IPAddress apIP(192,168,4,1);
      WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
      dnsServer.start(53, "*", apIP);

      server.on("/", HTTP_GET, []() {
        if (server.hostHeader() == "192.168.4.1") {
          handleRootPortal();
        } else {
          handleEmbeddedDashboard();
        }
      });
      server.on("/dashboard", HTTP_GET, handleEmbeddedDashboard);
      server.on("/save", HTTP_POST, handleSaveWiFi);

      // Local Web APIs for instant scanning & status
      server.on("/api/printer/scan", HTTP_ANY, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Headers", "*");
        scanBluetoothDevices();
        server.send(200, "application/json", lastScannedDevicesJson);
      });

      server.on("/api/printer/devices", HTTP_GET, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "application/json", "{\"devices\":" + lastScannedDevicesJson + "}");
      });

      server.on("/api/status", HTTP_GET, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        JsonDocument stDoc;
        stDoc["wifi_ssid"]   = WiFi.SSID();
        stDoc["ip_address"]  = WiFi.localIP().toString();
        stDoc["mac_address"] = WiFi.macAddress();
        stDoc["free_heap"]   = ESP.getFreeHeap();
        stDoc["wifi_rssi"]   = WiFi.RSSI();
        stDoc["version"]     = CURRENT_VERSION;
        stDoc["online"]      = true;
        String resStr;
        serializeJson(stDoc, resStr);
        server.send(200, "application/json", resStr);
      });

      server.on("/api/printer/scan", HTTP_GET, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        scanBluetoothDevices();
        server.send(200, "application/json", lastScannedDevicesJson);
      });

      server.onNotFound([]() {
        if (server.method() == HTTP_OPTIONS) {
          server.sendHeader("Access-Control-Allow-Origin", "*");
          server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
          server.sendHeader("Access-Control-Allow-Headers", "*");
          server.send(204);
          return;
        }
        handleEmbeddedDashboard();
      });
      server.begin();
      portalActive = true;
      Serial.println("[Portal] Concurrent AP & Local Web API started.");
      blinkLED(3, 200);
      digitalWrite(LED_PIN, HIGH);

      // Fetch printer settings
      fetchPrinterSettings();

    } else {
      Serial.println("[WiFi] Connection failed — starting captive portal...");
      if (lcd) { lcd->clear(); lcd->print("WiFi Failed!"); lcd->setCursor(0,1); lcd->print("Config mode..."); }
      delay(1500);
      startCaptivePortal();
    }
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  loop()
// ═════════════════════════════════════════════════════════════════════════════
unsigned long lastHeartbeatTime = 0;

void sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) return;
  JsonDocument doc;
  doc["wifi_ssid"]   = WiFi.SSID();
  doc["ip_address"]  = WiFi.localIP().toString();
  doc["mac_address"] = WiFi.macAddress();
  doc["free_heap"]   = ESP.getFreeHeap();
  doc["wifi_rssi"]   = WiFi.RSSI();
  doc["version"]     = CURRENT_VERSION;
  doc["online"]      = true;

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char buf[30];
    strftime(buf, sizeof(buf), "%d/%m/%Y %I:%M:%S %p", &timeinfo);
    doc["last_seen"] = String(buf);
  } else {
    doc["last_seen"] = "Just now";
  }

  String jsonStr;
  serializeJson(doc, jsonStr);
  updateSupabaseSetting("ESP32 Master Status", jsonStr);
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  // Walk-in button debounce → generate token
  if (digitalRead(BUTTON_PIN) == LOW) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      lastDebounceTime = millis();
      Serial.println("[Button] Walk-in button pressed. Generating token...");
      generateSupabaseToken();
    }
  }

  // Periodic: ESP32 Status Heartbeat every 5s
  if (WiFi.status() == WL_CONNECTED && (millis() - lastHeartbeatTime >= 5000)) {
    lastHeartbeatTime = millis();
    sendHeartbeat();
  }

  // Periodic: poll Supabase for scan/print requests every 2.5s
  if (WiFi.status() == WL_CONNECTED && (millis() - lastScanCheckTime >= 2500)) {
    lastScanCheckTime = millis();
    checkForScanRequest();
    checkForTestPrintRequest();
  }

  // Periodic: OTA update check
  if (WiFi.status() == WL_CONNECTED && (millis() - lastUpdateCheckTime >= updateCheckInterval)) {
    lastUpdateCheckTime = millis();
    checkForUpdates();
  }

  // Periodic: LCD IP refresh
  if (WiFi.status() == WL_CONNECTED && (millis() - lastStatusUpdateTime >= statusUpdateInterval)) {
    lastStatusUpdateTime = millis();
    if (lcd && !portalActive) {
      lcd->clear(); lcd->setCursor(0,0); lcd->print("Token Dispenser");
      lcd->setCursor(0,1); lcd->print(WiFi.localIP().toString().c_str());
    }
  }
}

// ─────────────────── LED helper ───────────────────────────────────────────────
void blinkLED(int count, int delayMs) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_PIN, HIGH); delay(delayMs);
    digitalWrite(LED_PIN, LOW);  delay(delayMs);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Captive Portal
// ═════════════════════════════════════════════════════════════════════════════
void startCaptivePortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Smart-Token-Dispenser");
  IPAddress apIP(192,168,4,1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  dnsServer.start(53,"*",apIP);

  // Scan networks for dropdown
  int n = WiFi.scanNetworks();
  ssidListHTML = "";
  for (int i = 0; i < n; i++) {
    ssidListHTML += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + " dBm)</option>";
  }

  server.on("/",     HTTP_GET,  handleRootPortal);
  server.on("/save", HTTP_POST, handleSaveWiFi);
  server.onNotFound([]() { server.sendHeader("Location","http://192.168.4.1/",true); server.send(302,"text/plain",""); });
  server.begin();
  portalActive = true;

  if (lcd) { lcd->clear(); lcd->print("Setup Mode"); lcd->setCursor(0,1); lcd->print("Token-Dispenser"); }
  Serial.println("[Portal] Captive portal started.");

  while (true) { dnsServer.processNextRequest(); server.handleClient(); delay(2); }
}

void handleRootPortal() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Smart Token - WiFi Setup</title>";
  html += "<style>body{font-family:sans-serif;background:#1a1a2e;color:#fff;display:flex;align-items:center;justify-content:center;min-height:100vh;margin:0;}";
  html += ".card{background:#16213e;padding:2rem;border-radius:16px;max-width:400px;width:90%;}";
  html += "h2{color:#7c83fd;text-align:center;}select,input{width:100%;padding:10px;margin:8px 0 16px;border-radius:8px;border:none;font-size:1rem;box-sizing:border-box;}";
  html += "button{width:100%;padding:12px;background:#7c83fd;color:#fff;border:none;border-radius:8px;font-size:1rem;cursor:pointer;}</style></head><body><div class='card'>";
  html += "<h2>&#x1F4F6; WiFi Setup</h2><p style='text-align:center;color:#aaa;font-size:0.9rem;'>Connect the Smart Token Dispenser to your WiFi</p>";
  html += "<form method='POST' action='/save'><label>Network</label><select name='ssid'>" + ssidListHTML + "</select>";
  html += "<label>Password</label><input type='password' name='pass' placeholder='Enter WiFi password'>";
  html += "<button type='submit'>Connect &#x27A1;</button></form></div></body></html>";
  server.send(200, "text/html", html);
}

void handleEmbeddedDashboard() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Smart Token - ESP32 Control Center</title>";
  html += "<style>body{font-family:sans-serif;background:#0f172a;color:#f8fafc;margin:0;padding:20px;}";
  html += ".card{background:#1e293b;border-radius:12px;padding:20px;margin-bottom:20px;border:1px solid #334155;}";
  html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:12px;margin:15px 0;}";
  html += ".stat{background:#0f172a;padding:12px;border-radius:8px;text-align:center;}.val{font-size:1.1rem;font-weight:bold;color:#38bdf8;}.lbl{font-size:0.75rem;color:#94a3b8;margin-top:4px;}";
  html += "button{background:#3b82f6;color:#fff;border:none;padding:12px;border-radius:8px;font-weight:600;cursor:pointer;width:100%;font-size:1rem;}button:hover{background:#2563eb;}";
  html += ".dev{background:#0f172a;border:1px solid #334155;border-radius:8px;padding:12px;margin-top:10px;display:flex;justify-content:space-between;align-items:center;}";
  html += ".dev-name{font-weight:600;}.dev-mac{font-size:0.8rem;color:#94a3b8;font-family:monospace;}.btn-sm{padding:6px 14px;font-size:0.8rem;width:auto;background:#10b981;}</style></head><body>";
  html += "<div style='max-width:650px;margin:0 auto;'><div style='display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid #1e293b;padding-bottom:15px;margin-bottom:20px;'>";
  html += "<h2 style='margin:0;color:#7c83fd;'>📟 ESP32 Control Center</h2><span style='background:#22c55e22;color:#4ade80;padding:4px 12px;border-radius:20px;font-weight:600;font-size:0.85rem;'>vijaybt.local</span></div>";
  html += "<div class='card'><h3 style='margin:0;'>📶 System Status</h3><div class='grid'>";
  html += "<div class='stat'><div class='val'>" + WiFi.localIP().toString() + "</div><div class='lbl'>IP Address</div></div>";
  html += "<div class='stat'><div class='val'>" + WiFi.SSID() + "</div><div class='lbl'>WiFi Network</div></div>";
  html += "<div class='stat'><div class='val'>" + String(WiFi.RSSI()) + " dBm</div><div class='lbl'>Signal</div></div>";
  html += "<div class='stat'><div class='val'>" + String(ESP.getFreeHeap() / 1024) + " KB</div><div class='lbl'>Free RAM</div></div></div></div>";
  html += "<div class='card'><h3 style='margin:0 0 15px;'>🖨️ Bluetooth Thermal Printer</h3>";
  html += "<button onclick='scanBT()'>🔍 Scan Bluetooth Devices</button><div id='bt-list'></div></div></div>";
  html += "<script>async function scanBT(){document.getElementById('bt-list').innerHTML='<div style=\"text-align:center;padding:15px;color:#94a3b8;\">Scanning nearby Bluetooth devices...</div>';try{let r=await fetch('/api/printer/scan');let d=await r.json();let html='';if(!d||d.length===0){html='<div style=\"text-align:center;color:#94a3b8;padding:15px;\">No nearby Bluetooth devices found.</div>';}else{d.forEach(dev=>{html+=`<div class=\"dev\"><div><div class=\"dev-name\">${dev.name||'Bluetooth Device'}</div><div class=\"dev-mac\">${dev.address} (${dev.rssi} dBm)</div></div><button class=\"btn-sm\" onclick=\"alert('Selected: '+ '${dev.address}')\">Connect</button></div>`;});}document.getElementById('bt-list').innerHTML=html;}catch(e){document.getElementById('bt-list').innerHTML='Scan failed';}}</script></body></html>";
  server.send(200, "text/html", html);
}

void handleSaveWiFi() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Credentials Saved</title>";
  html += "<style>body{font-family:sans-serif;background:#1a1a2e;color:#fff;display:flex;align-items:center;justify-content:center;min-height:100vh;margin:0;}";
  html += ".card{background:#16213e;padding:2rem;border-radius:16px;max-width:400px;width:90%;text-align:center;}";
  html += "h2{color:#2ecc71;}.step{background:#0f3460;padding:12px 16px;border-radius:10px;margin:10px 0;text-align:left;}";
  html += ".num{background:#7c83fd;color:#fff;border-radius:50%;width:24px;height:24px;display:inline-flex;align-items:center;justify-content:center;font-weight:700;margin-right:10px;}";
  html += "a.btn{display:block;padding:12px;background:#7c83fd;color:#fff;border-radius:8px;text-decoration:none;margin-top:20px;font-size:1rem;}</style></head><body>";
  html += "<div class='card'><h2>&#x2705; Credentials Saved!</h2><p>ESP32 is rebooting and connecting to <strong>" + ssid + "</strong>.</p>";
  html += "<div class='step'><span class='num'>1</span> Wait 10 seconds for the device to reboot</div>";
  html += "<div class='step'><span class='num'>2</span> Go to WiFi Settings &rarr; disconnect from <strong>Smart-Token-Dispenser</strong></div>";
  html += "<div class='step'><span class='num'>3</span> Reconnect to your home WiFi: <strong>" + ssid + "</strong></div>";
  html += "<div class='step'><span class='num'>4</span> Tap the button below to open the dashboard</div>";
  html += "<a class='btn' href='" + String(DASHBOARD_REDIRECT_URL) + "' target='_blank'>Open Dashboard &rarr;</a></div></body></html>";
  server.send(200, "text/html", html);
  delay(1500);
  ESP.restart();
}

// ═════════════════════════════════════════════════════════════════════════════
//  OTA Update
// ═════════════════════════════════════════════════════════════════════════════
void checkForUpdates() {
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  http.begin(client, VERSION_URL);
  int code = http.GET();
  if (code == 200) {
    int latestVer = http.getString().toInt();
    if (latestVer > CURRENT_VERSION) {
      Serial.printf("[OTA] New version %d found. Updating...\n", latestVer);
      if (lcd) { lcd->clear(); lcd->print("OTA Updating..."); }
      t_httpUpdate_return ret = httpUpdate.update(client, FIRMWARE_URL);
      if (ret == HTTP_UPDATE_OK) Serial.println("[OTA] Update successful. Rebooting.");
      else Serial.printf("[OTA] Failed. Error: %d\n", httpUpdate.getLastError());
    }
  }
  http.end();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Supabase polling: Scan request
// ═════════════════════════════════════════════════════════════════════════════
void checkForScanRequest() {
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Scan%20Request";
  http.begin(client, url);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc; DeserializationError err = deserializeJson(doc, payload);
    if (!err && doc.size() > 0) {
      String val = doc[0]["value"];
      if (val == "true") { scanBluetoothDevices(); }
    }
  }
  http.end();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Supabase polling: Test print request
// ═════════════════════════════════════════════════════════════════════════════
void checkForTestPrintRequest() {
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Test%20Print%20Request";
  http.begin(client, url);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc; DeserializationError err = deserializeJson(doc, payload);
    if (!err && doc.size() > 0) {
      String val = doc[0]["value"].as<String>();
      if (val != "" && val != "null" && val != "undefined") {
        Serial.printf("[Printer] Test print request: %s\n", val.c_str());
        printTicket(999, "Test Client", "Msg: " + val, 0);
        updateSupabaseSetting("Test Print Request", "");
      }
    }
  }
  http.end();
}

// ═════════════════════════════════════════════════════════════════════════════
//  BLE Device Scanner (replaces Classic BT scan)
// ═════════════════════════════════════════════════════════════════════════════
void scanBluetoothDevices() {
  Serial.println("[BLE] Starting BLE device discovery...");
  updateSupabaseSetting("Scan Request", "scanning");

  BLEScan* pScan = BLEDevice::getScan();
  pScan->setActiveScan(true);
  pScan->setInterval(160); // 100ms interval
  pScan->setWindow(80);    // 50ms window (50% duty cycle for WiFi radio coexistence)

  // Non-blocking/5-second scan
  BLEScanResults* results = pScan->start(5, false);
  int count = results ? results->getCount() : 0;
  Serial.printf("[BLE] Scan complete. Discovered %d real devices.\n", count);

  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice dev = results->getDevice(i);
    JsonObject obj = arr.add<JsonObject>();
    String devName = "";
    if (dev.haveName() && dev.getName().length() > 0) {
      devName = String(dev.getName().c_str());
    } else {
      devName = "Bluetooth Device";
    }
    obj["name"]    = devName;
    obj["address"] = dev.getAddress().toString().c_str();
    obj["rssi"]    = dev.getRSSI();
  }
  pScan->clearResults();
  BLEDevice::startAdvertising();

  serializeJson(doc, lastScannedDevicesJson);
  Serial.printf("[BLE] Scanned JSON: %s\n", lastScannedDevicesJson.c_str());
  updateSupabaseSetting("Scanned Bluetooth Printers", lastScannedDevicesJson);
  updateSupabaseSetting("Scan Request", "false");
}

// ═════════════════════════════════════════════════════════════════════════════
//  Supabase helpers
// ═════════════════════════════════════════════════════════════════════════════
void updateSupabaseSetting(String key, String value) {
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = String(SUPABASE_URL) + "/rest/v1/settings?key=eq." + key;
  url.replace(" ", "%20");
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  JsonDocument patchDoc; patchDoc["value"] = value;
  String body; serializeJson(patchDoc, body);
  int code = http.PATCH(body);
  if (code != 204 && code != 200)
    Serial.printf("[Supabase PATCH Error] key=%s code=%d\n", key.c_str(), code);
  http.end();
}

void fetchPrinterSettings() {
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Thermal%20Printer%20Settings";
  http.begin(client, url);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    JsonDocument doc; DeserializationError err = deserializeJson(doc, payload);
    if (!err && doc.size() > 0) {
      String val = doc[0]["value"];
      JsonDocument pd; DeserializationError pe = deserializeJson(pd, val);
      if (!pe) {
        printerConnectionMode = pd["connection"].as<String>();
        printerDeviceAddress  = pd["device"].as<String>();
        printerPaperWidth     = pd["paper"].as<String>();
        printerHeader         = pd["header"].as<String>();
        if (printerConnectionMode == "null") printerConnectionMode = "wire";
        if (printerDeviceAddress  == "null") printerDeviceAddress  = "";
        if (printerPaperWidth     == "null") printerPaperWidth     = "58mm";
        if (printerHeader         == "null") printerHeader         = "Welcome";
        Serial.printf("[Printer] Loaded: mode=%s addr=%s\n",
          printerConnectionMode.c_str(), printerDeviceAddress.c_str());
      }
    }
  }
  http.end();
}

// ═════════════════════════════════════════════════════════════════════════════
//  CAT Fun Printer BLE Protocol
// ═════════════════════════════════════════════════════════════════════════════

/**
 * CRC-8/SMBUS checksum used by the CAT Fun printer packet protocol.
 */
uint8_t crc8cat(const uint8_t* data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
    }
  }
  return crc;
}

/**
 * Build and send one CAT Fun packet:
 *   [0x51][0x78][CMD][0x00][LEN_L][LEN_H][...data...][CRC8][0xFF]
 *
 * dataLen must be <= 48 (one bitmap row). Uses a static buffer — not re-entrant.
 */
void catSendCmd(uint8_t cmd, const uint8_t* data, uint16_t dataLen) {
  if (!catWriteChar) { Serial.println("[CAT] No write characteristic!"); return; }
  if (dataLen > 48) dataLen = 48;

  static uint8_t pkt[56]; // 48 data + 8 header/footer
  pkt[0] = 0x51; pkt[1] = 0x78;
  pkt[2] = cmd;  pkt[3] = 0x00;
  pkt[4] = (uint8_t)(dataLen & 0xFF);
  pkt[5] = (uint8_t)((dataLen >> 8) & 0xFF);
  if (data && dataLen > 0) memcpy(pkt + 6, data, dataLen);
  pkt[6 + dataLen] = (data && dataLen > 0) ? crc8cat(data, dataLen) : 0;
  pkt[7 + dataLen] = 0xFF;

  catWriteChar->writeValue(pkt, dataLen + 8, false);
  delay(20); // Rate-limit; CAT printers overflow easily
}

/**
 * Connect to the CAT Fun printer over BLE GATT.
 * Tries the primary service UUID first; falls back to the alternate UUID.
 */
bool catConnect(const String& macAddr) {
  Serial.printf("[CAT] Connecting to %s...\n", macAddr.c_str());

  if (catClient) {
    if (catClient->isConnected()) catClient->disconnect();
    delete catClient; catClient = nullptr; catWriteChar = nullptr;
  }

  catClient = BLEDevice::createClient();
  BLEAddress addr(macAddr.c_str());

  if (!catClient->connect(addr)) {
    Serial.println("[CAT] BLE connect failed.");
    delete catClient; catClient = nullptr;
    return false;
  }
  Serial.println("[CAT] BLE GATT connected. Discovering services...");

  BLERemoteService* svc = catClient->getService(CAT_SERVICE_UUID);
  if (!svc) svc = catClient->getService(BLEUUID("0000ff00-0000-1000-8000-00805f9b34fb"));
  if (!svc) svc = catClient->getService(BLEUUID("49535343-fe7d-4ae5-8fa9-9fafd205e455"));
  if (!svc) svc = catClient->getService(BLEUUID("0000af00-0000-1000-8000-00805f9b34fb"));

  if (svc) {
    catWriteChar = svc->getCharacteristic(CAT_WRITE_UUID);
    if (!catWriteChar) catWriteChar = svc->getCharacteristic(BLEUUID("0000ff02-0000-1000-8000-00805f9b34fb"));
    if (!catWriteChar) catWriteChar = svc->getCharacteristic(BLEUUID("0000ff01-0000-1000-8000-00805f9b34fb"));
    if (!catWriteChar) catWriteChar = svc->getCharacteristic(BLEUUID("49535343-8841-43f4-a8d4-ecbe34729bb3"));
  }

  // Smart Auto-Discovery: If specific UUIDs missed, search all services for any writable characteristic
  if (!catWriteChar) {
    Serial.println("[CAT] Probing services for writable GATT characteristic...");
    auto services = catClient->getServices();
    if (services) {
      for (auto& s : *services) {
        auto chars = s.second->getCharacteristics();
        if (chars) {
          for (auto& c : *chars) {
            if (c.second->canWrite() || c.second->canWriteNoResponse()) {
              catWriteChar = c.second;
              Serial.printf("[CAT] Auto-discovered write characteristic: %s\n", c.second->getUUID().toString().c_str());
              break;
            }
          }
        }
        if (catWriteChar) break;
      }
    }
  }

  if (!catWriteChar) {
    Serial.println("[CAT] Write characteristic not found!");
    catClient->disconnect(); delete catClient; catClient = nullptr;
    return false;
  }

  Serial.println("[CAT] Printer connected and ready to print.");
  return true;
}

void catDisconnect() {
  if (catClient && catClient->isConnected()) catClient->disconnect();
  catWriteChar = nullptr;
}

// ─────────────────── Bitmap canvas helpers ────────────────────────────────────
void canvasClear()        { memset(canvas, 0, 48); }
void catSendRow()         { catSendCmd(0xA9, canvas, 48); }

/** Set pixel x (0–383) in the current canvas row */
void canvasSetPixel(int x) {
  if (x < 0 || x >= 384) return;
  canvas[x >> 3] |= (0x80 >> (x & 7));
}

// ─────────────────── Text renderer ───────────────────────────────────────────

/**
 * Render one text line onto the printer at the given scale.
 * Each character is 5 px wide + 1 px gap = 6 px (×scale).
 * The line occupies (8×scale) pixel rows.
 *
 * @param text   Null-terminated ASCII string
 * @param scale  Pixel magnification (1=tiny, 2=normal, 3=large, 5=huge token number)
 * @param center If true, centre the text on the 384-px paper
 */
void catPrintText(const char* text, int scale, bool center) {
  int len = strlen(text);
  int lineW = len * 6 * scale;
  int startX = center ? max(0, (384 - lineW) / 2) : 4;
  int totalRows = 8 * scale;

  for (int pr = 0; pr < totalRows; pr++) {
    canvasClear();
    int charRow = pr / scale; // which pixel row within character (0–7)
    int x = startX;
    for (int i = 0; i < len; i++) {
      uint8_t c = (uint8_t)text[i];
      if (c < 32 || c > 127) c = 63; // '?'
      for (int col = 0; col < 5; col++) {
        uint8_t colData = pgm_read_byte(&font5x8[c - 32][col]);
        bool on = (colData >> charRow) & 1;
        if (on) {
          for (int sx = 0; sx < scale; sx++) {
            canvasSetPixel(x + col * scale + sx);
          }
        }
      }
      x += 6 * scale; // advance to next character
    }
    catSendRow();
    delay(4); // inter-row pacing
  }
}

/** Print a solid horizontal line (thickness = number of pixel rows) */
void catPrintSeparator(int thickness) {
  for (int i = 0; i < thickness; i++) {
    memset(canvas, 0xFF, 48); // all pixels ON
    catSendRow();
    delay(4);
  }
}

/** Print blank (white) pixel rows */
void catPrintBlank(int rows) {
  canvasClear();
  for (int i = 0; i < rows; i++) { catSendRow(); delay(4); }
}

// ═════════════════════════════════════════════════════════════════════════════
//  printTicket  — renders and sends the full ticket
// ═════════════════════════════════════════════════════════════════════════════
void printTicket(int tokenNum, String customerName, String serviceType, int waitTime) {
  if (lcd) { lcd->clear(); lcd->print("Printing Ticket"); lcd->setCursor(0,1); lcd->print("Token: " + String(tokenNum)); }

  fetchPrinterSettings();

  if (printerConnectionMode == "bluetooth") {
    // ── CAT Fun BLE path ──────────────────────────────────────────────────
    if (printerDeviceAddress == "") {
      Serial.println("[CAT] No printer MAC configured. Set it in the dashboard.");
      if (lcd) { lcd->clear(); lcd->print("No Printer MAC!"); }
      return;
    }

    if (!catConnect(printerDeviceAddress)) {
      Serial.println("[CAT] Failed to connect to printer. Is it powered ON?");
      if (lcd) { lcd->clear(); lcd->print("BT Connect"); lcd->setCursor(0,1); lcd->print("Failed!"); }
      return;
    }

    // 1. Set energy/darkness: cmd 0xAF, data [0x35] (medium-dark, good default)
    uint8_t energyData[] = {0x35};
    catSendCmd(0xAF, energyData, 1);
    delay(80);

    // 2. Print-start command: cmd 0xA6, no data
    catSendCmd(0xA6, nullptr, 0);
    delay(80);

    // 3. Ticket content (bitmap rows)
    catPrintBlank(4);
    catPrintSeparator(3);
    catPrintBlank(3);
    catPrintText(printerHeader.c_str(), 2, true);       // org name, scale 2, centered
    catPrintBlank(2);
    catPrintText("WALK-IN TICKET", 1, true);
    catPrintBlank(2);
    catPrintSeparator(2);
    catPrintBlank(4);
    catPrintText("Your Token Number:", 1, true);
    catPrintBlank(3);
    catPrintText(String(tokenNum).c_str(), 5, true);    // huge digit, scale 5, centered
    catPrintBlank(3);
    catPrintText(serviceType.c_str(), 2, true);
    catPrintBlank(2);
    catPrintSeparator(2);
    catPrintBlank(2);
    catPrintText(("Customer: " + customerName).c_str(),           1, false);
    catPrintText(("Est. Wait: " + String(waitTime) + " mins").c_str(), 1, false);
    catPrintText(("Printed:  " + getLocalDateTime()).c_str(),     1, false);
    catPrintBlank(2);
    catPrintSeparator(2);
    catPrintBlank(20); // paper feed

    catDisconnect();
    Serial.println("[CAT] Ticket printed successfully via BLE.");

  } else {
    // ── Wired ESC/POS path (Serial2, PIN 17 = TX) ─────────────────────────
    String printData = "";
    printData += "\x1B\x40";                              // ESC @ initialize
    printData += "\x1B\x61\x01";                          // ESC a 1 center align
    printData += "\x1D\x21\x00\x1B\x45\x01";             // normal size + bold ON
    printData += printerHeader + "\n";
    printData += "WALK-IN TICKET\n";
    printData += "\x1B\x45\x00";                          // bold OFF
    printData += "--------------------------------\n";
    printData += "Your Token Number is:\n\n";
    printData += "\x1D\x21\x11";                          // double w+h
    printData += " " + String(tokenNum) + " \n\n";
    printData += "\x1D\x21\x00\x1B\x45\x01";
    printData += serviceType + "\n\x1B\x45\x00";
    printData += "--------------------------------\n";
    printData += "Customer: " + customerName + "\n";
    printData += "Est. Wait: " + String(waitTime) + " mins\n";
    printData += "Printed:  " + getLocalDateTime() + "\n";
    printData += "--------------------------------\n\n\n\n\n";
    printData += "\x1D\x56\x42\x00";                     // partial cut

    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    Serial.println("[Printer] Printing over wired Serial2...");
    Serial2.print(printData);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Date/Time helper
// ═════════════════════════════════════════════════════════════════════════════
String getLocalDateTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char buf[30];
    strftime(buf, sizeof(buf), "%d/%m/%Y %I:%M %p", &timeinfo);
    return String(buf);
  }
  return "--/--/---- --:--";
}

// ═════════════════════════════════════════════════════════════════════════════
//  generateSupabaseToken  — main walk-in token flow
// ═════════════════════════════════════════════════════════════════════════════
void generateSupabaseToken() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Error] WiFi disconnected. Skipping token generation.");
    if (lcd) { lcd->clear(); lcd->print("WiFi Offline!"); lcd->setCursor(0,1); lcd->print("Check Connection"); delay(2000); }
    blinkLED(3, 100); return;
  }

  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;

  int lastTokenNumber    = 0;
  int startingTokenNumber= 1;
  int currentServingToken= 0;
  int avgServiceTime     = 10;

  // 1. Last Generated Token
  http.begin(client, String(SUPABASE_URL)+"/rest/v1/settings?key=eq.Last%20Generated%20Token");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ")+SUPABASE_KEY);
  if (http.GET() == 200) {
    JsonDocument d; deserializeJson(d, http.getString());
    if (d.size()>0) lastTokenNumber = String(d[0]["value"].as<String>()).toInt();
  }
  http.end();

  // 2. Starting Token Number
  http.begin(client, String(SUPABASE_URL)+"/rest/v1/settings?key=eq.Starting%20Token%20Number");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ")+SUPABASE_KEY);
  if (http.GET() == 200) {
    JsonDocument d; deserializeJson(d, http.getString());
    if (d.size()>0) startingTokenNumber = String(d[0]["value"].as<String>()).toInt();
  }
  http.end();

  // 3. Current Serving Token
  http.begin(client, String(SUPABASE_URL)+"/rest/v1/settings?key=eq.Current%20Serving%20Token");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ")+SUPABASE_KEY);
  if (http.GET() == 200) {
    JsonDocument d; deserializeJson(d, http.getString());
    if (d.size()>0) currentServingToken = String(d[0]["value"].as<String>()).toInt();
  }
  http.end();

  // 4. Average Service Time
  http.begin(client, String(SUPABASE_URL)+"/rest/v1/settings?key=eq.Average%20Service%20Time");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ")+SUPABASE_KEY);
  if (http.GET() == 200) {
    JsonDocument d; deserializeJson(d, http.getString());
    if (d.size()>0) avgServiceTime = String(d[0]["value"].as<String>()).toInt();
  }
  http.end();

  int newTokenNumber = lastTokenNumber + 1;
  if (newTokenNumber < startingTokenNumber) newTokenNumber = startingTokenNumber;
  Serial.printf("[Supabase] New token: %d\n", newTokenNumber);

  // 5. Insert token record
  http.begin(client, String(SUPABASE_URL)+"/rest/v1/tokens");
  http.addHeader("Content-Type","application/json");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ")+SUPABASE_KEY);
  JsonDocument postDoc;
  postDoc["token_number"]  = newTokenNumber;
  postDoc["customer_name"] = "Walk-In";
  postDoc["phone_number"]  = "-";
  postDoc["email"]         = "-";
  postDoc["service_type"]  = "General Service";
  postDoc["source"]        = "Manual";
  postDoc["status"]        = "Waiting";
  postDoc["remarks"]       = "Generated via ESP32 Hardware Button";
  String requestBody; serializeJson(postDoc, requestBody);
  int insertCode = http.POST(requestBody);
  bool insertSuccess = (insertCode == 201 || insertCode == 200);
  if (!insertSuccess) {
    Serial.printf("[Supabase POST Error] code=%d\n", insertCode);
    if (lcd) { lcd->clear(); lcd->print("Insert Failed!"); lcd->setCursor(0,1); lcd->print("Error: "+String(insertCode)); delay(2000); }
  }
  http.end();

  if (insertSuccess) {
    // 6. Update Last Generated Token setting
    http.begin(client, String(SUPABASE_URL)+"/rest/v1/settings?key=eq.Last%20Generated%20Token");
    http.addHeader("Content-Type","application/json");
    http.addHeader("apikey", SUPABASE_KEY);
    http.addHeader("Authorization", String("Bearer ")+SUPABASE_KEY);
    JsonDocument pd; pd["value"] = String(newTokenNumber);
    String pb; serializeJson(pd, pb);
    int patchCode = http.PATCH(pb);
    http.end();

    if (patchCode == 204 || patchCode == 200) {
      int waitCount = newTokenNumber - currentServingToken - 1;
      if (waitCount < 0) waitCount = 0;
      int estimatedWait = waitCount * avgServiceTime;

      if (lcd) { lcd->clear(); lcd->print("Token Dispensed!"); lcd->setCursor(0,1); lcd->print("Number: "+String(newTokenNumber)); }

      printTicket(newTokenNumber, "Walk-In", "General Service", estimatedWait);
      blinkLED(2, 200);
      delay(3000);
    } else {
      Serial.printf("[Supabase PATCH Error] code=%d\n", patchCode);
      if (lcd) { lcd->clear(); lcd->print("DB Update Failed"); lcd->setCursor(0,1); lcd->print("Error: "+String(patchCode)); delay(2000); }
      blinkLED(3, 100);
    }
  } else {
    blinkLED(3, 100);
  }
}
