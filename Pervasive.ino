#include <SPI.h>
#include <MFRC522.h>
#include <ESP_WiFiManager.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#define RST_PIN         D3          // Pin for RFID reset pin
#define SS_PIN          D4         // Pin for SS RFID pin
#define RELAY_PIN       D8        // the Arduino pin, which connects to the IN pin of relay

MFRC522 mfrc522(SS_PIN, RST_PIN);

bool RFID_SETUP_CHECK(){
  Serial.println("Open");
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(5);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  bool isReady = mfrc522.PCD_PerformSelfTest();
  SPI.end();
  if(isReady){
    Serial.println("RFID IS READY TO USE");
    return true;
  } else {
    Serial.println("RFID IS NOT READY TO USE, CHECK CONNECTION");
    return false;
  }
}

void RFID_SETUP(){
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
}

bool check_RFID(){
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }
  Serial.println("RFID Present");
  return 1;
}

String read_RFID(){
  Serial.println("Reading RFID UID");
  String userid;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i], HEX); 
    userid += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.print("User UID: ");
  Serial.println(userid);
  SPI.end();
  return userid;
}

/*int POST_API (String uid, String user_status) {
  Serial.println("Begin API POST");
  HTTPClient http;
  http.begin("http://192.168.0.10:7070/api/userLog/albertque"); 
  http.addHeader("Content-Type", "application/json");
  Serial.println("UID: "+uid);
  Serial.println("Status: "+user_status);
  int httpResponseCode = http.POST("{\"uid\": \""+uid+"\",\"status\": \""+user_status+"\"}");
  Serial.println("StatusCode: "+httpResponseCode);
  if(httpResponseCode == 200) Serial.println("API POST OK, Response is 200");
  else if(httpResponseCode == 400) Serial.println("API POST OK, Status NOT OK, Response is 400");
  else Serial.println("API POST IS NOT OK, CHECK IP OR SERVER");
  return httpResponseCode;
}*/

void showBuzzer(){
  pinMode(D5, OUTPUT);
  digitalWrite(D5, HIGH);
  delay(1000);
  digitalWrite(D5,LOW);
}

void UnlockNormal(){
  pinMode(D6, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // unlock the door
  Serial.println("Door's Opened");
  delay(5000);
  digitalWrite(RELAY_PIN, HIGH);  // lock the door
  Serial.println("Door's Locked");
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin D5 as an output.
  pinMode(RELAY_PIN, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(RELAY_PIN, HIGH); // lock the door
  delay(5000);
  digitalWrite(RELAY_PIN, LOW);  // unlock the door
  delay(5000);
}
