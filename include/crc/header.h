#ifdef HEADER_CRC_ALGO
#define CRC_ALGO HEADER_CRC_ALGO

#include "crc/tables.h"
typedef CRC_DTYPE crcHeader_t;

#ifndef HEADER_COMPILATION
crcHeader_t calculateHeaderCrc(uint8_t *buffer, size_t size);
// void updateHeaderCrc(crcHeader_t *crc, uint8_t data);
// crcHeader_t getHeaderCrc(crcHeader_t crc);
void resetHeaderCrc(crcHeader_t *crc);
#endif

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
