#include <assert.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "CuTest.h"
#include "proto.h"

void TestMultipleDynamic(CuTest *tc)
{
  printf("running: %s\n", tc->name);
  resetParsing();
  char *message = "The quick brown fox jumps over the lazy dog";
  MultipleDynamicPacket tp = {.s = message, .vu = 0x1001, .vi = 0x2556};
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
#ifdef SAVE_STRING_SIZE
  CuAssertIntEquals(tc, strlen(message), received.s_len);
#endif
  CuAssertIntEquals(tc, 0x1001, received.vu);
  CuAssertIntEquals(tc, 0x2556, received.vi);
#ifdef MALLOC_ALLOCATOR
  free(data);
  free(received.s);
#endif
}
