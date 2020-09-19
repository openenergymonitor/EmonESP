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
#include "http.h"

#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>

static String get_http_internal(WiFiClient &client, const char *host, String &path, int port, bool secure)
{
  HTTPClient http;                      // Create class for HTTP TCP connections get_http
  
  DBUGF("Connecting to http%s://%s:%d%s", secure ? "s" : "", host, port, path.c_str());

  http.begin(client, host, port, path, secure);
  int httpCode = http.GET();
  if((httpCode > 0) && (httpCode == HTTP_CODE_OK))
  {
    String payload = http.getString();
    DEBUG.println(payload);
    http.end();
    return(payload);
  } else {
    http.end();
    return(String(F("server error: "))+http.errorToString(httpCode));
  }
}

// -------------------------------------------------------------------
// HTTPS SECURE GET Request
// url: N/A
// -------------------------------------------------------------------

String get_https(const char* fingerprint, const char* host, String &path, int httpsPort)
{
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setBufferSizes(512, 512);

  // IMPROVE: use a certificate
  if(false == client->setFingerprint(fingerprint)) {
    return F("Invalid fingerprint");
  }

  return get_http_internal(*client, host, path, httpsPort, true);
}

// -------------------------------------------------------------------
// HTTP GET Request
// url: N/A
// -------------------------------------------------------------------
String get_http(const char *host, String &path){
  WiFiClient client;
  return get_http_internal(client, host, path, 80, false);
} // end http_get
