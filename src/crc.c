#include <stdint.h>

#include "config.h"

#define CRC_DEFINE_META
#include "crc.h"
#ifndef DISABLE_CRC_CHECK

inline uint8_t reflect(uint8_t b)
{
  uint8_t r = 0;
  for(int i = 0; i < 8; ++i) {
    r |= ((b >> i) & 1) << (7 - i);
  }
  return r;
}

crcData_t calculateDataCrc(uint8_t *buffer, size_t size)
{
  uint16_t crc = crcData.init;
  while(size--) {
#ifdef CRC_DATA_REFIN
    crc = (crc << 8) ^ crcDataTable[((crc >> 8) ^ reflect(*buffer++)) & 0xff];
#else
    crc = (crc << 8) ^ crcDataTable[((crc >> 8) ^ *buffer++) & 0xff];
#endif
  }

#ifdef CRC_DATA_REFOUT
  return reflect(crc ^ crcData.xor);
#else
  return crc ^ crcData.xor;
#endif
}
void updateDataCrc(crcData_t *crc, uint8_t data)
{
#ifdef CRC_DATA_REFIN
  *crc = (*crc << 8) ^ crcDataTable[((*crc >> 8) ^ reflect(data)) & 0xff];
#else
  *crc = (*crc << 8) ^ crcDataTable[((*crc >> 8) ^ data) & 0xff];
#endif
}
crcData_t getDataCrc(crcData_t crc)
{
#ifdef CRC_DATA_REFOUT
  return reflect(crc ^ crcData.xor);
#else
  return crc ^ crcData.xor;
#endif
}
void resetDataCrc(crcData_t *crc)
{
  *crc = crcData.init;
}
#endif

crcHeader_t calculateHeaderCrc(uint8_t *buffer, size_t size)
{
  uint16_t crc = crcHeader.init;
  while(size--) {
#ifdef CRC_HEADER_REFIN
    crc = (crc << 8) ^ crcHeaderTable[((crc >> 8) ^ reflect(*buffer++)) & 0xff];
#else
    crc = (crc << 8) ^ crcHeaderTable[((crc >> 8) ^ *buffer++) & 0xff];
#endif
  }

#ifdef CRC_HEADER_REFOUT
  return reflect(crc ^ crcHeader.xor);
#else
  return crc ^ crcHeader.xor;
#endif
}
void updateHeaderCrc(crcHeader_t *crc, uint8_t data)
{
#ifdef CRC_HEADER_REFIN
  *crc = (*crc << 8) ^ crcHeaderTable[((*crc >> 8) ^ reflect(data)) & 0xff];
#else
  *crc = (*crc << 8) ^ crcHeaderTable[((*crc >> 8) ^ data) & 0xff];
#endif
}
crcHeader_t getHeaderCrc(crcHeader_t crc)
{
#ifdef CRC_HEADER_REFOUT
  return reflect(crc ^ crcHeader.xor);
#else
  return crc ^ crcHeader.xor;
#endif
}
void resetHeaderCrc(crcHeader_t *crc)
{
  *crc = crcHeader.init;
}
