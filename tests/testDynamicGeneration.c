#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "CuTest.h"
#include "packets.h"
#include "proto.h"

void TestDynamicVaruintGeneration(CuTest *tc)
{
  puts(tc->name);
  resetParsing();
  TestPacket4 tp = {
      .test1 = 0xEE, .varuint = 32 | (15 << 7) | (96 << 14), .test2 = 0xEE};
  size_t size;
  byte *data = generatePacket(4, (void *)&tp, &size);
  const byte mask = 0b10000000;
  byte reference[] = {0xEE, 0xEE, 32 | mask, 15 | mask, 96};

  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertIntEquals(tc, PACKET_HEADER_LENGTH + 5, size);

  int chk = 0x0;
  data += PACKET_HEADER_LENGTH;
  for(int i = 0; i < size - PACKET_HEADER_LENGTH; i++) {
    chk |= reference[i] ^ *(data++);
  }
  CuAssertIntEquals(tc, 0, chk);
}

void TestDynamicVarintGeneration(CuTest *tc)
{
  puts(tc->name);
  resetParsing();
  TestPacket5 tp = {.test1 = 0xEE, .varint = -35172, .test2 = 0xEE};
  size_t size;
  byte *data = generatePacket(5, (void *)&tp, &size);
  byte reference[] = {0xEE, 0xEE, 0xe4, 0x12, 0x82};

  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertIntEquals(tc, PACKET_HEADER_LENGTH + 5, size);

  int chk = 0x0;
  data += PACKET_HEADER_LENGTH;
  for(int i = 0; i < size - PACKET_HEADER_LENGTH; i++) {
    chk |= reference[i] ^ *(data++);
  }
  CuAssertIntEquals(tc, 0, chk);
}

void TestDynamicStrGeneration(CuTest *tc)
{
  puts(tc->name);
  resetParsing();
  char *message = "Hello world!";
  TestPacket3 tp = {.test1 = 0xEE, .message = message, .test2 = 0xEE};
  size_t size;
  byte *data = generatePacket(3, (void *)&tp, &size);
  byte reference[] = {0xEE, 0xEE, 0xC, 'H', 'e', 'l', 'l', 'o',
                      ' ',  'w',  'o', 'r', 'l', 'd', '!'};

  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertIntEquals(tc, PACKET_HEADER_LENGTH + 15, size);

  int chk = 0x0;
  data += PACKET_HEADER_LENGTH;
  for(int i = 0; i < size - PACKET_HEADER_LENGTH; i++) {
    chk |= reference[i] ^ *(data++);
  }
  CuAssertIntEquals(tc, 0, chk);
}
