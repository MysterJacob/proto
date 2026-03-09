#include <stdint.h>

#include "config.h"

#define CRC_DEFINE_META
#include "crc.h"

#ifndef DISABLE_CRC_CHECK
crcData_t calculateDataCrc(uint8_t *buffer, size_t size)
{
  crcData_t crc = crcData.init;
  while(size--) {
    crc = (crc << crcData.dataShift) ^
          crcDataTable[((crc >> crcData.indexShift) ^ *buffer++) & 0xff];
  }

  return crc ^ crcData.xor;
}
void updateDataCrc(crcData_t *crc, uint8_t data)
{
  *crc = (*crc << crcData.dataShift) ^
         crcDataTable[((*crc >> crcData.indexShift) ^ data) & 0xff];
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

crcHeader_t calculateHeaderCrc(uint8_t *buffer, size_t size)
{
  crcHeader_t crc = crcHeader.init;
  while(size--) {
    crc = (crc << crcHeader.dataShift) ^
          crcHeaderTable[((crc >> crcHeader.indexShift) ^ *buffer++) & 0xff];
  }

  return crc ^ crcHeader.xor;
}
// void updateHeaderCrc(crcHeader_t *crc, uint8_t data)
// {
//   *crc = (*crc << crcHeader.dataShift) ^
//          crcHeaderTable[((*crc >> crcHeader.indexShift) ^ data) & 0xff];
// }
// crcHeader_t getHeaderCrc(crcHeader_t crc)
// {
//   return crc ^ crcHeader.xor;
// }
void resetHeaderCrc(crcHeader_t *crc)
{
  *crc = crcHeader.init;
}
