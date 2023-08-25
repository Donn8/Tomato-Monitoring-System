#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <DHT.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Define NTP Client to get time (All of this is needed to have a correct timestamp format)
WiFiUDP ntpUDP;     
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Define the WiFi credentials
#define WIFI_SSID "realme 10"
#define WIFI_PASSWORD "i6tasip3"

// Define Sensors and Actuators
#define DHTPIN 4
#define DHTTYPE DHT22
#define Fan1 0
#define Fan2 13
DHT dht(DHTPIN, DHTTYPE);

// define LED pin
#define LED_PIN 2
// define time interval in milliseconds
#define INTERVAL 43200000 // 12 hours in milliseconds

#define soilMoisturePin A0
#define waterPumpPin 16
int soilMoistureValue;

// Define the Firebase credentials
#define API_KEY "AIzaSyBhXcw-A8i8M_rd1dp0PRqR4qhRd7DNZKw"
#define FIREBASE_PROJECT_ID "thesistometer"
#define USER_EMAIL "sychris0800@gmail.com"
#define USER_PASSWORD "123456789"

// Create an instance of the FirebaseData class
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

String uid;
String monitoringPath = "Monitoring/ORYKnhqk7pBTxVxvRzBN";
String recordsPath = "MonitoringRecords/XSZPr53wonitDgnu0ueg";
String testingPath = "TestingPurposes";
String plantLevelPath = "addplantslider/JjKxbd9cnvhq15NRSAVN";
String growthlevel = "plantlevel";
String actuatorstatePath = "actuatorstate/dO8Ti65BzN0hjJWepqqn";


//Define Parameters' Threshold
float minTemp = 0;
float maxTemp = 0;
float minHum = 50;
float maxHum = 70;
int minSM = 60;
int maxSM = 80;

// variables to store last LED state and time
int lastState = HIGH;
unsigned long lastTime = 0;

//Storing Actuators' State
bool LEDState = false;
bool Fan1State = false;
bool Fan2State = false;
bool PumpState = false;

void setup(){
// Initialize the serial port
  Serial.begin(9600);
  timeClient.begin();

  pinMode(LED_PIN, OUTPUT);
  pinMode(waterPumpPin, OUTPUT);
  pinMode(Fan1, OUTPUT);
  pinMode(Fan2, OUTPUT);
  digitalWrite(LED_PIN, lastState);
  
  // Initialize the WiFi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

//  delay(1000);
  
  // Assign the api key and credentials
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  // Set Firebase authentication credentials
  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);

  Serial.println("Getting User UID");
  while ((auth.token.uid) == ""){
    Serial.print('.');
  delay(1000);
  }
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  dht.begin();
}

void loop() {
  FirebaseJson content;
  FirebaseJson actuators;
  FirebaseJsonData result;

//delay(2000);
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // get current time
  unsigned long currentTime = millis();

  if(Firebase.Firestore.getDocument(&firebaseData, FIREBASE_PROJECT_ID, "", plantLevelPath.c_str(), growthlevel.c_str())){
    Serial.printf("ok\n%s\n\n",firebaseData.payload().c_str());
    //Create FirebaseJson object and set content with received payload
    FirebaseJson payload;
    payload.setJsonData(firebaseData.payload().c_str());
    //Get data from FirebaseJson object
    payload.get(result, "fields/plantlevel/integerValue", true);
    int plantLevel = result.intValue;

    if (plantLevel == 1){
    Serial.println("Plant Level: 1");
      minTemp = 22;
      maxTemp = 29;
    }else if (plantLevel == 2){
      Serial.println("Plant Level: 2");
      minTemp = 22;
      maxTemp = 25;
    }else if (plantLevel == 3){
      Serial.println("Plant Level: 3");
      minTemp = 25;
      maxTemp = 29;
    }else if (plantLevel == 4){
      Serial.println("Plant Level: 4");
      minTemp = 21;
      maxTemp = 25;
    }else if (plantLevel == 5){
      Serial.println("Plant Level: 5");
      minTemp = 20;
      maxTemp = 25;
    }
  }
  Serial.print("Min Temp: ");
  Serial.println(minTemp);
  Serial.print("Max Temp: ");
  Serial.println(maxTemp);
  Serial.print("Min Humidity: ");
  Serial.println(minHum);
  Serial.print("Max Humidity: ");
  Serial.println(maxHum);
  Serial.print("Min Soil Moisture: ");
  Serial.println(minSM);
  Serial.print("Max Soil Moisture: ");
  Serial.println(maxSM);
  Serial.println();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
 //   return;
  } else{
  //SENSOR READINGS
  Serial.println("SENSOR READINGS");
  Serial.print(F("Temperature: "));
  Serial.print(temperature);
  Serial.println(" °C");
  content.set("fields/temperature/stringValue", String(temperature).c_str());
  Serial.print(F("Humidity: "));
  Serial.print(humidity);
  Serial.println("%");
  content.set("fields/humidity/stringValue", String(humidity).c_str());


  soilMoistureValue = analogRead(soilMoisturePin);
  soilMoistureValue = map(soilMoistureValue, 1024, 520, 0, 100);
  Serial.print("Soil Moisture: ");
  Serial.print(soilMoistureValue);
  Serial.println("%");
  content.set("fields/soilmoisture/stringValue", String(soilMoistureValue).c_str());
  }
  // Set DateTime field with current time
  timeClient.update();
  time_t now = timeClient.getEpochTime();
  if (now == 0) {
    Serial.println("Error getting current time");
  } else {
    // Set timestamp field with current time
    char timestampStr[30];
    strftime(timestampStr, sizeof(timestampStr), "%FT%TZ", gmtime(&now));
    content.set("fields/DateTime/timestampValue", timestampStr);
  }

  /* UPDATING MONITORING and ACTUATORSTATE COLLECTION*/
  std::vector<struct fb_esp_firestore_document_write_t> writes;
    //A write object that will be written to the document.
    struct fb_esp_firestore_document_write_t update_write;

    //Set the write object write operation type.
    update_write.type = fb_esp_firestore_document_write_type_update;
    //Set the update document content
    update_write.update_document_content = content.raw();
    //Set the update document path
    update_write.update_document_path = monitoringPath.c_str();

    //Set the document mask field paths that will be updated
    update_write.update_masks = "DateTime,temperature,humidity,soilmoisture";

    //Set the precondition write on the document.
    update_write.current_document.exists = "true";

    //Add a write object to a write array.
    writes.push_back(update_write);

//  delay(1000);

  if (temperature <= minTemp || humidity <= minHum){
    digitalWrite(Fan1, HIGH);
    Fan1State = digitalRead(Fan1);
    actuators.set("fields/fan1/booleanValue", Fan1State % 2 == 0);
    Serial.println("Fan 1: ON!");

  }else {
    digitalWrite(Fan1, LOW);
    Fan1State = digitalRead(Fan1);
    actuators.set("fields/fan1/booleanValue", Fan1State % 2 == 0);
    Serial.println("Fan 1: OFF!");

    }
  if (temperature >= maxTemp || humidity >= maxHum) {
    digitalWrite(Fan2, HIGH);
    Fan2State = digitalRead(Fan2);
    actuators.set("fields/fan2/booleanValue", Fan2State % 2 == 0);
    Serial.println("Fan 2: ON!");

  }else {
    digitalWrite(Fan2, LOW);
    Fan2State = digitalRead(Fan2);
    actuators.set("fields/fan2/booleanValue", Fan2State % 2 == 0);
    Serial.println("Fan 2: OFF!");

    } 

  //Soil Moisture conditional statements
  if (soilMoistureValue <= minSM) {
    digitalWrite(waterPumpPin, HIGH);
    PumpState = digitalRead(waterPumpPin);
    actuators.set("fields/waterpump/booleanValue", PumpState % 2 == 0);
    Serial.println("PUMP: ON");
  } else if (soilMoistureValue > maxSM) { 
    digitalWrite(waterPumpPin, LOW);
    PumpState = digitalRead(waterPumpPin);
    actuators.set("fields/waterpump/booleanValue", PumpState % 2 == 0);
    Serial.println("PUMP: OFF");
  }
  
  //LED (12 hours running)
  // check if it's time to change LED state
  Serial.print("Current Time: ");
  Serial.println(currentTime);
  Serial.print("Last Time: ");
  Serial.println(lastTime);

  if (currentTime - lastTime >= INTERVAL) {
    // toggle LED state
    if (lastState == LOW) {
      digitalWrite(LED_PIN, HIGH);
      LEDState = digitalRead(LED_PIN);
      actuators.set("fields/bulb/booleanValue", LEDState % 2 == 0);      
      Serial.println("BULB: ON");
      lastState = HIGH;
    } else {
      digitalWrite(LED_PIN, LOW);
      LEDState = digitalRead(LED_PIN);
      actuators.set("fields/bulb/booleanValue", LEDState % 2 == 0);
      Serial.println("BULB: OFF");
      lastState = LOW;
    }
    // update last time
    lastTime = currentTime;
  } else{
    LEDState = digitalRead(LED_PIN);
    Serial.print("BulbState: ");
    Serial.println(LEDState ? "OFF" : "ON");
    actuators.set("fields/bulb/booleanValue", LEDState % 2 == 0);

    //update last time
    lastTime = currentTime;
  }
  
  //Update Actuators' State
    struct fb_esp_firestore_document_write_t update_actWrite;

    update_actWrite.type = fb_esp_firestore_document_write_type_update;

    update_actWrite.update_document_content = actuators.raw();
    update_actWrite.update_document_path = actuatorstatePath.c_str();

    //Set the document mask field paths that will be updated
    update_actWrite.update_masks = "fan1,fan2,waterpump,bulb";

    //Set the precondition write on the document.
    update_actWrite.current_document.exists = "true";

    //Add a write object to a write array.
    writes.push_back(update_actWrite);
  
  // Randomize Document ID
  String randomStr = "";
  const char* chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (int i = 0; i < 20; i++) {
    randomStr += chars[random(0, 61)];
  }
  uid = randomStr + String(millis());
  
//  delay(1000);

  /* FOR MONITORING RECORDS/HISTORY*/
  // Create and Send Document to Firestore
  Serial.print("Creating document... ");
// delay(1000);  // delay start for 3 seconds
  Serial.println("Done");
// delay(1000);  // delay start for 3 seconds
  Serial.println("");
  Serial.print("Sending document.... ");

  // Creates the Document and Send to Firestore
 // if (Firebase.Firestore.createDocument(&firebaseData, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, testingPath.c_str(), content.raw()))
  if (Firebase.Firestore.createDocument(&firebaseData, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, recordsPath.c_str(), content.raw()))
    Serial.printf("Success\n%s\n\n", firebaseData.payload().c_str());
  else {
    Serial.print(firebaseData.errorReason());
    }
  
//  delay(1000);

  // Commit Document and Update(Monitoring and Actuator State) Firestore
    if (Firebase.Firestore.commitDocument(&firebaseData, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, writes /* dynamic array of fb_esp_firestore_document_write_t */, "" /* transaction */)){
      Serial.printf("Updated\n%s\n\n", firebaseData.payload().c_str());
    }else{
      Serial.println(firebaseData.errorReason());
    }

}
