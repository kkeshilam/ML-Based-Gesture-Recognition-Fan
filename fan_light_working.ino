#include <ArduinoBLE.h>

/* ================== NUS UUIDs ================== */
#define NUS_SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_CHAR_UUID  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // Peripheral→Central (Notify)

/* ================== GPIO (through BC548) ================== */
const int powerSpeedPin = 2; // press = ON / next speed
const int offButtonPin  = 3; // press = OFF
const int lampPin       = 4; // LED low-side switch (HIGH = ON)

/* ================== State & Timing ================== */
bool fanOn = false;
unsigned long lastActionMs = 0;
const unsigned long ACTION_DEBOUNCE_MS = 300;

BLEDevice         peripheral;
BLECharacteristic rxChar;

String lineBuf;

/* ================== Helpers ================== */

void pressButton(int pin, uint16_t holdMs = 200) {
  digitalWrite(pin, HIGH);
  delay(holdMs);
  digitalWrite(pin, LOW);
}

void lampOn()  { digitalWrite(lampPin, HIGH); }
void lampOff() { digitalWrite(lampPin, LOW); }

/* Normalize command:
   - trim, lowercase, replace spaces with underscores
   - strip anything after ':' or ',' (confidence score etc.)
*/
String commandKey(String s) {
  s.trim();
  s.replace("\r", "");
  s.toLowerCase();
  s.replace(" ", "_");
  int cut = s.indexOf(':');
  if (cut < 0) cut = s.indexOf(',');
  if (cut > 0) s = s.substring(0, cut);
  while (s.indexOf("__") >= 0) s.replace("__", "_");
  return s;
}

void handleCommand(String raw) {
  String key = commandKey(raw);

  // Map gestures to equivalent commands
  if (key == "wave_on")  key = "on";
  if (key == "wave_off") key = "offs";
  if (key == "tap_on")   key = "off";
  if (key == "tap_off")  key = "ons";

  unsigned long now = millis();
  if (now - lastActionMs < ACTION_DEBOUNCE_MS) {
    Serial.print("[SKIP] Debounced: "); Serial.println(key);
    return;
  }
  lastActionMs = now;

  if (key == "ons") {
    lampOn();
    Serial.print("[LAMP] ON (src: "); Serial.print(raw); Serial.println(")");
  }
  else if (key == "offs") {
    lampOff();
    Serial.print("[LAMP] OFF (src: "); Serial.print(raw); Serial.println(")");
  }
  else if (key == "on") {
    pressButton(powerSpeedPin);
    if (!fanOn) {
      fanOn = true;
      Serial.print("[FAN] ON (src: "); Serial.print(raw); Serial.println(")");
    } else {
      Serial.print("[FAN] SPEED CHANGE (src: "); Serial.print(raw); Serial.println(")");
    }
  }
  else if (key == "off") {
    pressButton(offButtonPin);
    fanOn = false;
    Serial.print("[FAN] OFF (src: "); Serial.print(raw); Serial.println(")");
  }
  else {
    Serial.print("[INFO] Unmapped: "); Serial.println(raw);
  }
}

void processNUSChunk(const uint8_t* data, int len) {
  for (int i = 0; i < len; i++) {
    char c = (char)data[i];
    if (c == '\n') {
      String msg = lineBuf;
      lineBuf = "";
      if (msg.length() > 0) handleCommand(msg);
    } else if (c != '\r') {
      lineBuf += c;
    }
  }
}

/* ================== BLE Connect/Scan ================== */

bool subscribeToNUS(BLEDevice& dev) {
  if (!dev.discoverAttributes()) return false;
  rxChar = dev.characteristic(NUS_RX_CHAR_UUID);
  if (!rxChar) return false;
  if (!rxChar.canSubscribe()) return false;
  return rxChar.subscribe();
}

bool connectToPeripheral(BLEDevice& dev) {
  if (!dev.connect()) return false;
  Serial.println("Connected.");
  return true;
}

/* ================== Setup/Loop ================== */

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  pinMode(powerSpeedPin, OUTPUT);
  pinMode(offButtonPin,  OUTPUT);
  pinMode(lampPin,       OUTPUT);

  digitalWrite(powerSpeedPin, LOW);
  digitalWrite(offButtonPin,  LOW);
  digitalWrite(lampPin,       LOW);

  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1) { delay(1000); }
  }

  BLE.scan();
  Serial.println("Nano 33 BLE Sense - Central");
  Serial.println("Looking for device: XIAO-Gesture");
}

void loop() {
  BLEDevice dev = BLE.available();
  if (dev) {
    String name = dev.localName();
    if (name.length()) {
      Serial.print("Found: "); Serial.println(name);
    }

    if (name == "XIAO-Gesture") {
      BLE.stopScan();

      if (connectToPeripheral(dev) && subscribeToNUS(dev)) {
        Serial.println("Subscribed to NUS RX. Waiting for messages...");
        while (dev.connected()) {
          BLE.poll();
          if (rxChar.valueUpdated()) {
            int len = rxChar.valueLength();
            if (len > 0) {
              const uint8_t* raw = rxChar.value();
              processNUSChunk(raw, len);
            }
          }
        }
        Serial.println("Disconnected.");
      }

      lineBuf = "";
      rxChar = BLECharacteristic();
      BLE.scan();
    }
  }
  BLE.poll();
}
