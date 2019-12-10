/*
  This Unit connects to GRVOE B on M5Core.
  hx711 library fork from https://github.com/aguegu/ardulibs/tree/master/hx711
 
  20190920 sito LINEThingsの自動通信機能を追加 
*/


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <M5Stack.h>
#include "hx711.h"
HX711 scale(36, 26);// GROVE B

float Offset = 250000;
float Scale = 67.4;

//------------------
// LINE Things from 
//------------------

// Device Name: Maximum 30 bytes 
#define DEVICE_NAME "M5Stack"
// あなたのサービスUUIDを貼り付けてください
#define USER_SERVICE_UUID "xxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" //書き換える
// Notify UUID: トライアル版は値が固定される
#define NOTIFY_CHARACTERISTIC_UUID "62FBD229-6EDD-4D1A-B554-5C4E1BB29169"
// PSDI Service UUID: トライアル版は値が固定される
#define PSDI_SERVICE_UUID "E625601E-9E55-4597-A598-76018A0D293D"
// PSDI CHARACTERISTIC UUID: トライアル版は値が固定される
#define PSDI_CHARACTERISTIC_UUID "26E2B12B-85F0-4F3F-9FDD-91D114270E6E"

BLEServer* thingsServer;
BLESecurity* thingsSecurity;
BLEService* userService;
BLEService* psdiService;
BLECharacteristic* psdiCharacteristic;
BLECharacteristic* notifyCharacteristic;

bool deviceConnected = false;
bool oldDeviceConnected = false;

class serverCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
   deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};


void setup() {
  M5.begin();

  BLEDevice::init("");
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);

  // Security Settings
  BLESecurity *thingsSecurity = new BLESecurity();
  thingsSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  thingsSecurity->setCapability(ESP_IO_CAP_NONE);
  thingsSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  setupServices();
  startAdvertising();

  Serial.begin(115200);
  scale.setOffset(Offset);
  scale.setScale(Scale); 

  M5.Lcd.clear(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(50, 10);
  M5.Lcd.print("UNIT_WEIGHT \n");
  M5.Lcd.setCursor(15, 50);
  M5.Lcd.print("Cola-Yamamoto System ");
  M5.Lcd.setCursor(0, 90);
  M5.Lcd.print("The weight: ");
}

float weight;
float weight_display;

void loop(){
  M5.update();
  Serial.println(scale.averageValue());
  weight = 0;
  weight = ((float)((int)((scale.getGram()+0.005)*100)))/100;
  weight_display = weight*-1;
  // weight_display = weight;
  Serial.println(weight_display);


  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(YELLOW);

  if (weight_display < 200) {
    
    // LINE Botに紐づくWebhookにデータ送信
    const char *newValue=((String)weight).c_str();
    notifyCharacteristic->setValue(newValue);
    notifyCharacteristic->notify();

    M5.Lcd.clear(YELLOW);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(BLACK);
    M5.Lcd.setCursor(50, 10);
    M5.Lcd.print("UNIT_WEIGHT \n");
    M5.Lcd.setCursor(15, 50);
    M5.Lcd.print("Cola-Yamamoto System ");
    M5.Lcd.setCursor(0, 90);
    M5.Lcd.print("The weight: ");

    M5.Lcd.setCursor(150, 90);
    M5.Lcd.print(weight_display);//調整
    M5.Lcd.setCursor(0, 120);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.print("short !!");
    delay(5000); //5秒
    // delay(3600000); //1時間

  } else{
    M5.Lcd.clear(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.setCursor(50, 10);
    M5.Lcd.print("UNIT_WEIGHT \n");
    M5.Lcd.setCursor(15, 50);
    M5.Lcd.print("Cola-Yamamoto System ");
    M5.Lcd.setCursor(0, 90);
    M5.Lcd.print("The weight: ");

    M5.Lcd.setCursor(150, 90);
    M5.Lcd.print(weight_display);//調整
    delay(500); 
  }
  
}


void setupServices(void) {
  // Create BLE Server
  thingsServer = BLEDevice::createServer();
  thingsServer->setCallbacks(new serverCallbacks());

  // Setup User Service
  userService = thingsServer->createService(USER_SERVICE_UUID);

  // Notifyセットアップ
  notifyCharacteristic = userService->createCharacteristic(NOTIFY_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  notifyCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  BLE2902* ble9202 = new BLE2902();
  ble9202->setNotifications(true);
  ble9202->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  notifyCharacteristic->addDescriptor(ble9202);

  // Setup PSDI Service
  psdiService = thingsServer->createService(PSDI_SERVICE_UUID);
  psdiCharacteristic = psdiService->createCharacteristic(PSDI_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);
  psdiCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

  // Set PSDI (Product Specific Device ID) value
  uint64_t macAddress = ESP.getEfuseMac();
  psdiCharacteristic->setValue((uint8_t*) &macAddress, sizeof(macAddress));

  // Start BLE Services
  userService->start();
  psdiService->start();
}

void startAdvertising(void) {
  // Start Advertising
  BLEAdvertisementData scanResponseData = BLEAdvertisementData();
  scanResponseData.setFlags(0x06); // GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
  scanResponseData.setName(DEVICE_NAME);

  thingsServer->getAdvertising()->addServiceUUID(userService->getUUID());
  thingsServer->getAdvertising()->setScanResponseData(scanResponseData);
  thingsServer->getAdvertising()->start();
}

