#ifndef protoh
#define protoh

#define byte unsigned char
#define PACKET_HEADER_LENGTH 20
#define MAGIC1 0x57
#define MAGIC2 0x5f

enum datatype {
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

enum errorCodes {
  PERR_MALLOC_FAILED = 1,
  PERR_UNKNOWN_ID    = 2,
};

static unsigned char typeSizes[11] = {1, 2, 4, 8, 0, 1, 2, 4, 8, 0, 0};

typedef volatile struct {
  unsigned int length;
  unsigned int id;
  unsigned int seqNumber;
  unsigned int ackNumber;
  unsigned int checksum;
} PacketHeader;

extern const byte* const parserTable[];

void loadPacketTable();
void processByte(byte data);
void resetParsing();

const PacketHeader* getLastHeader();
const void* getLastPacket();
byte* generatePacket(unsigned int id, void* data);

int getLastErrorCode();
#endif
