#stolen from https://medium.com/@potekh.anastasia/a-beginners-guide-to-mqtt-understanding-mqtt-mosquitto-broker-and-paho-python-mqtt-client-990822274923
import sys
import paho.mqtt.client as paho

client = paho.Client()

if client.connect("localhost, 1883, 60") != 0: 
    print("Couldn't connect to the mqtt broker")
    sys.exit(1)

client.publish("test_topic", "Hi, paho mqtt client works fine!", 0)
client.disconnect()

