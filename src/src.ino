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
#include "app_config.h"
#include "wifi.h"
#include "web_server.h"
#include "ota.h"
#include "input.h"
#include "emoncms.h"
#include "mqtt.h"
#include "http.h"
#include "autoauth.h"

#include <time.h>     // time() ctime()
#include <sys/time.h> // struct timeval

#define TZ 1      // (utc+) TZ in hours
#define DST_MN 60 // use 60mn for summer time in some countries

#define TZ_MN ((TZ)*60)
#define TZ_SEC ((TZ)*3600)
#define DST_SEC ((DST_MN)*60)

WiFiUDP ntpUDP;
unsigned long last_ctrl_update = 0;
unsigned long last_pushbtn_check = 0;
bool pushbtn_action = false;
bool pushbtn_state = false;
bool last_pushbtn_state = false;

static uint32_t last_mem = 0;
static uint32_t start_mem = 0;
static unsigned long mem_info_update = 0;

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup()
{
  debug_setup();

  DEBUG.println();
  DEBUG.println();
  DEBUG.print("EmonESP ");
  DEBUG.println(ESP.getChipId());
  DEBUG.println("Firmware: " + currentfirmware);
  DEBUG.printf("Free: %d\n", ESP.getFreeHeap());

  DBUG("Node type: ");
  DBUGLN(node_type);

  // Read saved settings from the config
  config_load_settings();
  DBUGF("After config_load_settings: %d", ESP.getFreeHeap());

  configTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");

  DBUG("Node name: ");
  DBUGLN(node_name);

  // ---------------------------------------------------------
  // pin setup
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, !WIFI_LED_ON_STATE);

  pinMode(CONTROL_PIN, OUTPUT);
  digitalWrite(CONTROL_PIN, !CONTROL_PIN_ON_STATE);

// custom: analog output pins
  #ifdef VOLTAGE_OUT_PIN
  pinMode(4, OUTPUT);
  #endif
  // ---------------------------------------------------------

  // Initial LED on
  led_flash(3000, 100);

  // Initialise the WiFi
  wifi_setup();
  DBUGF("After wifi_setup: %d", ESP.getFreeHeap());
  led_flash(50, 50);

  // Bring up the web server
  web_server_setup();
  DBUGF("After web_server_setup: %d", ESP.getFreeHeap());
  led_flash(50, 50);

  // Start the OTA update systems
  ota_setup();
  DBUGF("After ota_setup: %d", ESP.getFreeHeap());

  // Start auto auth
  auth_setup();
  DBUGF("After auth_setup: %d", ESP.getFreeHeap());

  delay(100);

  start_mem = last_mem = ESP.getFreeHeap();
} // end setup

void led_flash(int ton, int toff)
{
  digitalWrite(WIFI_LED, WIFI_LED_ON_STATE);
  delay(ton);
  digitalWrite(WIFI_LED, WIFI_LED_ON_STATE);
  delay(toff);
}

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void loop()
{
  if (millis() > mem_info_update)
  {
    mem_info_update = millis() + 2000;
    uint32_t current = ESP.getFreeHeap();
    int32_t diff = (int32_t)(last_mem - current);
    if (diff != 0)
    {
      DEBUG.printf("Free memory %u - diff %d %d\n", current, diff, start_mem - current);
      last_mem = current;
    }
  }

  ota_loop();
  web_server_loop();
  wifi_loop();

  StaticJsonDocument<512> data;
  bool gotInput = input_get(data);

  if (wifi_client_connected())
  {
    mqtt_loop();
    if (gotInput)
    {
      emoncms_publish(data);
      event_send(data);
    }
  }

  auth_loop();

  // --------------------------------------------------------------
  // CONTROL UPDATE
  // --------------------------------------------------------------
  if ((millis() - last_ctrl_update) > 1000 || ctrl_update)
  {
    last_ctrl_update = millis();
    ctrl_update = false;
    ctrl_state = false;  // default OFF

    auto now = time(nullptr);
    auto datetimenow = localtime(&now);

    int timenow = datetimenow->tm_hour * 100 + datetimenow->tm_min;
    int datenow = getDateAsInt();
    int day = datetimenow->tm_wday;

    if (ctrl_mode == "Timer")
    {
    // 1. Timer
        if (timer_stop1 >= timer_start1 && (timenow >= timer_start1 && timenow < timer_stop1))
          ctrl_state = true;
        if (timer_stop1 < timer_start1 && (timenow >= timer_start1 || timenow < timer_stop1))
          ctrl_state = true;
    }
    else if (ctrl_mode == "On") // 2. On
      ctrl_state = true;
    else // 2'. Off (default)
      ctrl_state = false;

    // 3. Apply
    if (ctrl_state)
    {
      // ON
      digitalWrite(CONTROL_PIN, CONTROL_PIN_ON_STATE);
    }
    else
    {
      digitalWrite(CONTROL_PIN, !CONTROL_PIN_ON_STATE);
    }

    #ifdef VOLTAGE_OUT_PIN
    analogWrite(VOLTAGE_OUT_PIN, voltage_output);
    #endif
  }
  // --------------------------------------------------------------
  if (node_type == "smartplug" && (millis() - last_pushbtn_check) > 100)
  {
    last_pushbtn_check = millis();

    last_pushbtn_state = pushbtn_state;
    pushbtn_state = !digitalRead(0);

    if (pushbtn_state && last_pushbtn_state && !pushbtn_action) {
      pushbtn_action = true;
      if (ctrl_mode == "On")
        ctrl_mode = "Off";
      else
        ctrl_mode = "On";
      if (mqtt_server!=0) mqtt_publish("out/ctrlmode",String(ctrl_mode));

    }
    if (!pushbtn_state && !last_pushbtn_state)
      pushbtn_action = false;
  }

} // end loop

String getTime()
{
  auto now = time(nullptr);
  auto datetimenow = localtime(&now);

  auto hours = datetimenow->tm_hour;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  auto minutes = datetimenow->tm_min;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  auto seconds = datetimenow->tm_sec;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr + ":" + secondStr;
}

String getDate()
{
  // Based on https://github.com/PaulStoffregen/Time/blob/master/Time.cpp
  // currently assumes UTC timezone, instead of using this->_timeOffset
  auto now = time(nullptr);
  auto datetimenow = localtime(&now);

  auto month = datetimenow->tm_mon;
  auto day = datetimenow->tm_mday;

  String monthStr = ++month < 10 ? "0" + String(month) : String(month); // jan is month 1
  String dayStr = day < 10 ? "0" + String(day) : String(day);           // day of month
  return String(datetimenow->tm_year + 1900) + "-" + monthStr + "-" + dayStr;
}

int getDateAsInt()
{
  // Based on https://github.com/PaulStoffregen/Time/blob/master/Time.cpp
  // currently assumes UTC timezone, instead of using this->_timeOffset

  auto now = time(nullptr);
  auto datetimenow = localtime(&now);

  return ((datetimenow->tm_year + 1900) * 10000) + ((datetimenow->tm_mon + 1) * 100) + datetimenow->tm_mday;
}

void event_send(String &json)
{
  StaticJsonDocument<512> event;
  deserializeJson(event, json);
  event_send(event);
}

void event_send(JsonDocument &event)
{
  #ifdef ENABLE_DEBUG
  serializeJson(event, DEBUG_PORT);
  DBUGLN("");
  #endif
  web_server_event(event);
  mqtt_publish(event);
}
