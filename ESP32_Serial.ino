#include <HardwareSerial.h>
HardwareSerial MySerial(1);

#define RX 1
#define TX 3

void setup()
{
  MySerial.begin(115200, SERIAL_8N1, RX, TX);
}

void recvSerial()
{
  static uint8_t recvMessage;

  while(MySerial.available() > 0){
    recvMessage = MySerial.read();
  }
  Serial.println(recvMessage);
}

void sendSerial()
{
  int msg = 4;
  MySerial.write(msg);
}

void loop()
{
  recvSerial();
}