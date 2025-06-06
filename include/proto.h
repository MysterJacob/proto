#ifndef protoh
#define protoh
#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "datatypes.h"
#include "packets.h"

#define byte uint8_t
#define MAGIC_BYTES "\x57\x5F\xDE"
#define MAGIC_SIZE sizeof(MAGIC_BYTES) - 1
#define PACKET_HEADER_LENGTH (size_t)(sizeof(PacketHeader) + MAGIC_SIZE)

typedef enum {
  PERR_NOERR = 0,
  PERR_MALLOC_FAILED = 1,
  PERR_BUFFER_OVERFLOW = 2,
  PERR_UNKNOWN_ID = 3,
  PERR_LENGTH_MISMATCH = 4,
  PERR_ACK_MISMATCH = 5,
  PERR_SEQ_MISMATCH = 6,
  PERR_DATA_CRC_MISMATCH = 7,
  PERR_HDR_CRC_MISMATCH = 8
} errorCode;

typedef volatile struct __attribute((packed)) {
  uint16_t headerChecksum;
  uint16_t length;
  uint8_t id;
  uint16_t seqNumber;
  uint16_t ackNumber;
  uint16_t dataChecksum;
} PacketHeader;

void processByte(const byte data);

byte *generatePacket(const uint32_t id, const void *data, size_t *size);

int isNewPacketReady();
const size_t getPacketLength();
const uint32_t getPacket(PacketHeader *header, void *packetData);

typedef void (*PacketHandler)(const PacketHeader header, void *packetData);
void setPacketCallback(PacketHandler handler);

void resetParsing();
void hardResetParser();
int getLastErrorCode();

#endif
