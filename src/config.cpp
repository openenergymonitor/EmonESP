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
#include "config.h"
#include "web_server.h"

#include <Arduino.h>

// Wifi Network Strings
String esid = "";
String epass = "";

// Web server authentication (leave blank for none)
String www_username = "";
String www_password = "";

// EMONCMS SERVER strings
String emoncms_server = "";
String emoncms_path = "";
String emoncms_node = "";
String emoncms_apikey = "";
String emoncms_fingerprint = "";

// MQTT Settings
String mqtt_server = "";
String mqtt_topic = "";
String mqtt_user = "";
String mqtt_pass = "";
String mqtt_feed_prefix = "";

// -------------------------------------------------------------------
// Reset, wipes all settings
// -------------------------------------------------------------------

void config_reset()
{
  ESPHTTPServer.save_user_config("wifi_ssid", "");
  ESPHTTPServer.save_user_config("wifi_pass", "");
  ESPHTTPServer.save_user_config("emoncms_apikey", "");
  ESPHTTPServer.save_user_config("emoncms_server", "");
  ESPHTTPServer.save_user_config("emoncms_path", "");
  ESPHTTPServer.save_user_config("emoncms_node", "");
  ESPHTTPServer.save_user_config("emoncms_fingerprint", "");
  ESPHTTPServer.save_user_config("mqtt_server", "");
  ESPHTTPServer.save_user_config("mqtt_topic", "");
  ESPHTTPServer.save_user_config("mqtt_feed_prefix", "");
  ESPHTTPServer.save_user_config("mqtt_user", "");
  ESPHTTPServer.save_user_config("mqtt_pass", "");
  ESPHTTPServer.save_user_config("www_username", "");
  ESPHTTPServer.save_user_config("www_password", "");
}

// -------------------------------------------------------------------
// Load saved settings from userconfig.json
// -------------------------------------------------------------------
void config_load_settings()
{
  ESPHTTPServer.load_user_config("wifi_ssid", esid);
  ESPHTTPServer.load_user_config("wifi_pass", epass);
  ESPHTTPServer.load_user_config("emoncms_apikey", emoncms_apikey);
  ESPHTTPServer.load_user_config("emoncms_server", emoncms_server);
  ESPHTTPServer.load_user_config("emoncms_path", emoncms_path);
  ESPHTTPServer.load_user_config("emoncms_node", emoncms_node);
  ESPHTTPServer.load_user_config("emoncms_fingerprint", emoncms_fingerprint);
  ESPHTTPServer.load_user_config("mqtt_server", mqtt_server);
  ESPHTTPServer.load_user_config("mqtt_topic", mqtt_topic);
  ESPHTTPServer.load_user_config("mqtt_feed_prefix", mqtt_feed_prefix);
  ESPHTTPServer.load_user_config("mqtt_user", mqtt_user);
  ESPHTTPServer.load_user_config("mqtt_pass", mqtt_pass);
  ESPHTTPServer.load_user_config("www_username", www_username);
  ESPHTTPServer.load_user_config("www_password", www_password);
}




void config_save_emoncms(String server, String path, String node, String apikey, String fingerprint)
{
  emoncms_server = server;
  emoncms_path = path;
  emoncms_node = node;
  emoncms_apikey = apikey;
  emoncms_fingerprint = fingerprint;

  ESPHTTPServer.save_user_config("emoncms_server", emoncms_server);
  ESPHTTPServer.save_user_config("emoncms_path", emoncms_path);
  ESPHTTPServer.save_user_config("emoncms_node", emoncms_node);
  ESPHTTPServer.save_user_config("emoncms_apikey", emoncms_apikey);
  ESPHTTPServer.save_user_config("emoncms_fingerprint", emoncms_fingerprint);
}

void config_save_mqtt(String server, String topic, String prefix, String user, String pass)
{
  mqtt_server = server;
  mqtt_topic = topic;
  mqtt_feed_prefix = prefix;
  mqtt_user = user;
  mqtt_pass = pass;

  ESPHTTPServer.save_user_config("mqtt_server", mqtt_server);
  ESPHTTPServer.save_user_config("mqtt_topic", mqtt_topic);
  ESPHTTPServer.save_user_config("mqtt_feed_prefix", mqtt_feed_prefix);
  ESPHTTPServer.save_user_config("mqtt_user", mqtt_user);
  ESPHTTPServer.save_user_config("mqtt_pass", mqtt_pass);
}

void config_save_admin(String user, String pass)
{
  www_username = user;
  www_password = pass;

  ESPHTTPServer.save_user_config("www_username", www_username);
  ESPHTTPServer.save_user_config("www_password", www_password);

}

void config_save_wifi(String qsid, String qpass)
{
  esid = qsid;
  epass = qpass;

  ESPHTTPServer.save_user_config("wifi_ssid", qsid);
  ESPHTTPServer.save_user_config("wifi_pass", qpass);
}
