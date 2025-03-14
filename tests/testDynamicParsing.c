#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "CuTest.h"
#include "packets.h"
#include "proto.h"

void TestDynamicVaruintParsing(CuTest *tc)
{
  resetParsing();
  unsigned long long value = 0;
  for(int i = 0; i < 8 * sizeof(long long); i++) {
    TestPacket4 tp = {.test1 = 0xFF - i, .varuint = value, .test2 = i};

    size_t size;
    byte *data = generatePacket(4, (void *)&tp, &size);
    CuAssertIntEquals(tc, 0, getLastErrorCode());

    for(int j = 0; j < size; j++) {
      processByte(*data++);
    }
    const int errCode = getLastErrorCode();
    CuAssertIntEquals(tc, 0, errCode);
    TestPacket4 *recv = (TestPacket4 *)getLastPacket();
    CuAssertTrue(tc, recv != 0);
    CuAssertIntEquals(tc, 0xFF - i, recv->test1);
    CuAssertIntEquals(tc, value, recv->varuint);

    value <<= 1;
    value |= 1;
  }
}

void TestDynamicVarintParsing(CuTest *tc)
{
  resetParsing();
  int sign = 1;
  long long value = 0;
  for(int i = 0; i < 16 * sizeof(long long) - 2; i++) {
    TestPacket5 tp = {.test1 = 0xFF - i, .varint = value * sign, .test2 = i};

    size_t size;
    byte *data = generatePacket(5, (void *)&tp, &size);
    CuAssertIntEquals(tc, 0, getLastErrorCode());

    for(int j = 0; j < size; j++) {
      processByte(*data++);
    }
    const int errCode = getLastErrorCode();
    CuAssertIntEquals(tc, 0, errCode);
    TestPacket5 *recv = (TestPacket5 *)getLastPacket();
    CuAssertTrue(tc, recv != 0);
    CuAssertIntEquals(tc, 0xFF - i, recv->test1);
    CuAssertIntEquals(tc, value * sign, recv->varint);

    sign *= -1;
    if(sign == 1) {
      value <<= 1;
      value |= 1;
    }
  }
}

void TestDynamicStrParsing(CuTest *tc)
{
  resetParsing();
  char *message = "The quick brown fox jumps over the lazy dog";
  TestPacket3 tp = {.test1 = 0xEE, .message = message, .test2 = 0xEE};
  size_t size;
  byte *data = generatePacket(3, (void *)&tp, &size);

  for(int i = 0; i < size; i++) {
    processByte(*data++);
  }

  const int errCode = getLastErrorCode();
  CuAssertIntEquals(tc, 0, errCode);
  TestPacket3 *recv = (TestPacket3 *)getLastPacket();
  CuAssertTrue(tc, recv != 0);
  CuAssertIntEquals(tc, 0, strcmp(recv->message, message));
}
