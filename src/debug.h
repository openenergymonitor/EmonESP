#ifndef __DEBUG_H
#define __DEBUG_H

#undef DEBUG_PORT
#define DEBUG_PORT SerialDebug

#undef EMONTX_PORT
#define EMONTX_PORT SerialEmonTx

#include "MicroDebug.h"
#include "StreamSpy.h"

extern StreamSpy SerialDebug;
extern StreamSpy SerialEmonTx;

extern void debug_setup();

#endif // __DEBUG_H
