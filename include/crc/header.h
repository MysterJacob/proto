#ifdef HEADER_CRC_ALGO
#define CRC_ALGO HEADER_CRC_ALGO

#include "crc/tables.h"
typedef CRC_DTYPE crcHeader_t;
crcHeader_t calculateHeaderCrc(uint8_t *buffer, size_t size);
void updateHeaderCrc(crcHeader_t *crc, uint8_t data);
crcHeader_t getHeaderCrc(crcHeader_t crc);
void resetHeaderCrc(crcHeader_t *crc);

#ifdef CRC_DEFINE_META
const struct {
  const crcHeader_t init;
  const crcHeader_t poly;
  const crcHeader_t xor ;
} crcHeader = {CRC_INIT, CRC_POLY, CRC_XOR};

const crcHeader_t crcHeaderTable[] = CRC_TABLE;
#endif

#undef CRC_DTYPE
#undef CRC_INIT
#undef CRC_POLY
#undef CRC_XOR
#undef CRC_ALGO
#undef CRC_TABLE

#ifdef CRC_REFIN
#define CRC_HEADER_REFIN
#undef CRC_REFIN
#endif

#ifdef CRC_REFOUT
#define CRC_HEADER_REFOUT
#undef CRC_REFOUT
#endif
#undef CRC_ALGO
#else
#error Header crc type not defined
#endif
