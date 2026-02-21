#include "types.h"

#if CRC_ALGO == CRC32_POSIX
#include "crc/crc32/posix.h"
#elif CRC_ALGO == CRC16_IBM3740
#include "crc/crc16/ibm3740.h"
#elif CRC_ALGO == CRC16_XMODEM
#include "crc/crc16/xmodem.h"
#elif CRC_ALGO == CRC8_TECH3250
#include "crc/crc8/tech3250.h"
#else
#error Unknown crc type!
#endif
