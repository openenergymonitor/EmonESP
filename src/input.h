#ifndef _EMONESP_INPUT_H
#define _EMONESP_INPUT_H

#include <Arduino.h>

extern String last_datastr;
extern String input_string;

extern boolean input_get(String& data);

#endif // _EMONESP_INPUT_H
