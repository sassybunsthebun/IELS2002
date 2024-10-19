//LIBRARIES USED IN THIS SKETCH
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

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

///VARIABLES FOR SERIAL COMMUNICATION WITH AVR128DB48///

AVROutputPin = 21; 

/**
* @brief Initializes Wi-Fi connection and communcation with MQTT broker. Sets callback function for MQTT. 
*/
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  PinMode(AVROutputPin, OUTPUT);
}

/**
* @brief Connects to WiFi. If WiFi is not connected, print dots. If connected, print IP address.  
*/
void setup_wifi() {
  delay(10);
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
/**
* @brief The callback function that prints the message received from Raspberry Pi. 
* Prints every char in the char array and stores it in the String messageTemp. 
*/
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

  //If a message is recieved from raspi, send it to AVR128DB48
  //If a message is received on the topic esp32/output, check if the message is either "vibrate" or "off". 

  if (String(topic) == "esp32/output") {
    if(messageTemp == "vibrate"){
      Serial.println("vibrate");
      DigitalWrite(AVROutputPin, HIGH);

    }
    else if(messageTemp == "off"){
      Serial.println("off");
      DigitalWrite(AVROutputPin, LOW);
    }
  }
}

/**
* @brief Connects to MQTT. If MQTT is not connected, print an error message.  
*/

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
  //to signalize that the char array is over

  long now = millis();
  if (now - loopTimer > 500) {
    if (Serial.available()) {
      int bytesRead = readSerial(); 
      dataFromAVR[bytesRead] = 0;
      client.publish("esp32/output", dataFromAVR); 
    }
    loopTimer = now;
  }
}