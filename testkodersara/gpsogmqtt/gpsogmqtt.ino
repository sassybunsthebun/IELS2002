//LIBRARIES USED IN THIS SKETCH
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

///HOW TO WIRE THE GPS///

// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// Connect the GPS TX (transmit) pin to pin 16
// Connect the GPS RX (receive) pin to pin 17

///VARIABLES FOR THE GPS///

SoftwareSerial mySerial(16, 17); //We use the software serial pins to read from serial.
Adafruit_GPS GPS(&mySerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  false

uint32_t timer = millis();

///VARIABLES FOR MQTT COMMUNICATION///

//Wi-Fi connection and MQTT broker (mosquitto on Rasberry pi 3)

const char* ssid = "NTNU-IOT";
const char* password = "";

const char* mqtt_server = "10.25.18.93";

WiFiClient espClient;
PubSubClient client(espClient);

// Variables for data sent to AVR128DB48

long loopTimer = 0;
char dataFromAVR[128];

///VARIABLES FOR GPS//

char* finalGPSCoordinates;

/**
* @brief Initializes Wi-Fi connection and communcation with MQTT broker. Sets callback function for MQTT. 
*/
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  delay(1000);
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  mySerial.println(PMTK_Q_RELEASE);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT
  //If a message is recieved from raspi, send it to AVR128DB48
  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") { //esp incoming from pi
    if(messageTemp == "vibrate"){
      //Serial.println("vibrate"); //change to sending data via serial to AVR128DB48
      digital.write(2, HIGH); // send a "high" to AVR128DB48
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/**
* @brief Reads serial data from the AVR128DB48 and stores it in a char array. 
* @return bufferIndex, length of incoming data which is stored in the char array. 
*/

int readSerial() {
  int byteFromSerial = Serial.read();
  int bufferIndex = 0;
  while (byteFromSerial != -1 && bufferIndex < sizeof(dataFromAVR) - 1) {
    dataFromAVR[bufferIndex] = byteFromSerial;
    ++bufferIndex;
    byteFromSerial = Serial.read();
  }
  return bufferIndex;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //Every 500 milliseconds, read the data from AVR128DB48 and add a zero at the end of the char array

  long now = millis();
  if (now - loopTimer > 500) {
    if (Serial.available()) {
      int bytesRead = readSerial(); 
      dataFromAVR[bytesRead] = 0;
      client.publish("esp32/output", dataFromAVR); 
    }
    loopTimer = now;
  }

  ///CODE FOR THE GPS///
  
  char c = GPS.read();
  if ((c) && (GPSECHO))
    Serial.write(c);

  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA()))  
      return;
  }
  // approximately every 2 seconds or so, print out the current stats
  if (millis() - timer > 2000) {
    timer = millis(); // reset the timer

    Serial.print("Fix: "); Serial.print((int)GPS.fix);
    Serial.print(" quality: "); Serial.println((int)GPS.fixquality);
    if (GPS.fix) {
      Serial.print("Location: ");
      Serial.print(GPS.latitudeDegrees);
      Serial.print(", ");
      Serial.print(GPS.longitudeDegrees);

      char* GPSlat;
      char* GPSlon;
      dtostrf(GPS.latitudeDegrees, 1, 2, GPSlat);
      dtostrf(GPS.longitudeDegrees, 1, 2, GPSlon);
      finalGPSCoordinates = strcat(finalGPSCoordinates, GPSlat);
      finalGPSCoordinates = strcat(finalGPSCoordinates, ",");
      finalGPSCoordinates = strcat(finalGPSCoordinates, GPSlon);
      client.publish("esp32/output", finalGPSCoordinates);
    }
  }
}