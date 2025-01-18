#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"
#include "proto.h"
#include "packets.h"

void TestStaticParsing(CuTest *tc)
{
  loadPacketTable();
  resetParsing();
  union {
    volatile struct {
      PacketHeader header;
      TestPacket1 packet;
    } packet;
    const byte data[27];
  } data = {
      .packet.header = {.length    = 7,
                        .id        = 1,
                        .seqNumber = 0,
                        .ackNumber = 0,
                        .checksum  = 0x0000},
      .packet.packet = {1, 2, 3}
  };
  processByte(MAGIC1);
  processByte(MAGIC2);
  for(int i = 0; i < 25; i++) {
    processByte(data.data[i]);
  }
  const PacketHeader *ch = getLastHeader();

  CuAssertTrue(tc, ch != 0);
  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertIntEquals(tc, data.packet.header.length, ch->length);
  CuAssertIntEquals(tc, data.packet.header.id, ch->id);
  CuAssertIntEquals(tc, data.packet.header.seqNumber, ch->seqNumber);
  CuAssertIntEquals(tc, data.packet.header.ackNumber, ch->ackNumber);
  CuAssertIntEquals(tc, data.packet.header.checksum, ch->checksum);

  TestPacket1 *packet = getLastPacket();

  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertTrue(tc, packet != 0);
  CuAssertIntEquals(tc, data.packet.packet.sample8, packet->sample8);
  CuAssertIntEquals(tc, data.packet.packet.sample16, packet->sample16);
  CuAssertIntEquals(tc, data.packet.packet.sample32, packet->sample32);

  free((void *)packet);
}
