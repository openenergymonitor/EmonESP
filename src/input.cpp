#include "emonesp.h"
#include "input.h"

String input_string="";
String last_datastr="";

boolean input_get(String& data)
{
  boolean gotData = false;

  // If data from test API e.g `http://<IP-ADDRESS>/input?string=CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7`
  if(input_string.length() > 0) {
    data = input_string;
    input_string = "";
    gotData = true;
  }
  // If data received on serial
  else if (Serial.available()) {
    // Could check for string integrity here
    data = Serial.readStringUntil('\n');
    gotData = true;
  }

  if(gotData)
  {
    // Get rid of any whitespace, newlines etc
    data.trim();

    if(data.length() > 0) {
      DEBUG.printf("Got '%s'\n", data.c_str());
      last_datastr = data;
    } else {
      gotData = false;
    }
  }

  return gotData;
}
