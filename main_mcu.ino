#include <MAX30105.h>
#include <heartRate.h>
#include <spo2_algorithm.h>
#include <Protocentral_MAX30205.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>
#include <Wire.h>

//--------------BLE CLIENT---------------
#define SERVICE_UUID        "TO DEFINE"
#define CHARACTERISTIC_UUID "TO DEFINE"

BLEClient* pClient;
BLERemoteCharacteristic* pRemoteCharacteristic;
bool deviceConnected = false;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == "IMU_Device") {
      Serial.println("🔍 Found IMU_Device, connecting...");
      BLEDevice::getScan()->stop();
      pClient = BLEDevice::createClient();
      pClient->connect(&advertisedDevice);
      Serial.println("✅ Connected");

      BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
      if (pRemoteService == nullptr) {
        Serial.println("❌ Failed to find service");
        return;
      }

      pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
      if (pRemoteCharacteristic == nullptr) {
        Serial.println("❌ Failed to find characteristic");
        return;
      }

      // Register notification callback
      pRemoteCharacteristic->registerForNotify([](BLERemoteCharacteristic* pChar, uint8_t* data, size_t length, bool isNotify) {
        String imuData = String((char*)data);
        Serial.println("📥 IMU Data Received: " + imuData);

        // Simulate local HR data
        int heartRate = 75;
        float skinTemp = 36.5;

        Serial.print("❤️ HR: "); Serial.print(heartRate);
        Serial.print(" °C: "); Serial.println(skinTemp);

        // Combine data
        Serial.println("📦 Merged Data: " + imuData + ", HR=" + String(heartRate) + ", Temp=" + String(skinTemp));
      });

      deviceConnected = true;
    }
  }
};


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);


}

void loop() {
  // put your main code here, to run repeatedly:

}
