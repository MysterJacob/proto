#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "CuTest.h"
#include "proto.h"

void TestSimpleParsing(CuTest *tc)
{
  loadPacketTable();
  resetParsing();
  typedef struct {
    char c1;
    short c2;
    int c3;
  } s1;
  union {
    volatile struct {
      PacketHeader header;
      s1 packet;
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
  for(int i = 0; i < 27; i++) {
    processByte(data.data[i]);
  }
  const PacketHeader *ch = getLastHeader();
  CuAssertTrue(tc, ch != 0);
  CuAssertIntEquals(tc, data.packet.header.length, ch->length);
  CuAssertIntEquals(tc, data.packet.header.id, ch->id);
  CuAssertIntEquals(tc, data.packet.header.seqNumber, ch->seqNumber);
  CuAssertIntEquals(tc, data.packet.header.ackNumber, ch->ackNumber);
  CuAssertIntEquals(tc, data.packet.header.checksum, ch->checksum);

  const s1 *packet = getLastPacket();

  CuAssertTrue(tc, getLastErrorCode() == 0);
  CuAssertTrue(tc, packet != 0);
  CuAssertIntEquals(tc, data.packet.packet.c1, packet->c1);
  CuAssertIntEquals(tc, data.packet.packet.c2, packet->c2);
  CuAssertIntEquals(tc, data.packet.packet.c3, packet->c3);

  free((void *)packet);
}
