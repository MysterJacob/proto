#include "proto.h"

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

typedef const volatile struct __attribute__((packed)) {
	UINT8 test1;
	STRING message;
	UINT8 test2;
} TestPacket3;
