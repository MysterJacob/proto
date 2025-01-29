#ifndef protoh
#define protoh
#include <stddef.h>

#define byte unsigned char
#define PACKET_HEADER_LENGTH sizeof(PacketHeader) + 2
#define MAGIC1 0x57
#define MAGIC2 0x5f

enum datatype {
  TYPE_INT8 = 0,
  TYPE_INT16 = 1,
  TYPE_INT32 = 2,
  TYPE_INT64 = 3,
  TYPE_VARINT = 4,
  TYPE_UINT8 = 5,
  TYPE_UINT16 = 6,
  TYPE_UINT32 = 7,
  TYPE_UINT64 = 8,
  TYPE_VARUINT = 9,
  TYPE_STRING = 10
};

#define INT8 char
#define INT16 short
#define INT32 int
#define INT64 long int
#define VARINT long int
#define UINT8 unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned int
#define UINT64 unsigned long int
#define VARUINT unsigned long int
#define STRING char*

enum errorCodes {
  PERR_MALLOC_FAILED = 1,
  PERR_UNKNOWN_ID = 2,
  PERR_LENGTH_MISMATCH = 3,
  PERR_ACK_MISMATCH = 4,
  PERR_SEQ_MISMATCH = 5,
};

typedef volatile struct __attribute((packed)) {
  unsigned int length;
  unsigned int id;
  unsigned int seqNumber;
  unsigned int ackNumber;
  unsigned short checksum;
} PacketHeader;

extern const enum datatype* const parserTable[];

void processByte(byte data);

byte* generatePacket(const unsigned int id, const void* data, size_t* size);

const PacketHeader* getLastHeader();
const void* getLastPacket();

void resetParsing();
int getLastErrorCode();
#endif
