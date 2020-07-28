#include <StreamSpy.h>

#ifndef DEBUG_PORT
#define DEBUG_PORT Serial1
#endif

#ifndef EMONTX_PORT
#define EMONTX_PORT Serial
#endif

StreamSpy SerialDebug(DEBUG_PORT);
StreamSpy SerialEmonTx(EMONTX_PORT);

void debug_setup()
{
  DEBUG_PORT.begin(115200);
  SerialDebug.begin(2048);
  SerialDebug.onWrite([](const uint8_t *buffer, size_t size) {

  });

  EMONTX_PORT.begin(115200);
  SerialEmonTx.begin(2048);
  SerialEmonTx.onWrite([](const uint8_t *buffer, size_t size) {

  });
}
