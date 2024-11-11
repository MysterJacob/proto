#define byte unsigned char
#define PACKET_HEADER_LENGTH 20
#define MAGIC1 0x57
#define MAGIC2 0x5f

typedef volatile struct {
  unsigned int length;
  unsigned int id;
  unsigned int seqNumber;
  unsigned int ackNumber;
  unsigned int checksum;
} PacketHeader;

void processByte(byte data);
PacketHeader *getLastHeader();
void resetParsing();

void* getLastPacket();
byte* generatePacket(unsigned int id, void* data);
