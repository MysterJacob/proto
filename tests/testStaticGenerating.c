#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"
#include "packets.h"
#include "proto.h"

void TestStaticGenerating(CuTest *tc)
{
  loadPacketTable();
  resetParsing();
  TestPacket2 testPacket = {0xDD, 0xFFFF, 0xCCCCCCCC, 0xAAAAAAAA};

  unsigned int size = 0;

  byte *rawData = generatePacket(2, (void *)&testPacket, &size);
  for(int i = 0; i < size; i++) {
    processByte(rawData[i]);
  }
  CuAssertIntEquals(tc, 0, getLastErrorCode());

  const PacketHeader *header = getLastHeader();
  CuAssertIntEquals(tc, 2, header->id);

  TestPacket2 *received = getLastPacket();
  CuAssertTrue(tc, received != 0);

  CuAssertIntEquals(tc, testPacket.sampleu8, received->sampleu8);
  CuAssertIntEquals(tc, testPacket.sample16, received->sample16);
  CuAssertIntEquals(tc, testPacket.sampleU32, received->sampleU32);

  free((void *)rawData);
  free((void *)received);
}

void TestAckSq(CuTest *tc)
{
  loadPacketTable();
  resetParsing();

  TestPacket0 tp = {};
  unsigned int size;

  byte *rawData = generatePacket(0, (void *)&tp, &size);
  for(int h = 0; h < size; h++) {
    processByte(rawData[h]);
  }

  CuAssertIntEquals(tc, 0, getLastErrorCode());

//   const void* received = getLastPacket();
  const PacketHeader *header = getLastHeader();

  const unsigned int baseSeq = header->seqNumber + 1;
  const unsigned int baseAck = header->ackNumber + 1;

  for(int i = 0; i < 0xFFFF; i++) {
    byte *rawData = generatePacket(0, (void *)&tp, &size);
    for(int h = 0; h < size; h++) {
      processByte(rawData[h]);
    }

    CuAssertIntEquals(tc, 0, getLastErrorCode());

    const void* received = getLastPacket();
    const PacketHeader *header = getLastHeader();

    CuAssertIntEquals(tc, i + baseSeq, header->seqNumber);
    CuAssertIntEquals(tc, i + baseAck, header->ackNumber);

    free((void *)rawData);
    free((void *)received);
  }
}
