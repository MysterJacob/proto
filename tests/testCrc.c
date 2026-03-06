#include <assert.h>
#include <malloc.h>
#include <stddef.h>

#include "CuTest.h"
#include "proto.h"

void TestCrcCalculation(CuTest *tc)
{
  uint8_t buffer[] = {0x1c, 0x53, 0x29, 0x52, 0xe6, 0xaa, 0xb9, 0xb6, 0xa7,
                      0x54, 0xea, 0x93, 0x4e, 0x25, 0x26, 0x71, 0x85, 0x45,
                      0x07, 0x78, 0x6c, 0xa4, 0xf0, 0xed, 0x40, 0x97, 0x39,
                      0xeb, 0xa5, 0x81, 0xcd, 0xda, 0x57, 0x95, 0xdc, 0x69,
                      0x8f, 0xe7, 0xd4, 0xc9, 0xd8, 0x16, 0x79, 0x09};

  crcHeader_t headerCrc = calculateHeaderCrc(buffer, sizeof(buffer));

#if HEADER_CRC_ALGO == CRC32_POSIX
  CuAssertIntEquals(tc, 0xF28B401A, headerCrc);
#elif HEADER_CRC_ALGO == CRC16_IBM3740
  CuAssertIntEquals(tc, 0xD7A6, headerCrc);
#elif HEADER_CRC_ALGO == CRC16_XMODEM
  CuAssertIntEquals(tc, 0x4DDE, headerCrc);
#elif HEADER_CRC_ALGO == CRC8_TECH3250
  CuAssertIntEquals(tc, 0x7A, headerCrc);
#else
#error Unknown crc type!
#endif

  crcData_t dataCrc = calculateDataCrc(buffer, sizeof(buffer));
#if DATA_CRC_ALGO == CRC32_POSIX
  CuAssertIntEquals(tc, 0xF28B401A, dataCrc);
#elif DATA_CRC_ALGO == CRC16_IBM3740
  CuAssertIntEquals(tc, 0xD7A6, dataCrc);
#elif DATA_CRC_ALGO == CRC16_XMODEM
  CuAssertIntEquals(tc, 0x4DDE, dataCrc);
#elif DATA_CRC_ALGO == CRC8_TECH3250
  CuAssertIntEquals(tc, 0x7A, dataCrc);
#else
#error Unknown crc type!
#endif
}
