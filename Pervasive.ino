#include <SPI.h>
#include <MFRC522.h>
#include <ESP_WiFiManager.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define RST_PIN         D3          // Pin for RFID reset pin
#define SS_PIN          D4         // Pin for SS RFID pin
#define RELAY_PIN       D6        // The Arduino pin, which connects to the IN pin of relay
#define BUZZER_PIN      D5       // Pin for Buzzer INPUT pin
#define BUTTON_PIN      D2      // Pin for Button

MFRC522 mfrc522(SS_PIN, RST_PIN);
WiFiClientSecure espClient;
PubSubClient client(espClient);
ESP_WiFiManager wifiManager;

// MQTT Credentials
const char *mqtt_broker = "54d8ff23e4294bcc971994bb72f83839.s1.eu.hivemq.cloud";
const char *topic = "node/check";
const char *mqtt_username = "smartlock";
const char *mqtt_password = "Smartlock_pervasive2021";
const int mqtt_port = 8883;

bool MQTT_SETUP(){
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  
  String client_id = "esp8266-client-";
  client_id += String(WiFi.macAddress());
  Serial.printf("The client %s connects to the mqtt broker\n", client_id.c_str());
  if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
	  Serial.println("connected");      
    client.subscribe(topic);
	  return 1;      
  }else {
    Serial.print("failed with state ");
    Serial.print(client.state());
    return 0;
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
      Serial.print((char) payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}

// Setup check for RFID
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
  bool buttonPin = digitalRead(BUTTON_PIN)
  if(buttonPin == 1){
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

int POST_API (String uid, unsigned int nodeID) {
  Serial.println("Begin API POST");
  HTTPClient http;
  //http.begin("http://192.168.0.10:7070/api/userLog/albertque");
  http.addHeader("Content-Type", "application/json");
  Serial.println("UID: "+uid);
  Serial.println("nodeID: "+nodeID);
  int httpResponseCode = http.POST("{\"uid\":\""+uid+"\"}");
  Serial.println("StatusCode: "+httpResponseCode);
  if(httpResponseCode == 200) Serial.println("API POST OK, Response is 200");
  else if(httpResponseCode == 400) Serial.println("API POST OK, Status NOT OK, Response is 400");
  else Serial.println("API POST IS NOT OK, CHECK IP OR SERVER");
  return httpResponseCode;
}

void soundBuzzer() { // sound the buzzer indicating that user identification is valid or button is pressed
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);
}

void soundBuzzerDeny() { // sound the buzzer indicating that user identification is denied
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(1500);
  digitalWrite(BUZZER_PIN, LOW);
}

void unlockNormal() { // Normal Operation (LOW Trigger)
  pinMode(D6, OUTPUT);
  soundBuzzer();
  digitalWrite(RELAY_PIN, LOW); // unlock the door
  Serial.println("Door's Opened");
  delay(5000);
  digitalWrite(RELAY_PIN, HIGH);  // lock the door
  Serial.println("Door's Locked");
  soundBuzzer();
}

void lockDoor(){  //For locking the door
  pinMode(D6, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // lock the door
  Serial.println("Door's Locked");
}

bool checkButton(){ // function that checks the button if it is pressed or not
  pinMode(BUTTON_PIN, INPUT);
  bool check = digitalRead(BUTTON_PIN);
  if (check == 1) {
    return 1;
  } else {
    return 0;
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  delay(1000);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  bool connect = wifiManager.autoConnect("RLS-Library", "libraryumn"); // Wifi Manager Setup
  if (!connect) {
    Serial.println("Wi-Fi Failed to Connect");// Check Wi-Fi Connection
    return;
  }else{
    Serial.println("Wi-Fi Connected");
    espClient.setInsecure();
    
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
  bool mqttStatus = MQTT_SETUP();
  if(mqttStatus == false) {
    while(mqttStatus == false) {
      mqttStatus = MQTT_SETUP();
      delay(1000);
      Serial.print("mqttStatus: ");
      Serial.println(mqttStatus);
    }
  }
}

// the loop function runs over and over again forever
void loop() {
  lockDoor(); //Lock the door (Normal State)
  Serial.println("Check for RFID DATA");
  bool RFID_check = check_RFID();
  Serial.println("Check for Button Status");
  bool Button_check = checkButton();
  
  if(RFID_check == 0){
    if(Button_check == 1){ //Check if user pushes the button
      Serial.println("Button Pressed");
      soundBuzzer(); 
      unlockNormal();
      soundBuzzer();
      return;
    }
  }else if(RFID_check == 1){
    String RFID_UID = read_RFID();
    unsigned int nodeID = 1; // For multi-nodes operation
    Serial.println("RFID data received, continue to post");
    int HTTPresponseCode = POST_API(RFID_UID, nodeID); //API POST
    if(HTTPresponseCode == 200){
      unlockNormal();
    }else if(HTTPresponseCode == 400){
      soundBuzzerDeny();
      lockDoor();
      return;
    }
  }else{
    Serial.println("RFID_check Malfunction");
    return;s
  }
}
