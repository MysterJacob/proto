#include "proto.h"

#include <stdio.h>
#include <stdlib.h>

unsigned int packetCount               = 0;
static unsigned int* packetStaticSizes = 0;

static union {
  PacketHeader header;
  byte data[PACKET_HEADER_LENGTH];
} headerData            = {};
PacketHeader lastHeader = {};

const byte* packetData;
byte* packetWriterHead;
const byte* lastPacketData;

int lastErrorCode = 0;

unsigned int packetsReceived = 0;
unsigned int packetsSend     = 0;

/*
 * -2 -> First magic byte
 * -1 -> Second magic byte
 */
int currentlyParsedPacketLength = 0;
int currentlyParsedPacketId     = 0;
int currentPacketSize           = 0;

void reportError(int code)
{
  resetParsing();
  lastErrorCode = code;
}

void allocateMemoryForPacketData()
{
  if(currentlyParsedPacketLength == 0) {
    return;
  }
  packetData = malloc(currentlyParsedPacketLength);
  if(packetData == 0) {
    reportError(PERR_MALLOC_FAILED);
    return;
  }
  packetWriterHead = (byte*)packetData;
}

void parseHeaderData(byte data)
{
  headerData.data[currentPacketSize++] = data;
}

void parsePacketData(byte data)
{
  *(packetWriterHead++) = data;
  currentPacketSize++;
}

void receivePacket()
{
  if(currentPacketSize - PACKET_HEADER_LENGTH != headerData.header.length) {
    reportError(PERR_LENGTH_MISMATCH);
    return;
  }
  packetsReceived++;
  lastHeader     = headerData.header;
  lastPacketData = packetData;
  resetParsing();
}

enum {
  PSTATUS_DETECT     = 0,
  PSTATUS_HEADER     = 1,
  PSTATUS_STATICDATA = 2,
} parsingStatus      = PSTATUS_DETECT;
unsigned short magic = 0x0;
void processByte(byte data)
{
  switch(parsingStatus) {
    case PSTATUS_DETECT:
      magic = (magic << 8) & 0xFFFF;
      magic |= data;
      if((magic & 0xFF) == MAGIC2 && (magic >> 8) == MAGIC1) {
        parsingStatus = PSTATUS_HEADER;
      }
      break;
    case PSTATUS_HEADER:
      parseHeaderData(data);

      if(currentPacketSize >= PACKET_HEADER_LENGTH) {
        currentlyParsedPacketLength = headerData.header.length;
        currentlyParsedPacketId     = headerData.header.id;
        if(currentlyParsedPacketId >= packetCount) {
          reportError(PERR_UNKNOWN_ID);
          return;
        }

        allocateMemoryForPacketData();

        parsingStatus = PSTATUS_STATICDATA;
      }
      break;
    case PSTATUS_STATICDATA:
      parsePacketData(data);
      break;
  }
  // For now static, later on dynamic sizes will be included
  if(parsingStatus > PSTATUS_HEADER &&
     currentPacketSize == packetStaticSizes[currentlyParsedPacketId]) {
    receivePacket();
  }
}

void resetParsing()
{
  parsingStatus               = PSTATUS_DETECT;
  currentlyParsedPacketId     = 0;
  currentlyParsedPacketLength = 0;
  currentPacketSize           = 0;
  if(lastPacketData != packetData && packetData != 0) {
    free((void*)packetData);
  }
  packetData = 0;
}

void loadPacketTable()
{
  packetCount = 0;
  while(parserTable[packetCount]) {
    packetCount++;
  }

  if(packetStaticSizes) {
    free(packetStaticSizes);
  }

  packetStaticSizes = malloc(packetCount * sizeof(int));
  if(packetStaticSizes == 0) {
    reportError(PERR_MALLOC_FAILED);
    return;
  }

  for(int i = 0; i < packetCount; i++) {
    const byte* ptr      = (const byte*)parserTable[i];
    packetStaticSizes[i] = PACKET_HEADER_LENGTH;
    while(*ptr != 0xFF) {
      int fieldType = *ptr;
      packetStaticSizes[i] += typeSizes[fieldType];
      ptr += 1;
    }
  }
}

const PacketHeader* getLastHeader()
{
  if(packetsReceived == 0) {
    return 0;
  }
  return &lastHeader;
}

const void* getLastPacket()
{
  return lastPacketData;
}

int getLastErrorCode()
{
  return lastErrorCode;
}
