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
NTPClient timeClient(ntpUDP,"europe.pool.ntp.org",time_offset,60000);
unsigned long last_ctrl_update = 0;
unsigned long last_pushbtn_check = 0;
bool pushbtn_action = 0;
bool pushbtn_state = 0;
bool last_pushbtn_state = 0;

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
  timeClient.setTimeOffset(time_offset);

  // EmonEsp is designed for use with the following node types:
  // Sonoff S20 Smartplug
  #ifdef SMARTPLUG
    node_type = "smartplug";
    node_description = "Sonoff Smartplug";
    LEDpin = 13;
    CONTROLpin = 12; // or/and 16 test!!
  // Martin Harizanov's ProSmart WIFI Relay
  #elif WIFIRELAY
    node_type = "wifirelay";
    node_description = "WiFi Relay";
    LEDpin = 16;
    LEDpin_inverted = 0;
    CONTROLpin = 5;
  // Heatpump Monitor, used for heating on/off and flow temp control of FTC2B Controller
  #elif HPMON
    node_type = "hpmon";
    node_description = "Heatpump Monitor";
    LEDpin = 2;
    CONTROLpin = 5;
  // Default: ESP12E Huzzah WiFi adapter
  #else 
    node_type = "espwifi";
    node_description = "WiFi Emoncms Link";
    LEDpin = 2;
    CONTROLpin = 2;
  #endif
  
  Serial.print("Node type: ");
  Serial.println(node_type);

  unsigned long chip_id = ESP.getChipId();
  int chip_tmp = chip_id / 10000;
  chip_tmp = chip_tmp * 10000;
  node_id = (chip_id - chip_tmp);
  
  node_name = node_type + String(node_id);  
  node_describe = "describe:"+node_type;

  // ---------------------------------------------------------
  // pin setup
  pinMode(LEDpin, OUTPUT);
  pinMode(CONTROLpin, OUTPUT);
  digitalWrite(CONTROLpin,LOW);
  // custom: analog output pin
  if (node_type=="hpmon") pinMode(4,OUTPUT);
  // ---------------------------------------------------------
  
  // Initial LED on
  led_flash(3000,100);

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
  if (LEDpin_inverted) {
      digitalWrite(LEDpin,LOW); delay(ton); digitalWrite(LEDpin,HIGH); delay(toff);
  } else {
      digitalWrite(LEDpin,HIGH); delay(ton); digitalWrite(LEDpin,LOW); delay(toff);  
  }
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
        mqtt_publish_keyval(input);
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
    int timenow = timeClient.getHours()*100+timeClient.getMinutes();
    
    if (timer_stop1>=timer_start1 && (timenow>=timer_start1 && timenow<timer_stop1)) ctrl_state = 1;
    if (timer_stop2>=timer_start2 && (timenow>=timer_start2 && timenow<timer_stop2)) ctrl_state = 1;

    if (timer_stop1<timer_start1 && (timenow>=timer_start1 || timenow<timer_stop1)) ctrl_state = 1;
    if (timer_stop2<timer_start2 && (timenow>=timer_start2 || timenow<timer_stop2)) ctrl_state = 1;    

    // 2. On/Off
    if (ctrl_mode=="On") ctrl_state = 1;
    if (ctrl_mode=="Off") ctrl_state = 0;

    // 3. Apply
    if (ctrl_state) {
      // ON
      if (node_type=="espwifi") {
        digitalWrite(CONTROLpin,LOW);
      } else {
        digitalWrite(CONTROLpin,HIGH);
      }
    } else {
      // OFF
      if (node_type=="espwifi") {
        digitalWrite(CONTROLpin,HIGH);
      } else {
        digitalWrite(CONTROLpin,LOW);
      }
    }

    if (node_type=="hpmon") {
      analogWrite(4,voltage_output);
    }
  }
  // --------------------------------------------------------------
  if ((millis()-last_pushbtn_check)>100) {
    last_pushbtn_check = millis();

    last_pushbtn_state = pushbtn_state;
    pushbtn_state = !digitalRead(0);
    
    if (pushbtn_state && last_pushbtn_state && !pushbtn_action) {
      pushbtn_action = 1;
      if (ctrl_mode=="On") ctrl_mode = "Off"; else ctrl_mode = "On";
      if (mqtt_server!=0) mqtt_publish("out/ctrlmode",String(ctrl_mode));

    }
    if (!pushbtn_state && !last_pushbtn_state) pushbtn_action = 0;
  }
  
} // end loop

String getTime() {
    return timeClient.getFormattedTime();
}

void setTimeOffset() {
    timeClient.setTimeOffset(time_offset);
}

