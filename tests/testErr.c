#include <assert.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"
#include "proto.h"

#define TEST_COUNT 0x2000

void TestErrorDetection(CuTest *tc)
{
  printf("running: %s\n", tc->name);
  srand(0);
  resetParsing();

  char *message =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
      "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
      "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
      "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
      "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
      "occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
      "mollit anim id est laborum.";
  ErrorTestPacket tp = {
      .test1 = 0xEE, .m1 = message, .m2 = message, .test2 = 0xEE};
  size_t size;

  int badParsed = 0;
  int crcMissmatchCount = 0;

  for(size_t i = 0; i < TEST_COUNT; i++) {
    byte *data = generatePacket(ErrorTestPacket_ID, (void *)&tp, &size);
    CuAssertTrue(tc, data != 0);

#if PREAMBLE_SIZE != 0
    for(size_t j = 0; j < 0x1000; j++) {
      byte r = rand() % 0xFF;
      if(r == (PREAMBLE_BYTES & 0xFF)) r = 0x00;
      processByte(r);
    }
#endif

    const int v = rand() % (size - PREAMBLE_SIZE);
    const int s = 1 + rand() % (size - PREAMBLE_SIZE);

    int appliedNoise = 0;

    while(appliedNoise == 0) {
      for(size_t j = 0; j < size; j++) {
        byte noise = 0x00;
        if((j + v) % s == 0 && j > PREAMBLE_SIZE) noise = rand() % 0xFE + 1;
        data[j] ^= noise;
        appliedNoise |= noise;
      }
    }

    for(size_t j = 0; j < size; j++) {
      processByte(data[j]);
    }

    const int errCode = getLastErrorCode();
    const int newReady = isNewPacketReady();

    if(errCode == PERR_DATA_CRC_MISMATCH || errCode == PERR_HDR_CRC_MISMATCH)
      crcMissmatchCount++;

    if(errCode == PERR_NOERR && newReady == 1) {
      badParsed++;
#if defined(MALLOC_ALLOCATOR)
      ErrorTestPacket received = {};
      PacketHeader header;
      getPacket(&header, (void *)&received);
      free(received.m1);
      free(received.m2);
#endif
    }

#if defined(MALLOC_ALLOCATOR)
    free(data);
#endif
  }
  if(sizeof(crcData_t) == 1) {
    CuAssertTrue(tc, badParsed <= TEST_COUNT / 512);
  } else if(sizeof(crcData_t) == 2) {
    CuAssertTrue(tc, badParsed <= TEST_COUNT / 1600);
  } else {
    CuAssertTrue(tc, badParsed == 0);
  }
  CuAssertTrue(tc, crcMissmatchCount >= TEST_COUNT * 0.9);
}

void TestPacketInJunk(CuTest *tc)
{
#if PREAMBLE_SIZE != 0
  printf("running: %s\n", tc->name);
  srand(rand());
  resetParsing();

  char *message = "The quick brown fox jumps over the lazy dog";
  ErrorTestPacket tp = {
      .test1 = 0xEE, .m1 = message, .m2 = message, .test2 = 0xEE};
  size_t size;

  for(size_t i = 0; i < TEST_COUNT; i++) {
    byte *data = generatePacket(ErrorTestPacket_ID, (void *)&tp, &size);
    CuAssertTrue(tc, data != 0);

    for(size_t j = 0; j < 0x1000; j++) {
      byte r = rand() % 0xFF;
      if(r == (PREAMBLE_BYTES & 0xFF)) r = 0x00;
      processByte(r);
    }

    for(size_t j = 0; j < size; j++) {
      processByte(data[j]);
    }

    const int errCode = getLastErrorCode();
    CuAssertIntEquals(tc, 0, errCode);
    CuAssertTrue(tc, isNewPacketReady() == 1);

    ErrorTestPacket received = {};
    PacketHeader header;
    getPacket(&header, (void *)&received);

    CuAssertIntEquals(tc, 0, strcmp(received.m1, message));
    CuAssertIntEquals(tc, 0, strcmp(received.m2, message));
#if defined(MALLOC_ALLOCATOR)
    free(data);
    free(received.m1);
    free(received.m2);
#endif
  }
#else
  printf("skipping: %s\n", tc->name);
#endif
}

void TestTotalNoise(CuTest *tc)
{
#if PREAMBLE_SIZE != 0
  printf("running: %s\n", tc->name);
  srand(rand());
  resetParsing();

  for(size_t i = 0; i < TEST_COUNT; i++) {
#if PREAMBLE_SIZE != 0
    for(size_t i = 0; i < PREAMBLE_SIZE; i++) {
      processByte((PREAMBLE_BYTES << 8 * i) & 0xFF);
    }
#endif
    for(size_t j = 0; j < 0x1000; j++) {
      byte r = rand() % 0xFF;
      processByte(r);
    }
  }
#else
  printf("skipping: %s\n", tc->name);
#endif
}
