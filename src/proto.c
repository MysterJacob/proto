#include "proto.h"

#include <stddef.h>

static union {
  PacketHeader header;
  byte data[PACKET_HEADER_LENGTH];
} header;

/*
 * -2 -> First magic byte
 * -1 -> Second magic byte
 */
int currentHeaderSize = -2;

void processByte(byte data)
{
  if(currentHeaderSize >= PACKET_HEADER_LENGTH) {
    currentHeaderSize = -2;
  }
  switch(currentHeaderSize) {
    case -2:
      if(data == MAGIC1) {
        currentHeaderSize++;
      }
      break;
    case -1:
      if(data == MAGIC2) {
        currentHeaderSize++;
      }
      break;
    default:
      header.data[currentHeaderSize++] = data;
      break;
  }
}

PacketHeader *getLastHeader()
{
  if(currentHeaderSize == 20) {
    return &header.header;
  } else {
    return NULL;
  }
}

void resetParsing()
{
  currentHeaderSize = -2;
}
