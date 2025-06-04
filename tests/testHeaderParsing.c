#include <stdio.h>
#include <assert.h>
#include <stddef.h>

#include "CuTest.h"
#include "crc.h"
#include "proto.h"

void TestDetectingSimpleHeader(CuTest *tc)
{
  puts("Testing Simple Header Detection");
  hardResetParser();
  union {
    PacketHeader ph;
    byte data[sizeof(PacketHeader)];
  } testHeader = {
      .ph = {.length = 0x0,
             .id = 0x0,
             .seqNumber = 0xC8,
             .ackNumber = 0xD1FFC1,
             .checksum = 0x0e68}
  };
  for(int i = 0; i < 0xFF; i++) {
    processByte(0x8C);
    CuAssertTrue(tc, isNewPacketReady() == 0);
  }

  for(size_t i =0;i<MAGIC_SIZE;i++) {
    processByte(MAGIC_BYTES[i]);
  }
  for(int i = 0; i < sizeof(PacketHeader); i++) {
    processByte(testHeader.data[i]);
  }
  CuAssertTrue(tc, isNewPacketReady() != 0);
  PacketHeader header;
  getPacket(&header, NULL);
  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertIntEquals(tc, testHeader.ph.length, header.length);
  CuAssertIntEquals(tc, testHeader.ph.id, header.id);
  CuAssertIntEquals(tc, testHeader.ph.seqNumber, header.seqNumber);
  CuAssertIntEquals(tc, testHeader.ph.ackNumber, header.ackNumber);
}

unsigned short calculateCrc(byte *buffer, size_t size)
{
  unsigned short crc = 0x0000;
  while(size--)
    crc = (crc >> 8) ^ crc16_table[(crc ^ *buffer++) & 0xff];
  return crc;
}

void TestDetectingMultipleHeaders(CuTest *tc)
{
  puts("Testing Multiple Header Detection");
  resetParsing();
  union {
    PacketHeader ph;
    byte data[sizeof(PacketHeader)];
  } testHeader = {
      .ph = {.length = 0,
             .id = 0x0,
             .seqNumber = 0,
             .ackNumber = 0,
             .checksum = 0x000}
  };

  for(int i = 0; i < 0xFFFF; i++) {
    testHeader.ph.seqNumber = i;
    testHeader.ph.ackNumber = 0xFFFF - i;
    testHeader.ph.checksum =
        calculateCrc(testHeader.data + 2, sizeof(PacketHeader) - 2);

    for(size_t i =0;i<MAGIC_SIZE;i++) {
      processByte(MAGIC_BYTES[i]);
    }
    for(int i = 0; i < sizeof(PacketHeader); i++) {
      processByte(testHeader.data[i]);
    }

    CuAssertTrue(tc, isNewPacketReady() != 0);
    PacketHeader header;
    getPacket(&header, NULL);
    CuAssertIntEquals(tc, 0, getLastErrorCode());
    CuAssertIntEquals(tc, testHeader.ph.seqNumber, header.seqNumber);
    CuAssertIntEquals(tc, testHeader.ph.ackNumber, header.ackNumber);
  }
}
