#include "proto.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "parserTables.h"
#include "sanity.h"

static union {
  PacketHeader header;
  byte data[sizeof(PacketHeader)];
} parsedHeaderData = {};
PacketHeader lastHeaderData = {};

const byte* parsedPacketData;
byte* packetWriterHead;
const byte* lastPacketData;

int lastErrorCode = 0;

unsigned int totalPacketsSent = 0;
unsigned int totalPacketsReceived = 0;

size_t currentlyParsedPacketLength = 0;
size_t currentPacketSize = 0;
int currentlyParsedPacketId = 0;

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

void finishRecieiving()
{
  if(currentPacketSize - sizeof(PacketHeader) !=
     parsedHeaderData.header.length) {
    reportError(PERR_LENGTH_MISMATCH);
    return;
  }
#ifndef DISABLE_ACK_SEQ_CHECK
  if(parsedHeaderData.header.seqNumber != totalPacketsReceived) {
    reportError(PERR_SEQ_MISMATCH);
    return;
  }
  if(parsedHeaderData.header.ackNumber != totalPacketsSent) {
    reportError(PERR_ACK_MISMATCH);
    return;
  }
#endif

  totalPacketsReceived++;
  lastHeaderData = parsedHeaderData.header;
  lastPacketData = parsedPacketData;
  resetParsing();
}

enum {
  PSTATUS_DETECT = 0,
  PSTATUS_HEADER = 1,
  PSTATUS_STATICDATA = 2,
} parsingStatus = PSTATUS_DETECT;
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
      parseHeaderData(data);

      if(currentPacketSize >= sizeof(PacketHeader)) {
        currentlyParsedPacketLength = parsedHeaderData.header.length;
        currentlyParsedPacketId = parsedHeaderData.header.id;
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
     currentPacketSize - sizeof(PacketHeader) ==
         packetStaticSizes[currentlyParsedPacketId]) {
    finishRecieiving();
  }
}

size_t calculateVarintSize(long long data)
{
  size_t size = 0;
  long long mask = 0x7F;
  for(int i = 0; i <= sizeof(long long) * 8; i += 7) {
    if((mask & data) > 0) {
      size++;
    }
    mask <<= 7;
  }
  return size;
}

size_t calculateDynamicSize(const unsigned int id, const void* data)
{
  if(packetDynamicCount[id] == 0) {
    return 0;
  }

  size_t totalSize = 0;
  const void* dataPointer = data;
  dataPointer += packetStaticSizes[id];

  const enum datatype* dynamicFieldType =
      &parserTable[id][packetStaticCount[id]];

  for(int i = 0; i < packetDynamicCount[id]; i++) {
    switch(*dynamicFieldType++) {
      case TYPE_VARUINT: {
        totalSize += calculateVarintSize(*(long long*)dataPointer);
        dataPointer += sizeof(long long);
      } break;
      case TYPE_VARINT: {
        size_t size = calculateVarintSize(*(long long*)dataPointer);
        if(size < 2) {
          size = 2;
        }
        totalSize += size;
        dataPointer += sizeof(unsigned long long);
      } break;
      case TYPE_STRING:
        break;
      default:
        break;
    }
  }

  return totalSize;
}

size_t generateVaruint(unsigned long long varuint, byte* packetData)
{
  const char mask = 0b10000000;
  size_t size = 0;
  while(varuint > 0) {
    packetData[size++] = mask | (varuint & 0x7F);
    varuint >>= 7;
  }
  packetData[size - 1] ^= mask;
  return size;
}
size_t generateVarint(long long varuint, byte* packetData)
{
  byte signbit = 0x0;
  if(varuint < 0) {
    signbit = 0x80;
    varuint *= -1;
  }
  size_t size = generateVaruint(varuint, packetData);
  packetData[size - 2] &= 0x7F;
  packetData[size - 1] &= 0x7F;
  packetData[size - 1] |= signbit;
  return size;
}

byte* generatePacket(const unsigned int id, const void* data, size_t* size)
{
  if(id >= definedPacketCount) {
    reportError(PERR_UNKNOWN_ID);
    return 0;
  }
  size_t staticLength = packetStaticSizes[id];
  size_t dynamicLength = calculateDynamicSize(id, data);
  size_t totalSize = staticLength + dynamicLength + PACKET_HEADER_LENGTH;

  const PacketHeader header = {
      .length = staticLength + dynamicLength,
      .id = id,
      .seqNumber = totalPacketsSent,
      .ackNumber = totalPacketsReceived,
      .checksum = 0xCCCC,
  };

  byte* packetData = (byte*)malloc(totalSize * sizeof(byte));
  packetData[0] = MAGIC1;
  packetData[1] = MAGIC2;
  memcpy(packetData + 2, (void*)&header, sizeof(PacketHeader));
  memcpy(packetData + PACKET_HEADER_LENGTH, data, staticLength);

  if(packetDynamicCount[id] != 0) {
    const byte* dynamicData = data + staticLength;
    byte* packetDataWriter = packetData + PACKET_HEADER_LENGTH + staticLength;

    const enum datatype* dynamicFieldType =
        &parserTable[id][packetStaticCount[id]];

    for(int i = 0; i < packetDynamicCount[id]; i++) {
      switch(*dynamicFieldType++) {
        case TYPE_VARUINT: {
          const size_t varuintSize = generateVaruint(
              *(unsigned long long*)dynamicData, packetDataWriter);
          packetDataWriter += varuintSize;
          dynamicData += sizeof(unsigned long long);
        } break;
        case TYPE_VARINT: {
          const size_t varintSize =
              generateVarint(*(long long*)dynamicData, packetDataWriter);
          packetDataWriter += varintSize;
          dynamicData += sizeof(long long);
        } break;
        case TYPE_STRING:
          break;
        default:
          break;
      }
    }
  }

  *size = totalSize;
  totalPacketsSent++;
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
  parsingStatus = PSTATUS_DETECT;
  currentlyParsedPacketId = 0;
  currentlyParsedPacketLength = 0;
  currentPacketSize = 0;
  if(lastPacketData != parsedPacketData && parsedPacketData != 0) {
    free((void*)parsedPacketData);
  }
  parsedPacketData = 0;
}

int getLastErrorCode()
{
  return lastErrorCode;
}
