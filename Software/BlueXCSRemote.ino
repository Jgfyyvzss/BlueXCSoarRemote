
#include <BleKeyboard.h>
#include <Button2.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <LittleFS.h>

#define NUM_BUTTONS 8

BleKeyboard bleKeyboard("ESP32C3_Keyboard", "ESP32", 100);
Button2 buttons[NUM_BUTTONS];
const uint8_t buttonPins[NUM_BUTTONS] = {5, 8, 21, 2, 4, 3, 0, 1};

WebServer server(80);
bool otaEnabled = false;

// ==== OTA HTML Form ====
const char* uploadForm = R"rawliteral(
  <!DOCTYPE html><html><body>
<h2>BlueXCSRemote Firmware + keymap.txt</h2>
<form method='POST' action='/update' enctype='multipart/form-data'>
  Firmware: <input type='file' name='update'><input type='submit' value='Update'><br><br>
</form>
<form method='POST' action='/update_keys' enctype='multipart/form-data'>
  Keymap.txt: <input type='file' name='update'><input type='submit' value='Upload'><br>
</form>
</body></html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", uploadForm);
}

void handleUpdate() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Update.begin();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Update.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    Update.end(true);
  }
}

void handleUpdateKeys() {
  HTTPUpload& upload = server.upload();
  static File f;
  if (upload.status == UPLOAD_FILE_START) {
    f = LittleFS.open("/keymap.txt", "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (f) f.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (f) f.close();
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    loadKeymap();
  }
}

struct KeyCodeMap {
  const char* name;
  uint8_t code;
};

const KeyCodeMap keyCodeMap[] = {
  {"KEY_ESC", KEY_ESC},
  {"KEY_RETURN", KEY_RETURN},
  {"KEY_F1", KEY_F1},
  {"KEY_F2", KEY_F2},
  {"KEY_F3", KEY_F3},
  {"KEY_F4", KEY_F4},
  {"KEY_F5", KEY_F5},
  {"KEY_F6", KEY_F6},
  {"KEY_F7", KEY_F7},
  {"KEY_F8", KEY_F8},
  {"KEY_F9", KEY_F9},
  {"KEY_F10", KEY_F10}
};

uint8_t lookupKeyCode(const String& name) {
  for (const auto& entry : keyCodeMap) {
    if (name == entry.name) return entry.code;
  }
  return 0; // No match
}

void sendKey(const String& keyStr) {
  if (!bleKeyboard.isConnected() || keyStr.length() == 0) return;
  uint8_t code = lookupKeyCode(keyStr);
  if (code > 0) {
    bleKeyboard.write(code);
  }
}

String keymap[NUM_BUTTONS][3]; // click, double, long

void handleClick(Button2& btn) {
  int pin = btn.getPin();
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonPins[i] == pin) {
      sendKey(keymap[i][0]);
      break;
    }
  }
}

void handleDoubleClick(Button2& btn) {
  int pin = btn.getPin();
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonPins[i] == pin) {
      sendKey(keymap[i][1]);
      break;
    }
  }
}

void handleLongClick(Button2& btn) {
  int pin = btn.getPin();
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonPins[i] == pin) {
      sendKey(keymap[i][2]);
      break;
    }
  }
}

void loadKeymap() {
  File file = LittleFS.open("/keymap.txt", "r");
  if (!file) return;
  int i = 0;
  
  while (file.available() && i < NUM_BUTTONS) {
    String line = file.readStringUntil('\n');
    line.trim();

    int idx0 = line.indexOf(',');  // Skip label
    int idx1 = line.indexOf(',', idx0 + 1);
    int idx2 = line.indexOf(',', idx1 + 1);

    keymap[i][0] = line.substring(idx0 + 1, idx1);
    keymap[i][1] = line.substring(idx1 + 1, idx2);
    keymap[i][2] = line.substring(idx2 + 1);
    i++;
  }

  file.close();
}

void setupOTA() {
  /* server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", uploadForm);
  });

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Update.begin(true);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      Update.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      Update.end(true);
    }
  });
  server.begin();
  */
  server.on("/", handleRoot);
  server.on("/update", HTTP_POST, []() { server.send(200); }, handleUpdate);
  server.on("/update_keys", HTTP_POST, []() { server.send(200); }, handleUpdateKeys);
  server.begin(); 
}

void setup() {
  pinMode(buttonPins[0], INPUT_PULLUP);
  delay(1000);  // Allow time for button detection
  otaEnabled = digitalRead(buttonPins[0]) == LOW;

  if (otaEnabled) {
    setCpuFrequencyMhz(160);
    WiFi.mode(WIFI_AP);
    WiFi.softAP("BLUEXCREM", "12345678");  // Change SSID/PWD as needed
    delay(500);
   // IPAddress IP = WiFi.softAPIP();
    LittleFS.begin(true);
    setupOTA();
  } else {
    setCpuFrequencyMhz(80);
    LittleFS.begin(false);
    bleKeyboard.begin();
    loadKeymap();
    for (int i = 0; i < NUM_BUTTONS; i++) {
      buttons[i].begin(buttonPins[i]);
      buttons[i].setClickHandler(handleClick);
      buttons[i].setDoubleClickHandler(handleDoubleClick);
      buttons[i].setLongClickHandler(handleLongClick);
    }
  }
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
