/**
 * Smart Token Management System - Master ESP32 Ticket Dispenser
 * 
 * Hardware Description:
 * - ESP32 Development Board
 * - Walk-in Push-Button connected between GPIO 4 and GND
 * - LED Indicator connected to GPIO 2 (onboard LED)
 * - Secondary Serial Port (Serial2 on Pins RX2=16, TX2=17) for Wired Thermal Printer
 * 
 * Features:
 * - Captive Portal WiFi configuration with scan list & premium dark theme UI
 * - Redirection to the GitHub Pages Dashboard on success
 * - Wireless OTA updates via GitHub HTTP Auto-Updates over the internet
 * - Direct HTTPS integration with Supabase database
 * - Bluetooth Classic SPP & Wired ESC/POS Thermal Printer support
 * - Time Synchronization via NTP (UTC+5:30) for ticket printing
 */

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include <HTTPUpdate.h>
#include <time.h>

// Version and GitHub OTA configuration
const int CURRENT_VERSION = 1;
const char* VERSION_URL = "https://raw.githubusercontent.com/MONASKUMAR/smart-token-management-system/master/bin/master_version.txt";
const char* FIRMWARE_URL = "https://raw.githubusercontent.com/MONASKUMAR/smart-token-management-system/master/bin/esp32_master.bin";

unsigned long lastUpdateCheckTime = 0;
const unsigned long updateCheckInterval = 300000; // Check for firmware updates every 5 minutes

// Supabase Configuration
const char* SUPABASE_URL = "https://swqgfhtyfudkwvyuulzz.supabase.co";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InN3cWdmaHR5ZnVka3d2eXV1bHp6Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODE2MDE4ODIsImV4cCI6MjA5NzE3Nzg4Mn0.qbjAR4I8NfCFusutfws4I4oZJsbCx4TGeaYtfSyA1fc";
const char* DASHBOARD_REDIRECT_URL = "https://monaskumar.github.io/smart-token-management-system/";

// Hardware Pins
const int BUTTON_PIN = 4; // Push button pin (active LOW)
const int LED_PIN = 2;    // Status LED pin

// Web and DNS Servers
WebServer server(80);
DNSServer dnsServer;

// Bluetooth Serial for printer connection
BluetoothSerial SerialBT;

// WiFi settings storage
Preferences preferences;
String ssidListHTML = "";

// State variables
bool portalActive = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 400; // Debounce delay in ms

unsigned long lastScanCheckTime = 0;
const unsigned long scanCheckInterval = 7000; // Poll settings for scan requests every 7 seconds

// Global printer configurations (synchronized with Supabase)
String printerConnectionMode = "wire";
String printerDeviceAddress = "";
String printerPaperWidth = "58mm";
String printerHeader = "Welcome to our Clinic";

// Function Declarations
void blinkLED(int count, int delayMs);
void startCaptivePortal();
void handleRootPortal();
void handleSaveWiFi();
void checkForUpdates();
void generateSupabaseToken();
void checkForScanRequest();
void scanBluetoothDevices();
void updateSupabaseSetting(String key, String value);
void fetchPrinterSettings();
void printTicket(int tokenNum, String customerName, String serviceType, int waitTime);
String getLocalDateTime();

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize Preferences
  preferences.begin("wifi", false);

  // Check if button is held down on boot to force configuration portal
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("\n[Setup] Button held on boot! Clearing WiFi credentials...");
    preferences.clear();
    blinkLED(10, 100); // Fast blink to acknowledge
  }

  String storedSSID = preferences.getString("ssid", "");
  String storedPASS = preferences.getString("pass", "");

  if (storedSSID == "") {
    Serial.println("\n[Setup] No WiFi credentials stored. Launching setup portal...");
    startCaptivePortal();
  } else {
    Serial.print("\n[Setup] Stored WiFi SSID found: ");
    Serial.println(storedSSID);
    
    WiFi.begin(storedSSID.c_str(), storedPASS.c_str());
    
    Serial.print("[Setup] Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) { // 15 seconds timeout
      delay(500);
      Serial.print(".");
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED while connecting
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(LED_PIN, HIGH); // Solid LED on connection success
      Serial.println("\n[WiFi] Connected successfully!");
      Serial.print("[WiFi] IP Address: ");
      Serial.println(WiFi.localIP());
      
      // Start NTP Time Synchronization (India standard UTC+5:30)
      configTime(19800, 0, "pool.ntp.org");
      Serial.println("[Time] NTP Time sync configured.");

      // Initialize Bluetooth for printer connection & search
      SerialBT.begin("Smart-Token-Dispenser");
      Serial.println("[Bluetooth] Bluetooth Classic SPP initialized.");

      // Fetch initial printer configurations
      fetchPrinterSettings();

      // Check for GitHub updates right on boot
      checkForUpdates();
      
      // Success flash sequence
      blinkLED(3, 200);
      digitalWrite(LED_PIN, HIGH); // Solid LED back on
    } else {
      Serial.println("\n[WiFi] Connection failed. Falling back to setup portal...");
      digitalWrite(LED_PIN, LOW);
      startCaptivePortal();
    }
  }
}

void loop() {
  if (portalActive) {
    dnsServer.processNextRequest();
    server.handleClient();
  } else {
    // Check for GitHub updates periodically
    if (WiFi.status() == WL_CONNECTED) {
      if (millis() - lastUpdateCheckTime > updateCheckInterval) {
        lastUpdateCheckTime = millis();
        checkForUpdates();
      }
    }
    
    // Poll Supabase settings periodically to check if dashboard requested a Bluetooth Scan
    if (WiFi.status() == WL_CONNECTED) {
      if (millis() - lastScanCheckTime > scanCheckInterval) {
        lastScanCheckTime = millis();
        checkForScanRequest();
      }
    }

    // Read button state (active LOW)
    int buttonState = digitalRead(BUTTON_PIN);
    
    if (buttonState == LOW) {
      if ((millis() - lastDebounceTime) > debounceDelay) {
        lastDebounceTime = millis();
        Serial.println("\n[Master] Button pressed. Disbursing new manual ticket...");
        
        digitalWrite(LED_PIN, LOW); // Turn off LED during API call to show activity
        generateSupabaseToken();
        digitalWrite(LED_PIN, HIGH); // Turn LED back on when complete
      }
    }
  }
}

/**
 * Perform a clean LED blink sequence
 */
void blinkLED(int count, int delayMs) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}

/**
 * Configure and Start Captive Setup Portal
 */
void startCaptivePortal() {
  portalActive = true;
  
  // Abort any active connections to release the interface for scanning
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_AP_STA);
  delay(300); // Allow mode configuration to settle
  
  // Scan for local WiFi networks (with retry fallback)
  Serial.println("[Portal] Scanning networks...");
  int n = -1;
  int retry = 0;
  while (n < 0 && retry < 3) {
    n = WiFi.scanNetworks();
    if (n < 0) {
      Serial.printf("[Portal] Scan failed, retrying in 500ms... (%d/3)\n", retry + 1);
      delay(500);
    }
    retry++;
  }
  
  Serial.print("[Portal] Networks found: ");
  Serial.println(n);
  
  ssidListHTML = "";
  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      String encryptionType = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      ssidListHTML += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + " (" + encryptionType + ", Sig: " + String(WiFi.RSSI(i)) + "dBm)</option>";
    }
  }
  
  // Host an open Access Point
  WiFi.softAP("Smart-Token-Dispenser");
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  Serial.print("[Portal] Access Point Started. SSID: Smart-Token-Dispenser, IP: ");
  Serial.println(WiFi.softAPIP());
  
  // Route DNS requests to local AP IP
  dnsServer.start(53, "*", apIP);
  
  // Web Server Routes
  server.on("/", HTTP_GET, handleRootPortal);
  server.on("/save", HTTP_POST, handleSaveWiFi);
  server.onNotFound([]() {
    // Captive Portal redirect
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  });
  
  server.begin();
  Serial.println("[Portal] Web Server listening on port 80.");
}

/**
 * Serve Portal Configuration Page
 */
void handleRootPortal() {
  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<title>Dispenser WiFi Setup</title><style>";
  html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: linear-gradient(135deg, #1e1e2f 0%, #11111d 100%); color: #e0e0e0; margin: 0; padding: 20px; display: flex; justify-content: center; align-items: center; min-height: 100vh; }";
  html += ".card { background: rgba(255, 255, 255, 0.05); backdrop-filter: blur(10px); -webkit-backdrop-filter: blur(10px); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 16px; padding: 30px; width: 100%; max-width: 400px; box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37); text-align: center; }";
  html += "h2 { margin-top: 0; font-weight: 600; color: #ffffff; letter-spacing: 0.5px; }";
  html += "p { color: #a0a0ab; font-size: 0.9rem; margin-bottom: 25px; }";
  html += ".form-group { margin-bottom: 20px; text-align: left; }";
  html += "label { display: block; font-size: 0.8rem; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 8px; color: #0d6efd; font-weight: 600; }";
  html += "select, input[type='password'], input[type='text'] { width: 100%; padding: 12px; background: rgba(0, 0, 0, 0.2); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; color: #fff; font-size: 0.95rem; box-sizing: border-box; outline: none; }";
  html += "select option { background: #11111d; color: #fff; }";
  html += ".btn { width: 100%; padding: 14px; background: linear-gradient(90deg, #0d6efd 0%, #0b5ed7 100%); border: none; border-radius: 8px; color: white; font-weight: 600; font-size: 1rem; cursor: pointer; box-shadow: 0 4px 15px rgba(13, 110, 253, 0.3); }";
  html += ".btn:active { transform: scale(0.98); }";
  html += ".footer { margin-top: 25px; font-size: 0.75rem; color: #6c757d; }";
  html += "</style>";
  html += "<script>";
  html += "function toggleManualSSID() {";
  html += "  var select = document.getElementById('ssid');";
  html += "  var manualGroup = document.getElementById('manual_ssid_group');";
  html += "  var manualInput = document.getElementById('manual_ssid');";
  html += "  if (select.value === '__manual__') {";
  html += "    manualGroup.style.display = 'block';";
  html += "    manualInput.required = true;";
  html += "  } else {";
  html += "    manualGroup.style.display = 'none';";
  html += "    manualInput.required = false;";
  html += "  }";
  html += "}";
  html += "window.onload = function() { toggleManualSSID(); };";
  html += "</script>";
  html += "</head><body><div class='card'>";
  html += "<h2>WiFi Configuration</h2>";
  html += "<p>Connect your Token Dispenser to your local WiFi network.</p>";
  html += "<form method='POST' action='/save'>";
  html += "<div class='form-group'>";
  html += "<label for='ssid'>Select WiFi Network</label>";
  html += "<select id='ssid' name='ssid' onchange='toggleManualSSID()'>";
  html += ssidListHTML;
  html += "<option value='__manual__'>-- Enter SSID Manually --</option>";
  html += "</select></div>";
  html += "<div class='form-group' id='manual_ssid_group' style='display: none;'>";
  html += "<label for='manual_ssid'>WiFi SSID (Network Name)</label>";
  html += "<input type='text' id='manual_ssid' name='manual_ssid' placeholder='Enter WiFi Name'>";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label for='password'>Password</label>";
  html += "<input type='password' id='password' name='password' placeholder='Enter WiFi Password'>";
  html += "</div>";
  html += "<button type='submit' class='btn'>Connect Dispenser</button>";
  html += "</form>";
  html += "<div class='footer'>Smart Token Management System</div>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

/**
 * Handle Save WiFi settings request
 */
void handleSaveWiFi() {
  String ssid = server.arg("ssid");
  String pass = server.arg("password");
  
  if (ssid == "__manual__") {
    ssid = server.arg("manual_ssid");
  }
  
  if (ssid != "") {
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);
    
    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><meta http-equiv='refresh' content='5;url=" + String(DASHBOARD_REDIRECT_URL) + "'>";
    html += "<title>Config Saved</title><style>";
    html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: linear-gradient(135deg, #1e1e2f 0%, #11111d 100%); color: #e0e0e0; margin: 0; padding: 20px; display: flex; justify-content: center; align-items: center; min-height: 100vh; text-align: center; }";
    html += ".card { background: rgba(255, 255, 255, 0.05); backdrop-filter: blur(10px); -webkit-backdrop-filter: blur(10px); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 16px; padding: 40px 30px; width: 100%; max-width: 400px; box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37); }";
    html += "h2 { margin-top: 0; color: #2ec4b6; }";
    html += "p { color: #a0a0ab; font-size: 0.95rem; line-height: 1.6; }";
    html += ".spinner { border: 4px solid rgba(255,255,255,0.1); width: 40px; height: 40px; border-radius: 50%; border-left-color: #2ec4b6; animation: spin 1s linear infinite; margin: 20px auto; }";
    html += "@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }";
    html += "</style></head><body><div class='card'>";
    html += "<h2>Configuration Saved!</h2>";
    html += "<p>The dispenser is now connecting to your WiFi network. You will be redirected to the dashboard in 5 seconds...</p>";
    html += "<div class='spinner'></div>";
    html += "<p style='font-size: 0.8rem; color: #6c757d;'>If you are not redirected automatically, click <a href='" + String(DASHBOARD_REDIRECT_URL) + "' style='color: #2ec4b6;'>here</a>.</p>";
    html += "</div></body></html>";
    
    server.send(200, "text/html", html);
    delay(1000);
    
    Serial.println("[Portal] Connection credentials saved. Rebooting...");
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Error: SSID must not be empty.");
  }
}

/**
 * Connect to GitHub over the internet to check for updates.
 * Performs a download and update if a newer version is pushed to GitHub.
 */
void checkForUpdates() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  Serial.println("[Update] Checking for firmware updates on GitHub...");
  
  WiFiClientSecure client;
  client.setInsecure(); // GitHub raw content requires HTTPS, we bypass verification for robustness
  HTTPClient http;
  
  http.begin(client, VERSION_URL);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    payload.trim();
    int latestVersion = payload.toInt();
    Serial.printf("[Update] Local version: %d, Latest remote version: %d\n", CURRENT_VERSION, latestVersion);
    
    if (latestVersion > CURRENT_VERSION) {
      Serial.println("[Update] New version discovered on GitHub! Triggering HTTP update...");
      
      // Fast blink feedback to indicate update process has begun
      blinkLED(5, 80);
      
      // Perform the firmware update
      t_httpUpdate_return ret = httpUpdate.update(client, FIRMWARE_URL);
      
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("[Update Error] Failed (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
          break;
        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("[Update Error] No updates found during fetch.");
          break;
        case HTTP_UPDATE_OK:
          Serial.println("[Update Success] Firmware written! Rebooting device...");
          delay(1000);
          ESP.restart();
          break;
      }
    } else {
      Serial.println("[Update] Firmware is already at the latest version.");
    }
  } else {
    Serial.printf("[Update] Failed to fetch version file. HTTP Code: %d\n", httpCode);
  }
  http.end();
}

/**
 * Poll settings database table to verify if the dashboard triggered a scan request
 */
void checkForScanRequest() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  String url = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Scan%20Request";
  http.begin(client, url);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err && doc.size() > 0) {
      String val = doc[0]["value"];
      if (val == "true") {
        scanBluetoothDevices();
      }
    }
  }
  http.end();
}

/**
 * Scan for local Bluetooth Classic devices and upload them as JSON to settings
 */
void scanBluetoothDevices() {
  Serial.println("[Bluetooth] Starting Classic Bluetooth discovery...");
  
  // Set scan request state to "scanning" to alert the dashboard
  updateSupabaseSetting("Scan Request", "scanning");
  
  // Trigger discovery for 5 seconds
  BTScanResults* pResults = SerialBT.discover(5000);
  
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  
  if (pResults) {
    int count = pResults->getCount();
    Serial.printf("[Bluetooth] Discovery complete. Found %d devices.\n", count);
    for (int i = 0; i < count; i++) {
      BTAdvertisedDevice* device = pResults->getDevice(i);
      JsonObject obj = arr.add<JsonObject>();
      obj["name"] = device->getName().c_str();
      obj["address"] = device->getAddress().toString().c_str();
    }
  } else {
    Serial.println("[Bluetooth] Scan failed or no devices discovered.");
  }
  
  String jsonStr;
  serializeJson(doc, jsonStr);
  
  // Upload discovery results
  updateSupabaseSetting("Scanned Bluetooth Printers", jsonStr);
  
  // Reset scan request to "false" indicating completion
  updateSupabaseSetting("Scan Request", "false");
}

/**
 * Helper to update values in the Supabase settings table
 */
void updateSupabaseSetting(String key, String value) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  String url = String(SUPABASE_URL) + "/rest/v1/settings?key=eq." + key;
  url.replace(" ", "%20"); // Encode space characters
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  
  JsonDocument patchDoc;
  patchDoc["value"] = value;
  
  String patchBody;
  serializeJson(patchDoc, patchBody);
  
  int httpCode = http.PATCH(patchBody);
  if (httpCode != 204 && httpCode != 200) {
    Serial.printf("[Supabase PATCH Error] Key %s, Code: %d\n", key.c_str(), httpCode);
  }
  http.end();
}

/**
 * Fetch thermal printer settings from Supabase
 */
void fetchPrinterSettings() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  String url = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Thermal%20Printer%20Settings";
  http.begin(client, url);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err && doc.size() > 0) {
      String val = doc[0]["value"];
      JsonDocument printerDoc;
      DeserializationError parseErr = deserializeJson(printerDoc, val);
      if (!parseErr) {
        printerConnectionMode = printerDoc["connection"].as<String>();
        printerDeviceAddress = printerDoc["device"].as<String>();
        printerPaperWidth = printerDoc["paper"].as<String>();
        printerHeader = printerDoc["header"].as<String>();
        
        if (printerConnectionMode == "null") printerConnectionMode = "wire";
        if (printerDeviceAddress == "null") printerDeviceAddress = "";
        if (printerPaperWidth == "null") printerPaperWidth = "58mm";
        if (printerHeader == "null") printerHeader = "Welcome";
        
        Serial.printf("[Printer Settings] Loaded: connection=%s, address=%s, size=%s\n", 
          printerConnectionMode.c_str(), printerDeviceAddress.c_str(), printerPaperWidth.c_str());
      }
    }
  }
  http.end();
}

/**
 * Send raw ESC/POS commands to print the generated ticket
 */
void printTicket(int tokenNum, String customerName, String serviceType, int waitTime) {
  // Sync the latest printer rules from the DB before printing
  fetchPrinterSettings();

  // Construct standard ESC/POS formatted ticket
  String printData = "";
  
  printData += "\x1B\x40"; // ESC @ (Initialize printer)
  printData += "\x1B\x61\x01"; // ESC a 1 (Align center)
  
  // Receipt Header Text
  printData += "\x1D\x21\x00"; // GS ! 0 (Normal size)
  printData += "\x1B\x45\x01"; // ESC E 1 (Bold ON)
  printData += printerHeader + "\n";
  printData += "WALK-IN TICKET\n";
  printData += "\x1B\x45\x00"; // ESC E 0 (Bold OFF)
  
  // Layout separators (adjust length for 58mm vs 80mm wide rolls)
  if (printerPaperWidth == "80mm") {
    printData += "------------------------------------------------\n";
  } else {
    printData += "--------------------------------\n";
  }
  
  printData += "Your Token Number is:\n\n";
  printData += "\x1D\x21\x11"; // GS ! 0x11 (Double Width + Double Height Font)
  printData += " " + String(tokenNum) + " \n\n";
  
  printData += "\x1D\x21\x00"; // Normal size
  printData += "\x1B\x45\x01"; // Bold ON
  printData += serviceType + "\n";
  printData += "\x1B\x45\x00"; // Bold OFF
  
  if (printerPaperWidth == "80mm") {
    printData += "------------------------------------------------\n";
  } else {
    printData += "--------------------------------\n";
  }
  
  printData += "Customer: " + customerName + "\n";
  printData += "Est. Wait Time: " + String(waitTime) + " mins\n";
  printData += "Printed: " + getLocalDateTime() + "\n";
  
  if (printerPaperWidth == "80mm") {
    printData += "------------------------------------------------\n\n\n\n\n";
  } else {
    printData += "--------------------------------\n\n\n\n\n";
  }
  
  printData += "\x1D\x56\x42\x00"; // GS V 66 0 (Paper feed and partial cut)

  if (printerConnectionMode == "bluetooth") {
    if (printerDeviceAddress == "") {
      Serial.println("[Printer] Error: No Bluetooth MAC address configured.");
      return;
    }
    
    // Check connection and re-establish if disconnected
    if (!SerialBT.connected()) {
      Serial.println("[Printer] Connecting to Bluetooth thermal printer...");
      
      uint8_t address[6];
      int values[6];
      if (sscanf(printerDeviceAddress.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) == 6) {
        for (int i = 0; i < 6; i++) {
          address[i] = (uint8_t)values[i];
        }
        
        // Attempt connection (timeout 8 seconds)
        SerialBT.connect(address);
      }
    }
    
    if (SerialBT.connected()) {
      Serial.println("[Printer] Printing ticket over Bluetooth...");
      SerialBT.print(printData);
    } else {
      Serial.println("[Printer] Connection failed. Please ensure the printer is turned on.");
    }
  } else {
    // Wired mode: output over hardware Serial2
    // Pin 16 is RX, Pin 17 is TX. RX2/TX2 on ESP32 development board. Connect printer RX to ESP32 TX2 (Pin 17).
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    Serial.println("[Printer] Printing ticket over wired Serial2 (Pin 17)...");
    Serial2.print(printData);
  }
}

/**
 * Fetch synced local date/time from NTP
 */
String getLocalDateTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStringBuff[30];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%d/%m/%Y %I:%M %p", &timeinfo);
    return String(timeStringBuff);
  }
  return "--/--/---- --:--";
}

/**
 * Connect to Supabase to fetch the last generated token,
 * increment it (maintaining standard boundaries), insert the new walk-in record,
 * and update the setting.
 */
void generateSupabaseToken() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Error] WiFi disconnected. Skipping token generation.");
    blinkLED(3, 100);
    return;
  }
  
  WiFiClientSecure client;
  client.setInsecure(); // Disable SSL chain verification for simplicity and robustness
  HTTPClient http;
  
  int lastTokenNumber = 0;
  int startingTokenNumber = 100; // Default fallback
  int currentServingToken = 0;
  int avgServiceTime = 10;
  
  // 1. Fetch Last Generated Token setting
  String urlGetLast = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Last%20Generated%20Token";
  http.begin(client, urlGetLast);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err && doc.size() > 0) {
      String val = doc[0]["value"];
      lastTokenNumber = val.toInt();
    }
  } else {
    Serial.print("[Supabase GET Last Token Error] HTTP code: ");
    Serial.println(httpCode);
  }
  http.end();
  
  // 2. Fetch Starting Token Number setting
  String urlGetStart = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Starting%20Token%20Number";
  http.begin(client, urlGetStart);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  
  httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err && doc.size() > 0) {
      String val = doc[0]["value"];
      startingTokenNumber = val.toInt();
    }
  } else {
    Serial.print("[Supabase GET Starting Token Error] HTTP code: ");
    Serial.println(httpCode);
  }
  http.end();

  // 3. Fetch Current Serving Token and Average Service Delay for printing estimations
  String urlGetServing = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Current%20Serving%20Token";
  http.begin(client, urlGetServing);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  
  httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err && doc.size() > 0) {
      String val = doc[0]["value"];
      currentServingToken = val.toInt();
    }
  }
  http.end();

  String urlGetAvg = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Average%20Service%20Time";
  http.begin(client, urlGetAvg);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  
  httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err && doc.size() > 0) {
      String val = doc[0]["value"];
      avgServiceTime = val.toInt();
    }
  }
  http.end();
  
  // Compute new token number
  int newTokenNumber = lastTokenNumber + 1;
  if (newTokenNumber < startingTokenNumber) {
    newTokenNumber = startingTokenNumber;
  }
  
  Serial.print("[Supabase] Computed New Token: ");
  Serial.println(newTokenNumber);
  
  // 4. Insert new token record
  String urlPostToken = String(SUPABASE_URL) + "/rest/v1/tokens";
  http.begin(client, urlPostToken);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  
  JsonDocument postDoc;
  postDoc["token_number"] = newTokenNumber;
  postDoc["customer_name"] = "Walk-In";
  postDoc["phone_number"] = "-";
  postDoc["email"] = "-";
  postDoc["service_type"] = "General Service";
  postDoc["source"] = "Manual";
  postDoc["status"] = "Waiting";
  postDoc["remarks"] = "Generated via ESP32 Hardware Button";
  
  String requestBody;
  serializeJson(postDoc, requestBody);
  
  httpCode = http.POST(requestBody);
  bool insertSuccess = (httpCode == 201 || httpCode == 200);
  if (!insertSuccess) {
    Serial.print("[Supabase POST Token Error] HTTP code: ");
    Serial.println(httpCode);
    Serial.println(http.getString());
  }
  http.end();
  
  if (insertSuccess) {
    // 5. Update settings table Last Generated Token key
    String urlPatchLast = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Last%20Generated%20Token";
    http.begin(client, urlPatchLast);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", SUPABASE_KEY);
    http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
    
    JsonDocument patchDoc;
    patchDoc["value"] = String(newTokenNumber);
    
    String patchBody;
    serializeJson(patchDoc, patchBody);
    
    httpCode = http.PATCH(patchBody);
    if (httpCode == 204 || httpCode == 200) {
      Serial.println("[Supabase] Token database update complete!");
      
      // Calculate wait estimation: wait count = (newTokenNumber - currentServingToken)
      int waitCount = newTokenNumber - currentServingToken - 1;
      if (waitCount < 0) waitCount = 0;
      int estimatedWait = waitCount * avgServiceTime;
      
      // Print the physical ticket
      printTicket(newTokenNumber, "Walk-In", "General Service", estimatedWait);
      
      // Success flash feedback
      blinkLED(2, 200);
    } else {
      Serial.print("[Supabase PATCH Last Token Error] HTTP code: ");
      Serial.println(httpCode);
      Serial.println(http.getString());
      blinkLED(3, 100);
    }
    http.end();
  } else {
    blinkLED(3, 100);
  }
}
