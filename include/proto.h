#ifndef protoh
#define protoh
#include <stddef.h>
#include <stdint.h>

#define byte uint8_t
#define MAGIC_BYTES "\x57\x5F\xDE"
#define MAGIC_SIZE sizeof(MAGIC_BYTES) - 1
#define PACKET_HEADER_LENGTH (size_t)(sizeof(PacketHeader) + MAGIC_SIZE)

typedef enum {
  TYPE_INT8 = 1,
  TYPE_INT16 = 2,
  TYPE_INT32 = 3,
  TYPE_INT64 = 4,
  TYPE_VARINT = 5,
  TYPE_UINT8 = 6,
  TYPE_UINT16 = 7,
  TYPE_UINT32 = 8,
  TYPE_UINT64 = 9,
  TYPE_VARUINT = 11,
  TYPE_STRING = 12
} datatype;

#define INT8 int8_t
#define INT16 int16_t
#define INT32 int32_t
#define INT64 int64_t
#define VARINT int64_t
#define UINT8 uint8_t
#define UINT16 uint16_t
#define UINT32 uint32_t
#define UINT64 uint64_t
#define VARUINT uint64_t
#define STRING char*

typedef enum {
  PERR_NOERR = 0,
  PERR_MALLOC_FAILED = 1,
  PERR_BUFFER_OVERFLOW = 2,
  PERR_UNKNOWN_ID = 3,
  PERR_LENGTH_MISMATCH = 4,
  PERR_ACK_MISMATCH = 5,
  PERR_SEQ_MISMATCH = 6,
  PERR_CRC_MISMATCH = 7
} errorCode;

typedef volatile struct __attribute((packed)) {
  uint16_t checksum;
  uint32_t length;
  uint32_t id;
  uint32_t seqNumber;
  uint32_t ackNumber;
} PacketHeader;

void processByte(const byte data);

byte* generatePacket(const unsigned int id, const void* data, size_t* size);

int isNewPacketReady();
const size_t getPacketLength();
const uint32_t getPacket(PacketHeader* header, void* packetData);

typedef void (*PacketHandler)(const PacketHeader header, void* packetData);
void setPacketCallback(PacketHandler handler);

void resetParsing();
void hardResetParser();
int getLastErrorCode();

#endif
