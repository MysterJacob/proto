#ifndef protoh
#define protoh
#include <stddef.h>

#define byte unsigned char
#define PACKET_HEADER_LENGTH (size_t)(sizeof(PacketHeader) + 2)
#define MAGIC1 0x57
#define MAGIC2 0x5f

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

#define INT8 char
#define INT16 short
#define INT32 int
#define INT64 long int
#define VARINT long long int
#define UINT8 unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned int
#define UINT64 unsigned long int
#define VARUINT unsigned long long int
#define STRING char*

typedef enum {
  PERR_MALLOC_FAILED = 1,
  PERR_BUFFER_OVERFLOW = 2,
  PERR_UNKNOWN_ID = 3,
  PERR_LENGTH_MISMATCH = 4,
  PERR_ACK_MISMATCH = 5,
  PERR_SEQ_MISMATCH = 6,
} errorCode;

typedef volatile struct __attribute((packed)) {
  unsigned int length;
  unsigned int id;
  unsigned int seqNumber;
  unsigned int ackNumber;
  unsigned short checksum;
} PacketHeader;

extern const datatype* const parserTable[];

void processByte(const byte data);

byte* generatePacket(const unsigned int id, const void* data, size_t* size);

const PacketHeader* getLastHeader();
const void* getLastPacket();

void resetParsing();
void hardResetParser();
int getLastErrorCode();
#endif
