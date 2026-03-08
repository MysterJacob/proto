#ifdef __cplusplus
extern "C" {
#endif

#ifndef protoh
#define protoh
#ifndef HEADER_COMPILATION
#include <stddef.h>
#include <stdint.h>
#endif

#include "config.h"
#include "datatypes.h"
#include "packets.h"

typedef uint8_t byte;
#define PACKET_HEADER_LENGTH (size_t)(sizeof(PacketHeader) + PREAMBLE_SIZE)

#include "crc.h"
#if MAX_PACKET_SIZE < 256
typedef uint8_t packetLen_t;
#elif MAX_PACKET_SIZE < 65536
typedef uint16_t packetLen_t;
#elif MAX_PACKET_SIZE < 4294967296
typedef uint32_t packetLen_t;
#else
#error Packet too large!
#endif

#if PACKET_COUNT < 256
typedef uint8_t packetId_t;
#elif PACKET_COUNT < 65536
typedef uint16_t packetId_t;
#elif PACKET_COUNT < 4294967296
typedef uint32_t packetId_t;
#else
#error Too many packets!
#endif

typedef volatile struct __attribute((packed)) {
  packetId_t id;
  crcHeader_t headerChecksum;
#ifdef ACK_SEQ_CHECK
  uint8_t seqNumber;
  uint8_t ackNumber;
#endif

#ifdef DATA_CRC_CHECK
  crcData_t dataChecksum;
#endif
  packetLen_t length;
} PacketHeader;

typedef enum {
  PERR_NOERR = 0,
  PERR_MALLOC_FAILED = 1,
  PERR_BUFFER_OVERFLOW = 2,
  PERR_UNKNOWN_ID = 3,
  PERR_LENGTH_MISMATCH = 4,
  PERR_ACK_MISMATCH = 5,
  PERR_SEQ_MISMATCH = 6,
  PERR_DATA_CRC_MISMATCH = 7,
  PERR_HDR_CRC_MISMATCH = 8,
  PERR_UNEXPECTED_NULL = 9,
  PERR_PACKET_TOO_LARGE = 10,
} protoErrorCode;

void processByte(const byte data);

byte *generatePacket(const packetId_t id, const void *data, size_t *size);
byte *getLastGeneratedPacket();

int isNewPacketReady();
size_t getPacketLength();
size_t getPacketStructureSize();
uint32_t getPacket(PacketHeader *header, void *packetData);

typedef void (*PacketHandler)(const PacketHeader header, void *packetData);
void setPacketCallback(PacketHandler handler);

typedef void (*ErrorHandler)(const protoErrorCode errorCode);
void setErrorCallback(ErrorHandler handler);

void resetParsing();
void hardResetParser();
int getLastErrorCode();

#endif
#ifdef __cplusplus
}
#endif
