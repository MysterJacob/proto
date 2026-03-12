#if defined(DATA_CRC_ALGO) && !defined(DISABLE_CRC_CHECK)
#define CRC_ALGO DATA_CRC_ALGO

#include "crc/tables.h"
typedef CRC_DTYPE crcData_t;

#ifndef HEADER_COMPILATION
crcData_t calculateDataCrc(uint8_t *buffer, size_t size);
void updateDataCrc(crcData_t *crc, uint8_t data);
crcData_t getDataCrc(crcData_t crc);
void resetDataCrc(crcData_t *crc);
#endif

#ifdef CRC_DEFINE_META
const struct {
  const crcData_t init;
  const crcData_t poly;
  const crcData_t xor ;
  const uint8_t indexShift;
  const uint8_t dataShift;
} crcData = {CRC_INIT, CRC_POLY, CRC_XOR, CRC_INDEX_SHIFT, CRC_DATA_SHIFT};

const crcData_t crcDataTable[] = CRC_TABLE;
#endif

#if CRC_DATA_SHIFT == 0
#define CRC_DATA_SHIFT_SKIP
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

#endif
