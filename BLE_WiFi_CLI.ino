#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>

BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool scanning = false;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// helper: send a small message safely via BLE
void sendBLE(const String &msg) {
  if (!deviceConnected) return;
  const int chunkSize = 20;
  for (int i = 0; i < msg.length(); i += chunkSize) {
    String chunk = msg.substring(i, i + chunkSize);
    pTxCharacteristic->setValue(chunk.c_str());
    pTxCharacteristic->notify();
    delay(15);
  }
}

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; }
  void onDisconnect(BLEServer* pServer) { deviceConnected = false; scanning = false; }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String cmd = String(pCharacteristic->getValue().c_str());
    cmd.trim();

    if (cmd.length() == 0) return;
    Serial.println("Received command: " + cmd);

    if (cmd.equalsIgnoreCase("scan")) {
      scanning = true;
      sendBLE("Scanning WiFi continuously...\n");
    } 
    else if (cmd.equalsIgnoreCase("scan_once")) {
      sendBLE("Scanning once...\n");
      int n = WiFi.scanNetworks();
      if (n <= 0) {
        sendBLE("No networks.\n");
      } else {
        for (int i = 0; i < n; ++i) {
          sendBLE("SSID: " + WiFi.SSID(i) + "\n");
          sendBLE("RSSI: " + String(WiFi.RSSI(i)) + "\n");
          delay(100);
        }
      }
      WiFi.scanDelete();
    }
    else if (cmd.equalsIgnoreCase("stop")) {
      scanning = false;
      sendBLE("Stopped scanning.\n");
    } 
    else {
      sendBLE("Unknown command.\n");
    }
  }
};

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);

  BLEDevice::init("ESP32 BLE WiFi CLI");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  pServer->getAdvertising()->start();

  Serial.println("BLE ready. Commands: scan, scan_once, stop");
}

void loop() {
  if (deviceConnected && scanning) {
    int n = WiFi.scanNetworks();
    if (n <= 0) {
      sendBLE("No networks.\n");
    } else {
      for (int i = 0; i < n; ++i) {
        sendBLE("SSID: " + WiFi.SSID(i) + "\n");
        sendBLE("RSSI: " + String(WiFi.RSSI(i)) + "\n");
        delay(100);
      }
    }
    WiFi.scanDelete();
    delay(5000);
  } else {
    delay(200);
  }
}
