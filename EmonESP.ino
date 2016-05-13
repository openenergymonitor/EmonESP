/*
 * Serial to emoncms gateway
 * Adaptation of Chris Howells OpenEVSE ESP Wifi
 * by Trystan Lea, OpenEnergyMonitor
 * All adaptation GNU General Public License as below.
 */

/*
 * Copyright (c) 2015 Chris Howell
 *
 * This file is part of Open EVSE.
 * Open EVSE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * Open EVSE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with Open EVSE; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "FS.h"
#include <ArduinoJson.h>

ESP8266WebServer server(80);

//Default SSID and PASSWORD for AP Access Point Mode
const char* ssid = "emonESP";
const char* password = "emonesp";
String st;

String esid = "";
String epass = "";  
String apikey = "";

String connected_network = "";
String last_datastr = "";
String status_string = "";

//SERVER strings and interfers for OpenEVSE Energy Monotoring
const char* host = "emoncms.org";
const char* e_url = "/input/post.json?json=";

int wifi_mode = 0; 
int buttonState = 0;
int clientTimeout = 0;
int i = 0;
unsigned long Timer;
unsigned long packets_sent = 0;
unsigned long packets_success = 0;

void ResetEEPROM(){
  //Serial.println("Erasing EEPROM");
  for (int i = 0; i < 512; ++i) { 
    EEPROM.write(i, 0);
    //Serial.print("#"); 
  }
  EEPROM.commit();   
}

void handleHome() {
  SPIFFS.begin(); // mount the fs
  File f = SPIFFS.open("/home.html", "r");
  if (f) {
    String s = f.readString();
    server.send(200, "text/html", s);
    f.close();
  }
}

void handleSaveNetwork() {
  String s;
  String qsid = server.arg("ssid");
  String qpass = server.arg("pass");
 
  qpass.replace("%23", "#");
  qpass.replace('+', ' ');
  
  if (qsid != 0){
    
    for (int i = 0; i < 32; i++){
      if (i<qsid.length()) {
        EEPROM.write(i+0, qsid[i]);
      } else {
        EEPROM.write(i+0, 0);
      }
    }
    
    for (int i = 0; i < 32; i++){
      if (i<qpass.length()) {
        EEPROM.write(i+32, qpass[i]);
      } else {
        EEPROM.write(i+32, 0);
      }
    }
     
    EEPROM.commit();
    server.send(200, "text/html", "saved");
    delay(2000);
    WiFi.disconnect();
    ESP.reset(); 
  }
}

void handleSaveApikey() {
  apikey = server.arg("apikey");
  if (apikey!=0) {
    for (int i = 0; i < 32; i++){
      if (i<apikey.length()) {
        EEPROM.write(i+96, apikey[i]);
      } else {
        EEPROM.write(i+96, 0);
      }
    }
    EEPROM.commit();
    server.send(200, "text/html", "saved");
  }
}

void handleLastValues() {
    server.send(200, "text/html", last_datastr);
}

void status_add(String key,String value, bool comma) {
  status_string += "\"" + key + "\":\"" + value + "\"";
  if (comma) status_string += ",";
}

void handleStatus() {

  String s = "{";
  if (wifi_mode==0) {
    s += "\"mode\":\"STA\",";
  } else {
    s += "\"mode\":\"AP\",";
  }
  s += "\"networks\":["+st+"],";
  s += "\"ssid\":\""+esid+"\",";
  s += "\"pass\":\""+epass+"\",";
  s += "\"apikey\":\""+apikey+"\",";
  s += "\"ipaddress\":\"192.168.1.112\",";
  s += "\"packets_sent\":\"1800\",";
  s += "\"packets_sent\":\"3205\"";
  s += "}";
  server.send(200, "text/html", s);
}



void handleRst() {
  ResetEEPROM();
  EEPROM.commit();
  server.send(200, "text/html", "reset");
  WiFi.disconnect();
  delay(1000);
  ESP.reset();
}

void startAP() {
  Serial.print("Starting Access Point");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.print("Scan: ");
  int n = WiFi.scanNetworks();
  Serial.print(n);
  Serial.println(" networks found");
  for (int i = 0; i < n; ++i){
    st += "\""+WiFi.SSID(i)+"\"";
    if (i<n-1) st += ",";
  }
  delay(100);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  char tmpStr[40];
  sprintf(tmpStr,"MODE:AP IP Address: %d.%d.%d.%d",myIP[0],myIP[1],myIP[2],myIP[3]);
  Serial.println(tmpStr);
}

void setup() {
	delay(2000);
	Serial.begin(115200);
  Serial.println();
  Serial.println("emonESP Startup");
  EEPROM.begin(512);
  // ResetEEPROM();
  char tmpStr[40];
 
  for (int i = 0; i < 32; ++i){
    char c = char(EEPROM.read(i));
    if (c!=0) esid += c;
  }
  for (int i = 32; i < 96; ++i){
    char c = char(EEPROM.read(i));
    if (c!=0) epass += c;
  }
  for (int i = 96; i < 128; ++i){
    char c = char(EEPROM.read(i));
    if (c!=0) apikey += c;
  }

  // 1) If no network configured start up access point
  if (esid == 0)
  {
    startAP();
    wifi_mode = 2; // AP mode with no SSID in EEPROM    
  } 
  // 2) else try and connect to the configured network
  else
  {
    Serial.print("Connecting as Wifi Client to ");
    Serial.println(esid.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(esid.c_str(), epass.c_str());
    delay(50);
    int t = 0;
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED){
      
      delay(500);
      t++;
      if (t >= 20){
        Serial.println(" ");
        Serial.println("Trying Again...");
        delay(2000);
        WiFi.disconnect();
        WiFi.begin(esid.c_str(), epass.c_str());
        t = 0;
        attempt++;
        if (attempt >= 5){
          startAP();
          // AP mode with SSID in EEPROM, connection will retry in 5 minutes
          wifi_mode = 1;
          break;
        }
      }
    }
  }
  
	if (wifi_mode == 0){
    IPAddress myAddress = WiFi.localIP();
    sprintf(tmpStr,"MODE:CLIENT %d.%d.%d.%d",myAddress[0],myAddress[1],myAddress[2],myAddress[3]);
    Serial.println(tmpStr);
    connected_network = esid;
  }

  server.on("/", handleHome);
  server.on("/savenetwork", handleSaveNetwork);
  server.on("/saveapikey", handleSaveApikey);
  server.on("/status", handleStatus);
  server.on("/lastvalues",handleLastValues);
  server.on("/reset", handleRst);
	server.begin();
	Serial.println("HTTP server started");
  delay(100);
  Timer = millis();
}

void loop() {
  server.handleClient();

  /*
  int erase = 0;  
  buttonState = digitalRead(0);
  while (buttonState == LOW) {
    buttonState = digitalRead(0);
    erase++;
    if (erase >= 5000) {
      ResetEEPROM();
      int erase = 0;
      WiFi.disconnect();
      Serial.print("Finished...");
      delay(2000);
      ESP.reset();
    } 
  }*/

  // Remain in AP mode for 5 Minutes before resetting
  if (wifi_mode == 1){
     if ((millis() - Timer) >= 300000){
       ESP.reset();
       Serial.println("WIFI Mode = 1, resetting");
     }
  }   

  while(Serial.available()) {
    String data = Serial.readStringUntil('\n');
    // Could check for string integrity here
    
    last_datastr = data;

    if (wifi_mode == 0 && apikey != 0) 
    {
      // Use WiFiClient class to create TCP connections
      WiFiClient client;
      const int httpPort = 80;
      if (!client.connect(host, httpPort)) {
        return;
      }

      // We now create a URL for OpenEVSE RAPI data upload request
      String url = e_url;
      url += "{";
      // Copy across, data length -1 to remove new line
      for (int i = 0; i < data.length()-1; ++i){
          url += data[i];
      }
      url += ",psent:";
      url += packets_sent;
      url += ",psuccess:";
      url += packets_success;
      url += "}&apikey=";
      url += apikey.c_str();

      Serial.print("Emoncms request: ");
      packets_sent++;
      
      // This will send the request to the server
      client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
      // Handle wait for reply and timeout
      unsigned long timeout = millis();
      while (client.available() == 0) {
        if (millis() - timeout > 5000) {
          Serial.println(">>> Client Timeout !");
          client.stop();
          return;
        }
      }
      // Handle message receive
      while(client.available()){
        String line = client.readStringUntil('\r');
        if (line.startsWith("HTTP/1.1 200 OK")) {
          Serial.print("HTTP/1.1 200 OK");
          packets_success++;
        }
      }
      Serial.println();
    }
  }
}
