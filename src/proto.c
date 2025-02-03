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
size_t currentHeaderSize = 0;
int currentlyParsedPacketId = 0;

enum errorCode reportError(enum errorCode code)
{
  resetParsing();
  lastErrorCode = code;
  return code;
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
  parsedHeaderData.data[currentHeaderSize++] = data;
}

enum errorCode parsePacketData(byte data)
{
  if(currentPacketSize >= currentlyParsedPacketLength) {
    return reportError(PERR_BUFFER_OVERFLOW);
  }
  *(packetWriterHead++) = data;
  currentPacketSize++;
  return 0;
}

void finishRecieiving()
{
  if(currentPacketSize != parsedHeaderData.header.length) {
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
  PSTATUS_DYNAMICDATA = 3,
} parsingStatus = PSTATUS_DETECT;

unsigned short last2Bytes = 0x0;
void finishHeaderParsing()
{
  currentlyParsedPacketLength = parsedHeaderData.header.length;
  currentlyParsedPacketId = parsedHeaderData.header.id;

  if(currentlyParsedPacketId >= definedPacketCount) {
    reportError(PERR_UNKNOWN_ID);
    return;
  }

  if(currentlyParsedPacketLength == 0) {
    finishRecieiving();
  } else {
    allocateMemoryForPacketData();
    if(packetStaticSizes[currentlyParsedPacketId] == 0) {
      parsingStatus = PSTATUS_DYNAMICDATA;
    } else {
      parsingStatus = PSTATUS_STATICDATA;
    }
  }
}

void finishStaticDataParsing()
{
  if(packetDynamicCount[currentlyParsedPacketId] == 0) {
    finishRecieiving();
  } else {
    parsingStatus = PSTATUS_DYNAMICDATA;
  }
}

void processByte(const byte data)
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

      if(currentHeaderSize == sizeof(PacketHeader)) {
        finishHeaderParsing();
      }
      break;
    case PSTATUS_STATICDATA:
      if(parsePacketData(data) != 0) {
        return;
      }

      if(currentPacketSize == packetStaticSizes[currentlyParsedPacketId]) {
        finishStaticDataParsing();
      }
      break;
    case PSTATUS_DYNAMICDATA:
      finishRecieiving();
      break;
  }
}

size_t calculateVaruintSize(const unsigned long long data)
{
  size_t size = 0;
  long long mask = 0x7F;
  int i = 0;
  do {
    size++;
    mask <<= 7;
  } while((mask & data) > 0 && i <= sizeof(long long) * 8);
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
        totalSize += calculateVaruintSize(*(unsigned long long*)dataPointer);

        dataPointer += sizeof(long long);
      } break;
      case TYPE_VARINT: {
        long long value = *(long long*)dataPointer;
        if(value < 0) {
          value *= -1;
        }
        size_t size = calculateVaruintSize(value);
        if(size < 2) {
          size = 2;
        }
        totalSize += size;
        dataPointer += sizeof(unsigned long long);
      } break;
      case TYPE_STRING: {
        size_t length = strlen(*(char**)dataPointer);
        size_t size = length + calculateVaruintSize(length);
        totalSize += size;
        dataPointer += 1 + length + sizeof(unsigned long long);
      } break;
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
  size_t offset = size;
  if(offset < 2) {
    offset = 2;
  }
  packetData[offset - 2] &= 0x7F;
  packetData[offset - 1] &= 0x7F;
  packetData[offset - 1] |= signbit;
  return size;
}

size_t generateString(const char* str, byte* packetData, size_t* strLen)
{
  size_t len = strlen(str);
  *strLen = len;
  size_t varuintSize = generateVaruint(len, packetData);
  packetData += varuintSize;
  memcpy(packetData, str, len);
  return len + varuintSize;
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

  byte* packetData = (byte*)malloc(totalSize * sizeof(byte));

  packetData[0] = MAGIC1;
  packetData[1] = MAGIC2;
  memcpy(packetData + PACKET_HEADER_LENGTH, data, staticLength);

  if(packetDynamicCount[id] != 0) {
    const byte* dynamicData = data + staticLength;
    byte* dynamicDataWriter = packetData + PACKET_HEADER_LENGTH + staticLength;

    const enum datatype* dynamicFieldType =
        &parserTable[id][packetStaticCount[id]];

    for(int i = 0; i < packetDynamicCount[id]; i++) {
      switch(*dynamicFieldType++) {
        case TYPE_VARUINT: {
          const size_t varuintSize = generateVaruint(
              *(unsigned long long*)dynamicData, dynamicDataWriter);
          dynamicDataWriter += varuintSize;
          dynamicData += sizeof(unsigned long long);
        } break;
        case TYPE_VARINT: {
          const size_t varintSize =
              generateVarint(*(long long*)dynamicData, dynamicDataWriter);
          dynamicDataWriter += varintSize;
          dynamicData += sizeof(long long);
        } break;
        case TYPE_STRING: {
          size_t strLen;
          const size_t stringSize =
              generateString(*(char**)dynamicData, dynamicDataWriter, &strLen);
          dynamicDataWriter += stringSize;
          dynamicData += strLen;
        } break;
        default:
          break;
      }
    }
  }

  const PacketHeader header = {
      .length = staticLength + dynamicLength,
      .id = id,
      .seqNumber = totalPacketsSent,
      .ackNumber = totalPacketsReceived,
      .checksum = 0xCCCC,
  };
#ifndef DISABLE_CRC_CHECK
  // TODO
  // crc
#endif
  memcpy(packetData + 2, (void*)&header, sizeof(PacketHeader));

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

void hardResetParser()
{
  lastErrorCode = 0;
  totalPacketsSent = 0;
  totalPacketsReceived = 0;
  currentlyParsedPacketLength = 0;
  currentPacketSize = 0;
  currentlyParsedPacketId = 0;
  resetParsing();
}

void resetParsing()
{
  parsingStatus = PSTATUS_DETECT;
  currentlyParsedPacketId = 0;
  currentlyParsedPacketLength = 0;
  currentHeaderSize = 0;
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
