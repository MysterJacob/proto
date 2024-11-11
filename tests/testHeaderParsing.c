#include <assert.h>
#include <stddef.h>

#include "CuTest.h"
#include "proto.h"

void TestDetectingSimpleHeader(CuTest *tc)
{
  resetParsing();
  union {
    PacketHeader ph;
    byte data[PACKET_HEADER_LENGTH];
  } testHeader = {
      .ph = {.length         = 0xA,
             .id             = 0xB,
             .seqNumber = 0xC,
             .ackNumber      = 0xD,
             .checksum       = 0xE}
  };
  for(int i = 0; i < 0xFF; i++) {
    processByte(0xFF);
    PacketHeader *ch = getLastHeader();
    CuAssertTrue(tc, ch == 0);
  }
  processByte(0x57);
  processByte(0x5f);
  for(int i = 0; i < PACKET_HEADER_LENGTH; i++) {
    processByte(testHeader.data[i]);
  }
  PacketHeader *header = getLastHeader();
  CuAssertTrue(tc, header != NULL);
  CuAssertIntEquals(tc, testHeader.ph.length, header->length);
  CuAssertIntEquals(tc, testHeader.ph.length, header->length);
  CuAssertIntEquals(tc, testHeader.ph.length, header->length);
  CuAssertIntEquals(tc, testHeader.ph.length, header->length);
}

void TestDetectingMultipleHeaders(CuTest* tc)
{
  resetParsing();
  union {
    PacketHeader ph;
    byte data[PACKET_HEADER_LENGTH];
  } testHeader = {
      .ph = {.length         = 0xA,
             .id             = 0xB,
             .seqNumber = 0,
             .ackNumber      = 0xD,
             .checksum       = 0xE}
  };
  for(int i = 0; i < 0xFFFF; i++)
  {
    testHeader.ph.seqNumber = i;
    testHeader.ph.ackNumber = 0xFFFF - i;
    processByte(0x57);
    processByte(0x5f);
    for(int i = 0; i < PACKET_HEADER_LENGTH; i++) {
      processByte(testHeader.data[i]);
    }
    PacketHeader *header = getLastHeader();
    CuAssertTrue(tc, header != NULL);
    CuAssertIntEquals(tc, testHeader.ph.seqNumber, header->seqNumber);
    CuAssertIntEquals(tc, testHeader.ph.ackNumber, header->ackNumber);
  }
}

CuSuite *CuGetSuite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, TestDetectingSimpleHeader);
  SUITE_ADD_TEST(suite, TestDetectingMultipleHeaders);
  return suite;
}
