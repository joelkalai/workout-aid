#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <ESP_Google_Sheet_Client.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <U8g2lib.h>

// Wi-Fi credentials
#define WIFI_SSID "kalai"
#define WIFI_PASSWORD "joeldat1"
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// Google Sheets configuration
#define PROJECT_ID "iot-datalogging-461802"
#define CLIENT_EMAIL "iot-datalogging@iot-datalogging-461802.iam.gserviceaccount.com"
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQDQ/iddSWp33HLW\n6sW+xRZ2rTpqdq+bz/DcZDO9HP4uulwo3tPxDXgDDS4VApntg2Vnz5CV5hm3Vj5+\nMlg6A/exRp0qwWqGKbqMimfkkT44I1OJeU3l6NoQAdLjvfJc+rEwgcroHphilmGZ\nKg8Z4cONQ4H28EQLZpQ9VtuYm6XCry//KDG+lm1giK0NFQhliU5WGiH/WsH4m1sD\nFlQFfFFIYJRP3cYEhWmwCzGgZ5IWgstgi+IqdBjn6ey4xJm+BDk91FlTiYbkSonj\npnj1e+eRAmXkf6bWK77FvdhbSvs1AdLOYlZCqX6u6IZFW+SHa1r1wtM8uHfFNMhB\nSngGG2adAgMBAAECggEAMTdg+smCvqZWDwGrvtC4CQujYZYApe9St/wK6SKuQz/U\n2zUacT5sQD+21fDOYg4a79cEJCDTdobjYaGmgEcV6hQIiByTYvPpwBYKAFwp7Ft5\nXTsJWra6v6FGUiaVsErDNnqiV4Z4+WKTqleqxbwSaq52qToMtCiciVNulGI3J4Dg\nE2d04vrnV6H/Thx01RtKZjwuqFfckNnBomnOcJUVgLRxQyosaFefFmcu1r9DYLvk\nI7oKbqCZ/albz6MvO43BQbVvpHrVvP6G4y2Kpc6fS18gQaSntdVRzSMuQpM/9lnD\n55DnnFCVvJpumcA87ZICq23qfX+GcW2S00Hf5Nt+MQKBgQDsSryLq5/Tr2x7p8CL\nS+B6a5CkhZBGGwe7d5OveBiR7EuaVSvV1ZX/RYdf0dzsCK0dag177Xa5pwO6infX\nCPyj/ncj8HstvLT1yo0sHj2TKP+myBBquRxKDKq1jjeD8+uEnEgC8dPt7lRMLZfA\nD9bcZVHbxSnUq0PvGnsgDkd7lQKBgQDibIfXoTHLiTqIOn9PSPsiRAyOcbIvWmYk\nH9faqXJXyKnMWInrkEK8HctQqMdn5KkacRqh6IKVKoiixoWCA0OIWfvG+ZpKqQCT\nqdL7MT3dE9CgOrdRPNU1GmSID/BzDlWjyfiRP8M+cWex6Hp97PHJIAcbntVsBzSH\nS835AIE86QKBgGhjqCJoQubJNDr45GZlshlDVhZo71EZdfQLItK9UtV8t/XJtOD2\nSBsLg4AAfgJE3v4EsUpToUTplQsfS0xTqxFkRQw6nKCbIrMHthCMl5Vg4nizIBWW\nC/pm2C/3UW1ZIG5ogKPUZFVUGifBaK4wETMzC9P0qcNLmmqSn1yNEmV1AoGAEdEi\ne1hG19JLlc6sl+uRP2gFaOwPbmR5pxwOWzl8MuYqXlcVO0EVz0G07vro/gbjO55s\nzxLBu8UmDVD7zng9Ryx1gHnBp2BXGEeDv5pnWM8nWoRkSnMz/8K86GwAqr0VBhdt\nhNh0n6RC2aAGi4Qqnn3zsTu1b6Y9fuv7ID3m7YkCgYAAqoM113DeKIzp74/FT9TG\nnIsrwRUh2nODceyZKxDZ6xdRbpKOWDEYXXgH9fe3JffnOrEncpoOszVFYyEG2mwQ\ngE7Y/yDe3mgUBz+TM4luo0u5Tnk5icQoE6MehdQo0A2VxAIGe/KKQp8lxTmb3AMf\nmMXqDX0wOvwPfP/v9W29+g==\n-----END PRIVATE KEY-----\n";
const char spreadsheetId[] = "1DlwzTMuSuXTBrF_J-1WXtYfN7XLgRgGpto3rYzhW30o";  

// BLE configuration
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
static BLEAddress *pServerAddress = nullptr;
static boolean doConnect = false;
static boolean connected = false;
static boolean bleDisconnected = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;

// Display time handling
unsigned long lastTimeUpdate = 0;
const unsigned long restTimeInterval = 1000; // 1s interval

// Data handling
unsigned long lastValueTime = 0;
const int COLLECTION_TIMEOUT = 5000; // 5 seconds timeout
int timeLeft = 60;
bool newSetStarted = false;

// System states
enum SystemState { 
  STATE_BLE_SCANNING, 
  STATE_BLE_CONNECTED, 
  STATE_WIFI_CONNECTING, 
  STATE_GSHEET_UPLOADING 
};
SystemState currentState = STATE_BLE_SCANNING;

// Wi-Fi connection timeout
const unsigned long WIFI_TIMEOUT = 10000; // 10 seconds
unsigned long wifiStartTime = 0;

// Google Sheets client
bool gsheetReady = false;
unsigned long gsheetStartTime = 0;
const unsigned long GSHEET_TIMEOUT = 30000; // 30 seconds timeout

// Fatigue calculation variables
float meanHR = 70.0;
float imuFatigue = 0;
const float alpha = 0.1;
const float baselineHR = 70.0;

int collecting = 0;
int reps = 0;
int inRest = 1;
int fatigueState = 0;
int setCount = 0;

// Heart rate simulation
float getHeartRate() {
    static float currHR = 70.0; 
    float randomValue = (float)rand() / RAND_MAX;
    if (randomValue >= 0.3) {
        currHR += 2.0;
    } else {
        currHR -= 1.0;
    }
    currHR = constrain(currHR, 40.0, 180.0);
    return currHR;
}

// Data storage for sessions
struct SessionData {
  float meanHR;
  int repCount;
  unsigned long timestamp;
  float fatigue;
  int fatigueState;
  int setNumber;
};
#define MAX_SESSIONS 10
SessionData sessions[MAX_SESSIONS];
int sessionCount = 0;

// EMA update functions
float updateMeanHR(float newValue, float prevEMA, float alpha) {
  return alpha * newValue + (1 - alpha) * prevEMA;
}

// Fatigue score calculations
float HRFatigueScore() {
  return (meanHR/(220- 75)) * 70;
  //return constrain((meanHR / baselineHR) * 50.0f, 0.0f, 30.0f);
}

// Display function
void display() {
    static bool showWarning = true;
    static unsigned long lastToggleTime = 0;
    const unsigned long blinkInterval = 500; // ms
    unsigned long now = millis();
    float currentFatigue = imuFatigue + HRFatigueScore();

    // Toggle blinking text for warning
    if (now - lastToggleTime > blinkInterval) {
        showWarning = !showWarning;
        lastToggleTime = now;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);

    if (inRest) {
        // Only update rest timer once per second
        if (timeLeft > 0 && (now - lastTimeUpdate >= restTimeInterval)) {
            timeLeft--;
            lastTimeUpdate = now;
        }
        
        if (timeLeft > 0) {
            u8g2.setCursor(0, 20);
            u8g2.print("Rest Time Left:");
            u8g2.setCursor(30, 40);
            u8g2.print(timeLeft);
            u8g2.print(" sec");
        } else {
            u8g2.setCursor(0, 30);
            u8g2.print("! You can begin !");
        }
    } else {
        u8g2.setCursor(0, 12);
        u8g2.print("Reps:");
        u8g2.setCursor(70, 12);
        u8g2.print(reps);

        u8g2.setCursor(0, 30);
        u8g2.print("Fatigue:");
        u8g2.setCursor(70, 30);
        u8g2.print(currentFatigue, 1);
        
        u8g2.setCursor(0, 48);
        u8g2.print("Set:");
        u8g2.setCursor(70, 48);
        u8g2.print(setCount);

        if (currentFatigue > 85 && showWarning) {
            u8g2.setCursor(0, 60);
            u8g2.print("⚠ Please rest! ⚠");
            fatigueState = 1;
        }
    }

    u8g2.sendBuffer();
}

// BLE Client Callbacks
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
    bleDisconnected = false;
    Serial.println("Connected to BLE server");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    bleDisconnected = true;
    Serial.println("Disconnected from BLE server");
  }
};

// BLE Advertised Device Callbacks
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {
      Serial.println("Found target BLE device");
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
    }
  }
};

// BLE Notification Callback
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                          uint8_t* pData, 
                          size_t length, 
                          bool isNotify) {
  String receivedData = String((char*)pData, length);
  Serial.println("Received BLE Data: " + receivedData);
  
  // Parse the received data (format: "booleanVal,floatingPointVal,repCount")
  int firstComma = receivedData.indexOf(',');
  int secondComma = receivedData.indexOf(',', firstComma + 1);
  
  if (firstComma == -1 || secondComma == -1) {
    Serial.println("Invalid data format");
    return;
  }
  
  String boolStr = receivedData.substring(0, firstComma);
  String valueStr = receivedData.substring(firstComma + 1, secondComma);
  String repCountStr = receivedData.substring(secondComma + 1);
  
  int boolFlag = boolStr.toInt();
  imuFatigue = valueStr.toFloat();
  reps = repCountStr.toInt();

  if (boolFlag == 0) {  // Workout session started
    inRest = 0;
    collecting = 1;
    
    // Only reset for a new set, not for each rep
    if (newSetStarted) {
      timeLeft = 60;
      meanHR = baselineHR;
      fatigueState = 0;
      newSetStarted = false;
      setCount++;
      Serial.println("New set started");
    }
  } else {  // Workout session ended
    inRest = 1;
    collecting = 0;
    newSetStarted = true;  // Prepare for next set
    Serial.println("Set ended");
    
    // Store session data only when a set completes
    if (sessionCount < MAX_SESSIONS) {
      sessions[sessionCount].meanHR = meanHR;
      sessions[sessionCount].repCount = reps;
      sessions[sessionCount].timestamp = time(nullptr);
      sessions[sessionCount].fatigue = imuFatigue + HRFatigueScore();
      sessions[sessionCount].fatigueState = fatigueState;
      sessions[sessionCount].setNumber = setCount;
      sessionCount++;
      Serial.println("Session data stored for set " + String(setCount));
    }
  }
  
  lastValueTime = millis();
}

// Connect to BLE Server
bool connectToServer() {
  BLEClient* pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  
  if (!pClient->connect(*pServerAddress)) {
    Serial.println("BLE connection failed");
    return false;
  }

  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (!pRemoteService) {
    Serial.println("Service not found");
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (!pRemoteCharacteristic) {
    Serial.println("Characteristic not found");
    return false;
  }

  if(pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }

  return true;
}

// Upload session data to Google Sheets (Sheet3)
void uploadDataToSheets() {
  if (sessionCount == 0) return;

  FirebaseJson response;
  FirebaseJson valueRange;
  valueRange.add("majorDimension", "ROWS");

  FirebaseJsonArray rows;
  for (int i = 0; i < sessionCount; i++) {
    FirebaseJsonArray row;
    row.add(sessions[i].timestamp);    // Column A: Timestamp
    row.add(sessions[i].fatigue);      // Column B: Fatigue
    row.add(sessions[i].meanHR);       // Column C: MeanHR
    row.add(sessions[i].repCount);     // Column D: Rep Count
    row.add(sessions[i].fatigueState); // Column E: Fatigue State
    row.add(sessions[i].setNumber);    // Column F: Set Number
    rows.add(row);
  }

  valueRange.add("values", rows);

  // Upload to Sheet3
  if (GSheet.values.append(&response, spreadsheetId, "Sheet3!A2", &valueRange)) {
    Serial.println("Data uploaded to Google Sheets");
    sessionCount = 0;  // Clear stored sessions after upload
  } else {
    Serial.println("Google Sheets upload failed: " + String(GSheet.errorReason()));
  }
}

void setup() {
  Serial.begin(115200);
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(0, 20, "Starting...");
  u8g2.sendBuffer();
  
  // Initialize BLE scanning
  Serial.println("Starting BLE scanning...");
  BLEDevice::init("ESP32_BLE_Client");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop() {
  static unsigned long lastHRUpdate = 0;
  const unsigned long hrUpdateInterval = 1000; // Update HR every second
  
  display();

  // Update heart rate during workout sessions
  if (connected && collecting && (millis() - lastHRUpdate >= hrUpdateInterval)) {
    meanHR = updateMeanHR(getHeartRate(), meanHR, alpha);
    lastHRUpdate = millis();
  }

  switch (currentState) {
    case STATE_BLE_SCANNING:
      if (doConnect) {
        if (connectToServer()) {
          currentState = STATE_BLE_CONNECTED;
          Serial.println("BLE Connected");
          newSetStarted = true;  // Prepare for first set
          setCount = 0;          // Reset set counter
        } else {
          Serial.println("Failed to connect to BLE server");
          currentState = STATE_BLE_SCANNING;
        }
        doConnect = false;
      } else {
        // Restart BLE scan if no device found
        static unsigned long lastScan = 0;
        if (millis() - lastScan > 10000) {
          Serial.println("Restarting BLE scan...");
          BLEScan* pBLEScan = BLEDevice::getScan();
          pBLEScan->start(30);
          lastScan = millis();
        }
      }
      break;

    case STATE_BLE_CONNECTED:
      if (bleDisconnected) {
        Serial.println("BLE disconnected, moving to WiFi");
        BLEDevice::deinit();
        currentState = STATE_WIFI_CONNECTING;
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        wifiStartTime = millis();
        bleDisconnected = false;
      }
      break;

    case STATE_WIFI_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected!");
        configTime(0, 0, "pool.ntp.org");  // Configure NTP
        currentState = STATE_GSHEET_UPLOADING;
        gsheetStartTime = millis();
      } 
      else if (millis() - wifiStartTime > WIFI_TIMEOUT) {
        Serial.println("WiFi connection failed, returning to BLE scanning");
        BLEDevice::init("ESP32_BLE_Client");
        BLEScan* pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
        pBLEScan->setActiveScan(true);
        pBLEScan->start(30);
        currentState = STATE_BLE_SCANNING;
      }
      break;

    case STATE_GSHEET_UPLOADING:
      if (!GSheet.ready()) {
        // Initialize Google Sheets client if not already done
        if (!gsheetReady) {
          Serial.println("Initializing Google Sheets client...");
          GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
          gsheetReady = true;
        }
        
        // Check if timeout reached
        if (millis() - gsheetStartTime > GSHEET_TIMEOUT) {
          Serial.println("Google Sheets client timeout, returning to BLE scanning");
          gsheetReady = false;
          currentState = STATE_BLE_SCANNING;
        }
        break;
      }
      
      // Client is ready, upload data
      uploadDataToSheets();
      gsheetReady = false;
      
      // Return to BLE scanning
      Serial.println("Returning to BLE scanning");
      BLEDevice::init("ESP32_BLE_Client");
      BLEScan* pBLEScan = BLEDevice::getScan();
      pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
      pBLEScan->setActiveScan(true);
      pBLEScan->start(30);
      currentState = STATE_BLE_SCANNING;
      break;
  }

  delay(100);
}