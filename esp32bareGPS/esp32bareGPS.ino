//LIBRARIES USED IN THIS SKETCH
#include <Wire.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <Arduino.h>

///HOW TO WIRE THE GPS///

// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// Connect the GPS TX (transmit) pin to pin 16
// Connect the GPS RX (receive) pin to pin 17

///VARIABLES FOR THE GPS///

SoftwareSerial mySerial(16, 17); //We use the software serial pins to read from serial
//This is because the hardware pins are occupied. 
Adafruit_GPS GPS(&mySerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  false

uint32_t timer = millis();

/// Variables for data sent to AVR128DB48 ///

long loopTimer = 0;
char dataFromAVR[128];

/**
* @brief Initializes Wi-Fi connection and communcation with MQTT broker. Sets callback function for MQTT. Initializes website logic.
*/
void setup() {
  Serial.begin(115200);


  delay(1000);
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  //mySerial.println(PMTK_Q_RELEASE);
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

  //Every 500 milliseconds, read the data from AVR128DB48 and add a zero at the end of the char array

  long now = millis();
  if (now - loopTimer > 500) {
    if (Serial.available()) {
      int bytesRead = readSerial(); 
      dataFromAVR[bytesRead] = 0;
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
    if (GPS.fix) { //Fix is needed to receive GPS data. Otherwise it prints zeros
      char GPSlat[16];
      char GPSlon[16];
      dtostrf(GPS.latitude, 1, 2, GPSlat); //The GPS library has several different types of coordinates to choose from,
      //if this one is incoorect or insufficient, other options are avaliable such as GPS.lat and GPS.latitudeDegrees
      dtostrf(GPS.longitude, 1, 2, GPSlon);
      String combinedCoordinates = String(GPSlat) + "," + String(GPSlon);
      Serial.println(combinedCoordinates.c_str());
    }
    else {
    }
  }
}