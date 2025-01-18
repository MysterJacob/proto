#include "proto.h"
const byte TestPacket0_pte[] = {0xFF};
const byte TestPacket1_pte[] = {TYPE_INT8, TYPE_INT16, TYPE_INT32, 0xFF};
const byte TestPacket2_pte[] = {TYPE_UINT8, TYPE_INT16, TYPE_UINT32, TYPE_INT32, 0xFF};
const byte TestPacket3_pte[] = {TYPE_UINT8, TYPE_STRING, TYPE_UINT8, 0xFF};
const byte *const parserTable[] = {TestPacket0_pte, TestPacket1_pte, TestPacket2_pte, TestPacket3_pte, 0};
