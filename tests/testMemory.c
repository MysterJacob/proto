#include <assert.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

#include "CuTest.h"
#include "proto.h"

#define TEST_COUNT 0x2000
uint64_t loop()
{
  char *message = "The quick brown fox jumps over the lazy dog";
  MemoryTestPacket tp = {.t1 = message, .t2 = message};
  size_t size;

  uint64_t deltaSum = 0;

  for(int i = 0; i < TEST_COUNT; i++) {
    struct mallinfo2 miStart = mallinfo2();
    byte *data = generatePacket(MemoryTestPacket_ID, (void *)&tp, &size);

    for(int i = 0; i < size; i++) {
      processByte(*(data + i));
    }

    MemoryTestPacket received;
    PacketHeader header;
    getPacket(&header, (void *)&received);
#ifdef MALLOC_ALLOCATOR
    free(data);
    free(received.t1);
    free(received.t2);
#endif
    struct mallinfo2 miEnd = mallinfo2();

    const uint32_t delta = miEnd.uordblks - miStart.uordblks;
    deltaSum += delta;
  }
  return deltaSum / TEST_COUNT;
}
void TestMemory(CuTest *tc)
{
  puts(tc->name);
  resetParsing();
  struct mallinfo2 miStart = mallinfo2();

  const uint64_t avgDelta = loop();

  struct mallinfo2 miEnd = mallinfo2();
  const uint64_t totalDelta = miEnd.uordblks - miStart.uordblks;
  printf("Total heap start size: %lu bytes\n", miStart.uordblks);
  printf("Total heap end size: %lu bytes\n", miEnd.uordblks);

  printf("Total heap delta: %lu bytes\n", totalDelta);
  printf("Average heap delta: %lu bytes\n", avgDelta);

  CuAssertTrue(tc, avgDelta == 0);
  CuAssertTrue(tc, totalDelta <= 512);
}
