#include <stdio.h>

#include "CuTest.h"
#include "proto.h"

void TestCleanup()
{
  puts("\ncleaning up");
  resetParsing();
}
