#include <assert.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "CuTest.h"
#include "proto.h"

void TestDynamicVaruintParsing(CuTest *tc)
{
  printf("\n%s.....", tc->name);
  resetParsing();
  unsigned long long value = 0;
  for(int i = 0; i < 8 * sizeof(long long); i++) {
    TestPacket4 tp = {.test1 = 0xFF - i, .varuint = value, .test2 = i};

    size_t size;
    byte *data = generatePacket(TestPacket4_ID, (void *)&tp, &size);
    CuAssertTrue(tc, data != 0);
    CuAssertIntEquals(tc, 0, getLastErrorCode());

    for(int j = 0; j < size; j++) {
      processByte(*(data + j));
    }
    const int errCode = getLastErrorCode();
    CuAssertIntEquals(tc, 0, errCode);

    CuAssertTrue(tc, isNewPacketReady() != 0);
    TestPacket4 received = {};
    PacketHeader header;
    getPacket(&header, (void *)&received);

    CuAssertIntEquals(tc, 0xFF - i, received.test1);
    CuAssertIntEquals(tc, value, received.varuint);

    value <<= 1;
    value |= 1;
#ifdef MALLOC_ALLOCATOR
    free(data);
#endif
  }

  puts("OK");
}

void TestDynamicVarintParsing(CuTest *tc)
{
  printf("\n%s.....", tc->name);
  resetParsing();
  int sign = 1;
  long long value = 0;
  for(int i = 0; i < 16 * sizeof(long long) - 2; i++) {
    TestPacket5 tp = {.test1 = 0xFF - i, .varint = value * sign, .test2 = i};

    size_t size;
    byte *data = generatePacket(TestPacket5_ID, (void *)&tp, &size);
    CuAssertTrue(tc, data != 0);
    CuAssertIntEquals(tc, 0, getLastErrorCode());

    for(int j = 0; j < size; j++) {
      processByte(*(data + j));
    }
    const int errCode = getLastErrorCode();
    CuAssertIntEquals(tc, 0, errCode);
    CuAssertTrue(tc, isNewPacketReady() != 0);
    TestPacket5 received = {};
    PacketHeader header;
    getPacket(&header, (void *)&received);
    CuAssertIntEquals(tc, 0xFF - i, received.test1);
    CuAssertIntEquals(tc, value * sign, received.varint);

    sign *= -1;
    if(sign == 1) {
      value <<= 1;
      value |= 1;
    }
#ifdef MALLOC_ALLOCATOR
    free(data);
#endif
  }
  puts("OK");
}

void TestDynamicStrParsing(CuTest *tc)
{
  printf("\n%s.....", tc->name);
  resetParsing();
  char *message = "The quick brown fox jumps over the lazy dog";
  StringParsingTest tp = {.test1 = 0xEE, .message = message, .test2 = 0xEE};
  size_t size;
  byte *data = generatePacket(StringParsingTest_ID, (void *)&tp, &size);
  CuAssertTrue(tc, data != 0);

  for(int i = 0; i < size; i++) {
    processByte(*(data + i));
  }

  const int errCode = getLastErrorCode();
  CuAssertIntEquals(tc, 0, errCode);
  CuAssertTrue(tc, isNewPacketReady() != 0);
  StringParsingTest received = {};
  PacketHeader header;
  getPacket(&header, (void *)&received);

  CuAssertIntEquals(tc, 0, strcmp(received.message, message));
#ifdef MALLOC_ALLOCATOR
  free(data);
  free(received.message);
#endif

  puts("OK");
}

void TestEmptyString(CuTest *tc)
{
  printf("\n%s.....", tc->name);
  resetParsing();
  char *message = "";
  MultipleDynamicPacket tp = {message, 1234, 4312};
  size_t size;
  byte *data = generatePacket(MultipleDynamicPacket_ID, (void *)&tp, &size);
  CuAssertTrue(tc, data != 0);

  for(int i = 0; i < size; i++) {
    processByte(*(data + i));
  }

  const int errCode = getLastErrorCode();
  CuAssertIntEquals(tc, 0, errCode);
  CuAssertTrue(tc, isNewPacketReady() != 0);
  MultipleDynamicPacket received = {};
  PacketHeader header;
  getPacket(&header, (void *)&received);

  CuAssertIntEquals(tc, 0, strcmp(received.s, message));
  CuAssertIntEquals(tc, tp.vi, received.vi);
  CuAssertIntEquals(tc, tp.vu, received.vu);
#ifdef MALLOC_ALLOCATOR
  free(data);
  free(received.s);
#endif

  puts("OK");
}
