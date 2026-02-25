#include <stdio.h>

#include "CuTest.h"
#include "proto.h"

void TestCleanup(CuTest *tc)
{
  puts("\ncleaning up");
  resetParsing();
}
