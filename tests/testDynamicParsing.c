#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "CuTest.h"
#include "packets.h"
#include "proto.h"

void TestDynamicVaruintParsing(CuTest *tc)
{
  resetParsing();
  for(int i = 0; i < 255; i++) {
    TestPacket4 tp = {.test1 = 0xFF - i, .varuint = 65530 * i, .test2 = i};

    size_t size;
    byte *data = generatePacket(4, (void *)&tp, &size);
    CuAssertIntEquals(tc, 0, getLastErrorCode());

    for(int j = 0; j < size; j++) {
      processByte(*data++);
    }

    CuAssertIntEquals(tc, 0, getLastErrorCode());
    TestPacket4 recv = *(TestPacket4 *)getLastPacket();
    CuAssertIntEquals(tc, 0xFF - i, recv.test1);
    CuAssertIntEquals(tc, 65530 * i, recv.varuint);
  }
}
// void TestDynamicVarintGeneration(CuTest *tc)
// {
//   resetParsing();
//   TestPacket5 tp = {.test1 = 0xEE, .varint = -35172, .test2 = 0xEE};
//   size_t size;
//   byte *data = generatePacket(5, (void *)&tp, &size);
//   byte reference[] = {0xEE, 0xEE, 0xe4, 0x12, 0x82};
//
//   CuAssertIntEquals(tc, 0, getLastErrorCode());
//   CuAssertIntEquals(tc, PACKET_HEADER_LENGTH + 5, size);
//
//   int chk = 0x0;
//   data += PACKET_HEADER_LENGTH;
//   for(int i = 0; i < size - PACKET_HEADER_LENGTH; i++) {
//     chk |= reference[i] ^ *(data++);
//   }
//   CuAssertIntEquals(tc, 0, chk);
// }
// void TestDynamicStrGeneration(CuTest *tc)
// {
//   resetParsing();
//   char *message = "Hello world!";
//   TestPacket3 tp = {.test1 = 0xEE, .message = message, .test2 = 0xEE};
//   size_t size;
//   byte *data = generatePacket(3, (void *)&tp, &size);
//   byte reference[] = {0xEE, 0xEE, 0xC, 'H', 'e', 'l', 'l', 'o',
//                       ' ',  'w',  'o', 'r', 'l', 'd', '!'};
//
//   CuAssertIntEquals(tc, 0, getLastErrorCode());
//   CuAssertIntEquals(tc, PACKET_HEADER_LENGTH + 15, size);
//
//   int chk = 0x0;
//   data += PACKET_HEADER_LENGTH;
//   for(int i = 0; i < size - PACKET_HEADER_LENGTH; i++) {
//     chk |= reference[i] ^ *(data++);
//   }
//   CuAssertIntEquals(tc, 0, chk);
// }
