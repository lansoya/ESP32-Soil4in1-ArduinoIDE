#include <ModbusMaster.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// ===== RS485 Modbus Setup =====
ModbusMaster node;

#define RX_PIN 16
#define TX_PIN 17
#define DE_RE_PIN 4

void preTransmission() {
  digitalWrite(DE_RE_PIN, HIGH);
}

void postTransmission() {
  digitalWrite(DE_RE_PIN, LOW);
}

// ===== BLE Setup =====
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define PH_CHAR_UUID        "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define TEMP_CHAR_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define MOIST_CHAR_UUID     "6E400004-B5A3-F393-E0A9-E50E24DCCA9E"
#define EC_CHAR_UUID        "6E400005-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *phChar;
BLECharacteristic *tempChar;
BLECharacteristic *moistChar;
BLECharacteristic *ecChar;

void setup() {
  Serial.begin(115200);

  // RS485 init
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);
  Serial2.begin(4800, SERIAL_8N1, RX_PIN, TX_PIN);
  node.begin(1, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  // BLE init
  BLEDevice::init("ESP32-SoilSensor");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  phChar = pService->createCharacteristic(PH_CHAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  phChar->addDescriptor(new BLE2902());

  tempChar = pService->createCharacteristic(TEMP_CHAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  tempChar->addDescriptor(new BLE2902());

  moistChar = pService->createCharacteristic(MOIST_CHAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  moistChar->addDescriptor(new BLE2902());

  ecChar = pService->createCharacteristic(EC_CHAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  ecChar->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  Serial.println("BLE started with real sensor readings.");
}

void loop() {
  uint8_t result = node.readHoldingRegisters(0x0000, 4);
  if (result == node.ku8MBSuccess) {
    float moisture = node.getResponseBuffer(0) / 10.0;
    float temperature = node.getResponseBuffer(1) / 10.0;
    float conductivity = node.getResponseBuffer(2); // EC in µS/cm
    float ph = node.getResponseBuffer(3) / 10.0;

    // Send via BLE
    phChar->setValue(String(ph).c_str());
    phChar->notify();

    tempChar->setValue(String(temperature).c_str());
    tempChar->notify();

    moistChar->setValue(String(moisture).c_str());
    moistChar->notify();

    ecChar->setValue(String(conductivity).c_str());
    ecChar->notify();

    Serial.printf("Sent -> pH: %.1f, Temp: %.1f°C, Moist: %.1f%%, EC: %.0f µS/cm\n",
                  ph, temperature, moisture, conductivity);
  } else {
    Serial.println("Error reading Modbus registers");
  }

  delay(5000);
}