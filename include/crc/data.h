#if defined(DATA_CRC_ALGO) && !defined(DISABLE_CRC_CHECK)
#define CRC_ALGO DATA_CRC_ALGO

#include "crc/tables.h"

typedef CRC_DTYPE crcData_t;
crcData_t calculateDataCrc(uint8_t *buffer, size_t size);
void updateDataCrc(crcData_t *crc, uint8_t data);
crcData_t getDataCrc(crcData_t crc);
void resetDataCrc(crcData_t *crc);

#ifdef CRC_DEFINE_META
const struct {
  const crcData_t init;
  const crcData_t poly;
  const crcData_t xor ;
} crcData = {CRC_INIT, CRC_POLY, CRC_XOR};
const crcData_t *table;
const crcData_t crcDataTable[] = CRC_TABLE;
#endif

#undef CRC_DTYPE
#undef CRC_INIT
#undef CRC_POLY
#undef CRC_XOR
#undef CRC_ALGO
#undef CRC_TABLE

#ifdef CRC_REFIN
#define CRC_DATA_REFIN
#undef CRC_REFIN
#endif

#ifdef CRC_REFOUT
#define CRC_DATA_REFOUT
#undef CRC_REFOUT
#endif
#undef CRC_ALGO

#endif
