#include "proto.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "crc.h"
#include "parserTables.h"
#include "sanity.h"

extern const datatype* const parserTable[];

const size_t MagicSize = MAGIC_SIZE;
const size_t PacketHeaderLength = PACKET_HEADER_LENGTH;
const byte *MagicBytes = (byte *)MAGIC_BYTES;

static union {
  PacketHeader header;
  byte data[sizeof(PacketHeader)];
} prsHdrData = {};

void *prsPktData;
byte *pktWriter;

const size_t CrcSize = sizeof(prsHdrData.header.checksum);

byte newPktRdy = 0;
PacketHandler pktPrsCallback = NULL;

errorCode lstErrCode = 0;

uint32_t totalPacketsSent = 0;
uint32_t totalPacketsReceived = 0;

const datatype *prsDynField = 0;
size_t prsPktLength = 0;
size_t prsPktSize = 0;
size_t prsHdrSize = 0;
uint16_t prsPktCrc = 0x0000;
uint32_t prsPktId = 0;

const errorCode reportError(const errorCode code)
{
  resetParsing();
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

void dealocateLastPacket()
{
  newPktRdy = 0;
  free((void *)prsPktData);
  prsPktSize = 0;
  prsHdrSize = 0;
  prsPktLength = 0;
  prsPktData = 0;
  for(int i = 0; i < sizeof(PacketHeader); i++) {
    prsHdrData.data[i] = 0x00;
  }
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

void parseHeaderData(const byte data)
{
  prsHdrData.data[prsHdrSize++] = data;
}

enum {
  PSTATUS_DETECT = 0,
  PSTATUS_HEADER_NONCRC = 1,
  PSTATUS_HEADER = 2,
  PSTATUS_STATICDATA = 3,
  PSTATUS_DYNAMICDATA = 4,
} parsingStatus = PSTATUS_DETECT;

void restartParsing()
{
  parsingStatus = PSTATUS_DETECT;
  prsPktId = 0;
  prsPktCrc = 0x0000;
  prsDynField = 0;
}

void finishRecieiving()
{
  if(prsPktSize != packetStructSizes[prsPktId]) {
    reportError(PERR_LENGTH_MISMATCH);
    return;
  }

#ifndef DISABLE_CRC_CHECK
  if(prsPktCrc != prsHdrData.header.checksum) {
    reportError(PERR_CRC_MISMATCH);
    return;
  }
#endif

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
  newPktRdy = 1;
  restartParsing();
  if(pktPrsCallback != NULL) pktPrsCallback(prsHdrData.header, prsPktData);
}

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

const size_t getVaruint(const byte data, VARUINT *out)
{
  static VARUINT buffer = 0;
  static size_t size;

  const VARUINT value = data & 0x7F;
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
  VARUINT out;
  size_t size = getVaruint(data, &out);
  if(size == 0) {
    return 0;
  }

  int i = 0;
  do {
    parsePacketData(out & 0xFF);
    out >>= 8;
    i += 1;
  } while(i < sizeof(VARUINT));

  return size;
}

const size_t parseVarint(const byte data)
{
  static VARINT buffer = 0;
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
  } while(i < sizeof(VARINT));

  const size_t ret = size;
  size = 0;
  buffer = 0;
  lastMarkerBit = 1;

  return ret;
}

const size_t parseString(const byte data)
{
  static VARUINT stringLength = 0;
  static size_t parsedStringSize = 0;
  static char *string = 0;

  if(stringLength == 0) {
    getVaruint(data, &stringLength);
    if(stringLength == 0) {
      return 0;
    }

    string = malloc(sizeof(char) * stringLength);
    if(string == 0) {
      reportError(PERR_MALLOC_FAILED);
    }

    return 0;
  }

  string[parsedStringSize++] = data;

  if(stringLength != parsedStringSize) {
    return 0;
  }

  // Heresy
  long long straddr = (long long)string;
  for(int i = 0; i < sizeof(char *); i++) {
    parsePacketData(straddr & 0xFF);
    straddr >>= 8;
  }

  const size_t ret = stringLength;

  stringLength = 0;
  parsedStringSize = 0;
  return ret;
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

void calculateDynamicCrc(byte data)
{
  prsPktCrc = (prsPktCrc >> 8) ^ crc16_table[(prsPktCrc ^ data) & 0xff];
}

size_t magicPointer = 0;
void incrementMagicPointer(byte data)
{
  if((MagicBytes[magicPointer] ^ data) == 0x0)
    magicPointer++;
  else if((MagicBytes[0] ^ data) == 0x0)
    magicPointer = 1;
  else
    magicPointer = 0;
}
void processByte(const byte data)
{
  if(lstErrCode != 0) {
    return;
  }
  switch(parsingStatus) {
    case PSTATUS_DETECT:
      incrementMagicPointer(data);
      if(magicPointer >= MagicSize) {
        dealocateLastPacket();
        magicPointer = 0;
        parsingStatus = PSTATUS_HEADER_NONCRC;
      }

      break;
    case PSTATUS_HEADER_NONCRC:
      parseHeaderData(data);
      if(prsHdrSize == CrcSize) {
        parsingStatus = PSTATUS_HEADER;
      }

      break;
    case PSTATUS_HEADER:
      parseHeaderData(data);
      calculateDynamicCrc(data);
      if(prsHdrSize == sizeof(PacketHeader)) {
        finishHeaderParsing();
      }

      break;
    case PSTATUS_STATICDATA:
      calculateDynamicCrc(data);
      if(parsePacketData(data) != 0) {
        return;
      }
      if(prsPktSize == packetStaticSizes[prsPktId]) {
        finishStaticDataParsing();
      }

      break;
    case PSTATUS_DYNAMICDATA:
      calculateDynamicCrc(data);
      processDynamicData(data);
      if(*prsDynField == 0x0) {
        finishRecieiving();
      }

      break;
  }
}

const size_t calculateVaruintSize(const VARUINT data)
{
  size_t size = 0;
  VARINT mask = 0x7F;
  int i = 0;
  do {
    size++;
    mask <<= 7;
  } while((mask & data) > 0 && i <= sizeof(VARINT) * 8);
  return size;
}

const size_t calculateVarintSize(const VARINT data)
{
  VARINT value = data;
  if(value < 0) {
    value *= -1;
  }
  size_t size = calculateVaruintSize(value);
  if(size < 2) {
    size = 2;
  }
  return size;
}

const size_t calculateDynamicSize(const uint32_t id, const void *data)
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
        totalSize += calculateVaruintSize(*(VARUINT *)dataPointer);
        dataPointer += sizeof(VARUINT);
      } break;

      case TYPE_VARINT: {
        totalSize += calculateVarintSize(*(VARINT *)dataPointer);
        dataPointer += sizeof(VARINT);
      } break;

      case TYPE_STRING: {
        size_t length = strlen(*(STRING *)dataPointer);
        size_t size = length + calculateVaruintSize(length);
        totalSize += size;
        dataPointer += sizeof(STRING);
      } break;

      default:
        break;
    }
  }

  return totalSize;
}

const size_t generateVaruint(VARUINT varuint, byte *packetData)
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

const size_t generateVarint(VARINT varint, byte *packetData)
{
  byte signbit = 0x0;
  if(varint < 0) {
    signbit = 0x80;
    varint *= -1;
  }
  size_t size = generateVaruint(varint, packetData);
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

void generateDynamicData(const uint32_t id, const void *data,
                         const size_t staticLength, byte *genPktData)
{
  const byte *pktDynData = data + staticLength;
  byte *dynDataWriter = genPktData + PacketHeaderLength + staticLength;

  const datatype *genDynFieldType = &parserTable[id][packetStaticCount[id]];

  size_t datasize = 0;
  for(int i = 0; i < packetDynamicCount[id]; i++) {
    switch(*genDynFieldType++) {
      case TYPE_VARUINT: {
        datasize = generateVaruint(*(VARUINT *)pktDynData, dynDataWriter);
        pktDynData += sizeof(VARUINT);
      } break;

      case TYPE_VARINT: {
        datasize = generateVarint(*(VARINT *)pktDynData, dynDataWriter);
        pktDynData += sizeof(VARINT);
      } break;

      case TYPE_STRING: {
        size_t strLen;
        datasize =
            generateString(*(STRING *)pktDynData, dynDataWriter, &strLen);
        pktDynData += sizeof(STRING);
      } break;

      default:
        break;
    }
    dynDataWriter += datasize;
  }
}

uint16_t calculateStaticCrc(byte *buffer, size_t size)
{
  uint16_t crc = 0x0000;
  while(size--) {
    crc = (crc >> 8) ^ crc16_table[(crc ^ *buffer++) & 0xff];
  }
  return crc;
}

void packetInsertCrc(byte *data, size_t size)
{
  const size_t offset = MagicSize + CrcSize;
  uint16_t crc = calculateStaticCrc(data + offset, size - offset);
  memcpy(data + MagicSize, &crc, CrcSize);
}

byte *generatePacket(const uint32_t id, const void *data, size_t *size)
{
  if(id >= definedPacketCount) {
    reportError(PERR_UNKNOWN_ID);
    return 0;
  }

  size_t staticLength = packetStaticSizes[id];
  size_t dynamicLength = calculateDynamicSize(id, data);
  size_t pktSize = staticLength + dynamicLength + PacketHeaderLength;

  byte *const genPktData = (byte *)malloc(pktSize * sizeof(byte));

  memcpy(genPktData, MagicBytes, MagicSize);
  memcpy(genPktData + PacketHeaderLength, data, staticLength);

  if(packetDynamicCount[id] != 0) {
    generateDynamicData(id, data, staticLength, genPktData);
  }

  const PacketHeader genHdr = {
      .checksum = 0x0000,
      .length = staticLength + dynamicLength,
      .id = id,
      .seqNumber = totalPacketsSent,
      .ackNumber = totalPacketsReceived,
  };

  memcpy(genPktData + MagicSize, (void *)&genHdr, sizeof(PacketHeader));

  packetInsertCrc(genPktData, pktSize);

  totalPacketsSent++;
  *size = pktSize;
  return genPktData;
}

int isNewPacketReady()
{
  return newPktRdy;
}

const size_t getPacketLength()
{
  return prsHdrData.header.length;
}

const uint32_t getPacket(PacketHeader *header, void *packetData)
{
  if(newPktRdy == 0) return -1;
  if(header != NULL) {
    *header = prsHdrData.header;
  }
  if(packetData != NULL) {
    memcpy(packetData, prsPktData, prsPktSize);
  }
  return prsHdrData.header.id;
}

void setPacketCallback(PacketHandler handler)
{
  pktPrsCallback = handler;
}

void hardResetParser()
{
  totalPacketsSent = 0;
  totalPacketsReceived = 0;
  resetParsing();
}

void resetParsing()
{
  lstErrCode = 0;

  parsingStatus = PSTATUS_DETECT;

  prsPktId = 0;
  prsPktLength = 0;
  prsHdrSize = 0;
  prsPktSize = 0;
  prsPktCrc = 0x0000;
  prsDynField = 0;
  magicPointer = 0;

  if(newPktRdy) {
    dealocateLastPacket();
  }
  newPktRdy = 0;
  prsPktData = 0;
}

int getLastErrorCode()
{
  return lstErrCode;
}
