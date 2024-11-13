#define byte unsigned char
#define PACKET_HEADER_LENGTH 20
#define MAGIC1 0x57
#define MAGIC2 0x5f

enum type {
  TYPE_INT8    = 0,
  TYPE_INT16   = 1,
  TYPE_INT32   = 2,
  TYPE_INT64   = 3,
  TYPE_VARINT  = 4,
  TYPE_UINT8   = 5,
  TYPE_UINT16  = 6,
  TYPE_UINT32  = 7,
  TYPE_UINT64  = 8,
  TYPE_VARUINT = 9,
  TYPE_STRING  = 10
};

typedef volatile struct {
  unsigned int length;
  unsigned int id;
  unsigned int seqNumber;
  unsigned int ackNumber;
  unsigned int checksum;
} PacketHeader;

#ifndef PACKET_COUNT
#define PACKET_COUNT 0
#endif

extern unsigned char* parserTable[PACKET_COUNT];

void processByte(byte data);
PacketHeader* getLastHeader();
void resetParsing();

void* getLastPacket();
byte* generatePacket(unsigned int id, void* data);
