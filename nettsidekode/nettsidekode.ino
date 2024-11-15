//LIBRARIES USED IN THIS SKETCH
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

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

///VARIABLES FOR GPS///

char* finalGPSCoordinates;

///VARIABLES FOR WEBSITE///

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";

// HTML web page to handle 3 input fields (Linje, Retning til linje, Endestopp)
const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8"/>
    <title>AtB Reisehjelp</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <style>
      body {
        color: white;
        background-color: #37424a;
        font-family: verdana;
      }
    </style>
  </head>
  <body>
    <h2 style="color: #a2ad00; font-family: verdana">AtB Reisehjelp</h2>
    <form action="/get">
      <table>
      <tr>
        <td>Linje: </td>
        <td><input
          type="text"
          style="background-color: black; color: white; border: 2px solid #007c92"
          name="input1"
          required
        /></td>
      </tr>

      <tr>
        <td>Retning: </td>
        <td><input
          type="text"
          style="background-color: black; color: white; border: 2px solid #007c92"
          name="input2"
          required
        /></td>
      </tr>

      <tr>
        <td>Endestopp: </td>
        <td><input
          type="text"
          style="background-color: black; color: white; border: 2px solid #007c92"
          name="input3"
          required
        /></td>
      </tr>
      </table>
    <br />
      <button
        type="submit"
        style="
          background-color: #007c92;
          color: white;
          font-family: verdana;
        ">
        Finn nærmeste buss!
      </button>
    </form>

  </body>
</html>)rawliteral";

/**
* @brief Initializes the 404 not found page
*/

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

/**
* @brief Initializes Wi-Fi connection and communcation with MQTT broker. Sets callback function for MQTT. 
*/
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  setup_wifi();

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

 // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessageLinje; 
    String inputMessageRetning;
    String inputMessageEndestopp; 
    // GET Linje value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessageLinje = request->getParam(PARAM_INPUT_1)->value(); 
    } else {
      Serial.println("Error: No input on field one");
      request->send(400, "text/html", "Missing input on field 1 <br><a href=\"/\">Return to Home Page</a>");
      return;
    }
    // GET Retning value on <ESP_IP>/get?input2=<inputMessage>
    if (request->hasParam(PARAM_INPUT_2)) {
      inputMessageRetning = request->getParam(PARAM_INPUT_2)->value();
    } else {
      Serial.println("Error: No input on field two");
      request->send(400, "text/html", "Missing input on field 2 <br><a href=\"/\">Return to Home Page</a>");
      return;
    }
    // GET Endestopp value on <ESP_IP>/get?input3=<inputMessage>
    if (request->hasParam(PARAM_INPUT_3)) {
      inputMessageEndestopp = request->getParam(PARAM_INPUT_3)->value(); 
    } else {
      Serial.println("Error: No input on field three");
      request->send(400, "text/html", "Missing input on field 3 <br><a href=\"/\">Return to Home Page</a>");
      return;
    }
    Serial.print(inputMessageLinje);
    Serial.print(inputMessageRetning);
    Serial.print(inputMessageEndestopp);
    request->send(200, "text/html", "HTTP GET request sent to your AtB Reisehjelpdings <br><a href=\"/\">Return to Home Page</a> Tap back arrow to not lose your input information :)");
  });
  
  server.onNotFound(notFound);
  server.begin();
  
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