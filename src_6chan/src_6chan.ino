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
#include <ATM90E32.h>

/* Change to total number of Add-On Boards - Can not be more than 6 */
const int AddOnBoards = 0;

/***** CALIBRATION SETTINGS *****/
unsigned short lineFreq = 5231;         /*5231 for 60 hz 6 channel meter */
                                        /*135 for 50 hz 6 channel meter*/
unsigned short PGAGain = 21;            /*21 for 100A (2x), 42 for between 100A - 200A (4x)*/
unsigned short VoltageGain1 = 41820;     /*9v AC transformer.*/
unsigned short VoltageGain2 = 41820;     /*32428 - 12v AC Transformer*/

unsigned short CurrentGainCT1 = 25498;  /*SCT-013-000 100A/50mA*/  
unsigned short CurrentGainCT2 = 25498;  /*46539 - Magnalab 100A w/ built in burden resistor*/
unsigned short CurrentGainCT3 = 25498;
unsigned short CurrentGainCT4 = 25498;
unsigned short CurrentGainCT5 = 25498;
unsigned short CurrentGainCT6 = 25498;

unsigned long startMillis;  
unsigned long currentMillis; 
const unsigned long period = 1000;  //time interval in ms to send data

/****** Chip Select Pins ******/
/* 
   Each chip has its own pin (2 per board). The main board must be pins 5 and 4.
   Add-on boards have a jumper selection. Jumper bank "CS" are the
   left 3 current channels, and CS2 is the right 3.
   ONLY JUMPER ONE PIN PER BANK
*/
const int CS1_main = 5;
const int CS2_main = 4;

/*When using multiple add-on board - To make it simple, unless you have a specific reason to not use a certain CS pin
 * by default the jumper pins uncrement up from left to right on the jumper banks */
const int CS1_addon1 = 0;
const int CS2_addon1 = 16;
const int CS1_addon2 = 2;
const int CS2_addon2 = 17;
const int CS1_addon3 = 12;
const int CS2_addon3 = 21;
const int CS1_addon4 = 13;
const int CS2_addon4 = 22;
const int CS1_addon5 = 14;
const int CS2_addon5 = 25;
const int CS1_addon6 = 15;
const int CS2_addon6 = 26;

  /* Initialize ATM90E32 library for each IC */
  ATM90E32 main1(CS1_main, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3); //pass CS pin and calibrations to ATM90E32 library
  ATM90E32 main2(CS2_main, lineFreq, PGAGain, VoltageGain2, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6); //pass CS pin and calibrations to ATM90E32 library
  
  
  /*  Initialize add-on boards
   *  VoltageGain1 is used because by default the add-on boards read voltage
   *  from the main source of power
   *  Individual current gains can be changed here if needed
   *
   *  uncomment for however many add-on boards you require - each board needs two initializations for each chip */
  /*
    ATM90E32 AddOn1_1(CS1_addon1, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    ATM90E32 AddOn1_2(CS2_addon1, lineFreq, PGAGain, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);

    ATM90E32 AddOn2_1(CS1_addon2, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    ATM90E32 AddOn2_2(CS2_addon2, lineFreq, PGAGain, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);

    ATM90E32 AddOn3_1(CS1_addon3, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    ATM90E32 AddOn3_2(CS2_addon3, lineFreq, PGAGain, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);

    ATM90E32 AddOn4_1(CS1_addon4, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    ATM90E32 AddOn4_2(CS2_addon4, lineFreq, PGAGain,, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);

    ATM90E32 AddOn5_1(CS1_addon5, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    ATM90E32 AddOn5_2(CS2_addon5, lineFreq, PGAGain, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);
    
    ATM90E32 AddOn6_1(CS1_addon6, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    ATM90E32 AddOn6_2(CS2_addon6, lineFreq, PGAGain, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);
*/

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {

  Serial.begin(115200);

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
  main1.begin();
  delay(1000);
  main2.begin();

  if (AddOnBoards > 0)
  {
    AddOn1_1.begin();
    AddOn1_2.begin();
  
    if (AddOnBoards > 1)
    {
        AddOn2_1.begin();
        AddOn2_2.begin();
  
        if (AddOnBoards > 2)
        {
            AddOn3_1.begin();
            AddOn3_2.begin();
  
            if (AddOnBoards > 3)
            {
                AddOn4_1.begin();
                AddOn4_2.begin();
                      
                if (AddOnBoards > 4)
                {
                    AddOn5_1.begin();
                    AddOn5_2.begin();
                    
                    if (AddOnBoards > 5)
                    {
                        AddOn6_1.begin();
                        AddOn6_2.begin();
                    }
                }
            }
        }
    }
  }

  startMillis = millis();  //initial start time
} // end setup

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void loop()
{
  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)

  if (currentMillis - startMillis >= period)  //test whether the period has elapsed
  {
    ota_loop();
    web_server_loop();
    wifi_loop();
  
    /*Repeatedly fetch some values from the ATM90E32 */
    float voltage1, voltage2, currentCT1, currentCT2, currentCT3, currentCT4, currentCT5, currentCT6, current1, current2, totalCurrent, realPower1, realPower2, totalRealPower, powerFactor1, powerFactor2, temp, freq, totalWatts;
    float AddOn1_CT1, AddOn1_CT2, AddOn1_CT3, AddOn1_CT4, AddOn1_CT5, AddOn1_CT6, AddOn1_current1, AddOn1_current2, AddOn1_totalCurrent;
    float AddOn2_CT1, AddOn2_CT2, AddOn2_CT3, AddOn2_CT4, AddOn2_CT5, AddOn2_CT6, AddOn2_current1, AddOn2_current2, AddOn2_totalCurrent;
    float AddOn3_CT1, AddOn3_CT2, AddOn3_CT3, AddOn3_CT4, AddOn3_CT5, AddOn3_CT6, AddOn3_current1, AddOn3_current2, AddOn3_totalCurrent;
    float AddOn4_CT1, AddOn4_CT2, AddOn4_CT3, AddOn4_CT4, AddOn4_CT5, AddOn4_CT6, AddOn4_current1, AddOn4_current2, AddOn4_totalCurrent;
    float AddOn5_CT1, AddOn5_CT2, AddOn5_CT3, AddOn5_CT4, AddOn5_CT5, AddOn5_CT6, AddOn5_current1, AddOn5_current2, AddOn5_totalCurrent;
    float AddOn6_CT1, AddOn6_CT2, AddOn6_CT3, AddOn6_CT4, AddOn6_CT5, AddOn6_CT6, AddOn6_current1, AddOn6_current2, AddOn6_totalCurrent;
  
    unsigned short sys0 = main1.GetSysStatus0();  //EMMState0
    unsigned short sys1 = main1.GetSysStatus1();  //EMMState1
    unsigned short en0 = main1.GetMeterStatus0(); //EMMIntState0
    unsigned short en1 = main1.GetMeterStatus1(); //EMMInsState1
  
    unsigned short sys0_2 = main2.GetSysStatus0();
    unsigned short sys1_2 = main2.GetSysStatus1();
    unsigned short en0_2 = main2.GetMeterStatus0();
    unsigned short en1_2 = main2.GetMeterStatus1();
  
    Serial.println("Sys Status 1: S0:0x" + String(sys0, HEX) + " S1:0x" + String(sys1, HEX));
    Serial.println("Meter Status 1: EInt0:0x" + String(en0, HEX) + " EInt1:0x" + String(en1, HEX));
    Serial.println("Sys Status 2: S0:0x" + String(sys0_2, HEX) + " S1:0x" + String(sys1_2, HEX));
    Serial.println("Meter Status 2: EInt0:0x" + String(en0_2, HEX) + " EInt1:0x" + String(en1_2, HEX));
    delay(10);
  
    /* only 1 voltage channel is used on each IC */
    voltage1 = main1.GetLineVoltageA();
    voltage2 = main2.GetLineVoltageA();
  
    /* get current readings from each IC and add them up */
    currentCT1 = main1.GetLineCurrentA();
    currentCT2 = main1.GetLineCurrentB();
    currentCT3 = main1.GetLineCurrentC();
    currentCT4 = main2.GetLineCurrentA();
    currentCT5 = main2.GetLineCurrentB();
    currentCT6 = main2.GetLineCurrentC();
    current1 = currentCT1 + currentCT2 + currentCT3;
    current2 = currentCT4 + currentCT5 + currentCT6;
    totalCurrent = current1 + current2;
      
    realPower1 = main1.GetTotalActivePower();
    realPower2 = main2.GetTotalActivePower();
    totalRealPower = realPower1 + realPower2;
  
    powerFactor1 = main1.GetTotalPowerFactor();
    powerFactor2 = main2.GetTotalPowerFactor();
  
    temp = main1.GetTemperature();
    freq = main1.GetFrequency();
    totalWatts = (voltage1 * current1) + (voltage2 * current2);
  
    Serial.println("V1:" + String(voltage1) + "V");
    Serial.println("V2:" + String(voltage2) + "V");
    Serial.println("I1:" + String(currentCT1) + "A");
    Serial.println("I2:" + String(currentCT2) + "A");
    Serial.println("I3:" + String(currentCT3) + "A");
    Serial.println("I4:" + String(currentCT4) + "A");
    Serial.println("I5:" + String(currentCT5) + "A");
    Serial.println("I6:" + String(currentCT6) + "A");


    /* get current from add-on boards as needed */
    if (AddOnBoards > 0)
    {
      AddOn1_CT1 = AddOn1_1.GetLineCurrentA();
      AddOn1_CT2 = AddOn1_1.GetLineCurrentB();
      AddOn1_CT3 = AddOn1_1.GetLineCurrentC();
      AddOn1_CT4 = AddOn1_2.GetLineCurrentA();
      AddOn1_CT5 = AddOn1_2.GetLineCurrentB();
      AddOn1_CT6 = AddOn1_2.GetLineCurrentC();
      AddOn1_current1 = AddOn1_CT1 + AddOn1_CT2 + AddOn1_CT3;
      AddOn1_current2 = AddOn1_CT4 + AddOn1_CT5 + AddOn1_CT6;
      AddOn1_totalCurrent = AddOn1_current1 + AddOn1_current2;

      Serial.println("I1_1:" + String(AddOn1_CT1) + "A");
      Serial.println("I1_2:" + String(AddOn1_CT2) + "A");
      Serial.println("I1_3:" + String(AddOn1_CT3) + "A");
      Serial.println("I1_4:" + String(AddOn1_CT4) + "A");
      Serial.println("I1_5:" + String(AddOn1_CT5) + "A");
      Serial.println("I1_6:" + String(AddOn1_CT6) + "A");
    
      if (AddOnBoards > 1)
      {
          AddOn2_CT1 = AddOn2_1.GetLineCurrentA();
          AddOn2_CT2 = AddOn2_1.GetLineCurrentB();
          AddOn2_CT3 = AddOn2_1.GetLineCurrentC();
          AddOn2_CT4 = AddOn2_2.GetLineCurrentA();
          AddOn2_CT5 = AddOn2_2.GetLineCurrentB();
          AddOn2_CT6 = AddOn2_2.GetLineCurrentC();
          AddOn2_current1 = AddOn2_CT1 + AddOn2_CT2 + AddOn2_CT3;
          AddOn2_current2 = AddOn3_CT4 + AddOn2_CT5 + AddOn2_CT6;
          AddOn2_totalCurrent = AddOn2_current1 + AddOn2_current2;
      
          Serial.println("I1_1:" + String(AddOn1_CT1) + "A");
          Serial.println("I1_2:" + String(AddOn1_CT2) + "A");
          Serial.println("I1_3:" + String(AddOn1_CT3) + "A");
          Serial.println("I1_4:" + String(AddOn1_CT4) + "A");
          Serial.println("I1_5:" + String(AddOn1_CT5) + "A");
          Serial.println("I1_6:" + String(AddOn1_CT6) + "A");
    
          if (AddOnBoards > 2)
          {

    
              if (AddOnBoards > 3)
              {

                        
                  if (AddOnBoards > 4)
                  {

                      
                      if (AddOnBoards > 5)
                      {

                      }
                  }
              }
          }
      }
    }

    
    Serial.println("AP:" + String(totalRealPower));
    Serial.println("PF1:" + String(powerFactor1));
    Serial.println("PF2:" + String(powerFactor2));
    Serial.println(String(temp) + "C");
    Serial.println("f" + String(freq) + "Hz");
  
    /* post to emonCMS */
    /* this can be changed to send whatever data above that you want */
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
     startMillis = currentMillis; //save the start time
    //}
  } // end loop
}
