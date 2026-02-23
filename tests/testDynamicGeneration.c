#include <assert.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>

#include "CuTest.h"
#include "proto.h"

void TestDynamicVaruintGeneration(CuTest *tc)
{
  printf("\n%s.....", tc->name);
  resetParsing();
  TestPacket4 tp = {
      .test1 = 0xEE, .varuint = 32 | (15 << 7) | (96 << 14), .test2 = 0xEE};
  size_t size;
  byte *data = generatePacket(TestPacket4_ID, (void *)&tp, &size);
  CuAssertTrue(tc, data != 0);
  const byte mask = 0b10000000;
  byte reference[] = {0xEE, 0xEE, 32 | mask, 15 | mask, 96};

  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertIntEquals(tc, PACKET_HEADER_LENGTH + 5, size);

  int chk = 0x0;
  for(int i = 0; i < size - PACKET_HEADER_LENGTH; i++) {
    chk |= reference[i] ^ *(data + PACKET_HEADER_LENGTH + i);
  }
  CuAssertIntEquals(tc, 0, chk);
#ifdef MALLOC_ALLOCATOR
  free(data);
#endif
  puts("OK");
}

void TestDynamicVarintGeneration(CuTest *tc)
{
  printf("\n%s.....", tc->name);
  resetParsing();
  TestPacket5 tp = {.test1 = 0xEE, .varint = -35172, .test2 = 0xEE};
  size_t size;
  byte *data = generatePacket(TestPacket5_ID, (void *)&tp, &size);
  CuAssertTrue(tc, data != 0);
  byte reference[] = {0xEE, 0xEE, 0xe4, 0x12, 0x82};

  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertIntEquals(tc, PACKET_HEADER_LENGTH + 5, size);

  int chk = 0x0;
  for(int i = 0; i < size - PACKET_HEADER_LENGTH; i++) {
    chk |= reference[i] ^ *(data + PACKET_HEADER_LENGTH + i);
  }
  CuAssertIntEquals(tc, 0, chk);
#ifdef MALLOC_ALLOCATOR
  free(data);
#endif
  puts("OK");
}

void TestDynamicStrGeneration(CuTest *tc)
{
  printf("\n%s.....", tc->name);
  resetParsing();
  char *message = "Hello world!";
  StringParsingTest tp = {.test1 = 0xEE, .message = message, .test2 = 0xEE};
  size_t size;
  byte *data = generatePacket(StringParsingTest_ID, (void *)&tp, &size);
  CuAssertTrue(tc, data != 0);
  byte reference[] = {0xEE, 0xEE, 0xC, 'H', 'e', 'l', 'l', 'o',
                      ' ',  'w',  'o', 'r', 'l', 'd', '!'};

  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertIntEquals(tc, PACKET_HEADER_LENGTH + 15, size);

  int chk = 0x0;
  for(int i = 0; i < size - PACKET_HEADER_LENGTH; i++) {
    chk |= reference[i] ^ *(data + PACKET_HEADER_LENGTH + i);
  }
#ifdef MALLOC_ALLOCATOR
  free(data);
#endif

  CuAssertIntEquals(tc, 0, chk);
  puts("OK");
}
