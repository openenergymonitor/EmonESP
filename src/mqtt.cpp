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
#include "wifi.h"
#include "mqtt.h"
#include "config.h"

#include <Arduino.h>
#include <PubSubClient.h>             // MQTT https://github.com/knolleary/pubsubclient PlatformIO lib: 89
#include <WiFiClient.h>

WiFiClient espClient;                 // Create client for MQTT
PubSubClient mqttclient(espClient);   // Create client for MQTT

long lastMqttReconnectAttempt = 0;
int clientTimeout = 0;
int i = 0;


// -------------------------------------------------------------------
// MQTT Control callback for WIFI Relay and Sonoff smartplug
// -------------------------------------------------------------------
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  
  String topicstr = String(topic);
  String payloadstr = String((char *)payload);
  payloadstr = payloadstr.substring(0,length);
  
  DEBUG.println("Message arrived topic:["+topicstr+"] payload: ["+payloadstr+"]");

  // --------------------------------------------------------------------------
  // State 
  // --------------------------------------------------------------------------
  if (topicstr.compareTo("emon/"+node_name+"/in/ctrlmode")==0) {
    DEBUG.print("Status: ");
    if (payloadstr.compareTo("2")==0) {
      ctrl_mode = "Timer";
    } else if (payloadstr.compareTo("1")==0) {
      ctrl_mode = "On";
    } else if (payloadstr.compareTo("0")==0) {
      ctrl_mode = "Off";
    } else if (payloadstr.compareTo("Timer")==0) {
      ctrl_mode = "Timer";
    } else if (payloadstr.compareTo("On")==0) {
      ctrl_mode = "On";
    } else if (payloadstr.compareTo("Off")==0) {
      ctrl_mode = "Off";
    } else {
      ctrl_mode = "Off";
    }
    DEBUG.println(ctrl_mode);
  // --------------------------------------------------------------------------
  // Timer  
  // --------------------------------------------------------------------------
  } else if (topicstr.compareTo("emon/"+node_name+"/in/timer")==0) {
    DEBUG.print("Timer: ");
    if (payloadstr.length()==9) {
      String tstart = payloadstr.substring(0,4);
      String tstop = payloadstr.substring(5,9);
      timer_start1 = tstart.toInt();
      timer_stop1 = tstop.toInt();
      DEBUG.println(tstart+" "+tstop);
    }
    if (payloadstr.length()==19) {
      String tstart1 = payloadstr.substring(0,4);
      String tstop1 = payloadstr.substring(5,9);
      timer_start1 = tstart1.toInt();
      timer_stop1 = tstop1.toInt();
      String tstart2 = payloadstr.substring(10,14);
      String tstop2 = payloadstr.substring(15,19);
      timer_start2 = tstart2.toInt();
      timer_stop2 = tstop2.toInt();
      DEBUG.println(tstart1+":"+tstop1+" "+tstart2+":"+tstop2);
    }
  // --------------------------------------------------------------------------
  // Vout  
  // --------------------------------------------------------------------------
  } else if (topicstr.compareTo("emon/"+node_name+"/in/vout")==0) {
    DEBUG.print("Vout: ");
    voltage_output = payloadstr.toInt();
    DEBUG.println(voltage_output);
  // --------------------------------------------------------------------------
  // FlowT  
  // --------------------------------------------------------------------------
  } else if (topicstr.compareTo("emon/"+node_name+"/in/flowT")==0) {
    DEBUG.print("FlowT: ");
    float flow = payloadstr.toFloat();
    voltage_output = (int) (flow - 7.14)/0.0371;
    DEBUG.println(String(flow)+" vout:"+String(voltage_output));
  // --------------------------------------------------------------------------
  // Return device state
  // --------------------------------------------------------------------------
  } else if (topicstr.compareTo("emon/"+node_name+"/in/state")==0) {
    DEBUG.println("State: ");

    String s = "{";
    s += "\"ip\":\""+ipaddress+"\",";
    // s += "\"time\":\"" + String(getTime()) + "\",";
    s += "\"ctrlmode\":\"" + String(ctrl_mode) + "\",";
    s += "\"timer\":\"" + String(timer_start1)+" "+String(timer_stop1)+" "+String(timer_start2)+" "+String(timer_stop2) + "\",";
    s += "\"vout\":\"" + String(voltage_output) + "\"";
    s += "}";
    mqtt_publish("out/state",s);
  }
}

// -------------------------------------------------------------------
// MQTT Connect
// -------------------------------------------------------------------
boolean mqtt_connect()
{
  mqttclient.setServer(mqtt_server.c_str(), 1883);
  mqttclient.setCallback(mqtt_callback);
  
  DEBUG.println("MQTT Connecting...");
  String strID = String(ESP.getChipId());
  if (mqttclient.connect(strID.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {  // Attempt to connect
    DEBUG.println("MQTT connected");
    mqtt_publish_keyval(node_describe);
    
    String subscribe_topic = mqtt_topic + "/" + node_name + "/in/#";
    mqttclient.subscribe(subscribe_topic.c_str());
    
  } else {
    DEBUG.print("MQTT failed: ");
    DEBUG.println(mqttclient.state());
    return(0);
  }
  return (1);
}

// -------------------------------------------------------------------
// Publish to MQTT
// -------------------------------------------------------------------
void mqtt_publish(String topic_p2, String data)
{
    String topic = mqtt_topic + "/" + node_name + "/" + topic_p2;
    mqttclient.publish(topic.c_str(), data.c_str());
}

// -------------------------------------------------------------------
// Publish to MQTT
// Split up data string into sub topics: e.g
// data = CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7
// base topic = emon/emonesp
// MQTT Publish: emon/emonesp/CT1 > 3935 etc..
// -------------------------------------------------------------------
void mqtt_publish_keyval(String data)
{
  String mqtt_data = "";
  String topic = mqtt_topic + "/" + node_name + "/" + mqtt_feed_prefix;
  int i=0;
  while (int (data[i]) != 0)
  {
    // Construct MQTT topic e.g. <base_topic>/CT1 e.g. emonesp/CT1
    while (data[i]!=':'){
      topic+= data[i];
      i++;
      if (int(data[i])==0){
        break;
      }
    }
    i++;
    // Construct data string to publish to above topic
    while (data[i]!=','){
      mqtt_data+= data[i];
      i++;
      if (int(data[i])==0){
        break;
      }
    }
    // send data via mqtt
    //delay(100);
    //DEBUG.printf("%s = %s\r\n", topic.c_str(), mqtt_data.c_str());
    mqttclient.publish(topic.c_str(), mqtt_data.c_str());
    topic = mqtt_topic + "/" + node_name + "/" + mqtt_feed_prefix;
    mqtt_data="";
    i++;
    if (int(data[i]) == 0) break;
  }

  String ram_topic = mqtt_topic + "/" + node_name + "/" + mqtt_feed_prefix + "freeram";
  String free_ram = String(ESP.getFreeHeap());
  mqttclient.publish(ram_topic.c_str(), free_ram.c_str());
}

// -------------------------------------------------------------------
// MQTT state management
//
// Call every time around loop() if connected to the WiFi
// -------------------------------------------------------------------
void mqtt_loop()
{
  if (!mqttclient.connected()) {
    long now = millis();
    // try and reconnect continuously for first 5s then try again once every 10s
    if ( (now < 5000) || ((now - lastMqttReconnectAttempt)  > 10000) ) {
      lastMqttReconnectAttempt = now;
      if (mqtt_connect()) { // Attempt to reconnect
        lastMqttReconnectAttempt = millis();
        delay(100);
      }
    }
  } else {
    // if MQTT connected
    mqttclient.loop();
  }
}

void mqtt_restart()
{
  if (mqttclient.connected()) {
    mqttclient.disconnect();
  }
}

boolean mqtt_connected()
{
  return mqttclient.connected();
}
