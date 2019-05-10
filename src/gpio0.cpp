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

int button_interval_one = 100; // milliseconds for action, test.
int button_interval_two = (5 * 1000); // 5 seconds hold down AP mode.
int button_interval_three = (10 * 1000); // 10 seconds hold down GPIO0 for factory reset.
int timebuttonpressed;
bool buttonflag = false;
bool button_interval_one_passed = false;
bool button_interval_two_passed = false;
bool button_interval_three_passed = false;

bool gpio0_loop() { // check the button and do an action, or go into AP mode, or factory reset.

    if (digitalRead(0) == false) {
      return false;
    }

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
        Serial.println("Button!");
        button_interval_one_passed = true;
        return true;
      }

      if (timebuttonpressed + button_interval_two <= millis() && button_interval_two_passed == false) {
        Serial.println("AP mode starting..");
        wifi_mode = WIFI_MODE_AP_ONLY;
        startAP();
        button_interval_two_passed = true;
        return false;
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
}
