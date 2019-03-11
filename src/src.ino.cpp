# 1 "/var/folders/8c/gyvcglfx7f97kbqb2h83mm_r0000gn/T/tmpi6Fh0D"
#include <Arduino.h>
# 1 "/Users/admin/Documents/github/EmonESP-db/src/src.ino"
# 26 "/Users/admin/Documents/github/EmonESP-db/src/src.ino"
#include "emonesp.h"
#include "config.h"
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
NTPClient timeClient(ntpUDP,"europe.pool.ntp.org",0,60000);
unsigned long last_ctrl_update = 0;
unsigned long last_pushbtn_check = 0;
bool pushbtn_action = 0;
bool pushbtn_state = 0;
bool last_pushbtn_state = 0;
void setup();
void led_flash(int ton, int toff);
void loop();
String getTime();
#line 49 "/Users/admin/Documents/github/EmonESP-db/src/src.ino"
void setup() {
  delay(2000);

  Serial.begin(115200);
#ifdef DEBUG_SERIAL1
  Serial1.begin(115200);
#endif

  DEBUG.println();
  DEBUG.print("EmonESP ");
  DEBUG.println(ESP.getChipId());
  DEBUG.println("Firmware: "+ currentfirmware);


  config_load_settings();




  node_type = "smartplug";
  node_description = "SmartPlug";
  node_id = ESP.getChipId()/5120;

  node_name = node_type + String(node_id);
  node_describe = "describe:"+node_type;


  pinMode(LEDpin, OUTPUT);

  if (node_type=="smartplug") {
    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);
    pinMode(16, OUTPUT);
    led_flash(3000,100);
  } else if (node_type=="wifirelay") {
    pinMode(5, OUTPUT);
  } else if (node_type=="espwifi") {
    pinMode(2,OUTPUT);
    digitalWrite(2,HIGH);
  } else if (node_type=="hpmon") {
    pinMode(5,OUTPUT);
    digitalWrite(5,LOW);
    pinMode(4,OUTPUT);
  }


  wifi_setup();
  led_flash(50,50);


  web_server_setup();
  led_flash(50,50);


  ota_setup();

  DEBUG.println("Server started");


  auth_setup();


  timeClient.begin();

  delay(100);
}

void led_flash(int ton, int toff) {
  digitalWrite(LEDpin,LOW); delay(ton); digitalWrite(LEDpin,HIGH); delay(toff);
}




void loop()
{
  ota_loop();
  web_server_loop();
  wifi_loop();
  timeClient.update();

  String input = "";
  boolean gotInput = input_get(input);

  if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_AND_STA)
  {
    if(emoncms_apikey != 0 && gotInput) {
      emoncms_publish(input);
    }
    if(mqtt_server != 0)
    {
      mqtt_loop();
      if(gotInput) {
        mqtt_publish_keyval(input);
      }
    }
  }

  auth_loop();




  if ((millis()-last_ctrl_update)>1000 || ctrl_update) {
    last_ctrl_update = millis();
    ctrl_update = 0;
    ctrl_state = 0;


    int timenow = timeClient.getHours()*100+timeClient.getMinutes();

    if (timenow>=timer_start1 && timenow<timer_stop1) ctrl_state = 1;
    if (timenow>=timer_start2 && timenow<timer_stop2) ctrl_state = 1;


    if (ctrl_mode=="On") ctrl_state = 1;
    if (ctrl_mode=="Off") ctrl_state = 0;


    if (ctrl_state) {

      if (node_type=="smartplug") {
        digitalWrite(12,HIGH);
        digitalWrite(16,HIGH);
      } else if (node_type=="wifirelay") {
        digitalWrite(5,HIGH);
      } else if (node_type=="espwifi") {
        digitalWrite(2,LOW);
      } else if (node_type=="hpmon") {
        digitalWrite(5,HIGH);
      }
    } else {

      if (node_type=="smartplug") {
        digitalWrite(12,LOW);
        digitalWrite(16,LOW);
      } else if (node_type=="wifirelay") {
        digitalWrite(5,LOW);
      } else if (node_type=="espwifi") {
        digitalWrite(2,HIGH);
      } else if (node_type=="hpmon") {
        digitalWrite(5,LOW);
      }
    }

    if (node_type=="hpmon") {
      analogWrite(4,voltage_output);


    }
  }

  if ((millis()-last_pushbtn_check)>100) {
    last_pushbtn_check = millis();

    last_pushbtn_state = pushbtn_state;
    pushbtn_state = !digitalRead(0);

    if (pushbtn_state && last_pushbtn_state && !pushbtn_action) {
      pushbtn_action = 1;
      if (ctrl_mode=="On") ctrl_mode = "Off"; else ctrl_mode = "On";
      if (mqtt_server!=0) mqtt_publish("out/ctrlmode",String(ctrl_mode));

    }
    if (!pushbtn_state && !last_pushbtn_state) pushbtn_action = 0;
  }

}

String getTime() {
    return timeClient.getFormattedTime();
}