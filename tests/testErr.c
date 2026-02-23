#include <assert.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"
#include "proto.h"

#define TEST_SIZE 32 * 1000

void TestErrorDetection(CuTest *tc)
{
  printf("\n%s.....", tc->name);
  srand(0);
  resetParsing();

  char *message =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
      "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
      "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
      "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
      "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
      "occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
      "mollit anim id est laborum."
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
      "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
      "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
      "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
      "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
      "occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
      "mollit anim id est laborum."
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
      "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
      "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
      "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
      "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
      "occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
      "mollit anim id est laborum."
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

  int collisionCount = 0;
  for(int i = 0; i < TEST_SIZE; i++) {
    byte *data = generatePacket(ErrorTestPacket_ID, (void *)&tp, &size);
    CuAssertTrue(tc, data != 0);

    for(int j = 0; j < 0x1000; j++) {
      byte r = rand() % 0xFF;
      if(r == MAGIC_BYTES[0]) r = 0x00;
      processByte(r);
    }

    const int v = rand() % 15;
    for(int j = 0; j < size; j++) {
      byte noise = 0x00;
      if((j + v) % 15 == 0 && j > MAGIC_SIZE) noise = rand() % 0xFE + 1;
      processByte(data[j] ^ noise);
    }

    const int errCode = getLastErrorCode();

    if(errCode == PERR_NOERR || isNewPacketReady() == 1) {
      collisionCount++;
#ifdef MALLOC_ALLOCATOR
      ErrorTestPacket received = {};
      PacketHeader header;
      getPacket(&header, (void *)&received);
      free(received.m1);
      free(received.m2);
#endif
    }

#ifdef MALLOC_ALLOCATOR
    free(data);
#endif
  }
  CuAssertTrue(tc, collisionCount <= 10);
  puts("OK");
}

void TestPacketInJunk(CuTest *tc)
{
  printf("\n%s.....", tc->name);
  srand(rand());
  resetParsing();

  char *message = "The quick brown fox jumps over the lazy dog";
  ErrorTestPacket tp = {
      .test1 = 0xEE, .m1 = message, .m2 = message, .test2 = 0xEE};
  size_t size;

  for(int i = 0; i < TEST_SIZE; i++) {
    byte *data = generatePacket(ErrorTestPacket_ID, (void *)&tp, &size);
    CuAssertTrue(tc, data != 0);

    for(int j = 0; j < 0x1000; j++) {
      byte r = rand() % 0xFF;
      if(r == MAGIC_BYTES[0]) r = 0x00;
      processByte(r);
    }

    for(int j = 0; j < size; j++) {
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
#ifdef MALLOC_ALLOCATOR
    free(data);
    free(received.m1);
    free(received.m2);
#endif
  }
  puts("OK");
}

void TestTotalNoise(CuTest *tc)
{
  printf("\n%s.....", tc->name);
  srand(rand());
  resetParsing();

  for(int i = 0; i < TEST_SIZE; i++) {
    for(size_t i = 0; i < MAGIC_SIZE; i++) {
      processByte(MAGIC_BYTES[i]);
    }
    for(int j = 0; j < 0x1000; j++) {
      byte r = rand() % 0xFF;
      processByte(r);
    }
  }
  puts("OK");
}
