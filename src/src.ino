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
#include <NTPClient.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", time_offset * 60, 60000);
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

  timeClient.setTimeOffset(time_offset * 60);

  DBUG("Node name: ");
  DBUGLN(node_name);

  // ---------------------------------------------------------
  // pin setup
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, !WIFI_LED_ON_STATE);

  pinMode(CONTROL_PIN, OUTPUT);
  digitalWrite(CONTROL_PIN, !CONTROL_PIN_ON_STATE);

// custom: analog output pin
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

  // Time
  timeClient.begin();
  DBUGF("After timeClient.begin: %d", ESP.getFreeHeap());

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
  timeClient.update();

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
    divert_state = true; // default ON

    int timenow = timeClient.getHours() * 100 + timeClient.getMinutes();
    int datenow = getDateAsInt();
    int day = timeClient.getDay();

    // Diversion
    if (divert_mode == "Standby")
    {
      // 1. Standby
      if (datenow >= standby_start && datenow < standby_stop)
        divert_state = false;
    }
    else if (divert_mode == "Off") // 2. Off
      divert_state = false;
    else // 2. On (default)
      divert_state = true;

    // 3. Apply
    if (divert_state)
    {
      // ON
      digitalWrite(DIVERTION_PIN, DIVERTION_PIN_ON_STATE);
    }
    else
    {
      digitalWrite(DIVERTION_PIN, !DIVERTION_PIN_ON_STATE);
    }

    // Override
    if (ctrl_mode == "Timer")
    {
      // 1. Timer
      if (((node_type == "pvrouter") && day != 0 && day != 6) ||
          (node_type != "pvrouter"))
      {
        if (timer_stop1 >= timer_start1 && (timenow >= timer_start1 && timenow < timer_stop1))
          ctrl_state = true;
        if (timer_stop1 < timer_start1 && (timenow >= timer_start1 || timenow < timer_stop1))
          ctrl_state = true;
      }
      if ((node_type == "pvrouter" && (day == 0 || day == 6)) ||
          (node_type != "pvrouter"))
      {
        if (timer_stop2 >= timer_start2 && (timenow >= timer_start2 && timenow < timer_stop2))
          ctrl_state = true;
        if (timer_stop2 < timer_start2 && (timenow >= timer_start2 || timenow < timer_stop2))
          ctrl_state = true;
      }
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

#ifdef ROTATION_PIN
    if (rotation && (timenow == 0))
      digitalWrite(ROTATION_PIN, ROTATION_PIN_ON_STATE);
    else
      digitalWrite(ROTATION_PIN, !ROTATION_PIN_ON_STATE);
#endif

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

    if (pushbtn_state && last_pushbtn_state && !pushbtn_action)
    {
      DBUGF("Loop - Button pressed - '%s'", ctrl_mode.c_str());

      pushbtn_action = true;
      if (ctrl_mode == "On")
        ctrl_mode = "Off";
      else
        ctrl_mode = "On";

      DBUGF("Loop - Button pressed 2 - '%s'", ctrl_mode.c_str());

      if (!mqtt_server.isEmpty())
        mqtt_publish("out/ctrlmode", String(ctrl_mode));
    }
    if (!pushbtn_state && !last_pushbtn_state)
      pushbtn_action = false;
  }
} // end loop

String getTime()
{
  return timeClient.getFormattedTime();
}

#define LEAP_YEAR(Y) ((Y > 0) && !(Y % 4) && ((Y % 100) || !(Y % 400)))

String getDate()
{
  // Based on https://github.com/PaulStoffregen/Time/blob/master/Time.cpp
  // currently assumes UTC timezone, instead of using this->_timeOffset
  unsigned long rawTime = timeClient.getEpochTime() / 86400L; // in days
  unsigned long days = 0, year = 1970;
  uint8_t month;
  static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  while ((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365); // now it is days in this year, starting at 0
  days = 0;
  for (month = 0; month < 12; month++)
  {
    uint8_t monthLength;
    if (month == 1)
    { // february
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    }
    else
    {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength)
      break;
    rawTime -= monthLength;
  }
  String monthStr = ++month < 10 ? "0" + String(month) : String(month);     // jan is month 1
  String dayStr = ++rawTime < 10 ? "0" + String(rawTime) : String(rawTime); // day of month
  return String(year) + "-" + monthStr + "-" + dayStr;
}

int getDateAsInt()
{
  // Based on https://github.com/PaulStoffregen/Time/blob/master/Time.cpp
  // currently assumes UTC timezone, instead of using this->_timeOffset
  unsigned long rawTime = timeClient.getEpochTime() / 86400L; // in days
  unsigned long days = 0, year = 1970;
  uint8_t month;
  static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  while ((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365); // now it is days in this year, starting at 0
  days = 0;
  for (month = 0; month < 12; month++)
  {
    uint8_t monthLength;
    if (month == 1)
    { // february
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    }
    else
    {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength)
      break;
    rawTime -= monthLength;
  }
  return (year * 10000) + (++month * 100) + (++rawTime);
}

void setTimeOffset()
{
  timeClient.setTimeOffset(time_offset * 60);
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
