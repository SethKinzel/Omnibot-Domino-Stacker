
#ifndef SK_RC_Payload_h
#define SK_RC_Payload_h

#include <Arduino.h>

const byte address[10] = "ADDRESS01";

struct payloadStruct {
  int16_t lx = 0;
  int16_t ly = 0;
  int16_t rx = 0;
  int16_t ry = 0;
  int8_t lb1 = 0;
  int8_t rb1 = 0;
};

payloadStruct payload;

#endif