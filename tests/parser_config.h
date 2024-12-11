#ifndef PROTOCONF
#define PROTOCONF

#include "proto.h"
const byte TestPacket0_pte[] = {0xFF};
const byte TestPacket1_pte[] = {TYPE_INT8, TYPE_INT16, TYPE_INT32, 0xFF};
const byte TestPacket2_pte[] = {TYPE_UINT8, TYPE_INT16, TYPE_UINT32, TYPE_INT32, 0xFF};

const byte *const parserTable[] = {TestPacket0_pte, TestPacket1_pte, TestPacket2_pte, 0};

typedef const volatile struct __attribute__((packed)) {
} TestPacket0;

typedef const volatile struct __attribute__((packed)) {
	INT8 sample8;
	INT16 sample16;
	INT32 sample32;
} TestPacket1;

typedef const volatile struct __attribute__((packed)) {
	UINT8 sampleu8;
	INT16 sample16;
	UINT32 sampleU32;
	INT32 sample32;
} TestPacket2;
#endif
