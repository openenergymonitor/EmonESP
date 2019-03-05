/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Adaptation of Chris Howells OpenEVSE ESP Wifi
   by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
   All adaptation GNU General Public License as below.

   -------------------------------------------------------------------

   This file is part of OpenEnergyMonitor.org project.
   EmonESP is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   EmonESP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with EmonESP; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "emonesp.h"
#include "wifi.h"
#include "config.h"
#include <ESP8266WiFi.h>              // Connect to Wifi
#include <ESP8266mDNS.h>              // Resolve URL for update server etc.
#include <DNSServer.h>                // Required for captive portal


int button_interval_one = 100; // milliseconds for action, test.
int button_interval_two = (5 * 1000); // 5 seconds hold down AP mode.
int button_interval_three = (10 * 1000); // 10 seconds hold down GPIO0 for factory reset.
int timebuttonpressed;
bool buttonflag = false;
bool button_interval_one_passed = false;
bool button_interval_two_passed = false;
bool button_interval_three_passed = false;


DNSServer dnsServer;                  // Create class DNS server, captive portal re-direct
const byte DNS_PORT = 53;

// Access Point SSID, password & IP address. SSID will be softAP_ssid + chipID to make SSID unique
// const char *softAP_ssid = "emonESP";
const char* softAP_password = "";
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// Wifi Network Strings
String connected_network = "";
String status_string = "";
String ipaddress = "";

unsigned long Timer;
String st, rssi;

// -------------------------------------------------------------------
int wifi_mode = WIFI_MODE_STA;


// -------------------------------------------------------------------
// Start Access Point
// Access point is used for wifi network selection
// -------------------------------------------------------------------
void
startAP() {
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
  for (int i = 0; i < n; ++i) {
    st += "\"" + WiFi.SSID(i) + "\"";
    rssi += "\"" + String(WiFi.RSSI(i)) + "\"";
    if (i < n - 1)
      st += ",";
    if (i < n - 1)
      rssi += ",";
  }
  delay(100);

  WiFi.softAPConfig(apIP, apIP, netMsk);
  // Create Unique SSID e.g "emonESP_XXXXXX"
  // String softAP_ssid_ID = String(softAP_ssid) + "_" + String(node_id);
  WiFi.softAP(node_name.c_str(), softAP_password);

  // Setup the DNS server redirecting all the domains to the apIP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  IPAddress myIP = WiFi.softAPIP();
  char tmpStr[40];
  sprintf(tmpStr, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  DEBUG.print("AP IP Address: ");
  DEBUG.println(tmpStr);
  ipaddress = tmpStr;
}

// -------------------------------------------------------------------
// Start Client, attempt to connect to Wifi network
// -------------------------------------------------------------------
void
startClient() {
  DEBUG.print("Connecting to SSID: ");
  DEBUG.print(esid.c_str());
  DEBUG.print(" PSK:");
  DEBUG.println(epass.c_str());
  
  WiFi.hostname(node_name.c_str());

  //WiFi.mode(WIFI_STA);
  digitalWrite(LEDpin,LOW);
  WiFi.begin(esid.c_str(),epass.c_str());

  int t = 0;
  int attempt = 0;
  unsigned long start_connect = millis();
  while (WiFi.status() != WL_CONNECTED) {

    delay(200);
    digitalWrite(LEDpin,HIGH); delay(20); digitalWrite(LEDpin,LOW);

    // Timeout
    if ((millis()-start_connect)>30000) {
      delay(2000);
      digitalWrite(LEDpin,HIGH);
      ESP.reset(); // ESP.restart(); ?
    }

    if (digitalRead(0) == LOW) {
      startAP();
      wifi_mode = WIFI_MODE_AP_ONLY;
      break;
    }
  }

  if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_AND_STA) {

    IPAddress myAddress = WiFi.localIP();
    char tmpStr[40];
    sprintf(tmpStr, "%d.%d.%d.%d", myAddress[0], myAddress[1], myAddress[2],myAddress[3]);
    DEBUG.print("Connected, IP: ");
    DEBUG.println(tmpStr);
    // Copy the connected network and ipaddress to global strings for use in status request
    connected_network = esid;
    ipaddress = tmpStr;

    for (int f=0; f<10; f++) {
        digitalWrite(LEDpin,HIGH); 
        delay(80); 
        digitalWrite(LEDpin,LOW); 
        delay(20);
    }
    digitalWrite(LEDpin,HIGH); 
  } else {
    DEBUG.print("startClient wifi_mode is not STA??");
  }
}

void wifi_setup() {

  // WiFi.disconnect();
  // 1) If no network configured start up access point
  if (esid == 0 || esid == "" || digitalRead(0) == LOW) {
    startAP();
    wifi_mode = WIFI_MODE_AP_ONLY; // AP mode with no SSID in EEPROM
  }
  // 2) else try and connect to the configured network
  else {
    WiFi.mode(WIFI_STA);
    wifi_mode = WIFI_MODE_STA;
    startClient();
  }

  // Start hostname broadcast in STA mode
  if ((wifi_mode==WIFI_MODE_STA || wifi_mode==WIFI_MODE_AP_AND_STA)){
    if (MDNS.begin(node_name.c_str())) {
      MDNS.addService("http", "tcp", 80);
    }
  }

  Timer = millis();
}

void wifi_loop() {

  // GPIO0 button, set AP mode and factory reset.
  if (buttonflag == true && digitalRead(0) == HIGH) {
    Serial.println("Button released.");
    button_interval_one_passed = false;
    button_interval_two_passed = false;
    button_interval_three_passed = false;
    delay(10);
  }

  bool button = !digitalRead(0);

  if (button == false) {
    timebuttonpressed = 0;
    buttonflag = false;
  }
  else if (button == true && timebuttonpressed == 0) {
    timebuttonpressed = millis();
    Serial.println("Button Pressed...");
    Serial.println("5 seconds until AP mode");
    Serial.println("10 seconds until Factory Reset.");
    delay(10);
    buttonflag = true;
  }
  else if (button == true && timebuttonpressed > 0) {
    if (timebuttonpressed + button_interval_one <= millis() && button_interval_one_passed == false) {
      Serial.println("testing first interval.");
      button_interval_one_passed = true;
    }
    if (timebuttonpressed + button_interval_two <= millis() && button_interval_two_passed == false) {
      Serial.println("AP mode starting..");
      wifi_mode = WIFI_MODE_AP_ONLY;
      startAP();
      button_interval_two_passed = true;
    }
    if (timebuttonpressed + button_interval_three <= millis()) {
      Serial.println("Commencing factory reset.");
      delay(500);
      config_reset();
      ESP.eraseConfig();
      Serial.println("Factory reset complete! Resetting...");
      delay(500);
      ESP.reset();
    }
  }
// end GPIO0 button.


  dnsServer.processNextRequest(); // Captive portal DNS re-dierct

  // Remain in AP mode for 5 Minutes before resetting
  if (wifi_mode == WIFI_MODE_AP_STA_RETRY) {
    if ((millis() - Timer) >= 300000) {
      ESP.reset();
      DEBUG.println("WIFI Mode = 1, resetting");
    }
  }
}

void wifi_restart() {
  // Startup in STA + AP mode
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMsk);

  // Create Unique SSID e.g "emonESP_XXXXXX"
  // String softAP_ssid_ID = String(softAP_ssid) + "_" + String(ESP.getChipId());;
  WiFi.softAP(node_name.c_str(), softAP_password);

  // Setup the DNS server redirecting all the domains to the apIP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  wifi_mode = WIFI_MODE_AP_AND_STA;
  startClient();
}

void
wifi_scan() {
  DEBUG.println("WIFI Scan");
  int n = WiFi.scanNetworks();
  DEBUG.print(n);
  DEBUG.println(" networks found");
  st = "";
  rssi = "";
  for (int i = 0; i < n; ++i) {
    st += "\"" + WiFi.SSID(i) + "\"";
    rssi += "\"" + String(WiFi.RSSI(i)) + "\"";
    if (i < n - 1)
      st += ",";
    if (i < n - 1)
      rssi += ",";
  }
}

void
wifi_disconnect() {
  WiFi.disconnect();
}
