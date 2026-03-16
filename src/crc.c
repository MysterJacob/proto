#include <stdint.h>

#include "config.h"

#define CRC_DEFINE_META
#include "crc.h"

#if defined(DATA_CRC_CHECK)
crcData_t calculateDataCrc(uint8_t *buffer, size_t size)
{
  crcData_t crc = crcData.init;
  while(size--) {
#ifdef CRC_DATA_SHIFT_SKIP
    crc = crcDataTable[crc ^ *buffer++];
#else
    crc = (crc << crcData.dataShift) ^
          crcDataTable[((crc >> crcData.indexShift) ^ *buffer++) & 0xff];
#endif
  }

  return crc ^ crcData.xor;
}
void updateDataCrc(crcData_t *crc, uint8_t data)
{
#ifdef CRC_DATA_SHIFT_SKIP
  *crc = crcDataTable[*crc ^ data];
#else
  *crc = (*crc << crcData.dataShift) ^
         crcDataTable[((*crc >> crcData.indexShift) ^ data) & 0xff];
#endif
}
crcData_t getDataCrc(crcData_t crc)
{
  return crc ^ crcData.xor;
}
void resetDataCrc(crcData_t *crc)
{
  *crc = crcData.init;
}
#endif
#if HEADER_CRC_ALGO != DATA_CRC_ALGO || defined(JOIN_DATA_CRC)
crcHeader_t calculateHeaderCrc(uint8_t *buffer, size_t size)
{
  crcHeader_t crc = crcHeader.init;
  while(size--) {
#ifdef CRC_HEADER_SHIFT_SKIP
    crc = crcHeaderTable[crc ^ *buffer++];
#else
    crc = (crc << crcHeader.dataShift) ^
          crcHeaderTable[((crc >> crcHeader.indexShift) ^ *buffer++) & 0xff];
#endif
  }

  return crc ^ crcHeader.xor;
}
#if defined(JOIN_DATA_CRC)
void updateHeaderCrc(crcHeader_t *crc, uint8_t data)
{
  *crc = (*crc << crcHeader.dataShift) ^
         crcHeaderTable[((*crc >> crcHeader.indexShift) ^ data) & 0xff];
}
crcHeader_t getHeaderCrc(crcHeader_t crc)
{
  return crc ^ crcHeader.xor;
}
#endif
void resetHeaderCrc(crcHeader_t *crc)
{
  *crc = crcHeader.init;
}
#endif
