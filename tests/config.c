#include "proto.h"

const byte p0s[]            = {0xFF};
const byte p1s[]            = {TYPE_INT8, TYPE_INT16, TYPE_INT32, 0xFF};
const byte *const parserTable[] = {p0s, p1s, 0};
