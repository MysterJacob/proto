#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"
#include "packets.h"
#include "proto.h"

void TestStaticGeneration(CuTest *tc)
{
  puts("Testing Static Generation");
//   resetParsing();
  hardResetParser();
  TestPacket2 testPacket = {0xDD, 0xFFF, 0xCCCCCCCC, 0xAAAAAAA};

  size_t size = 0;

  byte *rawData = generatePacket(2, (void *)&testPacket, &size);
  for(int i = 0; i < size; i++) {
    processByte(rawData[i]);
  }
  CuAssertIntEquals(tc, 0, getLastErrorCode());

  CuAssertTrue(tc, isNewPacketReady() != 0);
  TestPacket2 received;
  PacketHeader header;
  getLastPacket(&header, (void *)&received);

  CuAssertIntEquals(tc, 2, header.id);

  CuAssertIntEquals(tc, testPacket.sampleu8, received.sampleu8);
  CuAssertIntEquals(tc, testPacket.sample16, received.sample16);
  CuAssertIntEquals(tc, testPacket.sampleU32, received.sampleU32);

  free((void *)rawData);
}

void TestAckSq(CuTest *tc)
{
  puts("Testing Ack/Seq");
  resetParsing();

  TestPacket0 tp = {};
  size_t size;

  byte *rawData = generatePacket(0, (void *)&tp, &size);
  for(int h = 0; h < size; h++) {
    processByte(rawData[h]);
  }

  CuAssertIntEquals(tc, 0, getLastErrorCode());

  CuAssertTrue(tc, isNewPacketReady() != 0);
  TestPacket2 received;
  PacketHeader header;
  getLastPacket(&header, (void *)&received);

  const unsigned int baseSeq = header.seqNumber + 1;
  const unsigned int baseAck = header.ackNumber + 1;

  for(int i = 0; i < 0xFFFF; i++) {
    byte *rawData = generatePacket(0, (void *)&tp, &size);
    for(int h = 0; h < size; h++) {
      processByte(rawData[h]);
    }

    CuAssertIntEquals(tc, 0, getLastErrorCode());

    CuAssertTrue(tc, isNewPacketReady() != 0);
    getLastPacket(&header, (void *)&received);

    CuAssertIntEquals(tc, i + baseSeq, header.seqNumber);
    CuAssertIntEquals(tc, i + baseAck, header.ackNumber);

    free((void *)rawData);
  }
}
