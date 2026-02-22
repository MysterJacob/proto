#include <malloc.h>
#include <stdio.h>

#include "lib/proto.h"

int main()
{
  SimplePacket packet = {.Field1 = 70, .Field2 = "ABCD1234"};

  size_t outsize;
  byte *serialized = generatePacket(SimplePacket_ID, (void *)&packet, &outsize);

  for(int i = 0; i < outsize; i++) {
    // processByte requires received bytes of serialized data as input
    processByte(*serialized++);
  }

  if(getLastErrorCode() != PERR_NOERR) {
    // ERROR!
    puts("Error while receiving");
  }
  if(isNewPacketReady()) {
    // New packet is ready
    PacketHeader header;
    void *receivedData = malloc(getPacketLength());
    const int id = getPacket(&header, receivedData);

    if(id == SimplePacket_ID) {
      puts("Received SimplePacket");
      SimplePacket *received = (SimplePacket *)receivedData;
      printf("Field1: %d\nField2: %s\n", received->Field1, received->Field2);
    }

    free(receivedData);
  }
}
