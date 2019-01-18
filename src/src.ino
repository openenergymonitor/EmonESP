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

#include "emonesp.h"
#include "config.h"
#include "wifi.h"
#include "web_server.h"
#include "ota.h"
#include "input.h"
#include "emoncms.h"
#include "mqtt.h"
#include "http.h"
#include "autoauth.h"
#include <NTPClient.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"europe.pool.ntp.org",0,60000);
unsigned long last_ctrl_update = 0;

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

  // Read saved settings from the config
  config_load_settings();

  // ---------------------------------------------------------
  // Hard-coded initial config for node_name and node_describe
  // ---------------------------------------------------------
  node_type = "smartplug";
  node_description = "Sonoff S20 Smartplug";
  node_id = 5;
  
  node_name = node_type + String(node_id);
  node_status = "emon/"+node_name+"/status";
  
  node_describe = "describe:"+node_type;
  // ---------------------------------------------------------

  pinMode(LEDpin, OUTPUT);

  if (node_type=="smartplug") {
    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);
    pinMode(16, OUTPUT);
    led_flash(3000,100);    
  } else if (node_type=="wifirelay") {
    pinMode(5, OUTPUT);
  } else if (node_type=="huzzah") {
    pinMode(2,OUTPUT);
    digitalWrite(2,HIGH);
  }

  // Initialise the WiFi
  wifi_setup();
  led_flash(50,50);

  // Bring up the web server
  web_server_setup();
  led_flash(50,50);

  // Start the OTA update systems
  ota_setup();

  DEBUG.println("Server started");

  // Start auto auth
  auth_setup();

  // Time
  timeClient.begin();
  
  delay(100);
} // end setup

void led_flash(int ton, int toff) {
  digitalWrite(LEDpin,LOW); delay(ton); digitalWrite(LEDpin,HIGH); delay(toff);
}

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void loop()
{
  ota_loop();
  web_server_loop();
  wifi_loop();
  timeClient.update();

  String input = "";
  boolean gotInput = input_get(input);

  if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_AND_STA)
  {
    if(emoncms_apikey != 0 && gotInput) {
      emoncms_publish(input);
    }
    if(mqtt_server != 0)
    {
      mqtt_loop();
      if(gotInput) {
        mqtt_publish(input);
      }
    }
  }

  auth_loop();

  // --------------------------------------------------------------
  // CONTROL UPDATE
  // --------------------------------------------------------------
  if ((millis()-last_ctrl_update)>1000 || ctrl_update) {
    last_ctrl_update = millis();
    ctrl_update = 0;
    ctrl_state = 0; // default off

    // 1. Timer
    int start1 = timer_start1.toInt();
    int stop1 = timer_stop1.toInt();
    int start2 = timer_start2.toInt();
    int stop2 = timer_stop2.toInt();
    int timenow = timeClient.getHours()*100+timeClient.getMinutes();
    
    if (timenow>=start1 && timenow<stop1) ctrl_state = 1;
    if (timenow>=start2 && timenow<stop2) ctrl_state = 1;

    // 2. On/Off
    if (ctrl_mode=="On") ctrl_state = 1;
    if (ctrl_mode=="Off") ctrl_state = 0;

    // 3. Apply
    if (ctrl_state) {
      // ON
      if (node_type=="smartplug") {
        digitalWrite(12,HIGH);
        digitalWrite(16,HIGH);
      } else if (node_type=="wifirelay") {
        digitalWrite(5,HIGH);
      } else if (node_type=="huzzah") {
        digitalWrite(2,LOW);
      }
    } else {
      // OFF
      if (node_type=="smartplug") {
        digitalWrite(12,LOW);
        digitalWrite(16,LOW);
      } else if (node_type=="wifirelay") {
        digitalWrite(5,LOW);
      } else if (node_type=="huzzah") {
        digitalWrite(2,HIGH);
      }
    }   
  }
  // --------------------------------------------------------------
  
} // end loop

String getTime() {
    return timeClient.getFormattedTime();
}

