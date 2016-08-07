/*
 * -------------------------------------------------------------------
 * EmonESP Serial to Emoncms gateway
 * -------------------------------------------------------------------
 * Adaptation of Chris Howells OpenEVSE ESP Wifi
 * by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
 * All adaptation GNU General Public License as below.
 *
 * -------------------------------------------------------------------
 *
 * This file is part of OpenEnergyMonitor.org project.
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


// Arduino espressif libs (tested with V2.3.0)
#include <ESP8266WiFi.h>              // Connect to Wifi
#include <WiFiClientSecure.h>         // Secure https GET request
#include <ESP8266WebServer.h>         // Config portal
#include <EEPROM.h>                   // Save config settings
#include "FS.h"                       // SPIFFS file-system: store web server html, CSS etc.
#include <ArduinoOTA.h>               // local OTA update from Arduino IDE
#include <ESP8266mDNS.h>              // Resolve URL for update server etc.
#include <ESP8266httpUpdate.h>        // remote OTA update from server
#include <ESP8266HTTPUpdateServer.h>  // upload update
#include <DNSServer.h>                // Required for captive portal
#include <PubSubClient.h>             // MQTT https://github.com/knolleary/pubsubclient PlatformIO lib: 89

ESP8266WebServer server(80);          //Create class for Web server
WiFiClientSecure client;              // Create class for HTTPS TCP connections get_https()
HTTPClient http;                      // Create class for HTTP TCP connections get_http
WiFiClient espClient;                 // Create client for MQTT
PubSubClient mqttclient(espClient);   // Create client for MQTT
ESP8266HTTPUpdateServer httpUpdater;  // Create class for webupdate handleWebUpdate()
DNSServer dnsServer;                  // Create class DNS server, captive portal re-direct
const byte DNS_PORT = 53;

// Access Point SSID, password & IP address. SSID will be softAP_ssid + chipID to make SSID unique
const char *softAP_ssid = "emonESP";
const char* softAP_password = "";
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// Web server authentication (leave blank for none)
const char* www_username = "emonesp";
const char* www_password = "emon";
String st, rssi;


/* hostname for mDNS. Should work at least on windows. Try http://emonesp.local */
const char *esp_hostname = "emonesp";

String esid = "";
String epass = "";


String connected_network = "";
String last_datastr = "";
String status_string = "";
String ipaddress = "";

//EMONCMS SERVER strings and interfers for emoncms.org
const char* e_url = "/input/post.json?json=";
const char* emoncmsorg_fingerprint = "7D:82:15:BE:D7:BC:72:58:87:7D:8E:40:D4:80:BA:1A:9F:8B:8D:DA";

String emoncms_server = "";
String emoncms_node = "";
String emoncms_apikey = "";
boolean emoncms_connected = false;
String test_serial="";

//MQTT Settings
String mqtt_server = "";
String mqtt_topic = "";
String mqtt_user = "";
String mqtt_pass = "";
long lastMqttReconnectAttempt = 0;

// -------------------------------------------------------------------
//OTA UPDATE SETTINGS
// -------------------------------------------------------------------
//UPDATE SERVER strings and interfers for upate server
// Array of strings Used to check firmware version
const char* u_host = "217.9.195.227";
const char* u_url = "/esp/firmware.php";

const char* firmware_update_path = "/upload";

// Get running firmware version from build tag environment variable
#define TEXTIFY(A) #A
#define ESCAPEQUOTE(A) TEXTIFY(A)
String currentfirmware = ESCAPEQUOTE(BUILD_TAG);
// -------------------------------------------------------------------


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
// Start Access Point
// Access point is used for wifi network selection
// -------------------------------------------------------------------
void startAP() {
  Serial.print("Starting AP");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.print("Scan: ");
  int n = WiFi.scanNetworks();
  Serial.print(n);
  Serial.println(" networks found");
  st = "";
  rssi = "";
  for (int i = 0; i < n; ++i){
    st += "\""+WiFi.SSID(i)+"\"";
    rssi += "\""+String(WiFi.RSSI(i))+"\"";
    if (i<n-1) st += ",";
    if (i<n-1) rssi += ",";
  }
  delay(100);

  WiFi.softAPConfig(apIP, apIP, netMsk);
  // Create Unique SSID e.g "emonESP_XXXXXX"
  String softAP_ssid_ID = String(softAP_ssid)+"_"+String(ESP.getChipId());;
  WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password);
    /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  IPAddress myIP = WiFi.softAPIP();
  char tmpStr[40];
  sprintf(tmpStr,"%d.%d.%d.%d",myIP[0],myIP[1],myIP[2],myIP[3]);
  Serial.print("AP IP Address: ");
  Serial.println(tmpStr);
  ipaddress = tmpStr;
}

// -------------------------------------------------------------------
// Start Client, attempt to connect to Wifi network
// -------------------------------------------------------------------
void startClient() {
  Serial.print("Connecting as client to ");
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
      Serial.println("Try Again...");
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
    Serial.print("Connected, IP: ");
    Serial.println(tmpStr);
    // Copy the connected network and ipaddress to global strings for use in status request
    connected_network = esid;
    ipaddress = tmpStr;
  }
}

#define EEPROM_ESID_SIZE          32
#define EEPROM_EPASS_SIZE         64
#define EEPROM_EMON_API_KEY_SIZE  32
#define EEPROM_EMON_SERVER_SIZE   45
#define EEPROM_EMON_NODE_SIZE     32
#define EEPROM_MQTT_SERVER_SIZE   45
#define EEPROM_MQTT_TOPIC_SIZE    32
#define EEPROM_MQTT_USER_SIZE     32
#define EEPROM_MQTT_PASS_SIZE     64
#define EEPROM_SIZE 512

#define EEPROM_ESID_START         0
#define EEPROM_ESID_END           (EEPROM_ESID_START + EEPROM_ESID_SIZE)
#define EEPROM_EPASS_START        EEPROM_ESID_END
#define EEPROM_EPASS_END          (EEPROM_EPASS_START + EEPROM_EPASS_SIZE)
#define EEPROM_EMON_API_KEY_START EEPROM_EPASS_END
#define EEPROM_EMON_API_KEY_END   (EEPROM_EMON_API_KEY_START + EEPROM_EMON_API_KEY_SIZE)
#define EEPROM_EMON_SERVER_START  EEPROM_EMON_API_KEY_END
#define EEPROM_EMON_SERVER_END    (EEPROM_EMON_SERVER_START + EEPROM_EMON_SERVER_SIZE)
#define EEPROM_EMON_NODE_START    EEPROM_EMON_SERVER_END
#define EEPROM_EMON_NODE_END      (EEPROM_EMON_NODE_START + EEPROM_EMON_NODE_SIZE)
#define EEPROM_MQTT_SERVER_START  EEPROM_EMON_NODE_END
#define EEPROM_MQTT_SERVER_END    (EEPROM_MQTT_SERVER_START + EEPROM_MQTT_SERVER_SIZE)
#define EEPROM_MQTT_TOPIC_START   EEPROM_MQTT_SERVER_END
#define EEPROM_MQTT_TOPIC_END     (EEPROM_MQTT_TOPIC_START + EEPROM_MQTT_TOPIC_SIZE)
#define EEPROM_MQTT_USER_START    EEPROM_MQTT_TOPIC_END
#define EEPROM_MQTT_USER_END      (EEPROM_MQTT_USER_START + EEPROM_MQTT_USER_SIZE)
#define EEPROM_MQTT_PASS_START    EEPROM_MQTT_USER_END
#define EEPROM_MQTT_PASS_END      (EEPROM_MQTT_PASS_START + EEPROM_MQTT_PASS_SIZE)

void ResetEEPROM(){
  //Serial.println("Erasing EEPROM");
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    EEPROM.write(i, 0);
    //Serial.print("#");
  }
  EEPROM.commit();
}

void load_EEPROM_settings(){

  EEPROM.begin(EEPROM_SIZE);
  for (int i = EEPROM_ESID_START; i < EEPROM_ESID_END; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) esid += (char) c;
  }

  for (int i = EEPROM_EPASS_START; i < EEPROM_EPASS_END; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) epass += (char) c;
  }
  for (int i = EEPROM_EMON_API_KEY_START; i < EEPROM_EMON_API_KEY_END; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) emoncms_apikey += (char) c;
  }
  for (int i = EEPROM_EMON_SERVER_START; i < EEPROM_EMON_SERVER_END; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) emoncms_server += (char) c;
  }
  for (int i = EEPROM_EMON_NODE_START; i < EEPROM_EMON_NODE_END; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) emoncms_node += (char) c;
  }
  for (int i = EEPROM_MQTT_SERVER_START; i < EEPROM_MQTT_SERVER_END; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) mqtt_server += (char) c;
  }
  for (int i = EEPROM_MQTT_TOPIC_START; i < EEPROM_MQTT_TOPIC_END; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) mqtt_topic += (char) c;
  }
  for (int i = EEPROM_MQTT_USER_START; i < EEPROM_MQTT_USER_END; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) mqtt_user += (char) c;
  }
  for (int i = EEPROM_MQTT_PASS_START; i < EEPROM_MQTT_PASS_END; ++i){
    byte c = EEPROM.read(i);
    if (c!=0 && c!=255) mqtt_pass += (char) c;
  }
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
  server.send(200, "text/html", "Turning AP Off");
  Serial.println("Turning AP Off");
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

  qpass.replace("%21", "!");
//  qpass.replace("%22", '"');
  qpass.replace("%23", "#");
  qpass.replace("%24", "$");
  qpass.replace("%25", "%");
  qpass.replace("%26", "&");
  qpass.replace("%27", "'");
  qpass.replace("%28", "(");
  qpass.replace("%29", ")");
  qpass.replace("%2A", "*");
  qpass.replace("%2B", "+");
  qpass.replace("%2C", ",");
  qpass.replace("%2D", "-");
  qpass.replace("%2E", ".");
  qpass.replace("%2F", "/");
  qpass.replace("%3A", ":");
  qpass.replace("%3B", ";");
  qpass.replace("%3C", "<");
  qpass.replace("%3D", "=");
  qpass.replace("%3E", ">");
  qpass.replace("%3F", "?");
  qpass.replace("%40", "@");
  qpass.replace("%5B", "[");
  qpass.replace("%5C", "'\'");
  qpass.replace("%5D", "]");
  qpass.replace("%5E", "^");
  qpass.replace("%5F", "_");
  qpass.replace("%60", "`");
  qpass.replace("%7B", "{");
  qpass.replace("%7C", "|");
  qpass.replace("%7D", "}");
  qpass.replace("%7E", "~");
  qpass.replace('+', ' ');

  qsid.replace("%21", "!");
//  qsid.replace("%22", '"');
  qsid.replace("%23", "#");
  qsid.replace("%24", "$");
  qsid.replace("%25", "%");
  qsid.replace("%26", "&");
  qsid.replace("%27", "'");
  qsid.replace("%28", "(");
  qsid.replace("%29", ")");
  qsid.replace("%2A", "*");
  qsid.replace("%2B", "+");
  qsid.replace("%2C", ",");
  qsid.replace("%2D", "-");
  qsid.replace("%2E", ".");
  qsid.replace("%2F", "/");
  qsid.replace("%3A", ":");
  qsid.replace("%3B", ";");
  qsid.replace("%3C", "<");
  qsid.replace("%3D", "=");
  qsid.replace("%3E", ">");
  qsid.replace("%3F", "?");
  qsid.replace("%40", "@");
  qsid.replace("%5B", "[");
  qsid.replace("%5C", "'\'");
  qsid.replace("%5D", "]");
  qsid.replace("%5E", "^");
  qsid.replace("%5F", "_");
  qsid.replace("%60", "`");
  qsid.replace("%7B", "{");
  qsid.replace("%7C", "|");
  qsid.replace("%7D", "}");
  qsid.replace("%7E", "~");
  qsid.replace('+', ' ');

  if (qsid != 0){
    for (int i = 0; i < EEPROM_ESID_SIZE; i++){
      if (i<qsid.length()) {
        EEPROM.write(i+EEPROM_ESID_START, qsid[i]);
      } else {
        EEPROM.write(i+EEPROM_ESID_START, 0);
      }
    }

    for (int i = 0; i < EEPROM_EPASS_SIZE; i++){
      if (i<qpass.length()) {
        EEPROM.write(i+EEPROM_EPASS_START, qpass[i]);
      } else {
        EEPROM.write(i+EEPROM_EPASS_START, 0);
      }
    }

    EEPROM.commit();
    server.send(200, "text/html", "saved");
    delay(2000);

    // Startup in STA + AP mode
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, netMsk);

  // Create Unique SSID e.g "emonESP_XXXXXX"
    String softAP_ssid_ID = String(softAP_ssid)+"_"+String(ESP.getChipId());;
    WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password);

    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);
    wifi_mode = 3;
    startClient();
  }
}

// -------------------------------------------------------------------
// Save Emoncms
// url: /saveemoncms
// -------------------------------------------------------------------
void handleSaveEmoncms() {
  emoncms_server = server.arg("server");
  emoncms_node = server.arg("node");
  emoncms_apikey = server.arg("apikey");
  if (emoncms_apikey!=0 && emoncms_server!=0 && emoncms_node!=0) {
    // save apikey to EEPROM
    for (int i = 0; i < EEPROM_EMON_API_KEY_SIZE; i++){
      if (i<emoncms_apikey.length()) {
        EEPROM.write(i+EEPROM_EMON_API_KEY_START, emoncms_apikey[i]);
      } else {
        EEPROM.write(i+EEPROM_EMON_API_KEY_START, 0);
      }
    }
    // save emoncms server to EEPROM max 45 characters
    for (int i = 0; i < EEPROM_EMON_SERVER_SIZE; i++){
      if (i<emoncms_server.length()) {
        EEPROM.write(i+EEPROM_EMON_SERVER_START, emoncms_server[i]);
      } else {
        EEPROM.write(i+EEPROM_EMON_SERVER_START, 0);
      }
    }
    // save emoncms node to EEPROM max 32 characters
    for (int i = 0; i < EEPROM_EMON_NODE_SIZE; i++){
      if (i<emoncms_node.length()) {
        EEPROM.write(i+EEPROM_EMON_NODE_START, emoncms_node[i]);
      } else {
        EEPROM.write(i+EEPROM_EMON_NODE_START, 0);
      }
    }
    EEPROM.commit();
    char tmpStr[109];
    sprintf(tmpStr,"Saved: %s %s %s",emoncms_server.c_str(),emoncms_node.c_str(),emoncms_apikey.c_str());
    Serial.println(tmpStr);
    server.send(200, "text/html", tmpStr);
  }
}

// -------------------------------------------------------------------
// Save MQTT Config
// url: /savemqtt
// -------------------------------------------------------------------
void handleSaveMqtt() {
  mqtt_server = server.arg("server");
  mqtt_topic = server.arg("topic");
  mqtt_user = server.arg("user");
  mqtt_pass = server.arg("pass");
  if (mqtt_server!=0 && mqtt_topic!=0) {
    // Save MQTT server max 45 characters
    for (int i = 0; i < EEPROM_MQTT_SERVER_SIZE; i++){
      if (i<mqtt_server.length()) {
        EEPROM.write(i+EEPROM_MQTT_SERVER_START, mqtt_server[i]);
      } else {
        EEPROM.write(i+EEPROM_MQTT_SERVER_START, 0);
      }
    }
    // Save MQTT topic max 32 characters
    for (int i = 0; i < EEPROM_MQTT_TOPIC_SIZE; i++){
      if (i<mqtt_topic.length()) {
        EEPROM.write(i+EEPROM_MQTT_TOPIC_START, mqtt_topic[i]);
      } else {
        EEPROM.write(i+EEPROM_MQTT_TOPIC_START, 0);
      }
    }
    // Save MQTT username max 32 characters
    for (int i = 0; i < EEPROM_MQTT_USER_SIZE; i++){
      if (i<mqtt_user.length()) {
        EEPROM.write(i+EEPROM_MQTT_USER_START, mqtt_user[i]);
      } else {
        EEPROM.write(i+EEPROM_MQTT_USER_START, 0);
      }
    }
    // Save MQTT pass max 64 characters
    for (int i = 0; i < EEPROM_MQTT_PASS_SIZE; i++){
      if (i<mqtt_pass.length()) {
        EEPROM.write(i+EEPROM_MQTT_PASS_START, mqtt_pass[i]);
      } else {
        EEPROM.write(i+EEPROM_MQTT_PASS_START, 0);
      }
    }

    EEPROM.commit();
    char tmpStr[80];
    sprintf(tmpStr,"Saved: %s %s %s %s",mqtt_server.c_str(),mqtt_topic.c_str(),mqtt_user.c_str(),mqtt_pass.c_str());
    Serial.println(tmpStr);
    server.send(200, "text/html", tmpStr);
    // If connected disconnect MQTT to trigger re-connect with new details
    if (mqttclient.connected()) {
      mqttclient.disconnect();
    }
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
  rssi = "";
  for (int i = 0; i < n; ++i){
    st += "\""+WiFi.SSID(i)+"\"";
    rssi += "\""+String(WiFi.RSSI(i))+"\"";
    if (i<n-1) st += ",";
    if (i<n-1) rssi += ",";
  }
  server.send(200, "text/plain","[" +st+ "],[" +rssi+"]");
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
  s += "\"rssi\":["+rssi+"],";

  s += "\"ssid\":\""+esid+"\",";
  s += "\"pass\":\""+epass+"\",";
  s += "\"srssi\":\""+String(WiFi.RSSI())+"\",";
  s += "\"ipaddress\":\""+ipaddress+"\",";
  s += "\"emoncms_server\":\""+emoncms_server+"\",";
  s += "\"emoncms_node\":\""+emoncms_node+"\",";
  s += "\"emoncms_apikey\":\""+emoncms_apikey+"\",";
  s += "\"emoncms_connected\":\""+String(emoncms_connected)+"\",";
  s += "\"packets_sent\":\""+String(packets_sent)+"\",";
  s += "\"packets_success\":\""+String(packets_success)+"\",";

  s += "\"mqtt_server\":\""+mqtt_server+"\",";
  s += "\"mqtt_topic\":\""+mqtt_topic+"\",";
  s += "\"mqtt_user\":\""+mqtt_user+"\",";
  s += "\"mqtt_pass\":\""+mqtt_pass+"\",";
  s += "\"mqtt_connected\":\""+String(mqttclient.connected())+"\",";

  s += "\"free_heap\":\""+String(ESP.getFreeHeap())+"\"";

  s += "}";
  server.send(200, "text/html", s);
}

// -------------------------------------------------------------------
// Reset config and reboot
// url: /reset
// -------------------------------------------------------------------
void handleRst() {
  ResetEEPROM();
  server.send(200, "text/html", "1");
  WiFi.disconnect();
  delay(1000);
  ESP.reset();
}

// -------------------------------------------------------------------
// Restart (Reboot)
// url: /restart
// -------------------------------------------------------------------
void handleRestart() {
  server.send(200, "text/html", "1");
  delay(1000);
  WiFi.disconnect();
  ESP.restart();
}


// -------------------------------------------------------------------
// Check for updates and display current version
// url: /firmware
// -------------------------------------------------------------------
String handleUpdateCheck() {
  Serial.println("Running: " + currentfirmware);
  // Get latest firmware version number
  String url = u_url;
  String latestfirmware = get_http(u_host, url);
  Serial.println("Latest: " + latestfirmware);
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
          str="No update, running latest firmware";
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

String get_https(const char* fingerprint, const char* host, String url, int httpsPort){
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
        return("ok");
      }
    }
  }
  else {
    return("HTTPS fingerprint no match");
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
    Serial.println(payload);
    http.end();
    return(payload);
  }
  else{
    http.end();
    return("server error: "+String(httpCode));
  }
} // end http_get

// -------------------------------------------------------------------
// MQTT Connect
// -------------------------------------------------------------------
boolean mqtt_connect() {
  mqttclient.setServer(mqtt_server.c_str(), 1883);
  Serial.println("MQTT Connecting...");
  String strID = String(ESP.getChipId());
  if (mqttclient.connect(strID.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {  // Attempt to connect
    Serial.println("MQTT connected");
    mqttclient.publish(mqtt_topic.c_str(), "connected"); // Once connected, publish an announcement..
  } else {
    Serial.print("MQTT failed: ");
    Serial.println(mqttclient.state());
    return(0);
  }
  return (1);
}

// -------------------------------------------------------------------
// Simulate Serial Input from emonTx
// -------------------------------------------------------------------
void handleTest(){
  test_serial = server.arg("serial");
  server.send(200, "text/html", test_serial);
  Serial.println(test_serial);
}




// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
	delay(2000);
	Serial.begin(115200);
  Serial.println();
  Serial.print("EmonESP ");
  Serial.println(ESP.getChipId());
  Serial.println("Firmware: "+ currentfirmware);

  // Read saved settings from EEPROM
  load_EEPROM_settings();

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

  // Start local OTA update server
  ArduinoOTA.begin();

  // Start hostname broadcast in STA mode
  if ((wifi_mode==0 || wifi_mode==3)){
    if (MDNS.begin(esp_hostname)) {
            MDNS.addService("http", "tcp", 80);
          }
  }

  // Setup firmware upload
  httpUpdater.setup(&server, firmware_update_path);

  // Start server & server root html /
  server.on("/", [](){
    if(www_username!="" && !server.authenticate(www_username, www_password) && wifi_mode == 0)
      return server.requestAuthentication();
    handleHome();
  });

  // Handle HTTP web interface button presses
  server.on("/savenetwork", handleSaveNetwork);
  server.on("/saveemoncms", handleSaveEmoncms);
  server.on("/savemqtt", handleSaveMqtt);
  server.on("/scan", handleScan);
  server.on("/apoff",handleAPOff);
  server.on("/firmware",handleUpdateCheck);
  server.on("/update",handleUpdate);
  server.on("/generate_204", handleHome);  //Android captive portal. Maybe not needed. Might be handled by notFound
  server.on("/fwlink", handleHome);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound

  server.on("/status", [](){
  if(www_username!="" && !server.authenticate(www_username, www_password) && wifi_mode == 0)
    return server.requestAuthentication();
  handleStatus();
  });
  server.on("/lastvalues", [](){
  if(www_username!="" && !server.authenticate(www_username, www_password) && wifi_mode == 0)
    return server.requestAuthentication();
  handleLastValues();
  });
  server.on("/reset", [](){
  if(www_username!="" && !server.authenticate(www_username, www_password) && wifi_mode == 0)
    return server.requestAuthentication();
  handleRst();
  });
  server.on("/restart", [](){
  if(www_username!="" && !server.authenticate(www_username, www_password) && wifi_mode == 0)
    return server.requestAuthentication();
  handleRestart();
  });

  server.on("/test", [](){
  if(www_username!="" && !server.authenticate(www_username, www_password) && wifi_mode == 0)
    return server.requestAuthentication();
  handleTest();
  });

  server.onNotFound([](){
  if(!handleFileRead(server.uri()))
    server.send(404, "text/plain", "NotFound");
  });

	server.begin();
	Serial.println("Server started");
  delay(100);
  Timer = millis();
  lastMqttReconnectAttempt = 0;
} // end setup

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void loop() {
  ArduinoOTA.handle();
  server.handleClient();          // Web server
  dnsServer.processNextRequest(); // Captive portal DNS re-dierct

  // If Wifi is connected & MQTT server has been set then connect to mqtt server
  if ((wifi_mode==0 || wifi_mode==3) && mqtt_server != 0){
    if (!mqttclient.connected()) {
      long now = millis();
      // try and reconnect continuously for first 5s then try again once every 10s
      if ( (now < 50000) || ((now - lastMqttReconnectAttempt)  > 100000) ) {
        lastMqttReconnectAttempt = now;
        if (mqtt_connect()) { // Attempt to reconnect
          lastMqttReconnectAttempt = 0;
        }
      }
    } else {
      // if MQTT connected
      mqttclient.loop();
      }
  }

  // Remain in AP mode for 5 Minutes before resetting
  if (wifi_mode == 1){
     if ((millis() - Timer) >= 300000){
       ESP.reset();
       Serial.println("WIFI Mode = 1, resetting");
     }
  }
  // If data received on serial
  while(Serial.available() || test_serial !="") {
    String data = "";
  // Could check for string integrity here
    if (Serial.available()){
      data = Serial.readStringUntil('\n');
      last_datastr = data;
    }

    if (test_serial !=""){
      data = test_serial;
      test_serial = "";
    }

    last_datastr = data;
    // If Wifi connected & emoncms server details are present
    if ((wifi_mode==0 || wifi_mode==3) && emoncms_apikey != 0){
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
      url += ",freeram:";
      url += String(ESP.getFreeHeap());
      url += "}&node=";
      url += emoncms_node;
      url += "&apikey=";
      url += emoncms_apikey;

      //Serial.println(url);
      packets_sent++;

      // Send data to Emoncms server
      String result="";
      if (emoncms_server=="emoncms.org"){
        // HTTPS on port 443 if emoncms.org
        result = get_https(emoncmsorg_fingerprint, emoncms_server.c_str(), url, 443);
      } else {
        // Plain HTTP if other emoncms server e.g EmonPi
        result = get_http(emoncms_server.c_str(), url);
      }
      if (result == "ok"){
        packets_success++;
        emoncms_connected = true;
      }
      else{
        emoncms_connected=false;
        Serial.print("Emoncms error: ");
        Serial.println(result);
      }

      // Send data to MQTT
      if (mqtt_server != 0){
        //char* buff = "";
        String buff ="";
        // Copy across, data length -1 to remove new line
        for (int i = 0; i < data.length()-1; ++i){
          buff += data[i];
        }
        mqttclient.publish(mqtt_topic.c_str(), buff.c_str());
      }


    } // end wifi connected
  } // end serial available

} // end loop
