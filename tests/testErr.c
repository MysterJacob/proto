#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"
#include "packets.h"
#include "proto.h"

#define TEST_SIZE 128 * 1000

void TestErrorDetection(CuTest *tc)
{
  puts(tc->name);
  srand(0);
  resetParsing();

  char *message = "The quick brown fox jumps over the lazy dog";
  TestPacket3 tp = {.test1 = 0xEE, .message = message, .test2 = 0xEE};
  size_t size;

  puts("Error detection testing, please wait");
  for(int i = 0; i < TEST_SIZE; i++) {
    byte *data = generatePacket(3, (void *)&tp, &size);

    for(int j = 0; j < 0x1000; j++) {
      byte r = rand() % 0xFF;
      if(r == MAGIC_BYTES[0]) r = 0x00;
      processByte(r);
    }

    processByte(0x00);
    processByte(0x00);

    for(int j = 0; j < size; j++) {
      byte noise = 0x00;
      if(rand() % 10 == 0) noise = rand() % 0xFF;
      processByte(data[j] ^ noise);
    }

    const int errCode = getLastErrorCode();
    CuAssertTrue(tc, errCode != 0);
    CuAssertTrue(tc, isNewPacketReady() == 0);

    free(data);
  }
}

void TestPacketInJunk(CuTest *tc)
{
  puts(tc->name);
  srand(rand());
  resetParsing();

  char *message = "The quick brown fox jumps over the lazy dog";
  TestPacket3 tp = {.test1 = 0xEE, .message = message, .test2 = 0xEE};
  size_t size;

  puts("Packet in noise detection, please wait");
  for(int i = 0; i < TEST_SIZE; i++) {
    byte *data = generatePacket(3, (void *)&tp, &size);

    for(int j = 0; j < 0x1000; j++) {
      byte r = rand() % 0xFF;
      if(r == MAGIC_BYTES[0]) r = 0x00;
      processByte(r);
    }

    for(int j = 0; j < size; j++) {
      processByte(data[j]);
    }

    const int errCode = getLastErrorCode();
    CuAssertTrue(tc, errCode == 0);
    CuAssertTrue(tc, isNewPacketReady() == 1);

    TestPacket3 received;
    PacketHeader header;
    getPacket(&header, (void *)&received);

    CuAssertIntEquals(tc, 0, strcmp(received.message, message));

    free(data);
    free(received.message);
  }
}

void TestTotalNoise(CuTest *tc)
{
  puts(tc->name);
  srand(rand());
  resetParsing();

  puts("Total noise immunity testing, please wait");
  for(int i = 0; i < TEST_SIZE; i++) {
    for(size_t i = 0; i < MAGIC_SIZE; i++) {
      processByte(MAGIC_BYTES[i]);
    }
    for(int j = 0; j < 0x1000; j++) {
      byte r = rand() % 0xFF;
      processByte(r);
    }
  }
}
