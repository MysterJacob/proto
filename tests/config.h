#include "proto.h"

const byte TestPacket0_pte[] = {0xFF};
const byte TestPacket1_pte[] = {TYPE_INT8, TYPE_INT16, TYPE_INT32, 0xFF};

const byte *const parserTable[] = {TestPacket0_pte, TestPacket1_pte, 0};

typedef volatile const struct {
} TestPacket0;

typedef volatile const struct {
	INT8 sample8;
	INT16 sample16;
	INT32 sample32;
} TestPacket1;

