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
} prsHdrData = {};
PacketHeader lstHdrData = {};

const byte *prsPktData;
byte *pktWriter;
const byte *lstPktData;

int lstErrCode = 0;

unsigned int totalPacketsSent = 0;
unsigned int totalPacketsReceived = 0;

const datatype *prsDynField = 0;
size_t prsPktLength = 0;
size_t prsPktSize = 0;
size_t prsHdrSize = 0;
unsigned int prsPktId = 0;

const errorCode reportError(const errorCode code)
{
  //   resetParsing();
  lstErrCode = code;
  return code;
}

void allocateMemoryForPacketData()
{
  if(prsPktLength == 0) {
    return;
  }
  prsPktData = malloc(packetStructSizes[prsPktId]);

  if(prsPktData == 0) {
    reportError(PERR_MALLOC_FAILED);
    return;
  }
  pktWriter = (byte *)prsPktData;
}

void parseHeaderData(const byte data)
{
  prsHdrData.data[prsHdrSize++] = data;
}

const errorCode parsePacketData(const byte data)
{
  if(prsPktSize >= prsPktLength) {
    return reportError(PERR_BUFFER_OVERFLOW);
  }
  *(pktWriter++) = data;
  prsPktSize++;
  return 0;
}

void finishRecieiving()
{
  if(prsPktSize != packetStructSizes[prsPktId]) {
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
  lstHdrData = prsHdrData.header;
  lstPktData = prsPktData;
  resetParsing();
}

enum {
  PSTATUS_DETECT = 0,
  PSTATUS_HEADER = 1,
  PSTATUS_STATICDATA = 2,
  PSTATUS_DYNAMICDATA = 3,
} parsingStatus = PSTATUS_DETECT;
void finishHeaderParsing()
{
  prsPktId = prsHdrData.header.id;
  prsPktLength = packetStructSizes[prsPktId];

  if(prsPktId >= definedPacketCount) {
    reportError(PERR_UNKNOWN_ID);
    return;
  }

  if(prsPktLength == 0) {
    finishRecieiving();
  } else {
    allocateMemoryForPacketData();

    if(packetDynamicCount[prsPktId] != 0) {
      prsDynField = &parserTable[prsPktId][packetStaticSizes[prsPktId]];
    }
    if(packetStaticSizes[prsPktId] == 0) {
      parsingStatus = PSTATUS_DYNAMICDATA;
    } else {
      parsingStatus = PSTATUS_STATICDATA;
    }
  }
}

void finishStaticDataParsing()
{
  if(packetDynamicCount[prsPktId] == 0) {
    finishRecieiving();
  } else {
    parsingStatus = PSTATUS_DYNAMICDATA;
  }
}

const size_t getVaruint(const byte data, unsigned long long *out)
{
  static unsigned long long buffer = 0;
  static size_t size;

  const unsigned long long value = data & 0x7F;
  const int marker = data & 0x80;

  buffer |= value << size;
  size += 7;

  if(marker == 0) {
    *out = buffer;
    buffer = 0;
    const size_t ret = size;
    size = 0;
    return ret;
  }

  return 0;
}

const size_t parseVaruint(const byte data)
{
  unsigned long long out;
  size_t size = getVaruint(data, &out);
  if(size == 0) {
    return 0;
  }

  int i = 0;
  do {
    parsePacketData(out & 0xFF);
    out >>= 8;
    i += 1;
  } while(i < sizeof(unsigned long long));

  return size;
}

const size_t parseVarint(const byte data)
{
  static long long buffer = 0;
  static int lastMarkerBit = 1;
  static size_t size;

  const int valueBits = data & 0x7F;
  const int markerBit = (data & 0x80) >> 7;

  buffer |= valueBits << size;
  size += 7;

  if(lastMarkerBit == 1) {
    lastMarkerBit = markerBit;
    return 0;
  }

  if(markerBit == 1) {
    buffer *= -1;
  }

  int i = 0;
  do {
    parsePacketData(buffer & 0xFF);
    buffer >>= 8;
    i += 1;
  } while(i < sizeof(long long));

  const size_t ret = size;
  size = 0;
  buffer = 0;
  lastMarkerBit = 1;

  return ret;
}

const size_t parseString(const byte data)
{
}

void processDynamicData(const byte data)
{
  switch(*prsDynField) {
    case TYPE_VARUINT:
      if(parseVaruint(data) != 0x0) {
        prsDynField++;
      }
      break;
    case TYPE_VARINT:
      if(parseVarint(data) != 0x0) {
        prsDynField++;
      }
      break;
    case TYPE_STRING:
      if(parseString(data) != 0x0) {
        prsDynField++;
      }
      break;
    default:
      break;
  }
}

unsigned short last2Bytes = 0x0;
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

      if(prsHdrSize == sizeof(PacketHeader)) {
        finishHeaderParsing();
      }
      break;
    case PSTATUS_STATICDATA:
      if(parsePacketData(data) != 0) {
        return;
      }

      if(prsPktSize == packetStaticSizes[prsPktId]) {
        finishStaticDataParsing();
      }
      break;
    case PSTATUS_DYNAMICDATA:
      processDynamicData(data);
      if(*prsDynField == 0x0) {
        finishRecieiving();
      }
      break;
  }
}

const size_t calculateVaruintSize(const unsigned long long data)
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

const size_t calculateVarintSize(const long long data)
{
  long long value = data;
  if(value < 0) {
    value *= -1;
  }
  size_t size = calculateVaruintSize(value);
  if(size < 2) {
    size = 2;
  }
  return size;
}

const size_t calculateDynamicSize(const unsigned int id, const void *data)
{
  if(packetDynamicCount[id] == 0) {
    return 0;
  }

  size_t totalSize = 0;
  const void *dataPointer = data;
  dataPointer += packetStaticSizes[id];

  const datatype *dynamicFieldType = &parserTable[id][packetStaticCount[id]];

  for(int i = 0; i < packetDynamicCount[id]; i++) {
    switch(*dynamicFieldType++) {
      case TYPE_VARUINT: {
        totalSize += calculateVaruintSize(*(unsigned long long *)dataPointer);
        dataPointer += sizeof(long long);
      } break;

      case TYPE_VARINT: {
        totalSize += calculateVarintSize(*(long long *)dataPointer);
        dataPointer += sizeof(unsigned long long);
      } break;

      case TYPE_STRING: {
        size_t length = strlen(*(char **)dataPointer);
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

const size_t generateVaruint(unsigned long long varuint, byte *packetData)
{
  const char mask = 0b10000000;
  size_t size = 0;
  do {
    packetData[size++] = mask | (varuint & 0x7F);
    varuint >>= 7;
  } while(varuint > 0);
  packetData[size - 1] ^= mask;
  return size;
}

const size_t generateVarint(long long varuint, byte *packetData)
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

const size_t generateString(const char *str, byte *packetData, size_t *strLen)
{
  size_t len = strlen(str);
  *strLen = len;
  size_t varuintSize = generateVaruint(len, packetData);
  packetData += varuintSize;
  memcpy(packetData, str, len);
  return len + varuintSize;
}

void generateDynamicData(const unsigned int id, const void *data,
                         const size_t staticLength, byte *genPktData)
{
  const byte *pktDynData = data + staticLength;
  byte *dynDataWriter = genPktData + PACKET_HEADER_LENGTH + staticLength;

  const datatype *genDynFieldType = &parserTable[id][packetStaticCount[id]];

  size_t datasize = 0;
  for(int i = 0; i < packetDynamicCount[id]; i++) {
    switch(*genDynFieldType++) {
      case TYPE_VARUINT: {
        datasize =
            generateVaruint(*(unsigned long long *)pktDynData, dynDataWriter);
        pktDynData += sizeof(unsigned long long);
      } break;

      case TYPE_VARINT: {
        datasize = generateVarint(*(long long *)pktDynData, dynDataWriter);
        pktDynData += sizeof(long long);
      } break;

      case TYPE_STRING: {
        size_t strLen;
        datasize = generateString(*(char **)pktDynData, dynDataWriter, &strLen);
        pktDynData += strLen;
      } break;

      default:
        break;
    }
    dynDataWriter += datasize;
  }
}

byte *generatePacket(const unsigned int id, const void *data, size_t *size)
{
  if(id >= definedPacketCount) {
    reportError(PERR_UNKNOWN_ID);
    return 0;
  }

  size_t staticLength = packetStaticSizes[id];
  size_t dynamicLength = calculateDynamicSize(id, data);
  size_t pktSize = staticLength + dynamicLength + PACKET_HEADER_LENGTH;

  byte *genPktData = (byte *)malloc(pktSize * sizeof(byte));

  genPktData[0] = MAGIC1;
  genPktData[1] = MAGIC2;
  memcpy(genPktData + PACKET_HEADER_LENGTH, data, staticLength);

  if(packetDynamicCount[id] != 0) {
    generateDynamicData(id, data, staticLength, genPktData);
  }

  const PacketHeader genHdr = {
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
  memcpy(genPktData + 2, (void *)&genHdr, sizeof(PacketHeader));

  *size = pktSize;
  totalPacketsSent++;
  return genPktData;
}

const PacketHeader *getLastHeader()
{
  if(totalPacketsReceived == 0) {
    return 0;
  }
  return &lstHdrData;
}

const void *getLastPacket()
{
  return lstPktData;
}

void hardResetParser()
{
  lstErrCode = 0;
  totalPacketsSent = 0;
  totalPacketsReceived = 0;
  prsPktLength = 0;
  prsPktSize = 0;
  prsPktId = 0;
  resetParsing();
}

void resetParsing()
{
  parsingStatus = PSTATUS_DETECT;
  prsPktId = 0;
  prsPktLength = 0;
  prsHdrSize = 0;
  prsPktSize = 0;
  prsDynField = 0;
  if(lstPktData != prsPktData && prsPktData != 0) {
    free((void *)prsPktData);
  }
  prsPktData = 0;
}

int getLastErrorCode()
{
  return lstErrCode;
}
