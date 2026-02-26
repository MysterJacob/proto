#include <assert.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "CuTest.h"
#include "proto.h"

void TestTooLargePacket(CuTest *tc)
{
  printf("running: %s\n", tc->name);
  resetParsing();
  char message[8193] = "ABCDEF";

  int i = 0;
  for(i = 0; i < sizeof(message) - 1; i++) {
    message[i] = 'a';
  }
  message[i] = 0x0;

  MultipleDynamicPacket tp = {.s = message, .vu = 0x1001, .vi = 0x2556};
  size_t size;
#ifdef MALLOC_ALLOCATOR
  byte *data = generatePacket(MultipleDynamicPacket_ID, (void *)&tp, &size);
#else
  generatePacket(MultipleDynamicPacket_ID, (void *)&tp, &size);
#endif

  const int errCode = getLastErrorCode();
  CuAssertIntEquals(tc, PERR_PACKET_TOO_LARGE, errCode);
  CuAssertTrue(tc, isNewPacketReady() == 0);
#ifdef MALLOC_ALLOCATOR
  free(data);
#endif
}
