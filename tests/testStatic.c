#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"
#include "proto.h"

void TestEmptyPacket(CuTest *tc)
{
  printf("running: %s\n", tc->name);
  hardResetParser();
  size_t size;
  byte *data = generatePacket(TestPacket0_ID, 0, &size);
  for(size_t i = 0; i < size; i++) {
    processByte(data[i]);
  }
  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertTrue(tc, isNewPacketReady() != 0);
  PacketHeader header;
  getPacket(&header, 0);

  CuAssertIntEquals(tc, TestPacket0_ID, header.id);
  CuAssertIntEquals(tc, 0, header.length);

#if defined(MALLOC_ALLOCATOR)
  free(data);
#endif
}
void TestStatic(CuTest *tc)
{
  printf("running: %s\n", tc->name);
  hardResetParser();
  srand(0xCAFEBAAE);
  for(size_t j = 0; j < 0x1000; j++) {
    TestPacket2 testPacket = {rand() & 0xFF, rand() & 0xFFFF,
                              rand() & 0xFFFFFFFF, rand() & 0xFFFFFFF};

    size_t size = 0;

    byte *data = generatePacket(TestPacket2_ID, (void *)&testPacket, &size);
    CuAssertTrue(tc, data != 0);
    for(size_t i = 0; i < size; i++) {
      processByte(data[i]);
    }
    CuAssertIntEquals(tc, 0, getLastErrorCode());
    CuAssertTrue(tc, isNewPacketReady() != 0);
    TestPacket2 received = {};
    PacketHeader header;
    getPacket(&header, (void *)&received);

    CuAssertIntEquals(tc, TestPacket2_ID, header.id);
    CuAssertIntEquals(tc, 11, header.length);

#if defined(SKIP_PACKET_LEN)
    CuAssertIntEquals(tc, PACKET_HEADER_LENGTH + 11 - sizeof(packetLen_t),
                      size);
#else
    CuAssertIntEquals(tc, PACKET_HEADER_LENGTH + 11, size);
#endif

    CuAssertIntEquals(tc, testPacket.sampleu8, received.sampleu8);
    CuAssertIntEquals(tc, testPacket.sample16, received.sample16);
    CuAssertIntEquals(tc, testPacket.sampleU32, received.sampleU32);
#if defined(MALLOC_ALLOCATOR)
    free(data);
#endif
  }
  resetParsing();
}
