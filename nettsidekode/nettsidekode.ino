#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

///Network credentials
const char* ssid = "NTNU-IOT";
const char* password = "";

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>AtB Reisehjelp</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body {color: white; background-color:#37424A;}
</style>
</head>
<body>
  <h2 style="color:#A2Ad00;">AtB Reisehjelp</h2>
    <form action="/get">
      Linje: <input type="text" style="background-color:black; color: white;" name="input1">
      <input type="submit" style="background-color:#A2Ad00; color: black;" value="Submit">
    </form><br>
    <form action="/get">
      Retning til linje: <input type="text" style="background-color:black; color: white;" name="input2">
      <input type="submit" style="background-color:#A2Ad00; color: black;" value="Submit">
    </form><br>
    <form action="/get">
      Endestopp: <input type="text"  style="background-color:black; color: white;" name="input3">
      <input type="submit" style="background-color:#A2Ad00; color: black;" value="Submit">
    </form>
  </body>
  </html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
    }
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  
}