#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "CuTest.h"
#include "packets.h"
#include "proto.h"

void TestDynamicVaruintGeneration(CuTest *tc)
{
  resetParsing();
  TestPacket4 tp = {
      .test1 = 0xEE, .varuint = 32 | (15 << 7) | (96 << 14), .test2 = 0xEE};
  size_t size;
  byte *data = generatePacket(4, (void *)&tp, &size);
  byte reference[] = {0xEE, 0xEE, 96 | 0b10000000, 15 | 0b1000000, 32};

  CuAssertIntEquals(tc, 0, getLastErrorCode());

  int chk = 0x0;
  data += 20;
  for(int i = 0; i < 15; i++) {
    chk |= reference[i] ^ *(data++);
  }
  CuAssertIntEquals(tc, 0, chk);
}
void TestDynamicStrGeneration(CuTest *tc)
{
  //   return;
  //   resetParsing();
  //   char *message = "Hello world!";
  //   TestPacket3 tp = {.test1 = 0xEE, .message = message, .test2 = 0xEE};
  //   unsigned int size;
  //   byte *data = generatePacket(3, (void *)&tp, &size);
  //   byte reference[] = {0xEE, 0xEE, 0xB , 'H', 'e', 'l', 'l', 'o', ' ',
  //                       'w',  'o', 'r', 'l', 'd', '!'};
  //
  //   CuAssertIntEquals(tc, 0, getLastErrorCode());
  //   int chk = 0x0;
  //   data += 20;
  //   for(int i = 0; i < 15; i++) {
  //     chk |= reference[i] ^ *(data++);
  //   }
  //   CuAssertIntEquals(tc, 0, chk);
}
