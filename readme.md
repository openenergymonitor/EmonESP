# EmonESP

ESP8266 WIFI serial to Emoncms link

![emonesp.jpg](emonesp.jpg)

### Installation

#### 1. Install ESP for Arduino IDE with Boards Manager

Insuructions from: https://github.com/esp8266/Arduino

- Install Arduino IDE 1.6.8 from the Arduino website.
- Start Arduino and open Preferences window.
- Enter http://arduino.esp8266.com/stable/package_esp8266com_index.json into Additional Board Manager URLs field. You can add multiple URLs, separating them with commas.
- Open Boards Manager from Tools > Board menu and install esp8266 platform 
- Select `Tools > Board > Generic ESP8266 Module` (required for EmonESP)

#### 2. Install ESP filesystem file uploader

Required to include `data` folder with html etc in the upload 

[Follow esp8266 filesystem instructions (copied  below):](https://github.com/esp8266/Arduino/blob/master/doc/filesystem.md)

- [Download the Arduino IDE plug-in (.zip)](https://github.com/esp8266/arduino-esp8266fs-plugin/releases/download/0.2.0/ESP8266FS-0.2.0.zip)
- Navigate to the `tools` folder in your Arduino sketchbook directory, (create directory if it doesn't exist)
- Unpack the plug-in into tools directory (the path will look like `<home_dir>/Arduino/tools/ESP8266FS/tool/esp8266fs.jar`)
- Restart Arduino

#### 3. Compile and Upload

- Open EmonESP.ino in the Arduino IDE.
- Compile and Upload as normal
- Upload home.html web page using the ESP8266 Sketch Data Upload tool under Arduino tools.

### Licence

GNU General Public License
