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
#include "mqtt.h"

int button_interval_one = 20; // milliseconds for action, test.
int button_interval_two = (6 * 1000); // seconds hold down AP mode.
int button_interval_three = (11 * 1000); // seconds hold down GPIO0 for factory reset.
int timebuttonpressed;
bool buttonflag = false;
bool button_interval_one_passed = false;
bool button_interval_two_passed = false;
bool button_interval_three_passed = false;

void led_flash(int ton, int toff) {
  digitalWrite(LEDpin,LOW); delay(ton); digitalWrite(LEDpin,HIGH); delay(toff);
}

void gpio0_loop() { // check the button and do an action, or go into AP mode, or factory reset.

    if (buttonflag == true && !digitalRead(0) == false) {
      Serial.println("Button Released.");
      button_interval_one_passed = false;
      button_interval_two_passed = false;
      button_interval_three_passed = false;
      delay(10);
    }

    if (!digitalRead(0) == false) {
      timebuttonpressed = 0;
      buttonflag = false;
    }
    else if (!digitalRead(0) == true && timebuttonpressed == 0) {
      timebuttonpressed = millis();
      //Serial.println("Button Pressed.");
      buttonflag = true;
    }

    if (buttonflag) {

      if (button_interval_one_passed == false) {
        led_flash(100, 100);
        Serial.println("Button interval one!");
        if (ctrl_mode=="On") ctrl_mode = "Off"; else ctrl_mode = "On";
        if (mqtt_server!=0) mqtt_publish("out/ctrlmode",String(ctrl_mode));
        button_interval_one_passed = true;
      }

      if (timebuttonpressed + button_interval_two <= millis() && button_interval_two_passed == false) {
        led_flash(300, 300);
        led_flash(300, 300);
        Serial.println("AP mode starting..");
        wifi_mode = WIFI_MODE_AP_ONLY;
        ctrl_mode = "OFF";
        startAP();
        button_interval_two_passed = true;
      }

      if (timebuttonpressed + button_interval_three <= millis()) {
        led_flash(800, 800);
        Serial.println("Commencing factory reset.");
        delay(500);
        config_reset();
        Serial.println("Factory reset complete! Resetting...");
        led_flash(800, 800);
        led_flash(800, 800);
        led_flash(800, 800);
        ESP.restart();
      }
    } //buttonflag
}// end GPIO0 button.
