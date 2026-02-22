#include <malloc.h>
#include <stdio.h>

#include "lib/proto.h"

void packetHandler(const PacketHeader header, void *packetData)
{
  if(header.id == SimplePacket_ID) {
    puts("Received SimplePacket");
    SimplePacket *received = (SimplePacket *)packetData;
    printf("Field1: %d\nField2: %s\n", received->Field1, received->Field2);
  }
}
void errorHandler(const protoErrorCode errorCode)
{
  // ERROR!
  puts("Error while receiving");
}

int main()
{
  setErrorCallback(errorHandler);
  setPacketCallback(packetHandler);

  SimplePacket packet = {.Field1 = 70, .Field2 = "ABCD1234"};

  size_t outsize;
  byte *serialized = generatePacket(SimplePacket_ID, (void *)&packet, &outsize);

  for(int i = 0; i < outsize; i++) {
    processByte(*serialized++);
  }
}
