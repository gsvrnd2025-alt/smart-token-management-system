/**
 * Smart Token Management System - Slave ESP32 Operator Terminal
 * 
 * Hardware Description:
 * - ESP32 Development Board
 * - "Next Token" Button connected between GPIO 4 and GND (active LOW)
 * - DFPlayer Mini (MP3 TF 16P) connected to:
 *     - VCC -> 5V
 *     - GND -> GND
 *     - TX -> GPIO 16 (ESP32 RX2)
 *     - RX -> GPIO 17 (ESP32 TX2) [Use 1K ohm resistor in series between TX2 and RX]
 *     - Busy Pin -> GPIO 5 (with internal pullup)
 * - LED Indicator on GPIO 2
 * 
 * Features:
 * - Captive Portal WiFi configuration with scan list & premium dark theme UI
 * - Redirection to the GitHub Pages Dashboard on success
 * - Wireless OTA updates via GitHub HTTP Auto-Updates over the internet
 * - Direct HTTPS integration with Supabase database (calling next token, voice readout)
 */

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <HTTPUpdate.h>

// Version and GitHub OTA configuration
const int CURRENT_VERSION = 1;
const char* VERSION_URL = "https://raw.githubusercontent.com/MONASKUMAR/smart-token-management-system/master/bin/slave_version.txt";
const char* FIRMWARE_URL = "https://raw.githubusercontent.com/MONASKUMAR/smart-token-management-system/master/bin/esp32_slave.bin";

unsigned long lastUpdateCheckTime = 0;
const unsigned long updateCheckInterval = 300000; // Check for firmware updates every 5 minutes

// Supabase Configuration
const char* SUPABASE_URL = "https://lziwnwdiyfdgyznngcma.supabase.co";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imx6aXdud2RpeWZkZ3l6bm5nY21hIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODQ3MDQ2OTYsImV4cCI6MjEwMDI4MDY5Nn0.Gpe-dZfVIjLhFT_VP__uawVjwPMbDciUfmFzWNz5hpc";

const char* DASHBOARD_REDIRECT_URL = "https://monaskumar.github.io/smart-token-management-system/";

// WiFi Configuration Settings (Optional fallback)
// Replace with your actual local Wi-Fi credentials to pre-configure/bypass the captive portal
const char* WIFI_SSID = "GSV_Electrical_Enterprises";
const char* WIFI_PASS = "@26Nov1996#";

// Hardware Pins
const int NEXT_BUTTON_PIN = 4; // Operator Desk Call Button (active LOW)
const int BUSY_PIN = 5;        // DFPlayer Mini BUSY pin (LOW = playing, HIGH = idle)
const int LED_PIN = 2;         // Status indicator LED

// DFPlayer and Serial configurations
HardwareSerial mySoftwareSerial(2); // Use ESP32 HardwareSerial 2
DFRobotDFPlayerMini myDFPlayer;

// Web and DNS Servers for Portal
WebServer server(80);
DNSServer dnsServer;

// WiFi settings storage
Preferences preferences;
String ssidListHTML = "";

// State variables
bool portalActive = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 400; // Debounce delay in ms

// Function Declarations
void blinkLED(int count, int delayMs);
void startCaptivePortal();
void handleRootPortal();
void handleSaveWiFi();
void checkForUpdates();
void triggerNextTokenSupabase();
void announceToken(const String& tokenStr);
void waitPlayComplete();
void playBuzzerChime();

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(NEXT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUSY_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize HardwareSerial2 (RX=16, TX=17) for DFPlayer Mini
  mySoftwareSerial.begin(9600, SERIAL_8N1, 16, 17);
  
  Serial.println("[DFPlayer] Connecting to DFPlayer Mini...");
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("[Error] DFPlayer Mini not detected. Check RX/TX connections and SD card.");
    // Blink LED to indicate DFPlayer failure
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_PIN, HIGH); delay(100);
      digitalWrite(LED_PIN, LOW); delay(100);
    }
  } else {
    Serial.println("[DFPlayer] Connected successfully.");
    myDFPlayer.volume(25); // Set speaker volume (0 to 30)
  }

  // Initialize Preferences
  preferences.begin("wifi", false);

  bool forcePortal = false;
  // Check if button is held down on boot to force configuration portal
  if (digitalRead(NEXT_BUTTON_PIN) == LOW) {
    Serial.println("\n[Setup] Button held on boot! Clearing WiFi credentials...");
    preferences.clear();
    blinkLED(10, 100); // Fast blink to acknowledge
    forcePortal = true;
  }

  String storedSSID = preferences.getString("ssid", "");
  String storedPASS = preferences.getString("pass", "");

  // If no stored credentials and portal is NOT forced, use hardcoded credentials if provided
  if (storedSSID == "" && !forcePortal) {
    if (String(WIFI_SSID) != " YOUR WIFI" && String(WIFI_SSID) != "") {
      storedSSID = WIFI_SSID;
      storedPASS = WIFI_PASS;
      Serial.println("\n[Setup] No stored WiFi credentials. Using hardcoded fallback details.");
      preferences.putString("ssid", storedSSID);
      preferences.putString("pass", storedPASS);
    }
  }

  if (storedSSID == "") {
    Serial.println("\n[Setup] No WiFi credentials stored. Launching setup portal...");
    startCaptivePortal();
  } else {
    Serial.print("\n[Setup] Stored WiFi SSID found: ");
    Serial.println(storedSSID);
    
    // Explicitly set mode and disconnect before begin to ensure a clean connection state
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
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
      
      // Enable concurrent SoftAP (Hybrid AP+STA Mode)
      // First: scan surrounding networks while still in STA mode (before switching to AP_STA)
      Serial.println("[Portal] Scanning nearby WiFi networks for portal list...");
      int n = WiFi.scanNetworks();
      Serial.printf("[Portal] Found %d networks.\n", n);
      ssidListHTML = "";
      if (n > 0) {
        for (int i = 0; i < n; i++) {
          String encryptionType = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
          ssidListHTML += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + " (" + encryptionType + ", Sig: " + String(WiFi.RSSI(i)) + "dBm)</option>";
        }
      }

      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP("Smart-Token-Operator");
      IPAddress apIP(192, 168, 4, 1);
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      dnsServer.start(53, "*", apIP);
      
      server.on("/", HTTP_GET, handleRootPortal);
      server.on("/save", HTTP_POST, handleSaveWiFi);
      server.onNotFound([]() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
      });
      server.begin();
      portalActive = true; // Run portal concurrently in the loop


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
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    // Check for GitHub updates periodically
    if (millis() - lastUpdateCheckTime > updateCheckInterval) {
      lastUpdateCheckTime = millis();
      checkForUpdates();
    }
    
    // Read Operator "Next" button
    int buttonState = digitalRead(NEXT_BUTTON_PIN);
    
    if (buttonState == LOW) {
      if ((millis() - lastDebounceTime) > debounceDelay) {
        lastDebounceTime = millis();
        Serial.println("\n[Slave] Calling next customer queue slot...");
        
        digitalWrite(LED_PIN, LOW); // Turn off LED to indicate active action
        triggerNextTokenSupabase();
        digitalWrite(LED_PIN, HIGH); // Turn LED back on
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
  
  // 1. Set mode to STA for scanning
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(200); // Allow mode configuration to settle
  
  // Scan for local WiFi networks (with retry fallback)
  Serial.println("[Portal] Scanning networks in STA mode...");
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
  
  // 2. Switch to AP_STA mode to host the captive portal
  WiFi.mode(WIFI_AP_STA);
  delay(200);
  
  // Host an open Access Point
  WiFi.softAP("Smart-Token-Operator");
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  Serial.print("[Portal] Access Point Started. SSID: Smart-Token-Operator, IP: ");
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
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Operator WiFi Setup</title><style>";
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
  html += "<p>Connect your Operator Terminal to your local WiFi network.</p>";
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
  html += "<button type='submit' class='btn'>Connect Terminal</button>";
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
    html += "<p>The operator terminal is now connecting to your WiFi network. You will be redirected to the dashboard in 5 seconds...</p>";
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
 * Connect to Supabase to call the next waiting token.
 * Completed any active serving token first, then finds the next waiting token,
 * updates it to serving, sets the current serving token setting, and reads it aloud.
 */
void triggerNextTokenSupabase() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Error] WiFi disconnected. Skipping queue call.");
    blinkLED(3, 100);
    return;
  }
  
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  // 1. Complete current serving tokens (set status = 'Completed' where status = 'Serving')
  String urlComplete = String(SUPABASE_URL) + "/rest/v1/tokens?status=eq.Serving";
  http.begin(client, urlComplete);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  
  int httpCode = http.PATCH("{\"status\":\"Completed\"}");
  if (httpCode != 204 && httpCode != 200) {
    Serial.print("[Supabase Complete serving Error] HTTP code: ");
    Serial.println(httpCode);
  }
  http.end();
  
  // 2. Fetch the next waiting token (status = 'Waiting', order = token_number asc, limit = 1)
  String urlGetNext = String(SUPABASE_URL) + "/rest/v1/tokens?status=eq.Waiting&order=token_number.asc&limit=1";
  http.begin(client, urlGetNext);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  
  httpCode = http.GET();
  int nextTokenId = -1;
  String nextTokenStr = "";
  String nextCustomerName = "";
  
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err && doc.size() > 0) {
      nextTokenId = doc[0]["id"];
      int tokenNum = doc[0]["token_number"];
      nextTokenStr = String(tokenNum);
      nextCustomerName = doc[0]["customer_name"].as<String>();
    }
  } else {
    Serial.print("[Supabase GET Next Waiting Error] HTTP code: ");
    Serial.println(httpCode);
  }
  http.end();
  
  if (nextTokenId != -1) {
    // 3. Set this token status to 'Serving'
    String urlSetServing = String(SUPABASE_URL) + "/rest/v1/tokens?id=eq." + String(nextTokenId);
    http.begin(client, urlSetServing);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", SUPABASE_KEY);
    http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
    
    httpCode = http.PATCH("{\"status\":\"Serving\"}");
    if (httpCode != 204 && httpCode != 200) {
      Serial.print("[Supabase Set Serving Error] HTTP code: ");
      Serial.println(httpCode);
    }
    http.end();
    
    // 4. Update Settings key 'Current Serving Token' to the token number
    String urlUpdateSetting = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Current%20Serving%20Token";
    http.begin(client, urlUpdateSetting);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", SUPABASE_KEY);
    http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
    
    String patchBody = "{\"value\":\"" + nextTokenStr + "\"}";
    httpCode = http.PATCH(patchBody);
    if (httpCode != 204 && httpCode != 200) {
      Serial.print("[Supabase Update Current Serving Error] HTTP code: ");
      Serial.println(httpCode);
    }
    http.end();
    
    Serial.println("\n==================================");
    Serial.print("  CALLING NOW: "); Serial.println(nextTokenStr);
    Serial.print("  Customer: "); Serial.println(nextCustomerName);
    Serial.println("==================================");
    
    // Vocal speech digits announcement
    announceToken(nextTokenStr);
  } else {
    // No tokens waiting, update setting to "0"
    String urlUpdateSetting = String(SUPABASE_URL) + "/rest/v1/settings?key=eq.Current%20Serving%20Token";
    http.begin(client, urlUpdateSetting);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", SUPABASE_KEY);
    http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
    
    httpCode = http.PATCH("{\"value\":\"0\"}");
    http.end();
    
    Serial.println("[Lobby Info] Queue is currently empty. No waiting customers.");
    // Play buzzer chime indicating empty queue
    playBuzzerChime();
  }
}

/**
 * Speak the token number digit-by-digit
 * 
 * Audio track mapping layout:
 * - 0001.mp3: "Token"
 * - 0002.mp3: "Number"
 * - 0003.mp3: "Zero"
 * - 0004.mp3: "One"
 * - 0005.mp3: "Two"
 * - 0006.mp3: "Three"
 * - 0007.mp3: "Four"
 * - 0008.mp3: "Five"
 * - 0009.mp3: "Six"
 * - 0010.mp3: "Seven"
 * - 0011.mp3: "Eight"
 * - 0012.mp3: "Nine"
 * - 0013.mp3: "Please proceed to counter one"
 */
void announceToken(const String& tokenStr) {
  Serial.println("[DFPlayer] Commencing Voice Announcement...");
  
  // 1. Play track 1 ("Token")
  myDFPlayer.play(1);
  waitPlayComplete();
  
  // 2. Play track 2 ("Number")
  myDFPlayer.play(2);
  waitPlayComplete();
  
  // 3. Process digits sequence
  for (int i = 0; i < tokenStr.length(); i++) {
    char ch = tokenStr.charAt(i);
    int trackNum = -1;
    
    if (ch >= '0' && ch <= '9') {
      trackNum = (ch - '0') + 3; // '0' is track 3, '1' is track 4, etc.
    }
    
    if (trackNum != -1) {
      Serial.print("[DFPlayer] Playing digit: "); Serial.println(ch);
      myDFPlayer.play(trackNum);
      waitPlayComplete();
    }
  }
  
  // 4. Play track 13 ("Please proceed to counter one")
  myDFPlayer.play(13);
  waitPlayComplete();
  
  Serial.println("[DFPlayer] Vocal announcement sequence completed.");
}

/**
 * Blocking wait until DFPlayer Mini finishes playing active track.
 * Reads high/low state of GPIO Pin connected to DFPlayer Busy pin.
 */
void waitPlayComplete() {
  delay(100); // Give DFPlayer 100ms to pull the BUSY pin low
  
  // While BUSY pin is LOW (0), track is playing. Wait.
  while (digitalRead(BUSY_PIN) == LOW) {
    delay(10);
  }
}

/**
 * Play a simple single chime track for empty lobby status
 */
void playBuzzerChime() {
  Serial.println("[DFPlayer] Playing empty queue chime...");
  myDFPlayer.play(14);
  waitPlayComplete();
}
