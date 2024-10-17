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

// Variable to check time between messages 

long lastMsg = 0;
char dataFromAVR[10]; 

///VARIABLES FOR SERIAL COMMUNICATION WITH AVR128DB48///

//insert variables for serial communication with avr128db48 here//


/**
* @brief Initializes Wi-Fi connection and communcation with MQTT broker. Sets callback function for MQTT. 
*/
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
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
    if(messageTemp == "on"){
      Serial.println("on"); //change to sending data via serial to AVR128DB48
    }
    else if(messageTemp == "off"){
      Serial.println("off");
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
* @return dataFromAVR
*/

String readSerialData() {

  if(Serial.available()){
    int dataFromAVR = Serial.read();
  }
  return dataFromAVR;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //Sends data from AVR to Raspberry pi 3

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    client.publish("esp32/output", dataFromAVR); 
  }
}