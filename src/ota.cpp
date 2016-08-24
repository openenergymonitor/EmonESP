#include "emonesp.h"
#include "ota.h"
#include "web_server.h"
#include "wifi.h"
#include "http.h"

#include <ArduinoOTA.h>               // local OTA update from Arduino IDE
#include <ESP8266httpUpdate.h>        // remote OTA update from server
#include <ESP8266HTTPUpdateServer.h>  // upload update
#include <FS.h>

ESP8266HTTPUpdateServer httpUpdater;  // Create class for webupdate handleWebUpdate()


// -------------------------------------------------------------------
//OTA UPDATE SETTINGS
// -------------------------------------------------------------------
//UPDATE SERVER strings and interfers for upate server
// Array of strings Used to check firmware version
const char* u_host = "217.9.195.227";
const char* u_url = "/esp/firmware.php";
const char* firmware_update_path = "/upload";

void ota_setup()
{
  // Start local OTA update server
  ArduinoOTA.begin();

  // Setup firmware upload
  httpUpdater.setup(&server, firmware_update_path);
}

void ota_loop()
{
  ArduinoOTA.handle();
}

String ota_get_latest_version()
{
  // Get latest firmware version number
  String url = u_url;
  return get_http(u_host, url);
}

t_httpUpdate_return ota_http_update()
{
  SPIFFS.end(); // unmount filesystem
  t_httpUpdate_return ret = ESPhttpUpdate.update("http://" + String(u_host) + String(u_url) + "?tag=" + currentfirmware);
  SPIFFS.begin(); //mount-file system
  return ret;
}
