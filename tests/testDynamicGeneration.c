#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "CuTest.h"
#include "packets.h"
#include "proto.h"

void TestDynamicVaruintGeneration(CuTest *tc)
{
  return;
  resetParsing();
  TestPacket4 tp = {.test1 = 0xEE, .varuint = 0x61BEA1, .test2 = 0xEE};
  unsigned int size;
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
