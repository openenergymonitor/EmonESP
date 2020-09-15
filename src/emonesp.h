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

#ifndef _EMONESP_H
#define _EMONESP_H

#include "Arduino.h"
#include "debug.h"
#include "profile.h"

String getTime();

void setTimeOffset();

#ifndef WIFI_LED
#define WIFI_LED 0
#endif

#ifndef WIFI_LED_ON_STATE
#define WIFI_LED_ON_STATE LOW
#endif

#ifndef WIFI_BUTTON
#define WIFI_BUTTON 0
#endif

#ifndef WIFI_BUTTON_PRESSED_STATE
#define WIFI_BUTTON_PRESSED_STATE LOW
#endif

#ifndef CONTROL_PIN
#define CONTROL_PIN 2
#endif

#ifndef CONTROL_PIN_ON_STATE
#define CONTROL_PIN_ON_STATE HIGH
#endif

#ifndef NODE_TYPE
#define NODE_TYPE "espwifi"
#endif

#ifndef NODE_DESCRIPTION
#define NODE_DESCRIPTION "WiFi EmonCMS Link"
#endif

#endif // _EMONESP_H
