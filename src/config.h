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

#ifndef _EMONESP_CONFIG_H
#define _EMONESP_CONFIG_H

#include <Arduino.h>

// -------------------------------------------------------------------
// Load and save the EmonESP config.
//
// This initial implementation saves the config to the EEPROM area of flash
// -------------------------------------------------------------------

extern int LEDpin;

// Global config varables
extern String node_type;
extern int node_id;
extern String node_name;
extern String node_describe;
extern String node_status;

// Wifi Network Strings
extern String esid;
extern String epass;

// Web server authentication (leave blank for none)
extern String www_username;
extern String www_password;

// EMONCMS SERVER strings
extern String emoncms_server;
extern String emoncms_path;
extern String emoncms_node;
extern String emoncms_apikey;
extern String emoncms_fingerprint;

// MQTT Settings
extern String mqtt_server;
extern String mqtt_topic;
extern String mqtt_user;
extern String mqtt_pass;
extern String mqtt_feed_prefix;

// Timer Settings 
extern String timer_start1;
extern String timer_stop1;
extern String timer_start2;
extern String timer_stop2;

extern String ctrl_mode;
extern bool ctrl_update;
extern bool ctrl_state;

// -------------------------------------------------------------------
// Load saved settings
// -------------------------------------------------------------------
extern void config_load_settings();

// -------------------------------------------------------------------
// Save the EmonCMS server details
// -------------------------------------------------------------------
extern void config_save_emoncms(String server, String path, String node, String apikey, String fingerprint);

// -------------------------------------------------------------------
// Save the MQTT broker details
// -------------------------------------------------------------------
extern void config_save_mqtt(String server, String topic, String prefix, String user, String pass);
extern void config_save_mqtt_server(String server);

// -------------------------------------------------------------------
// Save the admin/web interface details
// -------------------------------------------------------------------
extern void config_save_admin(String user, String pass);

// -------------------------------------------------------------------
// Save the admin/web interface details
// -------------------------------------------------------------------
extern void config_save_timer(String start1, String stop1, String start2, String stop2);

// -------------------------------------------------------------------
// Save the Wifi details
// -------------------------------------------------------------------
extern void config_save_wifi(String qsid, String qpass);


// -------------------------------------------------------------------
// Reset the config back to defaults
// -------------------------------------------------------------------
extern void config_reset();

#endif // _EMONESP_CONFIG_H
