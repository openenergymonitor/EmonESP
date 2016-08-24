#ifndef _EMONESP_OTA_H
#define _EMONESP_OTA_H

#include <Arduino.h>
#include <ESP8266httpUpdate.h>

void ota_setup();
void ota_loop();
String ota_get_latest_version();
t_httpUpdate_return ota_http_update();

#endif // _EMONESP_OTA_H
