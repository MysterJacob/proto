#include "proto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int definedPacketCount        = 0;
static unsigned int* packetStaticSizes = 0;

static union {
  PacketHeader header;
  byte data[PACKET_HEADER_LENGTH];
} parsedHeaderData          = {};
PacketHeader lastHeaderData = {};

const byte* parsedPacketData;
byte* packetWriterHead;
const byte* lastPacketData;

int lastErrorCode = 0;

unsigned int totalPacketsReceived = 0;
unsigned int totalPacketsSent     = 0;

int currentlyParsedPacketLength = 0;
int currentlyParsedPacketId     = 0;
int currentPacketSize           = 0;

void reportError(int code)
{
  resetParsing();
  lastErrorCode = code;
}

void loadPacketTable()
{
  definedPacketCount = 0;
  while(parserTable[definedPacketCount]) {
    definedPacketCount++;
  }

  if(packetStaticSizes) {
    free(packetStaticSizes);
  }

  packetStaticSizes = malloc(definedPacketCount * sizeof(int));
  if(packetStaticSizes == 0) {
    reportError(PERR_MALLOC_FAILED);
    return;
  }

  for(int i = 0; i < definedPacketCount; i++) {
    const byte* ptr      = (const byte*)parserTable[i];
    packetStaticSizes[i] = PACKET_HEADER_LENGTH;
    while(*ptr != 0xFF) {
      int fieldType = *ptr;
      packetStaticSizes[i] += typeSizes[fieldType];
      ptr += 1;
    }
  }
}

void allocateMemoryForPacketData()
{
  if(currentlyParsedPacketLength == 0) {
    return;
  }
  parsedPacketData = malloc(currentlyParsedPacketLength);
  if(parsedPacketData == 0) {
    reportError(PERR_MALLOC_FAILED);
    return;
  }
  packetWriterHead = (byte*)parsedPacketData;
}

void parseHeaderData(byte data)
{
  parsedHeaderData.data[currentPacketSize++] = data;
}

void parsePacketData(byte data)
{
  *(packetWriterHead++) = data;
  currentPacketSize++;
}

void finishReceiving()
{
  if(currentPacketSize - sizeof(PacketHeader) !=
     parsedHeaderData.header.length) {
    reportError(PERR_LENGTH_MISMATCH);
    return;
  }
  totalPacketsReceived++;
  lastHeaderData = parsedHeaderData.header;
  lastPacketData = parsedPacketData;
  resetParsing();
}

enum {
  PSTATUS_DETECT     = 0,
  PSTATUS_HEADER     = 1,
  PSTATUS_STATICDATA = 2,
} parsingStatus           = PSTATUS_DETECT;
unsigned short last2Bytes = 0x0;
void processByte(byte data)
{
  switch(parsingStatus) {
    case PSTATUS_DETECT:
      last2Bytes = (last2Bytes << 8) & 0xFFFF;
      last2Bytes |= data;
      if((last2Bytes & 0xFF) == MAGIC2 && (last2Bytes >> 8) == MAGIC1) {
        parsingStatus = PSTATUS_HEADER;
      }
      break;
    case PSTATUS_HEADER:
      // FIXME
      parseHeaderData(data);

      if(currentPacketSize >= sizeof(PacketHeader)) {
        currentlyParsedPacketLength = parsedHeaderData.header.length;
        currentlyParsedPacketId     = parsedHeaderData.header.id;
        if(currentlyParsedPacketId >= definedPacketCount) {
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
  // TODO
  // For now static, later on dynamic sizes will be included
  if(parsingStatus > PSTATUS_HEADER &&
     currentPacketSize == packetStaticSizes[currentlyParsedPacketId] - 2) {
    finishReceiving();
  }
}

byte* generatePacket(unsigned int id, void* data, unsigned int* size)
{
  if(id >= definedPacketCount) {
    reportError(PERR_UNKNOWN_ID);
    return 0;
  }
  unsigned int staticLength  = packetStaticSizes[id] - PACKET_HEADER_LENGTH;
  unsigned int dynamicLength = 0;
  unsigned int totalSize = staticLength + dynamicLength + PACKET_HEADER_LENGTH;

  const PacketHeader header = {
      .length    = staticLength + dynamicLength,
      .id        = id,
      .seqNumber = totalPacketsSent,
      .ackNumber = totalPacketsReceived,
      .checksum  = 0xCCCC,
  };

  byte* packetData = (byte*)malloc(totalSize * sizeof(byte));
  packetData[0]    = MAGIC1;
  packetData[1]    = MAGIC2;
  memcpy(packetData + 2, (void*)&header, PACKET_HEADER_LENGTH - 2);
  memcpy(packetData + PACKET_HEADER_LENGTH, data, staticLength);

  // TODO
  // Dynamic data

  *size = totalSize;
  totalPacketsSent += 1;
  return packetData;
}

const PacketHeader* getLastHeader()
{
  if(totalPacketsReceived == 0) {
    return 0;
  }
  return &lastHeaderData;
}

const void* getLastPacket()
{
  return lastPacketData;
}

void resetParsing()
{
  parsingStatus               = PSTATUS_DETECT;
  currentlyParsedPacketId     = 0;
  currentlyParsedPacketLength = 0;
  currentPacketSize           = 0;
  if(lastPacketData != parsedPacketData && parsedPacketData != 0) {
    free((void*)parsedPacketData);
  }
  parsedPacketData = 0;
}

int getLastErrorCode()
{
  return lastErrorCode;
}
