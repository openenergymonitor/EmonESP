/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Adaptation of Chris Howells OpenEVSE ESP Wifi
   by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
<<<<<<< HEAD

   Modified to use CircuitSetup.us Energy Meter by jdeglavina
=======
>>>>>>> 4fd788b6b35715701dd5849cfbc5ec5a4c2f0062
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

// for ATM90E32 board
#include <SPI.h>
#include <ATM90E32_SPI.h>

<<<<<<< HEAD
unsigned short LineGain = 7481; //0x1D39 
unsigned short VoltageGain = 32428; //0x7EAC - default value is for a 12v AC Transformer
unsigned short CurrentGainCT1 = 46539; //0xB5CB - 
unsigned short CurrentGainCT2 = 46539; //0xB5CB - 

#ifdef ESP8266
const int CS_pin = 16;
=======
#ifdef ESP8266
ATM90E32 eic(16); //set CS pin
>>>>>>> 4fd788b6b35715701dd5849cfbc5ec5a4c2f0062
/*
  D5/14 - CLK
  D6/12 - MISO
  D7/13 - MOSI
<<<<<<< HEAD
*/
#endif

#ifdef ESP32
const int CS_pin = 5;
=======
  D0/16 - CS
*/
#endif
#ifdef ESP32
ATM90E32 eic(5); //set CS pin
>>>>>>> 4fd788b6b35715701dd5849cfbc5ec5a4c2f0062
/*
  18 - CLK
  19 - MISO
  23 - MOSI
<<<<<<< HEAD
*/
#endif

#ifdef ARDUINO_ESP8266_WEMOS_D1MINI  // WeMos mini and D1 R2
const int CS_pin = D8; // WEMOS SS pin
#endif

#ifdef ARDUINO_ESP8266_ESP12  // Adafruit Huzzah
const int CS_pin = 15; // HUZZAH SS pins ( 0 or 15)
#endif

#ifdef ARDUINO_ARCH_SAMD //M0 board
const int CS_pin = 10; // M0 SS pin
#endif 

#ifdef __AVR_ATmega32U4__ //32u4 board
const int CS_pin = 10; // 32u4 SS pin
#endif 

#if !(defined ARDUINO_ESP8266_WEMOS_D1MINI || defined ARDUINO_ESP8266_ESP12 || defined ARDUINO_ARCH_SAMD || defined __AVR_ATmega32U4__ || defined ESP32 || defined ESP8266)
const int CS_pin = SS; // Use default SS pin for unknown Arduino
#endif

ATM90E32 eic(CS_pin, LineGain, VoltageGain, CurrentGainCT1, CurrentGainCT2); //pass CS pin and calibrations to ATM90E32 library

=======
  5 - CS
*/
#else
ATM90E32 eic(5); //set CS pin
#endif

>>>>>>> 4fd788b6b35715701dd5849cfbc5ec5a4c2f0062
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
  //DEBUG.println(ESP.getChipId());
  DEBUG.println("Firmware: " + currentfirmware);

  // Read saved settings from the config
  config_load_settings();

  // Initialise the WiFi
  wifi_setup();

  // Bring up the web server
  web_server_setup();

  // Start the OTA update systems
  ota_setup();

  DEBUG.println("Server started");

  /*Initialise the ATM90E32 + SPI port */
  Serial.println("Start ATM90E32");
  eic.begin();
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

<<<<<<< HEAD
  /*Repeatedly fetch some values from the ATM90E32 */
  float voltageA, voltageC, totalVoltage, currentCT1, currentCT2, totalCurrent, realPower, powerFactor, temp, freq, totalWatts;
=======

  /*Repeatedly fetch some values from the ATM90E32 */
  float voltageA, voltageC, totalVoltage, currentA, currentC, totalCurrent, realPower, powerFactor, temp, freq, totalWatts;
>>>>>>> 4fd788b6b35715701dd5849cfbc5ec5a4c2f0062

  unsigned short sys0 = eic.GetSysStatus0();
  unsigned short sys1 = eic.GetSysStatus1();
  unsigned short en0 = eic.GetMeterStatus0();
  unsigned short en1 = eic.GetMeterStatus1();

  Serial.println("Sys Status: S0:0x" + String(sys0, HEX) + " S1:0x" + String(sys1, HEX));
  Serial.println("Meter Status: E0:0x" + String(en0, HEX) + " E1:0x" + String(en1, HEX));
  delay(10);

  voltageA = eic.GetLineVoltageA();
  // Voltage B is not used
  voltageC = eic.GetLineVoltageC();
  totalVoltage = voltageA + voltageC ;
<<<<<<< HEAD
  currentCT1 = eic.GetLineCurrentA();
  // Current B is not used
  currentCT2 = eic.GetLineCurrentC();
  totalCurrent = currentCT1 + currentCT2;
=======
  currentA = eic.GetLineCurrentA();
  // Current B is not used
  currentC = eic.GetLineCurrentC();
  totalCurrent = currentA + currentC;
>>>>>>> 4fd788b6b35715701dd5849cfbc5ec5a4c2f0062
  realPower = eic.GetTotalActivePower();
  powerFactor = eic.GetTotalPowerFactor();
  temp = eic.GetTemperature();
  freq = eic.GetFrequency();
<<<<<<< HEAD
  totalWatts = (voltageA * currentCT1) + (voltageC * currentCT2);

  Serial.println("VA:" + String(voltageA) + "V");
  Serial.println("VC:" + String(voltageC) + "V");
  Serial.println("IA:" + String(currentCT1) + "A");
  Serial.println("IC:" + String(currentCT2) + "A");
=======
  totalWatts = (voltageA * currentA) + (voltageC * currentC);

  Serial.println("VA:" + String(voltageA) + "V");
  Serial.println("VC:" + String(voltageC) + "V");
  Serial.println("IA:" + String(currentA) + "A");
  Serial.println("IC:" + String(currentC) + "A");
>>>>>>> 4fd788b6b35715701dd5849cfbc5ec5a4c2f0062
  Serial.println("AP:" + String(realPower));
  Serial.println("PF:" + String(powerFactor));
  Serial.println(String(temp) + "C");
  Serial.println("f" + String(freq) + "Hz");

  String postStr = "VA:";
  postStr += String(voltageA);
  postStr += ",VC:";
  postStr += String(voltageC);
  postStr += ",totV:";
  postStr += String(totalVoltage);
  postStr += ",IA:";
<<<<<<< HEAD
  postStr += String(currentCT1);
  postStr += ",IC:";
  postStr += String(currentCT2);
=======
  postStr += String(currentA);
  postStr += ",IC:";
  postStr += String(currentC);
>>>>>>> 4fd788b6b35715701dd5849cfbc5ec5a4c2f0062
  postStr += ",totI:";
  postStr += String(totalCurrent);
  postStr += ",AP:";
  postStr += String(realPower);
  postStr += ",PF:";
  postStr += String(powerFactor);
  postStr += ",t:";
  postStr += String(temp);
  postStr += ",W:";
  postStr += String(totalWatts);



  //boolean gotInput = input_get(postStr);

  //if (wifi_mode == WIFI_MODE_CLIENT || wifi_mode == WIFI_MODE_AP_AND_STA)
  //{
  //if (gotInput) { //(emoncms_apikey != 0 && gotInput) {
  Serial.println(postStr);
  emoncms_publish(postStr);
  //}
  /*
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

