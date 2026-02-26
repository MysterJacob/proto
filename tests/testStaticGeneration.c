#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"
#include "proto.h"

void TestStaticGeneration(CuTest *tc)
{
  printf("running: %s\n", tc->name);
  hardResetParser();
  srand(0xCAFEBAAE);
  for(int i = 0; i < 0x1000; i++) {
    TestPacket2 testPacket = {rand() & 0xFF, rand() & 0xFFFF,
                              rand() & 0xFFFFFFFF, rand() & 0xFFFFFFF};

    size_t size = 0;

    byte *data = generatePacket(TestPacket2_ID, (void *)&testPacket, &size);
    CuAssertTrue(tc, data != 0);
    for(int i = 0; i < size; i++) {
      processByte(data[i]);
    }
    CuAssertIntEquals(tc, 0, getLastErrorCode());

    CuAssertTrue(tc, isNewPacketReady() != 0);
    TestPacket2 received = {};
    PacketHeader header;
    getPacket(&header, (void *)&received);

    CuAssertIntEquals(tc, 2, header.id);

    CuAssertIntEquals(tc, testPacket.sampleu8, received.sampleu8);
    CuAssertIntEquals(tc, testPacket.sample16, received.sample16);
    CuAssertIntEquals(tc, testPacket.sampleU32, received.sampleU32);
#ifdef MALLOC_ALLOCATOR
    free(data);
#endif
  }
  resetParsing();
}
