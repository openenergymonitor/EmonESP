/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Adaptation of Chris Howells OpenEVSE ESP Wifi
   by Trystan Lea, Glyn Hudson, OpenEnergyMonitor

   Modified to use CircuitSetup.us Energy Meter by jdeglavina
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
#include "wifi.h"
#include "web_server.h"
#include "ota.h"
#include "input.h"
#include "emoncms.h"
#include "mqtt.h"

// for ATM90E32 energy meter
#include <SPI.h>
#include <ATM90E32_3.h>


/***** CALIBRATION SETTINGS *****/
unsigned short LineGain = 0; /* 7481 - 0x1D39 */
unsigned short VoltageGain1 = 41820; /* 9v AC transformer. */
                                     /* 32428 - 12v AC Transformer */
unsigned short VoltageGain2 = 41820; /* Voltage1 source (AC Transformer plug) is connected to Voltage2 on the 6 channel board */
                                     /* If using a second voltage source, sever jumper connection, use 2 pin header */
                                     /* and calibrate this setting seperately */

unsigned short CurrentGainCT1 = 25498;  /* SCT-013-000 100A/50mA */
                                        /* 46539 - Magnalab 100A */
unsigned short CurrentGainCT2 = 25498;  
unsigned short CurrentGainCT3 = 25498;
unsigned short CurrentGainCT4 = 25498;
unsigned short CurrentGainCT5 = 25498;
unsigned short CurrentGainCT6 = 25498;

const int CS_pin1 = 5;
const int CS_pin2 = 4;
/*
  18 - CLK
  19 - MISO
  23 - MOSI
*/

ATM90E32 eic1(CS_pin1, LineGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3); //pass CS pin1 and calibrations to ATM90E32 library
ATM90E32 eic2(CS_pin2, LineGain, VoltageGain2, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6); //pass CS pin2 and calibrations to ATM90E32 library

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
  delay(2000);

  Serial.begin(115200);
#ifdef DEBUG_SERIAL1
  Serial1.begin(115200);
#endif

  DEBUG.println();
  DEBUG.print("EmonESP ");
  DEBUG.println("Firmware: " + currentfirmware);

  /* Read saved settings from the config */
  config_load_settings();

  /* Initialise the WiFi */
  wifi_setup();

  /* Bring up the web server */
  web_server_setup();

  /* Start the OTA update systems */
  ota_setup();

  DEBUG.println("Server started");

  /* Initialise the ATM90E32 + SPI port */
  Serial.println("Start ATM90E32");
  eic1.begin();
  delay(1000);
  eic2.begin();
  delay(1000);


} // end setup

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void loop()
{
  ota_loop();
  web_server_loop();
  wifi_loop();

  /*Repeatedly fetch some values from the ATM90E32 */
  float voltage1, voltage2, currentCT1, currentCT2, currentCT3, currentCT4, currentCT5, currentCT6, current1, current2, totalCurrent, realPower1, realPower2, totalRealPower, powerFactor1, powerFactor2, temp, freq, totalWatts;

  unsigned short sys0 = eic1.GetSysStatus0();
  unsigned short sys1 = eic1.GetSysStatus1();
  unsigned short en0 = eic1.GetMeterStatus0();
  unsigned short en1 = eic1.GetMeterStatus1();

  unsigned short sys0_2 = eic2.GetSysStatus0();
  unsigned short sys1_2 = eic2.GetSysStatus1();
  unsigned short en0_2 = eic2.GetMeterStatus0();
  unsigned short en1_2 = eic2.GetMeterStatus1();

  Serial.println("Sys Status 1: S0:0x" + String(sys0, HEX) + " S1:0x" + String(sys1, HEX));
  Serial.println("Meter Status 1: E0:0x" + String(en0, HEX) + " E1:0x" + String(en1, HEX));
  Serial.println("Sys Status 2: S0:0x" + String(sys0_2, HEX) + " S1:0x" + String(sys1_2, HEX));
  Serial.println("Meter Status 2: E0:0x" + String(en0_2, HEX) + " E1:0x" + String(en1_2, HEX));
  delay(10);

  /* only 1 voltage channel is used on each IC */ 
  voltage1 = eic1.GetLineVoltageA();
  voltage2 = eic2.GetLineVoltageA();

  /* get current readings from each IC and add them up */
  currentCT1 = eic1.GetLineCurrentA();
  currentCT2 = eic1.GetLineCurrentB();
  currentCT3 = eic1.GetLineCurrentC();
  currentCT4 = eic2.GetLineCurrentA();
  currentCT5 = eic2.GetLineCurrentB();
  currentCT6 = eic2.GetLineCurrentC();
  current1 = currentCT1 + currentCT2 + currentCT3;
  current2 = currentCT4 + currentCT5 + currentCT6;
  totalCurrent = current1 + current2;
  
  realPower1 = eic1.GetTotalActivePower();
  realPower2 = eic2.GetTotalActivePower();
  totalRealPower = realPower1 + realPower2;
  
  powerFactor1 = eic1.GetTotalPowerFactor();
  powerFactor2 = eic2.GetTotalPowerFactor();
  
  temp = eic1.GetTemperature();
  freq = eic1.GetFrequency();
  totalWatts = (voltage1 * current1) + (voltage2 * current2);

  Serial.println("V1:" + String(voltage1) + "V");
  Serial.println("V2:" + String(voltage2) + "V");
  Serial.println("I1:" + String(currentCT1) + "A");
  Serial.println("I2:" + String(currentCT2) + "A");
  Serial.println("I3:" + String(currentCT3) + "A");
  Serial.println("I4:" + String(currentCT4) + "A");
  Serial.println("I5:" + String(currentCT5) + "A");
  Serial.println("I6:" + String(currentCT6) + "A");
  Serial.println("AP:" + String(totalRealPower));
  Serial.println("PF1:" + String(powerFactor1));
  Serial.println("PF2:" + String(powerFactor2));
  Serial.println(String(temp) + "C");
  Serial.println("f" + String(freq) + "Hz");

/* post to emonCMS */
  String postStr = "V1:";
  postStr += String(voltage1);
  postStr += ",V2:";
  postStr += String(voltage2);
  postStr += ",I1:";
  postStr += String(currentCT1);
  postStr += ",I2:";
  postStr += String(currentCT2);
  postStr += ",I1:";
  postStr += String(currentCT3);
  postStr += ",I2:";
  postStr += String(currentCT4);
  postStr += ",I1:";
  postStr += String(currentCT5);
  postStr += ",I2:";
  postStr += String(currentCT6);
  postStr += ",totI:";
  postStr += String(totalCurrent);
  postStr += ",AP:";
  postStr += String(totalRealPower);
  postStr += ",PF:";
  postStr += String(powerFactor1);
  postStr += ",t:";
  postStr += String(temp);
  postStr += ",W:";
  postStr += String(totalWatts);



  /* boolean gotInput = input_get(postStr); */

  /* 
  if (wifi_mode == WIFI_MODE_CLIENT || wifi_mode == WIFI_MODE_AP_AND_STA)
  {
  if (gotInput) { //(emoncms_apikey != 0 && gotInput) { 
  */
  Serial.println(postStr);
  emoncms_publish(postStr);
  /*
  }

    if (mqtt_server != 0)
    {
    mqtt_loop();
    if (gotInput) {
      mqtt_publish(postStr);
    }
    }
  */
  delay(1000);
  //}
} // end loop

