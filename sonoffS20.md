# Sonoff S20

**Sonoff S20 programmer:**

A standard USB to UART programing cable can be used to upload firmware to the SonOffS20 smart plug. Connect up power and RX/TX lines to the 4 pin programming connector on the plug:

![sonoffs20.png](docs/sonoffs20.png)

To upload firmware, hold down the push button, reset the power to the plug (unplug and plug the programmer back in) and then keep holding down the push button until the firmware upload progress starts. 

Saftey: The plug does not need to be connected to mains when programming. It's advised not to connect to the mains when the case is open from a saftey perspective.

**Firmware modifications:**

1. git checkout control_merge 
2. The sonoff S20 LED is on pin 13, change LEDpin to 13 in config.cpp
3. In src.ino, uncommment "#include http.h" and "#include autoauth.h".
4. Rename node_type to "smartplug"
5. Set node\_id as required, each plug must have a unique node\_id in the house.
5. Uncomment auth\_setup(); and auth\_loop();

**Compilation:**

Sonoff S20 smart plugs can have either the ESP8266 or **ESP8285** core.

Select in Arduino > Tools:

- Generic ESP8285 Module
- Flash Size "1M (512K SPIFFS)"

Compile and upload both the firmware and the sketch data.


### Setup

1. SmartPlug creates a WIFI access point, connect to access point, enter home WIFI network.

2. Login to emoncms, if UDP broadcast is enabled, smart plug will apear in emoncms device list with a popup asking to connect.

3. One connected the smart plug can be scheduled using the scheduler.
