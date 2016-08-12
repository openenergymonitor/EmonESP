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

// Uncomment to use hardware UART 1 for debug else use UART 0
//#define DEBUG_SERIAL1
#ifdef DEBUG_SERIAL1
#define DEBUG Serial1
#else
#define DEBUG Serial
#endif

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
String www_username = "";
String www_password = "";
String st, rssi;


/* hostname for mDNS. Should work at least on windows. Try http://emonesp.local */
const char *esp_hostname = "emonesp";

// Wifi Network Strings
String esid = "";
String epass = "";
String connected_network = "";
String last_datastr = "";
String status_string = "";
String ipaddress = "";

//EMONCMS SERVER strings
const char* e_url = "/input/post.json?json=";
String emoncms_server = "";
String emoncms_node = "";
String emoncms_apikey = "";
String emoncms_fingerprint = "";
boolean emoncms_connected = false;
String test_serial="";

//MQTT Settings
String mqtt_server = "";
String mqtt_topic = "";
String mqtt_user = "";
String mqtt_pass = "";
String mqtt_feed_prefix = "";
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

#define WIFI_MODE_STA           0
#define WIFI_MODE_AP_STA_RETRY  1
#define WIFI_MODE_AP_ONLY       2
#define WIFI_MODE_AP_AND_STA    3

int wifi_mode = WIFI_MODE_STA;
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
  DEBUG.print("Starting AP");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  DEBUG.print("Scan: ");
  int n = WiFi.scanNetworks();
  DEBUG.print(n);
  DEBUG.println(" networks found");
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

  // Setup the DNS server redirecting all the domains to the apIP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  IPAddress myIP = WiFi.softAPIP();
  char tmpStr[40];
  sprintf(tmpStr,"%d.%d.%d.%d",myIP[0],myIP[1],myIP[2],myIP[3]);
  DEBUG.print("AP IP Address: ");
  DEBUG.println(tmpStr);
  ipaddress = tmpStr;
}

// -------------------------------------------------------------------
// Start Client, attempt to connect to Wifi network
// -------------------------------------------------------------------
void startClient() {
  DEBUG.print("Connecting as client to ");
  DEBUG.print(esid.c_str());
  DEBUG.print(" epass:");
  DEBUG.println(epass.c_str());
  WiFi.begin(esid.c_str(), epass.c_str());

  delay(50);

  int t = 0;
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    t++;
    if (t >= 20){
      DEBUG.println(" ");
      DEBUG.println("Try Again...");
      delay(2000);
      WiFi.disconnect();
      WiFi.begin(esid.c_str(), epass.c_str());
      t = 0;
      attempt++;
      if (attempt >= 5){
        startAP();
        // AP mode with SSID in EEPROM, connection will retry in 5 minutes
        wifi_mode = WIFI_MODE_AP_STA_RETRY;
        break;
      }
    }
  }

  if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_AND_STA){
    IPAddress myAddress = WiFi.localIP();
    char tmpStr[40];
    sprintf(tmpStr,"%d.%d.%d.%d",myAddress[0],myAddress[1],myAddress[2],myAddress[3]);
    DEBUG.print("Connected, IP: ");
    DEBUG.println(tmpStr);
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
#define EEPROM_EMON_FINGERPRINT_SIZE  60
#define EEPROM_MQTT_FEED_PREFIX_SIZE  10
#define EEPROM_WWW_USER_SIZE      16
#define EEPROM_WWW_PASS_SIZE      16
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
#define EEPROM_EMON_FINGERPRINT_START  EEPROM_MQTT_PASS_END
#define EEPROM_EMON_FINGERPRINT_END    (EEPROM_EMON_FINGERPRINT_START + EEPROM_EMON_FINGERPRINT_SIZE)
#define EEPROM_MQTT_FEED_PREFIX_START  EEPROM_EMON_FINGERPRINT_END
#define EEPROM_MQTT_FEED_PREFIX_END    (EEPROM_MQTT_FEED_PREFIX_START + EEPROM_MQTT_FEED_PREFIX_SIZE)
#define EEPROM_WWW_USER_START     EEPROM_MQTT_FEED_PREFIX_END
#define EEPROM_WWW_USER_END       (EEPROM_WWW_USER_START + EEPROM_WWW_USER_SIZE)
#define EEPROM_WWW_PASS_START     EEPROM_WWW_USER_END
#define EEPROM_WWW_PASS_END       (EEPROM_WWW_PASS_START + EEPROM_WWW_PASS_SIZE)

// -------------------------------------------------------------------
// Reset EEPROM, wipes all settings
// -------------------------------------------------------------------
void ResetEEPROM(){
  //DEBUG.println("Erasing EEPROM");
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    EEPROM.write(i, 0);
    //DEBUG.print("#");
  }
  EEPROM.commit();
}

void EEPROM_read_srting(int start, int count, String& val) {
  for (int i = 0; i < count; ++i){
    byte c = EEPROM.read(start+i);
    if (c!=0 && c!=255) val += (char) c;
  }
}

void EEPROM_write_string(int start, int count, String val) {
  for (int i = 0; i < count; ++i){
    if (i<val.length()) {
      EEPROM.write(start+i, val[i]);
    } else {
      EEPROM.write(start+i, 0);
    }
  }
}

// -------------------------------------------------------------------
// Load saved settings from EEPROM
// -------------------------------------------------------------------
void load_EEPROM_settings(){

  EEPROM.begin(EEPROM_SIZE);

  // Load WiFi values
  EEPROM_read_srting(EEPROM_ESID_START, EEPROM_ESID_SIZE, esid);
  EEPROM_read_srting(EEPROM_EPASS_START, EEPROM_EPASS_SIZE, epass);

  // EmonCMS settings
  EEPROM_read_srting(EEPROM_EMON_API_KEY_START, EEPROM_EMON_API_KEY_SIZE, emoncms_apikey);
  EEPROM_read_srting(EEPROM_EMON_SERVER_START, EEPROM_EMON_SERVER_SIZE, emoncms_server);
  EEPROM_read_srting(EEPROM_EMON_NODE_START, EEPROM_EMON_NODE_SIZE, emoncms_node);
  EEPROM_read_srting(EEPROM_EMON_FINGERPRINT_START, EEPROM_EMON_FINGERPRINT_SIZE, emoncms_fingerprint);

  // MQTT settings
  EEPROM_read_srting(EEPROM_MQTT_SERVER_START, EEPROM_MQTT_SERVER_SIZE, mqtt_server);
  EEPROM_read_srting(EEPROM_MQTT_TOPIC_START, EEPROM_MQTT_TOPIC_SIZE, mqtt_topic);
  EEPROM_read_srting(EEPROM_MQTT_FEED_PREFIX_START, EEPROM_MQTT_FEED_PREFIX_SIZE, mqtt_feed_prefix);
  EEPROM_read_srting(EEPROM_MQTT_USER_START, EEPROM_MQTT_USER_SIZE, mqtt_user);
  EEPROM_read_srting(EEPROM_MQTT_PASS_START, EEPROM_MQTT_PASS_SIZE, mqtt_pass);

  // Web server credentials
  EEPROM_read_srting(EEPROM_WWW_USER_START, EEPROM_WWW_USER_SIZE, www_username);
  EEPROM_read_srting(EEPROM_WWW_PASS_START, EEPROM_WWW_PASS_SIZE, www_password);
}


// -------------------------------------------------------------------
// Helper function to decode the URL values
// -------------------------------------------------------------------
void decodeURI(String& val)
{
    val.replace("%21", "!");
//    val.replace("%22", '"');
    val.replace("%23", "#");
    val.replace("%24", "$");
    val.replace("%26", "&");
    val.replace("%27", "'");
    val.replace("%28", "(");
    val.replace("%29", ")");
    val.replace("%2A", "*");
    val.replace("%2B", "+");
    val.replace("%2C", ",");
    val.replace("%2D", "-");
    val.replace("%2E", ".");
    val.replace("%2F", "/");
    val.replace("%3A", ":");
    val.replace("%3B", ";");
    val.replace("%3C", "<");
    val.replace("%3D", "=");
    val.replace("%3E", ">");
    val.replace("%3F", "?");
    val.replace("%40", "@");
    val.replace("%5B", "[");
    val.replace("%5C", "'\'");
    val.replace("%5D", "]");
    val.replace("%5E", "^");
    val.replace("%5F", "_");
    val.replace("%60", "`");
    val.replace("%7B", "{");
    val.replace("%7C", "|");
    val.replace("%7D", "}");
    val.replace("%7E", "~");
    val.replace('+', ' ');

    // Decode the % char last as there is always the posibility that the decoded
    // % char coould be followed by one of the other replaces
    val.replace("%25", "%");
}

// -------------------------------------------------------------------
// Load Home page
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
// Wifi scan /scan not currently used
// url: /scan
// -------------------------------------------------------------------
void handleScan() {
  DEBUG.println("WIFI Scan");
  int n = WiFi.scanNetworks();
  DEBUG.print(n);
  DEBUG.println(" networks found");
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
// Handle turning Access point off
// url: /apoff
// -------------------------------------------------------------------
void handleAPOff() {
  server.send(200, "text/html", "Turning AP Off");
  DEBUG.println("Turning AP Off");
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

  decodeURI(qsid);
  decodeURI(qpass);

  esid = qsid;
  epass = qpass;

  if (qsid != 0){
    EEPROM_write_string(EEPROM_ESID_START, EEPROM_ESID_SIZE, qsid);
    EEPROM_write_string(EEPROM_EPASS_START, EEPROM_EPASS_SIZE, qpass);

    EEPROM.commit();
    server.send(200, "text/html", "saved");
    delay(2000);

    // Startup in STA + AP mode
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, netMsk);

    // Create Unique SSID e.g "emonESP_XXXXXX"
    String softAP_ssid_ID = String(softAP_ssid)+"_"+String(ESP.getChipId());;
    WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password);

    // Setup the DNS server redirecting all the domains to the apIP
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);
    wifi_mode = WIFI_MODE_AP_AND_STA;
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
  emoncms_fingerprint = server.arg("fingerprint");

  // save apikey to EEPROM
  EEPROM_write_string(EEPROM_EMON_API_KEY_START, EEPROM_EMON_API_KEY_SIZE, emoncms_apikey);

  // save emoncms server to EEPROM max 45 characters
  EEPROM_write_string(EEPROM_EMON_SERVER_START, EEPROM_EMON_SERVER_SIZE, emoncms_server);

  // save emoncms node to EEPROM max 32 characters
  EEPROM_write_string(EEPROM_EMON_NODE_START, EEPROM_EMON_NODE_SIZE, emoncms_node);

  // save emoncms HTTPS fingerprint to EEPROM max 60 characters
  EEPROM_write_string(EEPROM_EMON_FINGERPRINT_START, EEPROM_EMON_FINGERPRINT_SIZE, emoncms_fingerprint);

  EEPROM.commit();

  // BUG: Potential buffer overflow issue the values emoncms_xxx come from user
  //      input so could overflow the buffer no matter the length
  char tmpStr[200];
  sprintf(tmpStr,"Saved: %s %s %s %s",emoncms_server.c_str(),emoncms_node.c_str(),emoncms_apikey.c_str(),emoncms_fingerprint.c_str());
  DEBUG.println(tmpStr);
  server.send(200, "text/html", tmpStr);
}

// -------------------------------------------------------------------
// Save MQTT Config
// url: /savemqtt
// -------------------------------------------------------------------
void handleSaveMqtt() {
  mqtt_server = server.arg("server");
  mqtt_topic = server.arg("topic");
  mqtt_feed_prefix = server.arg("prefix");
  mqtt_user = server.arg("user");
  mqtt_pass = server.arg("pass");

  // Save MQTT server max 45 characters
  EEPROM_write_string(EEPROM_MQTT_SERVER_START, EEPROM_MQTT_SERVER_SIZE, mqtt_server);

  // Save MQTT topic max 32 characters
  EEPROM_write_string(EEPROM_MQTT_TOPIC_START, EEPROM_MQTT_TOPIC_SIZE, mqtt_topic);

  // Save MQTT topic separator max 10 characters
  EEPROM_write_string(EEPROM_MQTT_FEED_PREFIX_START, EEPROM_MQTT_FEED_PREFIX_SIZE, mqtt_feed_prefix);

  // Save MQTT username max 32 characters
  EEPROM_write_string(EEPROM_MQTT_USER_START, EEPROM_MQTT_USER_SIZE, mqtt_user);

  // Save MQTT pass max 64 characters
  EEPROM_write_string(EEPROM_MQTT_PASS_START, EEPROM_MQTT_PASS_SIZE, mqtt_pass);

  EEPROM.commit();

  char tmpStr[200];
  // BUG: Potential buffer overflow issue the values mqtt_xxx come from user
  //      input so could overflow the buffer no matter the length
  sprintf(tmpStr,"Saved: %s %s %s %s %s",mqtt_server.c_str(),mqtt_topic.c_str(),mqtt_feed_prefix.c_str(),mqtt_user.c_str(),mqtt_pass.c_str());
  DEBUG.println(tmpStr);
  server.send(200, "text/html", tmpStr);

  // If connected disconnect MQTT to trigger re-connect with new details
  if (mqttclient.connected()) {
    mqttclient.disconnect();
  }
}

// -------------------------------------------------------------------
// Save the web site user/pass
// url: /saveadmin
// -------------------------------------------------------------------
void handleSaveAdmin() {
  String quser = server.arg("user");
  String qpass = server.arg("pass");

  decodeURI(quser);
  decodeURI(qpass);

  www_username = quser;
  www_password = qpass;

  EEPROM_write_string(EEPROM_WWW_USER_START, EEPROM_WWW_USER_SIZE, quser);
  EEPROM_write_string(EEPROM_WWW_PASS_START, EEPROM_WWW_PASS_SIZE, qpass);

  EEPROM.commit();
  server.send(200, "text/html", "saved");
}


// -------------------------------------------------------------------
// Last values on atmega serial
// url: /lastvalues
// -------------------------------------------------------------------
void handleLastValues() {
  server.send(200, "text/html", last_datastr);
}

// -------------------------------------------------------------------
// Returns status json
// url: /status
// -------------------------------------------------------------------
void handleStatus() {

  String s = "{";
  if (wifi_mode==WIFI_MODE_STA) {
    s += "\"mode\":\"STA\",";
  } else if (wifi_mode==WIFI_MODE_AP_STA_RETRY || wifi_mode==WIFI_MODE_AP_ONLY) {
    s += "\"mode\":\"AP\",";
  } else if (wifi_mode==WIFI_MODE_AP_AND_STA) {
    s += "\"mode\":\"STA+AP\",";
  }
  s += "\"networks\":["+st+"],";
  s += "\"rssi\":["+rssi+"],";

  s += "\"ssid\":\""+esid+"\",";
  //s += "\"pass\":\""+epass+"\",";
  s += "\"srssi\":\""+String(WiFi.RSSI())+"\",";
  s += "\"ipaddress\":\""+ipaddress+"\",";
  s += "\"emoncms_server\":\""+emoncms_server+"\",";
  s += "\"emoncms_node\":\""+emoncms_node+"\",";
  s += "\"emoncms_apikey\":\""+emoncms_apikey+"\",";
  s += "\"emoncms_fingerprint\":\""+emoncms_fingerprint+"\",";
  s += "\"emoncms_connected\":\""+String(emoncms_connected)+"\",";
  s += "\"packets_sent\":\""+String(packets_sent)+"\",";
  s += "\"packets_success\":\""+String(packets_success)+"\",";

  s += "\"mqtt_server\":\""+mqtt_server+"\",";
  s += "\"mqtt_topic\":\""+mqtt_topic+"\",";
  s += "\"mqtt_feed_prefix\":\""+mqtt_feed_prefix+"\",";
  s += "\"mqtt_user\":\""+mqtt_user+"\",";
  s += "\"mqtt_pass\":\""+mqtt_pass+"\",";
  s += "\"mqtt_connected\":\""+String(mqttclient.connected())+"\",";

  s += "\"www_username\":\""+www_username+"\",";
  s += "\"www_password\":\""+www_password+"\",";

  s += "\"free_heap\":\""+String(ESP.getFreeHeap())+"\",";
  s += "\"version\":\""+currentfirmware+"\"";

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
// Handle test input API
// url /test
// e.g http://192.168.0.75/test?serial=CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7
// -------------------------------------------------------------------
void handleTest(){
  test_serial = server.arg("serial");
  server.send(200, "text/html", test_serial);
  DEBUG.println(test_serial);
}

// -------------------------------------------------------------------
// Check for updates and display current version
// url: /firmware
// -------------------------------------------------------------------
String handleUpdateCheck() {
  DEBUG.println("Running: " + currentfirmware);
  // Get latest firmware version number
  String url = u_url;
  String latestfirmware = get_http(u_host, url);
  DEBUG.println("Latest: " + latestfirmware);
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
  DEBUG.println("UPDATING...");
  delay(500);
  t_httpUpdate_return ret = ESPhttpUpdate.update("http://" + String(u_host) + String(u_url) + "?tag=" + currentfirmware);
  String str="error";
  switch(ret) {
    case HTTP_UPDATE_FAILED:
      str = DEBUG.printf("Update failed error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      str="No update, running latest firmware";
      break;
    case HTTP_UPDATE_OK:
      str="Update done!";
      break;
  }
  server.send(400,"text/html",str);
  DEBUG.println(str);
  SPIFFS.begin(); //mount-file system
}

// -------------------------------------------------------------------
// HTTPS SECURE GET Request
// url: N/A
// -------------------------------------------------------------------

String get_https(const char* fingerprint, const char* host, String url, int httpsPort){
  // Use WiFiClient class to create TCP connections
  if (!client.connect(host, httpsPort)) {
    DEBUG.print(host + httpsPort); //debug
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
      DEBUG.println(line); //debug
      if (line.startsWith("HTTP/1.1 200 OK")) {
        return("ok");
      }
    }
  }
  else {
    return("HTTPS fingerprint no match");
  }
  return("error " + String(host));
}

// -------------------------------------------------------------------
// HTTP GET Request
// url: N/A
// -------------------------------------------------------------------
String get_http(const char* host, String url){
  http.begin(String("http://") + host + String(url));
  int httpCode = http.GET();
  if((httpCode > 0) && (httpCode == HTTP_CODE_OK)){
    String payload = http.getString();
    DEBUG.println(payload);
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
  DEBUG.println("MQTT Connecting...");
  String strID = String(ESP.getChipId());
  if (mqttclient.connect(strID.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {  // Attempt to connect
    DEBUG.println("MQTT connected");
    mqttclient.publish(mqtt_topic.c_str(), "connected"); // Once connected, publish an announcement..
  } else {
    DEBUG.print("MQTT failed: ");
    DEBUG.println(mqttclient.state());
    return(0);
  }
  return (1);
}

// -------------------------------------------------------------------
// Publish to MQTT
// Split up data string into sub topics: e.g
// data = CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7
// base topic = emon/emonesp
// MQTT Publish: emon/emonesp/CT1 > 3935 etc..
// -------------------------------------------------------------------
void mqtt_publish(String base_topic, String prefix, String data){
  String mqtt_data = "";
  String topic = base_topic  +"/" + prefix;
  int i=0;
  while (int(data[i])!=0){
    // Construct MQTT topic e.g. <base_topic>/CT1 e.g. emonesp/CT1
    while (data[i]!=':'){
      topic+= data[i];
      i++;
      if (int(data[i])==0){
        break;
      }
    }
    i++;
    // Construct data string to publish to above topic
    while (data[i]!=','){
      mqtt_data+= data[i];
      i++;
      if (int(data[i])==0){
        break;
      }
    }
    // send data via mqtt
    //delay(100);
    DEBUG.printf("%s = %s\r\n", topic.c_str(), mqtt_data.c_str());
    mqttclient.publish(topic.c_str(), mqtt_data.c_str());
    topic = base_topic + "/" + prefix;
    mqtt_data="";
    i++;
    if (int(data[i])==0) break;
  }
}

// -------------------------------------------------------------------
// MQTT state management
//
// Call every time around loop() if connected to the WiFi
// -------------------------------------------------------------------
void mqtt_loop()
{
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

void emoncms_publish(String data)
{
  // We now create a URL for server data upload
  String url = e_url;
  url += "{";
  // Copy across, data length
  for (int i = 0; i < data.length(); ++i){
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

  DEBUG.println(url); delay(10);
  packets_sent++;

  // Send data to Emoncms server
  String result="";
  if (emoncms_fingerprint!=0){
    // HTTPS on port 443 if HTTPS fingerprint is present
    DEBUG.println("HTTPS Enabled"); delay(10);
    result = get_https(emoncms_fingerprint.c_str(), emoncms_server.c_str(), url, 443);
  } else {
    // Plain HTTP if other emoncms server e.g EmonPi
    DEBUG.println("Plain old HTTP"); delay(10);
    result = get_http(emoncms_server.c_str(), url);
  }
  if (result == "ok"){
    packets_success++;
    emoncms_connected = true;
  }
  else{
    emoncms_connected=false;
    DEBUG.print("Emoncms error: ");
    DEBUG.println(result);
  }
}

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
  delay(2000);
  Serial.begin(115200);
  #ifdef DEBUG_SERIAL1
  Serial1.begin(115200);
  #endif
  DEBUG.println();
  DEBUG.print("EmonESP ");
  DEBUG.println(ESP.getChipId());
  DEBUG.println("Firmware: "+ currentfirmware);

  // Read saved settings from EEPROM
  load_EEPROM_settings();

  WiFi.disconnect();
  // 1) If no network configured start up access point
  if (esid == 0 || esid == "")
  {
    startAP();
    wifi_mode = WIFI_MODE_AP_ONLY; // AP mode with no SSID in EEPROM
  }
  // 2) else try and connect to the configured network
  else
  {
    WiFi.mode(WIFI_STA);
    wifi_mode = WIFI_MODE_STA;
    startClient();
  }

  // Start local OTA update server
  ArduinoOTA.begin();

  // Start hostname broadcast in STA mode
  if ((wifi_mode==WIFI_MODE_STA || wifi_mode==WIFI_MODE_AP_AND_STA)){
    if (MDNS.begin(esp_hostname)) {
      MDNS.addService("http", "tcp", 80);
    }
  }

  // Setup firmware upload
  httpUpdater.setup(&server, firmware_update_path);

  // Start server & server root html /
  server.on("/", [](){
    if(www_username!="" && !server.authenticate(www_username.c_str(), www_password.c_str()) && wifi_mode == WIFI_MODE_STA)
      return server.requestAuthentication();
    handleHome();
  });

  // Handle HTTP web interface button presses
  server.on("/savenetwork", handleSaveNetwork);
  server.on("/saveemoncms", handleSaveEmoncms);
  server.on("/savemqtt", handleSaveMqtt);
  server.on("/saveadmin", handleSaveAdmin);
  server.on("/scan", handleScan);
  server.on("/apoff",handleAPOff);
  server.on("/firmware",handleUpdateCheck);
  server.on("/update",handleUpdate);
  server.on("/generate_204", handleHome);  //Android captive portal. Maybe not needed. Might be handled by notFound
  server.on("/fwlink", handleHome);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound

  server.on("/status", [](){
  if(www_username!="" && !server.authenticate(www_username.c_str(), www_password.c_str()) && wifi_mode == WIFI_MODE_STA)
    return server.requestAuthentication();
  handleStatus();
  });
  server.on("/lastvalues", [](){
  if(www_username!="" && !server.authenticate(www_username.c_str(), www_password.c_str()) && wifi_mode == WIFI_MODE_STA)
    return server.requestAuthentication();
  handleLastValues();
  });
  server.on("/reset", [](){
  if(www_username!="" && !server.authenticate(www_username.c_str(), www_password.c_str()) && wifi_mode == WIFI_MODE_STA)
    return server.requestAuthentication();
  handleRst();
  });
  server.on("/restart", [](){
  if(www_username!="" && !server.authenticate(www_username.c_str(), www_password.c_str()) && wifi_mode == WIFI_MODE_STA)
    return server.requestAuthentication();
  handleRestart();
  });

  server.on("/test", [](){
  if(www_username!="" && !server.authenticate(www_username.c_str(), www_password.c_str()) && wifi_mode == WIFI_MODE_STA)
    return server.requestAuthentication();
  handleTest();
  });

  server.onNotFound([](){
  if(!handleFileRead(server.uri()))
    server.send(404, "text/plain", "NotFound");
  });

  server.begin();
  DEBUG.println("Server started");
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
  if ((wifi_mode==WIFI_MODE_STA || wifi_mode==WIFI_MODE_AP_AND_STA) && mqtt_server != 0){
    mqtt_loop();
  }

  // Remain in AP mode for 5 Minutes before resetting
  if (wifi_mode == WIFI_MODE_AP_STA_RETRY){
     if ((millis() - Timer) >= 300000){
       ESP.reset();
       DEBUG.println("WIFI Mode = 1, resetting");
     }
  }

  // If data received on serial
  while(Serial.available() || test_serial !="") {
    String data = "";
  // Could check for string integrity here
    if (Serial.available()){
      data = Serial.readStringUntil('\n');

      // Get rid of any whitespace, newlines etc
      data.trim();
    }
    // If serial from test API e.g `http://<IP-ADDRESS>/test?serial=CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7`
    if (test_serial !=""){
      data = test_serial;
      test_serial = "";
    }
    DEBUG.println(data);

    last_datastr = data;
    // If Wifi connected & emoncms server details are present
    if (wifi_mode==WIFI_MODE_STA || wifi_mode==WIFI_MODE_AP_AND_STA)
    {
      // Send data to EmonCMS
      if(emoncms_apikey != 0){
        emoncms_publish(data);
      }

      // Send data to MQTT
      if (mqtt_server != 0){
        DEBUG.print("MQTT publish base-topic: "); DEBUG.println(mqtt_topic);
        mqtt_publish(mqtt_topic, mqtt_feed_prefix, data);
        String ram_topic = mqtt_topic + "/" + mqtt_feed_prefix + "freeram";
        String free_ram = String(ESP.getFreeHeap());
        mqttclient.publish(ram_topic.c_str(), free_ram.c_str());
        ram_topic = "";
        free_ram ="";
      }
    } // end wifi connected
  } // end serial available
} // end loop
