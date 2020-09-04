//ESP related
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>

//One Wire DS18B20 related
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire  ds(D6);
DallasTemperature sensors(&ds);

ESP8266WebServer server(80);

bool ota_flag = true;
uint16_t time_elapsed = 0;
String data = "";

void setup() {
  pinMode(2, OUTPUT);
  
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
  
//  WiFi.begin(ssid, password);
//  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
//    Serial.println("Connection Failed! Rebooting...");
//    delay(5000);
//    ESP.restart();
//  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

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
    server.send(200,"text/plain", "<h1>Temperature is</h1><h3>Data:</h3> <h4>"+ data +"</h4>");
  });

  server.begin();
}

void loop() {
  if(ota_flag)
  {
    uint16_t time_start = millis();
    while(time_elapsed < 15000)
    {
      ArduinoOTA.handle();
      time_elapsed = millis()-time_start;
      delay(10);
    }
    ota_flag = false;
  }
  server.handleClient();
  digitalWrite(2, !digitalRead(2));

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
  delay(1000);
}
