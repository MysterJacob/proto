#define byte unsigned char

typedef struct {
    unsigned int length;
    unsigned int id;
    unsigned int sequenceNumber;
    unsigned int ackNumber;
    unsigned int checksum;
} PacketHeader;

void processByte(byte data);
PacketHeader* getCurrentHeader();
