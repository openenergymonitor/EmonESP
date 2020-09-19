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
#include "app_config.h"
#include <ESP8266WiFi.h>              // Connect to Wifi
#include <ESP8266mDNS.h>              // Resolve URL for update server etc.
#include <DNSServer.h>                // Required for captive portal

DNSServer dnsServer;                  // Create class DNS server, captive portal re-direct
const byte DNS_PORT = 53;

// Access Point SSID, password & IP address. SSID will be softAP_ssid + chipID to make SSID unique
// const char *softAP_ssid = "emonESP";
const char* softAP_password = "";
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
int apClients = 0;

// Wifi Network Strings
String connected_network = "";
String ipaddress = "";

int client_disconnects = 0;
bool client_retry = false;
unsigned long client_retry_time = 0;

#ifdef WIFI_LED
#ifndef WIFI_LED_ON_STATE
#define WIFI_LED_ON_STATE LOW
#endif

#ifndef WIFI_LED_AP_TIME
#define WIFI_LED_AP_TIME 1000
#endif

#ifndef WIFI_LED_AP_CONNECTED_TIME
#define WIFI_LED_AP_CONNECTED_TIME 100
#endif

#ifndef WIFI_LED_STA_CONNECTING_TIME
#define WIFI_LED_STA_CONNECTING_TIME 500
#endif

int wifiLedState = !WIFI_LED_ON_STATE;
unsigned long wifiLedTimeOut = millis();
#endif

#ifndef WIFI_BUTTON
#define WIFI_BUTTON 0
#endif

#ifndef WIFI_BUTTON_AP_TIMEOUT
#define WIFI_BUTTON_AP_TIMEOUT              (5 * 1000)
#endif

#ifndef WIFI_BUTTON_FACTORY_RESET_TIMEOUT
#define WIFI_BUTTON_FACTORY_RESET_TIMEOUT   (10 * 1000)
#endif

#ifndef WIFI_CLIENT_RETRY_TIMEOUT
#define WIFI_CLIENT_RETRY_TIMEOUT (5 * 60 * 1000)
#endif

int wifiButtonState = HIGH;
unsigned long wifiButtonTimeOut = millis();
bool apMessage = false;

// -------------------------------------------------------------------
// Start Access Point
// Access point is used for wifi network selection
// -------------------------------------------------------------------
void
startAP() {
  DBUGLN("Starting AP");

  if (wifi_mode_is_sta()) {
    WiFi.disconnect(true);
  }

  WiFi.enableAP(true);

  WiFi.softAPConfig(apIP, apIP, netMsk);
  // Create Unique SSID e.g "emonESP_XXXXXX"
  // String softAP_ssid_ID = String(softAP_ssid) + "_" + String(node_id);
  // Pick a random channel out of 1, 6 or 11
  int channel = (random(3) * 5) + 1;
  WiFi.softAP(node_name.c_str(), softAP_password, channel);

  // Setup the DNS server redirecting all the domains to the apIP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  IPAddress myIP = WiFi.softAPIP();
  char tmpStr[40];
  sprintf(tmpStr, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  DEBUG.print(F("AP IP Address: "));
  DEBUG.println(tmpStr);
  ipaddress = tmpStr;
    
  apClients = 0;
}

// -------------------------------------------------------------------
// Start Client, attempt to connect to Wifi network
// -------------------------------------------------------------------
void
startClient()
{
  DEBUG.print(F("Connecting to SSID: "));
  DEBUG.println(esid.c_str());
  //DEBUG.print(F(" PSK:"));
  //DEBUG.println(epass.c_str());
  
  client_disconnects = 0;

  WiFi.begin(esid.c_str(), epass.c_str());
  WiFi.hostname(node_name.c_str());
  WiFi.enableSTA(true);
}

static void wifi_start()
{
  // 1) If no network configured start up access point
  DBUGVAR(esid);
  if (esid == 0 || esid == "")
  {
    startAP();
  }
  // 2) else try and connect to the configured network
  else 
  {
    startClient();
  }
}

void wifi_onStationModeGotIP(const WiFiEventStationModeGotIP &event)
{
  #ifdef WIFI_LED
    wifiLedState = WIFI_LED_ON_STATE;
    digitalWrite(WIFI_LED, wifiLedState);
  #endif

  IPAddress myAddress = WiFi.localIP();
  char tmpStr[40];
  sprintf_P(tmpStr, PSTR("%d.%d.%d.%d"), myAddress[0], myAddress[1], myAddress[2], myAddress[3]);
  ipaddress = tmpStr;
  DEBUG.print(F("Connected, IP: "));
  DEBUG.println(tmpStr);

  // Copy the connected network and ipaddress to global strings for use in status request
  connected_network = esid;

  // Clear any error state
  client_disconnects = 0;
  client_retry = false;

  if (MDNS.begin(node_name.c_str())) {
    MDNS.addService("http", "tcp", 80);
    MDNSResponder::hMDNSService hService = MDNS.addService(NULL, "emonesp", "tcp", 80);
    if(hService) {
      MDNS.addServiceTxt(hService, "node_type", node_type.c_str());
    }
  } else {
    DEBUG_PORT.println(F("Failed to start mDNS"));
  }
}

void wifi_onStationModeDisconnected(const WiFiEventStationModeDisconnected &event)
{
  DBUG("WiFi dissconnected: ");
  DBUGLN(
  WIFI_DISCONNECT_REASON_UNSPECIFIED == event.reason ? F("WIFI_DISCONNECT_REASON_UNSPECIFIED") :
  WIFI_DISCONNECT_REASON_AUTH_EXPIRE == event.reason ? F("WIFI_DISCONNECT_REASON_AUTH_EXPIRE") :
  WIFI_DISCONNECT_REASON_AUTH_LEAVE == event.reason ? F("WIFI_DISCONNECT_REASON_AUTH_LEAVE") :
  WIFI_DISCONNECT_REASON_ASSOC_EXPIRE == event.reason ? F("WIFI_DISCONNECT_REASON_ASSOC_EXPIRE") :
  WIFI_DISCONNECT_REASON_ASSOC_TOOMANY == event.reason ? F("WIFI_DISCONNECT_REASON_ASSOC_TOOMANY") :
  WIFI_DISCONNECT_REASON_NOT_AUTHED == event.reason ? F("WIFI_DISCONNECT_REASON_NOT_AUTHED") :
  WIFI_DISCONNECT_REASON_NOT_ASSOCED == event.reason ? F("WIFI_DISCONNECT_REASON_NOT_ASSOCED") :
  WIFI_DISCONNECT_REASON_ASSOC_LEAVE == event.reason ? F("WIFI_DISCONNECT_REASON_ASSOC_LEAVE") :
  WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED == event.reason ? F("WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED") :
  WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD == event.reason ? F("WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD") :
  WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD == event.reason ? F("WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD") :
  WIFI_DISCONNECT_REASON_IE_INVALID == event.reason ? F("WIFI_DISCONNECT_REASON_IE_INVALID") :
  WIFI_DISCONNECT_REASON_MIC_FAILURE == event.reason ? F("WIFI_DISCONNECT_REASON_MIC_FAILURE") :
  WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT == event.reason ? F("WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT") :
  WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT == event.reason ? F("WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT") :
  WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS == event.reason ? F("WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS") :
  WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID == event.reason ? F("WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID") :
  WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID == event.reason ? F("WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID") :
  WIFI_DISCONNECT_REASON_AKMP_INVALID == event.reason ? F("WIFI_DISCONNECT_REASON_AKMP_INVALID") :
  WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION == event.reason ? F("WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION") :
  WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP == event.reason ? F("WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP") :
  WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED == event.reason ? F("WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED") :
  WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED == event.reason ? F("WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED") :
  WIFI_DISCONNECT_REASON_BEACON_TIMEOUT == event.reason ? F("WIFI_DISCONNECT_REASON_BEACON_TIMEOUT") :
  WIFI_DISCONNECT_REASON_NO_AP_FOUND == event.reason ? F("WIFI_DISCONNECT_REASON_NO_AP_FOUND") :
  WIFI_DISCONNECT_REASON_AUTH_FAIL == event.reason ? F("WIFI_DISCONNECT_REASON_AUTH_FAIL") :
  WIFI_DISCONNECT_REASON_ASSOC_FAIL == event.reason ? F("WIFI_DISCONNECT_REASON_ASSOC_FAIL") :
  WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT == event.reason ? F("WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT") :
  F("UNKNOWN"));

  client_disconnects++;

  MDNS.end();
}

void
wifi_setup() {
#ifdef WIFI_LED
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, wifiLedState);
#endif

  randomSeed(analogRead(0));

  // If we have an SSID configured at this point we have likely
  // been running another firmware, clear the results
  if(wifi_is_client_configured()) {
    WiFi.persistent(true);
    WiFi.disconnect();
    ESP.eraseConfig();
  }

  // Stop the WiFi module
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);

  static auto _onStationModeConnected = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected &event) { DBUGF("Connected to %s", event.ssid.c_str()); });
  static auto _onStationModeGotIP = WiFi.onStationModeGotIP(wifi_onStationModeGotIP);
  static auto _onStationModeDisconnected = WiFi.onStationModeDisconnected(wifi_onStationModeDisconnected);
  static auto _onSoftAPModeStationConnected = WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected &event) {
    apClients++;
  });
  static auto _onSoftAPModeStationDisconnected = WiFi.onSoftAPModeStationDisconnected([](const WiFiEventSoftAPModeStationDisconnected &event) {
    apClients--;
  });

  wifi_start();
}

void wifi_loop() 
{
  Profile_Start(wifi_loop);

  bool isClient = wifi_mode_is_sta();
  bool isClientOnly = wifi_mode_is_sta_only();
//  bool isAp = wifi_mode_is_ap();
  bool isApOnly = wifi_mode_is_ap_only();

  if(isClient) {
    MDNS.update();
  }
#ifdef WIFI_LED
  if ((isApOnly || !WiFi.isConnected()) &&
      millis() > wifiLedTimeOut)
  {
    wifiLedState = !wifiLedState;
    digitalWrite(WIFI_LED, wifiLedState);

    int ledTime = isApOnly ? (0 == apClients ? WIFI_LED_AP_TIME : WIFI_LED_AP_CONNECTED_TIME) : WIFI_LED_STA_CONNECTING_TIME;
    wifiLedTimeOut = millis() + ledTime;
  }
#endif

#if defined(WIFI_LED) && WIFI_BUTTON == WIFI_LED
  digitalWrite(WIFI_BUTTON, HIGH);
  pinMode(WIFI_BUTTON, INPUT_PULLUP);
#endif

  // Pressing the boot button for 5 seconds will turn on AP mode, 10 seconds will factory reset
  int button = digitalRead(WIFI_BUTTON);

#if defined(WIFI_LED) && WIFI_BUTTON == WIFI_LED
  pinMode(WIFI_BUTTON, OUTPUT);
  digitalWrite(WIFI_LED, wifiLedState);
#endif

  //DBUGF("%lu %d %d", millis() - wifiButtonTimeOut, button, wifiButtonState);
  if(wifiButtonState != button)
  {
    wifiButtonState = button;
    if(LOW == button) {
      DBUGF("Button pressed");
      wifiButtonTimeOut = millis();
    } else {
      DBUGF("Button released");
      if(millis() > wifiButtonTimeOut + WIFI_BUTTON_AP_TIMEOUT) {
        startAP();
      }
    }
  }

  if(LOW == wifiButtonState && millis() > wifiButtonTimeOut + WIFI_BUTTON_FACTORY_RESET_TIMEOUT)
  {
    delay(1000);

    config_reset();
    ESP.eraseConfig();

    delay(50);
    ESP.reset();
  }

  // Manage state while connecting
  if(isClientOnly && !WiFi.isConnected())
  {
    // If we have failed to connect turn on the AP
    if(client_disconnects > 2) {
      startAP();
      client_retry = true;
      client_retry_time = millis() + WIFI_CLIENT_RETRY_TIMEOUT;
    }
  }

  // Remain in AP mode for 5 Minutes before resetting
  if(isApOnly && 0 == apClients && client_retry && millis() > client_retry_time) {
    DEBUG.println(F("client re-try, resetting"));
    delay(50);
    ESP.reset();
  }

  dnsServer.processNextRequest(); // Captive portal DNS re-dierct

  Profile_End(wifi_loop, 5);
}

void wifi_restart() {
  wifi_disconnect();
  wifi_start();
}

void wifi_disconnect() {
  wifi_turn_off_ap();
  if (wifi_mode_is_sta()) {
    WiFi.disconnect(true);
  }
}

void wifi_turn_off_ap()
{
  if(wifi_mode_is_ap())
  {
    WiFi.softAPdisconnect(true);
    dnsServer.stop();
  }
}

void wifi_turn_on_ap()
{
  DBUGF("wifi_turn_on_ap %d", WiFi.getMode());
  if(!wifi_mode_is_ap()) {
    startAP();
  }
}

bool wifi_client_connected()
{
  return WiFi.isConnected() && (WIFI_STA == (WiFi.getMode() & WIFI_STA));
}
