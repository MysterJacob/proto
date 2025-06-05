#include "proto.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "crc.h"
#include "parserTables.h"
#include "sanity.h"

#define CRC_INIT 0x7AB3
#define CRC_XOR 0x1201

extern const datatype *const parserTable[];

const size_t MagicSize = MAGIC_SIZE;
const size_t PacketHeaderLength = PACKET_HEADER_LENGTH;
const byte *MagicBytes = (byte *)MAGIC_BYTES;

static union {
  PacketHeader header;
  byte data[sizeof(PacketHeader)];
} prsHdrData = {};

void *prsPktData;
byte *pktWriter;

const size_t CrcSize = sizeof(prsHdrData.header.headerChecksum);

byte newPktRdy = 0;
PacketHandler pktPrsCallback = NULL;

errorCode lstErrCode = 0;

uint32_t totalPacketsSent = 0;
uint32_t totalPacketsReceived = 0;

const datatype *prsDynField = 0;
size_t pktRqStrcSize = 0;
size_t pktStrcSize = 0;

size_t prsPktLen = 0;

size_t prsHdrSize = 0;

uint16_t prsPktCrc = CRC_INIT;
uint32_t prsPktId = 0;

const errorCode reportError(const errorCode code)
{
  resetParsing();
  lstErrCode = code;
  return code;
}

void allocateMemoryForPacketData()
{
  if(pktRqStrcSize == 0) {
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

  prsPktLen = 0;
  prsHdrSize = 0;
  pktStrcSize = 0;
  pktRqStrcSize = 0;

  prsPktData = 0;
  for(int i = 0; i < sizeof(PacketHeader); i++) {
    prsHdrData.data[i] = 0x00;
  }
}

void parsePacketData(const byte data)
{
  if(pktStrcSize >= pktRqStrcSize) {
    reportError(PERR_BUFFER_OVERFLOW);
  }
  *(pktWriter++) = data;
  pktStrcSize++;
}

void parseHeaderData(const byte data)
{
  prsHdrData.data[prsHdrSize++] = data;
}

enum {
  PSTATUS_DETECT = 0,
  PSTATUS_HEADER = 2,
  PSTATUS_STATICDATA = 3,
  PSTATUS_DYNAMICDATA = 4,
} parsingStatus = PSTATUS_DETECT;

void restartParsing()
{
  parsingStatus = PSTATUS_DETECT;
  prsPktId = 0;
  prsPktCrc = CRC_INIT;
  prsDynField = 0;
}

uint16_t getDynamicCrc()
{
  return prsPktCrc ^ CRC_XOR;
}

void finishRecieiving()
{
  if(prsPktLen != prsHdrData.header.length || pktStrcSize != pktRqStrcSize) {
    reportError(PERR_LENGTH_MISMATCH);
    return;
  }

#ifndef DISABLE_CRC_CHECK
  if(getDynamicCrc() != prsHdrData.header.dataChecksum) {
    reportError(PERR_DATA_CRC_MISMATCH);
    return;
  }
#endif

#ifndef DISABLE_ACK_SEQ_CHECK
  if(prsHdrData.header.seqNumber != totalPacketsReceived) {
    reportError(PERR_SEQ_MISMATCH);
    return;
  }
  if(prsHdrData.header.ackNumber != totalPacketsSent) {
    reportError(PERR_ACK_MISMATCH);
    return;
  }
#endif

  totalPacketsReceived++;
  newPktRdy = 1;
  restartParsing();
  if(pktPrsCallback != NULL) pktPrsCallback(prsHdrData.header, prsPktData);
}

static uint16_t calculateCrc(byte *buffer, size_t size)
{
  uint16_t crc = CRC_INIT;
  while(size--) {
    crc = (crc >> 8) ^ crc16_table[(crc ^ *buffer++) & 0xff];
  }
  return crc ^ CRC_XOR;
}

void finishHeaderParsing()
{
  const uint16_t prsHdrCrc = prsHdrData.header.headerChecksum;
  prsHdrData.header.headerChecksum = 0x0000;
  const uint16_t calculatedHdrCrc =
      calculateCrc(prsHdrData.data, sizeof(PacketHeader));
  prsHdrData.header.headerChecksum = prsHdrCrc;

  if(calculatedHdrCrc != prsHdrCrc) {
    reportError(PERR_HDR_CRC_MISMATCH);
    return;
  }

  prsPktId = prsHdrData.header.id;
  if(prsPktId >= definedPacketCount) {
    reportError(PERR_UNKNOWN_ID);
    return;
  }
  pktRqStrcSize = packetStructSizes[prsPktId];

  if(pktRqStrcSize == 0) {
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

struct {
  VARUINT buffer;
  size_t size;
} vuinPrsData = {0, 0};
const size_t getVaruint(const byte data, VARUINT *out)
{
  const VARUINT value = data & 0x7F;
  const int marker = data & 0x80;

  vuinPrsData.buffer |= value << vuinPrsData.size;
  vuinPrsData.size += 7;

  if(marker == 0) {
    *out = vuinPrsData.buffer;
    vuinPrsData.buffer = 0;
    const size_t ret = vuinPrsData.size;
    vuinPrsData.size = 0;
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

struct {
  VARINT buffer;
  int lastMarkerBit;
  size_t size;
} vinPrsData = {0, 1, 0};
const size_t parseVarint(const byte data)
{
  const int valueBits = data & 0x7F;
  const int markerBit = (data & 0x80) >> 7;

  vinPrsData.buffer |= valueBits << vinPrsData.size;
  vinPrsData.size += 7;

  if(vinPrsData.lastMarkerBit == 1) {
    vinPrsData.lastMarkerBit = markerBit;
    return 0;
  }

  if(markerBit == 1) {
    vinPrsData.buffer *= -1;
  }

  int i = 0;
  do {
    parsePacketData(vinPrsData.buffer & 0xFF);
    vinPrsData.buffer >>= 8;
    i += 1;
  } while(i < sizeof(VARINT));

  const size_t ret = vinPrsData.size;
  vinPrsData.size = 0;
  vinPrsData.buffer = 0;
  vinPrsData.lastMarkerBit = 1;

  return ret;
}

struct {
  VARUINT stringLength;
  size_t parsedStringSize;
  char *string;
} strPrsData = {0, 0, 0};
const size_t parseString(const byte data)
{
  if(strPrsData.stringLength == 0) {
    getVaruint(data, &strPrsData.stringLength);
    if(strPrsData.stringLength == 0) {
      return 0;
    }

    strPrsData.string = malloc(sizeof(char) * strPrsData.stringLength);
    if(strPrsData.string == 0) {
      reportError(PERR_MALLOC_FAILED);
    }

    return 0;
  }

  strPrsData.string[strPrsData.parsedStringSize++] = data;

  if(strPrsData.stringLength != strPrsData.parsedStringSize) {
    return 0;
  }

  // Heresy
  long long straddr = (long long)strPrsData.string;
  for(int i = 0; i < sizeof(char *); i++) {
    parsePacketData(straddr & 0xFF);
    straddr >>= 8;
  }

  const size_t ret = strPrsData.stringLength;

  strPrsData.stringLength = 0;
  strPrsData.parsedStringSize = 0;
  return ret;
}

void resetDynamicParsers()
{
  strPrsData.parsedStringSize = 0;
  if(strPrsData.string != 0 &&
     strPrsData.parsedStringSize != strPrsData.stringLength)
    free(strPrsData.string);
  strPrsData.string = 0;
  strPrsData.stringLength = 0;

  vinPrsData.buffer = 0;
  vinPrsData.lastMarkerBit = 1;
  vinPrsData.size = 0;

  vuinPrsData.buffer = 0;
  vuinPrsData.size = 0;
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
  switch(parsingStatus) {
    case PSTATUS_DETECT:
      incrementMagicPointer(data);
      if(magicPointer >= MagicSize) {
        dealocateLastPacket();
        magicPointer = 0;
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
      calculateDynamicCrc(data);
      parsePacketData(data);
      if(lstErrCode) return;
      prsPktLen++;

      if(prsPktLen == packetStaticSizes[prsPktId]) {
        finishStaticDataParsing();
      }

      break;
    case PSTATUS_DYNAMICDATA:
      calculateDynamicCrc(data);
      processDynamicData(data);

      if(lstErrCode) return;
      prsPktLen++;
      if(*prsDynField == 0x0 || prsPktLen >= prsHdrData.header.length) {
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

// void packetInsertCrc(byte *data, size_t size)
// {
//   const size_t offset = MagicSize + CrcSize;
//   uint16_t crc = calculateStaticCrc(data + offset, size - offset);
//   memcpy(data + MagicSize, &crc, CrcSize);
// }

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

  const uint16_t dataCrc = calculateCrc(genPktData + PacketHeaderLength,
                                        staticLength + dynamicLength);

  PacketHeader genHdr = {
      .headerChecksum = 0x0000,
      .length = staticLength + dynamicLength,
      .id = id,
      .seqNumber = totalPacketsSent,
      .ackNumber = totalPacketsReceived,
      .dataChecksum = dataCrc,
  };

  genHdr.headerChecksum = calculateCrc((void *)&genHdr, sizeof(PacketHeader));

  memcpy(genPktData + MagicSize, (void *)&genHdr, sizeof(PacketHeader));

  totalPacketsSent++;
  if(size != NULL) *size = pktSize;
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
    memcpy(packetData, prsPktData, pktRqStrcSize);
  }
  newPktRdy = 0;
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
  prsPktLen = 0;
  prsHdrSize = 0;
  pktStrcSize = 0;
  prsDynField = 0;
  magicPointer = 0;
  pktRqStrcSize = 0;
  prsPktCrc = CRC_INIT;

  resetDynamicParsers();

  if(newPktRdy) {
    dealocateLastPacket();
  }

  newPktRdy = 0;
  prsPktData = 0;
}

int getLastErrorCode()
{
  errorCode cpy = lstErrCode;
  lstErrCode = PERR_NOERR;
  return cpy;
}
