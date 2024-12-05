/*
  Denne koden krever en ferdig konfigurert "AVR-IoT Cellular Mini" med SIM-kort og "DxCore" installert.

  Kilde:
  - https://github.com/microchip-pic-avr-solutions/avr-iot-cellular-arduino-library/blob/main/examples/mqtt_custom_broker/mqtt_custom_broker.ino
*/

//inkluderer alle relevante bibliotek
#include <Arduino.h>
#include <ecc608.h>
#include <led_ctrl.h>
#include <log.h>
#include <lte.h>
#include <mqtt_client.h>

//Skriver inn navnet på topicen som AVRen subscriber (lytter) til og topicen AVRen publisher (sender) data til.
#define MQTT_SUB_TOPIC "ESP"
#define MQTT_PUB_TOPIC "testTopic"

//Info som kreves for å koble til brokeren.
#define MQTT_THING_NAME "AVR_cellular_IOT"
#define MQTT_BROKER     "84.214.173.115"
#define MQTT_PORT       1883
#define MQTT_USE_TLS    false
#define MQTT_USE_ECC    false
#define MQTT_KEEPALIVE  60

void setup() {
    //Log er coren sin versjon av Serial.
    Log.begin(115200);

    //starter opp leds på AVRen som signaliserer når enheten sender og mottar data over LTE.
    LedCtrl.begin();
    LedCtrl.startupCycle();

    Log.info(F("Starting MQTT with custom broker"));

    //Kobler til LTE nettverk
    if (!Lte.begin()) {
        Log.error(F("Failed to connect to operator"));

        //Programmet kjører ikke dersom AVRen ikke klarer å koble til LTE.
        while (1) {}
    }

    // Kobler til MQTT broker
    if (MqttClient.begin(MQTT_THING_NAME,
                         MQTT_BROKER,
                         MQTT_PORT,
                         MQTT_USE_TLS,
                         MQTT_KEEPALIVE,
                         MQTT_USE_ECC)) {
        MqttClient.subscribe(MQTT_SUB_TOPIC);
    } else {
        Log.rawf(F("\r\n"));
        Log.error(F("Failed to connect to broker"));

        //Programmet kjører ikke dersom AVRen ikke klarer å koble til MQTT broker.
        while (1) {}
    }
}

//Funksjon som leser knappetrykk og sender tilsvarende ID (som kun er indexen til knappene) sammen
//med koordinater som en string over MQTT.
void btnPub()
{
  for(uint8_t j = 0; j < 9; j++){
    //Liste med knapper, for plassbesparende kode
    static uint8_t pins[] = {PIN_PD1, PIN_PD3, PIN_PD4, PIN_PD5, PIN_PD7, PIN_PB5, PIN_PD0, PIN_PE1, PIN_PE2};

    if(digitalRead(pins[j]) == HIGH){
      String stopp = String("Stopp nr. " + String(j));
      Log.info(stopp.c_str());

      //Siden GPS vi har ikke funker med "DxCore" har vi her for å teste manuelt lagt inn koordinater.
      int32_t coordsX1 = 63;
      int32_t coordsY1 = 10;

      int32_t coordsX2 = 4166630;
      int32_t coordsY2 = 4075721;

      //Publisher data på topic.
      String stoppPUB = String(String(j) + "," + String(coordsX1) + "." + String(coordsX2) + "," + String(coordsY1) + "." + String(coordsY2));
      MqttClient.publish(MQTT_PUB_TOPIC, stoppPUB.c_str());

      //Enkel, men blokkerende debounce.
      while(digitalRead(pins[j]) == HIGH){}
    }
  }
}

//Funksjon som mottar data fra topic over MQTT. Vibrerer så vibrasjonsmotoren dersom dataen oppfyller kravene.
void readSub()
{
  static bool wait = false;
  static int distFromStop = 300;

  //Leser fra topicen
  String message = MqttClient.readMessage(MQTT_SUB_TOPIC);
  
  //Registrerer når data dukker opp
  if (message != "") {
    Log.infof(F("Got new message: %s\r\n"), message.c_str());
    
    //Omgjør data fra topic til int slik at den enkelt kan brukes i betingelsen til if-setninger.
    //Wait-variabelen brukes for å unngå at motoren skal vibrere konstant. Når bussen kjører vekk fra busstoppet
    //hvor brukeren går på bussen vil message-variabelen øke slik at vibrasjonsmotoren igjen vil kunne vibrere.
    int messageNum = message.toInt();
    if(messageNum <= distFromStop && wait == false){
      Log.info("Vibrerer");
      digitalWrite(PIN_PD6, HIGH);
      delay(2000);
      digitalWrite(PIN_PD6, LOW);

      wait = true;
    }

    if(messageNum > distFromStop + 50){
      wait = false;
    }
  }
  //delay(2000);
}

//Loop-funksjon hvor funksjonene kjører.
void loop()
{
  btnPub();
  readSub();
}
