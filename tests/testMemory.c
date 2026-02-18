#include <assert.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "CuTest.h"
#include "proto.h"

#define TEST_COUNT 0x2000
void TestMemory(CuTest *tc)
{
  printf("\n%s.....", tc->name);
  resetParsing();
  char *message =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do"
      "eiusmod  tempor incididunt ut labore et dolore magna aliqua. Ut"
      "enim ad minim "
      "veniam, quis nostrud exercitation ullamco laboris"
      "nisi ut aliquip ex ea "
      "commodo consequat. Duis aute irure dolor in"
      "reprehenderit in voluptate "
      "velit esse cillum dolore eu fugiat"
      "nulla pariatur. Excepteur sint "
      "occaecat cupidatat non proident,"
      "sunt in culpa qui officia deserunt "
      "mollit anim id est laborum.";
  MemoryTestPacket tp = {.t1 = message, .t2 = message};
  size_t size;

  for(int i = 0; i < TEST_COUNT; i++) {
    byte *data = generatePacket(MemoryTestPacket_ID, (void *)&tp, &size);

    for(int i = 0; i < size; i++) {
      processByte(*(data + i));
    }

    MemoryTestPacket received = {};
    PacketHeader header;
    getPacket(&header, (void *)&received);
#ifdef MALLOC_ALLOCATOR
    free(data);
    if(getLastErrorCode() == PERR_NOERR) {
      free(received.t1);
      free(received.t2);
    }
#endif
  }

  puts("OK");
}
