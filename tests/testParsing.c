#include <assert.h>
#include <stddef.h>

#include "CuTest.h"
#include "packets.h"
#include "proto.h"

void TestStaticParsing(CuTest *tc)
{
  resetParsing();
  union {
    volatile struct {
      PacketHeader header;
      TestPacket1 packet;
    } packet;
    const byte data[27];
  } data = {
      .packet.header = {.length = 7,
                        .id = 1,
                        .seqNumber = 0,
                        .ackNumber = 0,
                        .checksum = 0x8c4a},
      .packet.packet = {1, 2, 3}
  };
  processByte(MAGIC1);
  processByte(MAGIC2);
  for(int i = 0; i < sizeof(PacketHeader) + sizeof(TestPacket1); i++) {
    processByte(data.data[i]);
  }

  CuAssertTrue(tc, isNewPacketReady() != 0);
  TestPacket1 packet;
  PacketHeader header;
  getLastPacket(&header, (void *)&packet);

  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertIntEquals(tc, data.packet.header.length, header.length);
  CuAssertIntEquals(tc, data.packet.header.id, header.id);
  CuAssertIntEquals(tc, data.packet.header.seqNumber, header.seqNumber);
  CuAssertIntEquals(tc, data.packet.header.ackNumber, header.ackNumber);
  CuAssertIntEquals(tc, data.packet.header.checksum, header.checksum);

  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertIntEquals(tc, data.packet.packet.sample8, packet.sample8);
  CuAssertIntEquals(tc, data.packet.packet.sample16, packet.sample16);
  CuAssertIntEquals(tc, data.packet.packet.sample32, packet.sample32);
}
