#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "CuTest.h"
#include "proto.h"

void TestDetectingSimpleHeader(CuTest *tc)
{
  resetParsing();
  union {
    PacketHeader ph;
    byte data[PACKET_HEADER_LENGTH];
  } testHeader = {
      .ph = {.length    = 0x0,
             .id        = 0x0,
             .seqNumber = 0xC8,
             .ackNumber = 0xD1FFC1,
             .checksum  = 0xEEFF}
  };
  for(int i = 0; i < 0xFF; i++) {
    processByte(0xFF);
    const PacketHeader *ch = getLastHeader();
    CuAssertTrue(tc, ch == 0);
  }
  processByte(0x57);
  processByte(0x5f);
  for(int i = 0; i < PACKET_HEADER_LENGTH; i++) {
    processByte(testHeader.data[i]);
  }
  const PacketHeader *header = getLastHeader();
  CuAssertTrue(tc, header != 0);
  CuAssertIntEquals(tc, 0, getLastErrorCode());
  CuAssertIntEquals(tc, testHeader.ph.length, header->length);
  CuAssertIntEquals(tc, testHeader.ph.length, header->length);
  CuAssertIntEquals(tc, testHeader.ph.length, header->length);
  CuAssertIntEquals(tc, testHeader.ph.length, header->length);
}

void TestDetectingMultipleHeaders(CuTest *tc)
{
  resetParsing();
  union {
    PacketHeader ph;
    byte data[PACKET_HEADER_LENGTH];
  } testHeader = {
      .ph = {.length    = 0,
             .id        = 0x0,
             .seqNumber = 0,
             .ackNumber = 0,
             .checksum  = 0xE}
  };
  for(int i = 0; i < 0xFFFF; i++) {
    testHeader.ph.seqNumber = i;
    testHeader.ph.ackNumber = 0xFFFF - i;
    processByte(0x57);
    processByte(0x5f);
    for(int i = 0; i < PACKET_HEADER_LENGTH - 2; i++) {
      processByte(testHeader.data[i]);
    }
    const PacketHeader *header = getLastHeader();
    CuAssertTrue(tc, header != 0);
    CuAssertIntEquals(tc, 0, getLastErrorCode());
    CuAssertIntEquals(tc, testHeader.ph.seqNumber, header->seqNumber);
    CuAssertIntEquals(tc, testHeader.ph.ackNumber, header->ackNumber);
  }
}
