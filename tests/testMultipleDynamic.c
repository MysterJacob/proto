#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "CuTest.h"
#include "proto.h"

void TestMultipleDynamic(CuTest *tc)
{
  printf("%s.....", tc->name);
  resetParsing();
  char *message = "The quick brown fox jumps over the lazy dog";
  MultipleDynamicPacket tp = {.s = message, .vu = 0x1001, .vi = 0x2556};
  size_t size;
  byte *data = generatePacket(MultipleDynamicPacket_ID, (void *)&tp, &size);

  for(int i = 0; i < size; i++) {
    processByte(*data++);
  }

  const int errCode = getLastErrorCode();
  CuAssertIntEquals(tc, 0, errCode);
  CuAssertTrue(tc, isNewPacketReady() != 0);
  MultipleDynamicPacket received;
  PacketHeader header;
  getPacket(&header, (void *)&received);

  CuAssertIntEquals(tc, 0, strcmp(received.s, message));
  CuAssertIntEquals(tc, 0x1001, received.vu);
  CuAssertIntEquals(tc, 0x2556, received.vi);
  puts("OK");
}
