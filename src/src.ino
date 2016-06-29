/*
 * -------------------------------------------------------------------
 * EmonESP Serial to emoncms gateway
 * -------------------------------------------------------------------
 * Adaptation of Chris Howells OpenEVSE ESP Wifi
 * by Trystan Lea, OpenEnergyMonitor
 * All adaptation GNU General Public License as below.
 *
 * -------------------------------------------------------------------
 *
 * This file is part of OpenEnergyMonitor project.
 * EmonESP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * EmonESP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with EmonESP; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "FS.h"
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <ESP8266httpUpdate.h>

ESP8266WebServer server(80);
// Create class to for HTTPS nand http TCP connections
WiFiClientSecure client;
WiFiClient clienthttp;
HTTPClient http;

//Default SSID and PASSWORD for AP Access Point Mode
const char* ssid = "emonESP";
const char* password = "emonesp";
const char* www_username = "";
const char* www_password = "";
String st;

String esid = "";
String epass = "";
String apikey = "";

String connected_network = "";
String last_datastr = "";
String status_string = "";
String ipaddress = "";

//EMONCMS SERVER strings and interfers for emoncms.org
const char* host = "emoncms.org";
const int httpsPort = 443;
const char* e_url = "/input/post.json?json=";
const char* fingerprint = "B6:44:19:FF:B8:F2:62:46:60:39:9D:21:C6:FB:F5:6A:F3:D9:1A:79";


//----------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------OTA UPDATE SETTINGS-------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------
//UPDATE SERVER strings and interfers for upate server
// Array of strings Used to check firmware version
const char* u_host = "lab.megni.co.uk";
const char* u_url = "/esp/firmware.php";

// Get running firmware version from build tag environment variable
#define TEXTIFY(A) #A
#define ESCAPEQUOTE(A) TEXTIFY(A)
String currentfirmware = ESCAPEQUOTE(BUILD_TAG);


//----------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------

// Wifi mode
// 0 - STA (Client)
// 1 - AP with STA retry
// 2 - AP only
// 3 - AP + STA
int wifi_mode = 0;
int clientTimeout = 0;
int i = 0;
unsigned long Timer;
unsigned long packets_sent = 0;
unsigned long packets_success = 0;


String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

// -------------------------------------------------------------------
// Start Access Point, starts on 192.168.4.1
// Access point is used for wifi network selection
// -------------------------------------------------------------------
void startAP() {
  Serial.print("Starting Access Point");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.print("Scan: ");
  int n = WiFi.scanNetworks();
  Serial.print(n);
  Serial.println(" networks found");
  st = "";
  for (int i = 0; i < n; ++i){
    st += "\""+WiFi.SSID(i)+"\"";
    if (i<n-1) st += ",";
  }
  delay(100);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  char tmpStr[40];
  sprintf(tmpStr,"%d.%d.%d.%d",myIP[0],myIP[1],myIP[2],myIP[3]);
  Serial.print("Access Point IP Address: ");
  Serial.println(tmpStr);
  ipaddress = tmpStr;
}

// -------------------------------------------------------------------
// Start Client, attempt to connect to Wifi network
// -------------------------------------------------------------------
void startClient() {
  Serial.print("Connecting as Wifi Client to ");
  Serial.print(esid.c_str());
  Serial.print(" epass:");
  Serial.println(epass.c_str());
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

  if (wifi_mode == 0 || wifi_mode == 3){
    IPAddress myAddress = WiFi.localIP();
    char tmpStr[40];
    sprintf(tmpStr,"%d.%d.%d.%d",myAddress[0],myAddress[1],myAddress[2],myAddress[3]);
    Serial.print("Connected, IP Address: ");
    Serial.println(tmpStr);
    // Copy the connected network and ipaddress to global strings for use in status request
    connected_network = esid;
    ipaddress = tmpStr;
  }
}

void ResetEEPROM(){
  //Serial.println("Erasing EEPROM");
  for (int i = 0; i < 512; ++i) {
    EEPROM.write(i, 0);
    //Serial.print("#");
  }
  EEPROM.commit();
}

// -------------------------------------------------------------------
// Load SPIFFS Home page
// url: /
// -------------------------------------------------------------------
void handleHome() {
  SPIFFS.begin(); // mount the fs
  File f = SPIFFS.open("/home.html", "r");
  if (f) {
    String s = f.readString();
    server.send(200, "text/html", s);
    f.close();
  }
}

// -------------------------------------------------------------------
// Handle turning Access point off
// url: /apoff
// -------------------------------------------------------------------
void handleAPOff() {
  server.send(200, "text/html", "Turning Access Point Off");
  Serial.println("Turning Access Point Off");
  WiFi.disconnect();
  delay(1000);
  ESP.reset();
  //delay(2000);
  //WiFi.mode(WIFI_STA);
}

// -------------------------------------------------------------------
// Save selected network to EEPROM and attempt connection
// url: /savenetwork
// -------------------------------------------------------------------
void handleSaveNetwork() {
  String s;
  String qsid = server.arg("ssid");
  String qpass = server.arg("pass");
  esid = qsid;
  epass = qpass;

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

    // Startup in STA + AP mode
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid, password);
    wifi_mode = 3;
    startClient();
  }
}

// -------------------------------------------------------------------
// Save apikey
// url: /saveapikey
// -------------------------------------------------------------------
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

// -------------------------------------------------------------------
// Wifi scan /scan not currently used
// url: /scan
// -------------------------------------------------------------------
void handleScan() {
  Serial.println("WIFI Scan");
  int n = WiFi.scanNetworks();
  Serial.print(n);
  Serial.println(" networks found");
  st = "";
  for (int i = 0; i < n; ++i){
    st += "\""+WiFi.SSID(i)+"\"";
    if (i<n-1) st += ",";
  }
  server.send(200, "text/plain","["+st+"]");
}

// -------------------------------------------------------------------
// url: /lastvalues
// Last values on atmega serial
// -------------------------------------------------------------------
void handleLastValues() {
  server.send(200, "text/html", last_datastr);
}

// -------------------------------------------------------------------
// url: /status
// returns wifi status
// -------------------------------------------------------------------
void handleStatus() {

  String s = "{";
  if (wifi_mode==0) {
    s += "\"mode\":\"STA\",";
  } else if (wifi_mode==1 || wifi_mode==2) {
    s += "\"mode\":\"AP\",";
  } else if (wifi_mode==3) {
    s += "\"mode\":\"STA+AP\",";
  }
  s += "\"networks\":["+st+"],";
  s += "\"ssid\":\""+esid+"\",";
  s += "\"pass\":\""+epass+"\",";
  s += "\"apikey\":\""+apikey+"\",";
  s += "\"ipaddress\":\""+ipaddress+"\",";
  s += "\"packets_sent\":\""+String(packets_sent)+"\",";
  s += "\"packets_success\":\""+String(packets_success)+"\"";
  s += "}";
  server.send(200, "text/html", s);
}

// -------------------------------------------------------------------
// Reset config and reboot
// url: /reset
// -------------------------------------------------------------------
void handleRst() {
  ResetEEPROM();
  EEPROM.commit();
  server.send(200, "text/html", "reset");
  WiFi.disconnect();
  delay(1000);
  ESP.reset();
}


// -------------------------------------------------------------------
// Check for updates and display current version
// url: /firmware
// -------------------------------------------------------------------
String handleUpdateCheck() {
  Serial.println("Running firmware: " + currentfirmware);
  // Get latest firmware version number
  String url = u_url;
  String latestfirmware = get_http(u_host, url);
  Serial.println("Latest firmware: " + latestfirmware);
  // Update web interface with firmware version(s)
  String s = "{";
  s += "\"current\":\""+currentfirmware+"\",";
  s += "\"latest\":\""+latestfirmware+"\"";
  s += "}";
  server.send(200, "text/html", s);
  return (latestfirmware);
}


// -------------------------------------------------------------------
// Update firmware
// url: /update
// -------------------------------------------------------------------
void handleUpdate() {
  SPIFFS.end(); // unmount filesystem
  Serial.println("UPDATING...");
  delay(500);
  t_httpUpdate_return ret = ESPhttpUpdate.update("http://" + String(u_host) + String(u_url) + "?tag=" + currentfirmware);
  String str="error";
  switch(ret) {
      case HTTP_UPDATE_FAILED:
          str = printf("Update failed error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;
      case HTTP_UPDATE_NO_UPDATES:
          str="No update, already running latest firmware";
          break;
      case HTTP_UPDATE_OK:
          str="Update done!";
          break;
  }
  server.send(400,"text/html",str);
  Serial.println(str);
  SPIFFS.begin(); //mount-file system
}

// -------------------------------------------------------------------
// HTTPS SECURE GET Request
// url: N/A
// -------------------------------------------------------------------

String get_https(const char* fingerprint,const char* host, String url, int httpsPort){
  // Use WiFiClient class to create TCP connections
  if (!client.connect(host, httpsPort)) {
    Serial.print(host + httpsPort); //debug
    return("Connection error");
  }
  if (client.verify(fingerprint, host)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
     // Handle wait for reply and timeout
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        client.stop();
        return("Client Timeout");
      }
    }
    // Handle message receive
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.println(line); //debug
      if (line.startsWith("HTTP/1.1 200 OK")) {
        return("OK");
      }
    }
  }
  else {
    return("HTTP fingerprint doesn't match");
  }
  return("error" + String(host));
} // end https_get

// -------------------------------------------------------------------
// HTTP GET Request
// url: N/A
// -------------------------------------------------------------------
String get_http(const char* host, String url){
  http.begin(String("http://") + host + String(url));
  int httpCode = http.GET();
  if((httpCode > 0) && (httpCode == HTTP_CODE_OK)){
    String payload = http.getString();
    http.end();
    return(payload);
  }
  else{
    http.end();
    return("server error: "+httpCode);
  }
} // end http_get


// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
	delay(2000);
	Serial.begin(115200);
  Serial.println();
  Serial.println("EmonESP Startup");
  Serial.println("Firmware: "+ currentfirmware);

  // Read saved settings from EEPROM
  EEPROM.begin(512);
  for (int i = 0; i < 32; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) esid += (char) c;
  }

  for (int i = 32; i < 96; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) epass += (char) c;
  }
  for (int i = 96; i < 128; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) apikey += (char) c;
  }

  WiFi.disconnect();
  // 1) If no network configured start up access point
  if (esid == 0 || esid == "")
  {
    startAP();
    wifi_mode = 2; // AP mode with no SSID in EEPROM
  }
  // 2) else try and connect to the configured network
  else
  {
    WiFi.mode(WIFI_STA);
    wifi_mode = 0;
    startClient();
  }
  ArduinoOTA.begin();
  server.on("/", [](){
    if(www_username!="" && !server.authenticate(www_username, www_password))
      return server.requestAuthentication();
    handleHome();
  });
  // Handle HTTP web interface button presses
  server.on("/savenetwork", handleSaveNetwork);
  server.on("/saveapikey", handleSaveApikey);
  server.on("/status", handleStatus);
  server.on("/lastvalues",handleLastValues);
  server.on("/reset", handleRst);
  server.on("/scan", handleScan);
  server.on("/apoff",handleAPOff);
  server.on("/firmware",handleUpdateCheck);
  server.on("/update",handleUpdate);
  server.onNotFound([](){
  if(!handleFileRead(server.uri()))
    server.send(404, "text/plain", "FileNotFound");
  });

	server.begin();
	Serial.println("HTTP server started");
  delay(100);
  Timer = millis();
} // end setup

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  // Remain in AP mode for 5 Minutes before resetting
  if (wifi_mode == 1){
     if ((millis() - Timer) >= 300000){
       ESP.reset();
       Serial.println("WIFI Mode = 1, resetting");
     }
  }
  // If data received on serial
  while(Serial.available()) {
    String data = Serial.readStringUntil('\n');
    // Could check for string integrity here
    last_datastr = data;
    // If Wifi connected
    if ((wifi_mode==0 || wifi_mode==3) && apikey != 0){
      // We now create a URL for server data upload
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

      // Send data to Emoncms server
      String result = get_https(fingerprint, host, url, httpsPort);
      if (result == "OK"){
        Serial.print(result);
        packets_success++;
      }
    } // end wifi connected
  } // end serial available

} // end loop
