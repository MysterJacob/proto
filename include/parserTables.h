#include "proto.h"
const byte TestPacket0_pte[] = {0xFF};
const byte TestPacket1_pte[] = {TYPE_INT8, TYPE_INT16, TYPE_INT32, 0xFF};
const byte TestPacket2_pte[] = {TYPE_UINT8, TYPE_INT16, TYPE_UINT32, TYPE_INT32, 0xFF};
const byte TestPacket3_pte[] = {TYPE_UINT8, TYPE_UINT8, TYPE_STRING, 0xFF};
const byte TestPacket4_pte[] = {TYPE_UINT8, TYPE_UINT8, TYPE_VARUINT, 0xFF};
const byte *const parserTable[] = {TestPacket0_pte, TestPacket1_pte, TestPacket2_pte, TestPacket3_pte, TestPacket4_pte, 0};
const unsigned int definedPacketCount = 5;
const unsigned int packetStaticSizes[] = {0, 7, 11, 2, 2, 0x00};
const unsigned int packetStaticCount[] = {0, 3, 4, 2, 2, 0x00};