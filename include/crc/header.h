#ifdef HEADER_CRC_ALGO
#define CRC_ALGO HEADER_CRC_ALGO

#include "crc/tables.h"
typedef CRC_DTYPE crcHeader_t;

#ifndef HEADER_COMPILATION
#if HEADER_CRC_ALGO != DATA_CRC_ALGO

crcHeader_t calculateHeaderCrc(uint8_t *buffer, size_t size);
void resetHeaderCrc(crcHeader_t *crc);

#ifdef CRC_DEFINE_META
const struct {
  const crcHeader_t init;
  const crcHeader_t poly;
  const crcHeader_t xor ;
  const uint8_t indexShift;
  const uint8_t dataShift;
} crcHeader = {CRC_INIT, CRC_POLY, CRC_XOR, CRC_INDEX_SHIFT, CRC_DATA_SHIFT};

const crcHeader_t crcHeaderTable[] = CRC_TABLE;
#endif

#else
#define calculateHeaderCrc calculateDataCrc
#define resetHeaderCrc resetDataCrc
#endif

#endif


#if CRC_DATA_SHIFT == 0
#define CRC_HEADER_SHIFT_SKIP
#endif

#undef CRC_DTYPE
#undef CRC_INIT
#undef CRC_POLY
#undef CRC_XOR
#undef CRC_ALGO
#undef CRC_TABLE
#undef CRC_DATA_SHIFT
#undef CRC_INDEX_SHIFT
#undef CRC_ALGO

#else
#error Header crc type not defined
#endif
