//ESP related
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>

//One Wire DS18B20 related
#include <OneWire.h>
#include <DallasTemperature.h>

//Parameters for thermal 2-state regulator
#define RELAY_PIN D5
#define FAN_OFF_TEMP 45
#define HISTERESIS 10
#define OVERSHOT 1.5
#define WIFI_ACTIVE_TIME 15*60*1000
#define RESET_ACTIVE_TIME 30*1000

OneWire  ds(D6);
DallasTemperature sensors(&ds);

ESP8266WebServer server(80);

bool ota_flag = true;
bool hist_flag = false;

uint16_t time_elapsed = 0;
String data = "";
String system_status = "OK";

void setup() {
//  pinMode(2, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
    
  sensors.begin();
  
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  delay(100);

  IPAddress localIp(192,168,1,1);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);

  WiFi.softAPConfig(localIp, gateway, subnet);
  WiFi.softAP("IOT_Thermometer");
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/restart",[](){
    server.send(200,"text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
  });

  
  server.on("/flash",[](){
    server.send(200,"text/plain", "Ready to flash...");
    ota_flag = true;
    time_elapsed = 0;
  });

  server.on("/", [](){
    server.send(200,"text/plain", "Temperature is: " + data + " System status: " + system_status);
  });

  server.begin();
}

void loop() {
  if(ota_flag)
  {
    uint16_t time_start = millis();
    while(time_elapsed < RESET_ACTIVE_TIME)
    {
      ArduinoOTA.handle();
      time_elapsed = millis()-time_start;
      delay(10);
    }
    ota_flag = false;
  }
  server.handleClient();

  if(millis()>WIFI_ACTIVE_TIME)
  {
  WiFi.disconnect(true); delay(5); // disable WIFI altogether
  WiFi.mode(WIFI_OFF); delay(5);
  WiFi.forceSleepBegin(); delay(5);
  }

/*One Wire */
//#################################################
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.println(sensors.getTempCByIndex(0));
  data = sensors.getTempCByIndex(0);
//#################################################
/* One Wire */  
//  if(data.toInt() < 30) digitalWrite(2, !digitalRead(2));

  //Check if temp reached it highest acceptable point and turn on fans
  if((data.toInt() > FAN_OFF_TEMP + HISTERESIS)&&(hist_flag==false))
    {
    digitalWrite(RELAY_PIN, true);
    hist_flag = true;
    system_status = "FANS ON"; 
    }
    
  //Turn off fans and reset hist flag responsible for controll if fans are working
  if(data.toInt() < FAN_OFF_TEMP)
    {
    digitalWrite(RELAY_PIN, false);
    hist_flag = false;
    system_status = "FANS OFF";       
    }
  //Turn off fans if temp reached maximum overshot temperature , set system status ERROR 
  if((data.toInt() > FAN_OFF_TEMP + OVERSHOT * HISTERESIS)&&(hist_flag==true))
    {
    digitalWrite(RELAY_PIN, false);
    hist_flag = true;
    system_status = "ERROR"; 
    }

//  delay(1000);
}
