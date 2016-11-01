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
#include "pixel.h"

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
        delay(2000);

        Serial.begin(115200);
#ifdef DEBUG_SERIAL1
        Serial1.begin(115200);
#endif

      pixel_begin();
      pixel_rgb_demo();
      delay(5000);
      DEBUG.println("Pixel off");
      pixel_off();
        DEBUG.println();
        DEBUG.print("EmonESP ");
        DEBUG.println(ESP.getChipId());
        DEBUG.println("Firmware: "+ currentfirmware);

        // Read saved settings from the config

        config_load_settings();
        set_pixel(15,40,20,0);
        // Initialise the WiFi
        wifi_setup();

        // Bring up the web server
        set_pixel(14,0,(int8_t)40,0);
        web_server_setup();

        // Start the OTA update systems
        ota_setup();
        //set_pixel(13,40,40,0);


        DEBUG.println("Server started");

        delay(100);
} // end setup

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void loop()
{
        ota_loop();
        web_server_loop();
        wifi_loop();

        String input = "";
        boolean gotInput = input_get(input);
        if (wifi_mode==WIFI_MODE_STA || wifi_mode==WIFI_MODE_AP_AND_STA)
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
                #ifdef PIXEL
                if(gotInput) {
                        pixel_off();
                        float val = get_CT1_val(input);
                        DEBUG.println("val of:" + String(val));
                        light_to_pixel(val);
                }
                #endif

        }
} // end loop
