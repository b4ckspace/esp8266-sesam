#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <SimpleTimer.h>
#include <SSD1306.h>
#include <SSD1306Ui.h>
#include "static.h"
#include "settings.h"

const PROGMEM char text_html[] = "text/html";
const PROGMEM char text_css[] = "text/css";

////////////////////////////////////////////////////////
// Awesome code....

ESP8266WebServer server(80);
SimpleTimer timer;
WiFiClient wifiClient;

SSD1306   display(0x3c, D3, D4);
SSD1306Ui ui     ( &display );

typedef struct {
  const char* name;
  const char* route;
  uint8_t pin;
  uint16_t holdMs;
} t_endpoint;

t_endpoint endpoints[] = {
  { "Garagentor", "/relais/garageDoor", D7, 1000 },
  { "Haustür", "/relais/frontDoor",  D6, 1000 }
};

void setup() {
  delay(1000);
  Serial.begin(115200);

  
  // initialize dispaly
  display.init();
  display.clear();
  display.display();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);
    
  Serial.println();
  Serial.print("Configuring access point...");


  WiFi.hostname(HOSTNAME_ESP);
  WiFi.mode(WIFI_MODE);

 
  if (WiFi.SSID() != ssid) {
    WiFi.begin(ssid, password);
    WiFi.persistent(true);
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\n");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  display.clear();
  display.drawString(64, 10, "Client running");
  display.drawString(64, 20, WiFi.localIP().toString());  
  display.display();
    

  
  server.on("/", []() {
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    
    server.send_P(200, text_html, static_index_html);
  });


  server.on("/relais", []() {
    DynamicJsonBuffer jsonBuffer;
    
    JsonObject& root = jsonBuffer.createObject();
    JsonArray& jsonEndpoints = root.createNestedArray("relais");
    
    for(t_endpoint endpoint : endpoints) {
      JsonObject& entry = jsonBuffer.createObject();
      entry["name"] = endpoint.name;
      entry["url"] = endpoint.route;
    
      jsonEndpoints.add(entry);
    }

    String response = "";
    root.printTo(response);

    server.send(200, "application/json", response);
  });

  for(t_endpoint endpoint : endpoints) {

    pinMode(endpoint.pin, OUTPUT);
    
    server.on(endpoint.route, [endpoint]() {
      digitalWrite(endpoint.pin, HIGH);
      
      timer.setTimeout(endpoint.holdMs, [endpoint]() {
        digitalWrite(endpoint.pin, LOW);
      });   
  
      server.sendHeader("Pragma", "no-cache");
      server.sendHeader("Expires", "0");
      server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");

      server.send(200, "text/plain", "OK");
    });
  }
    
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  timer.run();
}
