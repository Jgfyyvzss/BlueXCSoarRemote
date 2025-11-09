// SSID & Pwd at void setupOTA() IP defaults 192.168.4.1
//IO pins defined below #includes
#include <Arduino.h>
#include <Hardware.h>
#include <BleKeyboard.h>
#include <Button2.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <LittleFS.h>

#define NUM_BUTTONS 10

BleKeyboard bleKeyboard("BXRKeyboard", "ESP32", 100);
Button2 buttons[NUM_BUTTONS];
// If not using external buttons to XCSoar, decrease count, remove pins, modify data/keymap.txt
// Do not use IO9. Avoid IO2, IO8
// USB on top IO 5, 8, 21, 2, 4, 3, 0, 1, 10, 20
// USB on bottom IO 0, 3, 10, 4, 6, 20, 5, 7, 
const uint8_t buttonPins[NUM_BUTTONS] = {5, 8, 21, 0, 3, 4, 2, 1, 20, 10};

// Button mapping reference:
// Index 0 - Top          = IO pin 5 
// Index 1 - TopRight     = IO pin 8  
// Index 2 - BotRight     = IO pin 21
// Index 3 - Left         = IO pin 0 
// Index 4 - Right        = IO pin 3
// Index 5 - Up           = IO pin 4
// Index 6 - Down         = IO pin 2
// Index 7 - Centre_Click = IO pin 1
// Index 8 - FrTop        = IO pin 20
// Index 9 - FrBot        = IO pin 10 

void handleClick(Button2& btn);
void handleDoubleClick(Button2& btn);
void handleLongClick(Button2& btn);
void loadKeymap();
void sendKey(const String& keyStr);
uint8_t lookupKeyCode(const String& name);

// OTA server
WebServer server(80);
bool otaEnabled = false;

//file upload
File keymapUploadFile;
String uploadBuffer = "";

// OTA HTML Form
void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html><head><title>ESP32C3 Keyboard OTA</title>";
  html += "<style>";
  html += "body { font-family: Arial; margin: 40px; background: #f0f0f0; }";
  html += ".container { background: white; padding: 20px; border-radius: 8px; max-width: 600px; }";
  html += ".upload-section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 4px; }";
  html += "input[type=file] { margin: 10px 0; }";
  html += "button { background: #007cba; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }";
  html += "button:hover { background: #005a87; }";
  html += "textarea { width: 100%; height: 200px; font-family: monospace; }";
  html += "</style></head><body>";
  html += "<div class=\"container\">";
  html += "<h1>ESP32C3 Keyboard OTA Update</h1>";
  html += "<div class=\"upload-section\">";
  html += "<h3>Firmware Update</h3>";
  html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
  html += "<input type='file' name='update' accept='.bin'>";
  html += "<button type='submit'>Update Firmware</button>";
  html += "</form></div>";
  html += "<div class=\"upload-section\">";
  html += "<h3>Keymap Management</h3>";
  html += "<button onclick=\"downloadKeymap()\">Download Current Keymap</button>";
  html += "<form method='POST' action='/keymap' enctype='multipart/form-data' style=\"margin-top:10px;\">";
  html += "<input type='file' name='keymap' accept='.txt'>";
  html += "<button type='submit'>Upload Keymap</button>";
  html += "</form>";
  html += "<textarea id=\"keymap-display\" readonly></textarea>";
  html += "</div></div>";
  html += "<script>";
  html += "function downloadKeymap() { window.location.href = '/keymap'; }";
  html += "fetch('/keymap').then(r => r.text()).then(d => document.getElementById('keymap-display').value = d);";
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}

// OTA 
void handleUpdate() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void handleUpdateEnd() {
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  if (!Update.hasError()) {
    delay(1000);
    ESP.restart();
  }
}

void handleKeymapUpload() {
  Serial.println("DEBUG: handleKeymapUpload called");
  HTTPUpload& upload = server.upload();
  Serial.printf("DEBUG: Upload status = %d\n", upload.status);
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("DEBUG: Starting upload of %s\n", upload.filename.c_str());
    uploadBuffer = ""; // Clear buffer
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    Serial.printf("DEBUG: Writing %u bytes\n", upload.currentSize);
    // Collect data in buffer
    for (size_t i = 0; i < upload.currentSize; i++) {
      uploadBuffer += (char)upload.buf[i];
    }
    Serial.printf("DEBUG: Buffer now %d bytes\n", uploadBuffer.length());
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    Serial.printf("DEBUG: Upload ended, total buffer: %d bytes\n", uploadBuffer.length());
    Serial.println("DEBUG: Buffer content:");
    Serial.println(uploadBuffer);
  }
}

void handleKeymapUploadEnd() {
  Serial.println("DEBUG: handleKeymapUploadEnd called");
  
  if (uploadBuffer.length() > 0) {
    Serial.println("DEBUG: Writing buffer to file...");
    
    // Write the complete buffer to file
    File file = LittleFS.open("/keymap.txt", "w");
    if (file) {
      size_t written = file.print(uploadBuffer);
      file.close();
      
      Serial.printf("DEBUG: Wrote %u bytes to file\n", written);
      
      // Verify by reading back
      File verifyFile = LittleFS.open("/keymap.txt", "r");
      if (verifyFile) {
        String readBack = verifyFile.readString();
        verifyFile.close();
        Serial.printf("DEBUG: Read back %d bytes\n", readBack.length());
        Serial.println("DEBUG: Read back content:");
        Serial.println(readBack);
        
        loadKeymap(); // Reload keymap
        server.send(200, "text/plain", "Keymap uploaded successfully");
        Serial.println("DEBUG: Success response sent");
      } else {
        Serial.println("DEBUG: Could not verify file");
        server.send(500, "text/plain", "Could not verify upload");
      }
    } else {
      Serial.println("DEBUG: Could not open file for writing");
      server.send(500, "text/plain", "Could not save keymap");
    }
  } else {
    Serial.println("DEBUG: No data in upload buffer");
    server.send(500, "text/plain", "No data received");
  }
  
  uploadBuffer = ""; // Clear buffer
}


void handleKeymapDownload() {
  File file = LittleFS.open("/keymap.txt", "r");
    String content = file.readString();
    file.close();
    server.send(200, "text/plain", content);
}

//START KEYMAP
// Keymap character mapping
struct KeyCodeMap {
  const char* name;
  uint8_t code;
};

const KeyCodeMap keyCodeMap[] = {
  {"KEY_ESC", KEY_ESC},
  {"KEY_RETURN", KEY_RETURN},
  {"KEY_TAB", KEY_TAB},
  {"KEY_BACKSPACE", KEY_BACKSPACE},
  {"KEY_DELETE", KEY_DELETE},
  {"KEY_INSERT", KEY_INSERT},
  {"KEY_HOME", KEY_HOME},
  {"KEY_END", KEY_END},
  {"KEY_PAGE_UP", KEY_PAGE_UP},
  {"KEY_PAGE_DOWN", KEY_PAGE_DOWN},
  {"KEY_UP_ARROW", KEY_UP_ARROW},
  {"KEY_DOWN_ARROW", KEY_DOWN_ARROW},
  {"KEY_LEFT_ARROW", KEY_LEFT_ARROW},
  {"KEY_RIGHT_ARROW", KEY_RIGHT_ARROW},
  {"KEY_F1", KEY_F1},
  {"KEY_F2", KEY_F2},
  {"KEY_F3", KEY_F3},
  {"KEY_F4", KEY_F4},
  {"KEY_F5", KEY_F5},
  {"KEY_F6", KEY_F6},
  {"KEY_F7", KEY_F7},
  {"KEY_F8", KEY_F8},
  {"KEY_F9", KEY_F9},
  {"KEY_F10", KEY_F10},
  {"KEY_F11", KEY_F11},
  {"KEY_F12", KEY_F12}
};

uint8_t lookupKeyCode(const String& name) {
  String trimmedName = name;
  trimmedName.trim();
  
  if (trimmedName.length() == 0) return 0;
  
  if (trimmedName.length() == 1) {
    char ch = trimmedName.charAt(0);
    return (uint8_t)ch;
  }
  
  for (const auto& entry : keyCodeMap) {
    if (trimmedName == entry.name) {
      return entry.code;
    }
  }
  
  Serial.printf("WARNING: Key not found: '%s'\n", trimmedName.c_str());
  return 0;
}

void sendKey(const String& keyStr) {
  if (!bleKeyboard.isConnected()) {
    Serial.println("DEBUG: BLE not connected");
    return;
  }
  
  String trimmed = keyStr;
  trimmed.trim();
  
  if (trimmed.length() == 0) {
    Serial.println("DEBUG: Empty key string, not sending");
    return;
  }
  
  uint8_t code = lookupKeyCode(trimmed);
  if (code > 0) {
    Serial.printf("DEBUG: Sending key '%s' (code: %d)\n", trimmed.c_str(), code);
    bleKeyboard.write(code);
  } else {
    Serial.printf("DEBUG: Key not sent - lookup returned 0 for: '%s'\n", trimmed.c_str());
  }
}

String keymap[NUM_BUTTONS][3];

void handleClick(Button2& btn) {
  int pin = btn.getPin();
  Serial.printf(">>> CLICK on pin %d\n", pin);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonPins[i] == pin) {
      Serial.printf("    Button index %d, sending: '%s'\n", i, keymap[i][0].c_str());
      sendKey(keymap[i][0]);
      break;
    }
  }
}

void handleDoubleClick(Button2& btn) {
  int pin = btn.getPin();
  Serial.printf(">>> DOUBLE-CLICK on pin %d\n", pin);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonPins[i] == pin) {
      Serial.printf("    Button index %d, sending: '%s'\n", i, keymap[i][1].c_str());
      sendKey(keymap[i][1]);
      break;
    }
  }
}

void handleLongClick(Button2& btn) {
  int pin = btn.getPin();
  Serial.printf(">>> LONG-CLICK on pin %d\n", pin);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonPins[i] == pin) {
      Serial.printf("    Button index %d, sending: '%s'\n", i, keymap[i][2].c_str());
      sendKey(keymap[i][2]);
      break;
    }
  }
}

void loadKeymap() {
  Serial.println("========================================");
  Serial.println("Loading keymap from /keymap.txt...");
  Serial.println("========================================");
  
  File file = LittleFS.open("/keymap.txt", "r");
  if (!file) {
    Serial.println("ERROR: Failed to open keymap.txt");
    return;
  }
  
  int i = 0;
  int lineNum = 0;
  
  while (file.available() && i < NUM_BUTTONS) {
    String line = file.readStringUntil('\n');
    lineNum++;
    line.trim();
    
    if (line.length() == 0 || line.startsWith("#")) {
      Serial.printf("Line %d: Skipped (comment/empty)\n", lineNum);
      continue;
    }

    Serial.printf("Line %d: %s\n", lineNum, line.c_str());

    int idx0 = line.indexOf(',');
    int idx1 = line.indexOf(',', idx0 + 1);
    int idx2 = line.indexOf(',', idx1 + 1);

    if (idx0 == -1 || idx1 == -1 || idx2 == -1) {
      Serial.printf("  ERROR: Invalid format (missing commas)\n");
      continue;
    }

    keymap[i][0] = line.substring(idx0 + 1, idx1);
    keymap[i][1] = line.substring(idx1 + 1, idx2);
    keymap[i][2] = line.substring(idx2 + 1);
    
    keymap[i][0].trim();
    keymap[i][1].trim();
    keymap[i][2].trim();

    Serial.printf("  -> Button %d (IO Pin %d):\n", i, buttonPins[i]);
    Serial.printf("     Click:  '%s'\n", keymap[i][0].c_str());
    Serial.printf("     Double: '%s'\n", keymap[i][1].c_str());
    Serial.printf("     Long:   '%s'\n", keymap[i][2].c_str());
    
    i++;
  }

  file.close();
  Serial.println("========================================");
  Serial.printf("Keymap loaded: %d buttons configured\n", i);
  Serial.println("========================================");
}
// END KEYMAP

// OTA Server setup
void setupOTA() {
  Serial.println("DEBUG: setupOTA starting...");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP("BXRemote", "12345678");
  
  Serial.println("OTA Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Register handlers
  server.on("/", HTTP_GET, handleRoot);
  server.on("/update", HTTP_POST, handleUpdateEnd, handleUpdate);
  
  // Register keymap handlers - use separate paths for clarity
  server.on("/keymap", HTTP_GET, handleKeymapDownload);
  server.on("/keymap", HTTP_POST, handleKeymapUploadEnd, handleKeymapUpload);
  
  // Add debug handler to test server is working
  server.on("/test", HTTP_GET, []() {
    Serial.println("DEBUG: /test endpoint called");
    server.send(200, "text/plain", "Server is working!");
  });
  
  server.onNotFound([]() {
    Serial.printf("DEBUG: Unknown request: %s %s\n", 
                  server.method() == HTTP_GET ? "GET" : "POST", 
                  server.uri().c_str());
    server.send(404, "text/plain", "Not Found");
  });
  
  server.begin();
  Serial.println("DEBUG: OTA server started");
}

//Setup
void setup() {
  // Initialize Serial FIRST - before any other code
  Serial.begin(115200);
  delay(1000);  // Give Serial time to initialize
  Serial.println("=== ESP32C3 Keyboard Starting ===");
  
  pinMode(buttonPins[0], INPUT_PULLUP);
  delay(100);  // Allow time for button detection
  otaEnabled = digitalRead(buttonPins[0]) == LOW;
  
  Serial.printf("OTA Mode: %s\n", otaEnabled ? "ENABLED" : "DISABLED");

  if (otaEnabled) {
    Serial.println("Starting OTA mode...");
    setCpuFrequencyMhz(160);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP("BLUEXCREM", "12345678");  // Your original SSID/PWD
    delay(500);
    
    Serial.println("WiFi AP started");
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
    
    // Initialize LittleFS with detailed logging
    Serial.println("Initializing LittleFS...");
    if (!LittleFS.begin(true)) {
      Serial.println("LittleFS initialization failed! Trying format...");
      if (!LittleFS.format()) {
        Serial.println("LittleFS format failed!");
      } else {
        Serial.println("LittleFS formatted successfully");
        if (!LittleFS.begin(false)) {
          Serial.println("LittleFS still failed after format!");
        } else {
          Serial.println("LittleFS initialized after format");
        }
      }
    } else {
      Serial.println("LittleFS initialized successfully");
      Serial.printf("Total: %u bytes, Used: %u bytes\n", 
                    LittleFS.totalBytes(), LittleFS.usedBytes());
    }
    
    // Test if keymap file exists
    if (LittleFS.exists("/keymap.txt")) {
      File testFile = LittleFS.open("/keymap.txt", "r");
      if (testFile) {
        Serial.printf("Existing keymap file size: %u bytes\n", testFile.size());
        testFile.close();
      }
    } else {
      Serial.println("No existing keymap file found");
    }
    
    setupOTA();
  } else {
    Serial.println("Starting BLE keyboard mode...");
    setCpuFrequencyMhz(80);
    
    Serial.println("Initializing LittleFS for BLE mode...");
    LittleFS.begin(false);
    
    Serial.println("Starting BLE keyboard...");
    bleKeyboard.begin();
    
    Serial.println("Loading keymap...");
    loadKeymap();
    
    Serial.println("Setting up buttons...");
      for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].begin(buttonPins[i]);
        buttons[i].setLongClickTime(800);
        buttons[i].setClickHandler(handleClick);
        buttons[i].setDoubleClickHandler(handleDoubleClick);
        buttons[i].setLongClickDetectedHandler(handleLongClick);  // Immediate trigger
    }
    Serial.println("BLE keyboard setup complete");
  }
  
  Serial.println("=== Setup Complete ===");
}

void loop() {
  if (otaEnabled) {
    server.handleClient();
    delay(10);
  } else {
    for (int i = 0; i < NUM_BUTTONS; i++) {
      buttons[i].loop();
    }
  }
  delay(10);
}
