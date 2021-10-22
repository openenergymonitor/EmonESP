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
#include "input.h"
#include "emoncms.h"
#include "espal.h"

#include <ESP8266WiFi.h>

String input_string="";
String last_datastr="";

bool input_get(JsonDocument &data)
{
  bool gotLine = false;
  bool gotData = false;
  String line;

  // If data from test API e.g `http://<IP-ADDRESS>/input?string=CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7`
  if(input_string.length() > 0) {
    line = input_string;
    input_string = "";
    gotLine = true;
  }
  // If data received on serial
  else if (EMONTX_PORT.available()) {
    // Could check for string integrity here
    line = EMONTX_PORT.readStringUntil('\n');
    gotLine = true;
  }

  if(gotLine)
  {
    // Get rid of any whitespace, newlines etc
    line.trim();

    int len = line.length();
    if(len > 0) 
    {
      DEBUG.printf_P(PSTR("Got '%s'\n"), line.c_str());

      for(int i = 0; i < len; i++)
      {
        String name = "";

        // Get the name
        while (i < len && line[i] != ':') {
          name += line[i++];
        }

        if (i++ >= len) {
          break;
        }

        // Get the value
        String value = "";
        while (i < len && line[i] != ','){
          value += line[i++];
        }

        DBUGVAR(name);
        DBUGVAR(value);

        if(name.length() > 0 && value.length() > 0)
        {
          // IMPROVE: check that value is only a number, toDouble() will skip white space and and chars after the number
          data[name] = value.toDouble();
          gotData = true;
        }
      }
    }
  }

  // Append some system info
  if(gotData) {
    data[F("freeram")] = ESPAL.getFreeHeap();
    data[F("srssi")] = WiFi.RSSI();
    data[F("psent")] = packets_sent;
    data[F("psuccess")] = packets_success;

    last_datastr.clear();
    serializeJson(data, last_datastr);
  }

  return gotData;
}
