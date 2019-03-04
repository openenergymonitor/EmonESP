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
#include "FSWebServerLib.h"

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
const char *softAP_ssid = "emonESP";
const char* softAP_password = "";
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// hostname for mDNS. Should work at least on windows. Try http://emonesp.local
const char *esp_hostname = "emonesp";

// Wifi Network Strings
String connected_network = "";
String status_string = "";
String ipaddress = "";

unsigned long Timer;
String st, rssi;

#ifdef WIFI_LED
#ifndef WIFI_LED_ON_STATE
#define WIFI_LED_ON_STATE LOW
#endif

#ifndef WIFI_LED_AP_TIME
#define WIFI_LED_AP_TIME 1000
#endif

#ifndef WIFI_LED_STA_CONNECTING_TIME
#define WIFI_LED_STA_CONNECTING_TIME 500
#endif

int wifiLedState = !WIFI_LED_ON_STATE;
unsigned long wifiLedTimeOut = millis();
#endif

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
  String softAP_ssid_ID =
    String(softAP_ssid) + "_" + String(ESP.getChipId());;
  WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password);

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
  DEBUG.println(esid.c_str());
  // DEBUG.print(" epass:");
  // DEBUG.println(epass.c_str());
  WiFi.hostname(esp_hostname);
  WiFi.begin(esid.c_str(), epass.c_str());

  delay(50);

  int t = 0;
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
#ifdef WIFI_LED
    wifiLedState = !wifiLedState;
    digitalWrite(WIFI_LED, wifiLedState);
#endif

    delay(500);
    t++;
    // push and hold boot button after power on to skip stright to AP mode
    if (t >= 20
#if !defined(WIFI_LED) || 0 != WIFI_LED
        || digitalRead(0) == LOW
#endif
       ) {
      DEBUG.println(" ");
      DEBUG.println("Try Again...");
      delay(2000);
      WiFi.disconnect();
      WiFi.begin(esid.c_str(), epass.c_str());
      t = 0;
      attempt++;
      if (attempt >= 5 || digitalRead(0) == LOW) {
        startAP();
        // AP mode with SSID, connection will retry in 5 minutes
        wifi_mode = WIFI_MODE_AP_STA_RETRY;
        break;
      }
    }
  }

  if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_AND_STA) {
#ifdef WIFI_LED
    wifiLedState = WIFI_LED_ON_STATE;
    digitalWrite(WIFI_LED, wifiLedState);
#endif

    IPAddress myAddress = WiFi.localIP();
    char tmpStr[40];
    sprintf(tmpStr, "%d.%d.%d.%d", myAddress[0], myAddress[1], myAddress[2],
            myAddress[3]);
    DEBUG.print("Connected, IP: ");
    DEBUG.println(tmpStr);
    // Copy the connected network and ipaddress to global strings for use in status request
    connected_network = esid;
    ipaddress = tmpStr;
  }
}

void
wifi_setup() {
#ifdef WIFI_LED
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, wifiLedState);
#endif

  WiFi.disconnect();
  // 1) If no network configured start up access point
  if (esid == 0 || esid == "") {
    startAP();
    wifi_mode = WIFI_MODE_AP_ONLY; // AP mode with no SSID
  }
  // 2) else try and connect to the configured network
  else {
    WiFi.mode(WIFI_STA);
    wifi_mode = WIFI_MODE_STA;
    startClient();
  }

  // Start hostname broadcast in STA mode
  if ((wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_AND_STA)) {
    if (MDNS.begin(esp_hostname)) {
      MDNS.addService("http", "tcp", 80);
    }
  }

  Timer = millis();
}

void
wifi_loop() {
#ifdef WIFI_LED
  if (wifi_mode == WIFI_MODE_AP_ONLY && millis() > wifiLedTimeOut) {
    wifiLedState = !wifiLedState;
    digitalWrite(WIFI_LED, wifiLedState);
    wifiLedTimeOut = millis() + WIFI_LED_AP_TIME;
  }
#endif


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

void
wifi_restart() {
  // Startup in STA + AP mode
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMsk);

  // Create Unique SSID e.g "emonESP_XXXXXX"
  String softAP_ssid_ID =
    String(softAP_ssid) + "_" + String(ESP.getChipId());;
  WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password);

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
