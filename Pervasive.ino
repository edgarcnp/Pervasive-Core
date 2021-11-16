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
#define RELAY_PIN       D6        // The Arduino pin, which connects to the IN pin of relay
#define BUZZER_PIN      D5       // Pin for Buzzer INPUT pin
#define BUTTON_PIN      D2      // Pin for Button

MFRC522 mfrc522(SS_PIN, RST_PIN);
ESP_WiFiManager wifiManager;

bool RFID_SETUP_CHECK() {
  Serial.println("Open");
  while (!Serial);                   // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();                      // Init SPI bus
  mfrc522.PCD_Init();              // Init MFRC522
  delay(5);                       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  bool isReady = mfrc522.PCD_PerformSelfTest();
  SPI.end();
  if (isReady) {
    Serial.println("RFID IS READY TO USE");
    return true;
  } else {
    Serial.println("RFID IS NOT READY TO USE, CHECK CONNECTION");
    return false;
  }
}

void RFID_SETUP() {
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
}

bool check_RFID() {
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

bool checkButton(){
  pinMode(BUTTON_PIN, INPUT);
  if(BUTTON_PIN == HIGH){
    return 1;
  }else{
    return 0;
  }
}

String read_RFID() {
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

//int POST_API (String uid) {
//  Serial.println("Begin API POST");
//  HTTPClient http;
//  http.begin("http://192.168.0.10:7070/api/userLog/albertque");
//  http.addHeader("Content-Type", "application/json");
//  Serial.println("UID: "+uid);
//  int httpResponseCode = http.POST("{\"uid\": \""+uid+"\"}");
//  Serial.println("StatusCode: "+httpResponseCode);
//  if(httpResponseCode == 200) Serial.println("API POST OK, Response is 200");
//  else if(httpResponseCode == 400) Serial.println("API POST OK, Status NOT OK, Response is 400");
//  else Serial.println("API POST IS NOT OK, CHECK IP OR SERVER");
//  return httpResponseCode;
// }

void soundBuzzer() { // kalo axel jadi beli paket promo
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);
}

void soundBuzzerDeny() { // kalo axel jadi beli paket promo
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(1500);
  digitalWrite(BUZZER_PIN, LOW);
}

void unlockNormal() { // Normal Operation
  pinMode(D6, OUTPUT);
  soundBuzzer();
  digitalWrite(RELAY_PIN, LOW); // unlock the door
  Serial.println("Door's Opened");
  delay(5000);
  digitalWrite(RELAY_PIN, HIGH);  // lock the door
  Serial.println("Door's Locked");
  delay(5000);
}

void lockDoor(){  //For locking the door
  pinMode(D6, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // lock the door
  Serial.println("Door's Locked");
}

// the setup function runs once when you press reset or power the board
void setup() {
  delay(1000);
  int nodeID = 1; // For multi-nodes operation
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  bool connect = wifiManager.autoConnect("RLS-Library", "libraryumn"); // Wifi Manager Setup
  if (!connect) {
    Serial.println("Wi-Fi Failed to Connect");// Check Wi-Fi Connection
    return;
  }else{
    Serial.println("Wi-Fi Connected");
  }
  bool rfidStatus = RFID_SETUP_CHECK();
  if (rfidStatus == false) {
    Serial.println("RFID SETUP FAILED, CHECK CONNECTION");
    while (rfidStatus == false) {
      rfidStatus = RFID_SETUP_CHECK();
      delay(1000);
      Serial.print("rfidStatus: ");
      Serial.println(rfidStatus);
    }
  }
}

// the loop function runs over and over again forever
void loop() {
  // If card is not present then check button
  // Check RFID UID 
  // POST UID - Check UID @Backend
  // Receive ResponseCode, if(400, UnlockNormal()), if(200, soundBuzzerDeny(), continue)
  while(true){
    //Lock the door continously
    lockDoor();
  
    //Check for RFID Data
    Serial.println("Check for RFID DATA");
    bool RFID_check = check_RFID();
    bool Button_check = checkButton();
    RFID_SETUP();
    
    while(RFID_check == 0 && Button_check == 0){
      RFID_check = check_RFID();
    }
  
    if(Button_check == 1){
     Serial.println("Button Pressed"); //Check if user pushes button
     soundBuzzer(); 
     unlockNormal();
     break;
    }
    
    String RFID_UID = read_RFID();
    Serial.println("RFID data received, continue to post");
  //  int respondCode = POST_API(RFID_UID);
  //  if(respondCode == 200){
  //    soundBuzzer();
  //    unlockNormal();
  //  }else if(respondCode == 400){
  //    soundBuzzerDeny();
  //    lockDoor();
  //    return;
  //  }else{
  //    Serial.println("No Respond Code!");
  //    soundBuzzerDeny();
  //    lockDoor();
  //    return;
  //  }
  }
}
