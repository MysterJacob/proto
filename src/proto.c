#include "proto.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crc.h"
#include "datatypes.h"
#include "parserTables.h"
#include "sanity.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CRC_INIT 0xFFFF
#define CRC_XOR 0x1211

extern const datatype *const parserTable[];

const size_t MagicSize = MAGIC_SIZE;
const byte *MagicBytes = (byte *)MAGIC_BYTES;
const size_t CrcSize = 2;

protoErrorCode lstErrCode = 0;

struct {
  const size_t totalLength;
  union {
    PacketHeader header;
    byte data[sizeof(PacketHeader)];
  } u;
  size_t currentSize;
  size_t magicPointer;
} header = {PACKET_HEADER_LENGTH, {}, 0, 0};

struct {
  byte newReady;
  uint16_t id;
  size_t requiredSize;
  size_t currentSize;
  size_t currentLen;
  uint16_t crc;
  const datatype *dynamicFieldPointer;
  byte *writer;
#ifdef BUFFER_ALLOCATOR
  byte data[BUFFER_SIZE];
#endif
#ifdef MALLOC_ALLOCATOR
  void *data;
#endif
} pkt = {0, 0, 0, 0, 0, 0, 0};

PacketHandler pktPrsCallback = NULL;
ErrorHandler errCallback = NULL;

uint32_t totalPacketsSent = 0;
uint32_t totalPacketsReceived = 0;
const protoErrorCode reportError(const protoErrorCode code)
{
  resetParsing();
  lstErrCode = code;
  if(errCallback != NULL) errCallback(code);
  return code;
}

void allocateMemoryForPacketData()
{
  if(pkt.requiredSize == 0) {
    return;
  }

#ifdef MALLOC_ALLOCATOR
  pkt.data = malloc(packetStructSizes[pkt.id]);

  if(pkt.data == 0) {
    reportError(PERR_MALLOC_FAILED);
    return;
  }
  pkt.writer = (byte *)pkt.data;
#endif
#ifdef BUFFER_ALLOCATOR
  for(int i = 0; i < BUFFER_SIZE; i++)
    pkt.data[i] = 0;
  pkt.writer = &pkt.data[0];
#endif
}

void resetDynamicParsers();
void freeLastPacket()
{
  resetDynamicParsers();
  header.currentSize = 0;

#ifdef MALLOC_ALLOCATOR
  if(pkt.data != 0) free((void *)pkt.data);
  pkt.data = 0;
#endif
#ifdef BUFFER_ALLOCATOR
  for(int i = 0; i < BUFFER_SIZE; i++)
    pkt.data[i] = 0;
#endif

  pkt.newReady = 0;
  pkt.currentLen = 0;
  pkt.currentSize = 0;
  pkt.requiredSize = 0;

  for(int i = 0; i < sizeof(PacketHeader); i++) {
    header.u.data[i] = 0x00;
  }
}
void writeBuffer(const byte *data, size_t size)
{
  if(pkt.currentSize + size > pkt.requiredSize) {
    reportError(PERR_BUFFER_OVERFLOW);
    return;
  }
#ifdef BUFFER_ALLOCATOR
  if(pkt.currentSize + size > BUFFER_SIZE) {
    reportError(PERR_BUFFER_OVERFLOW);
    return;
  }
#endif
  pkt.currentSize += size;
  for(int i = 0; i < size; i++) {
    *(pkt.writer++) = *data++;
  }
}
void writeByte(const byte data)
{
  if(pkt.currentSize >= pkt.requiredSize) {
    reportError(PERR_BUFFER_OVERFLOW);
    return;
  }
#ifdef BUFFER_ALLOCATOR
  if(pkt.currentSize >= BUFFER_SIZE) {
    reportError(PERR_BUFFER_OVERFLOW);
    return;
  }
#endif
  *(pkt.writer++) = data;
  pkt.currentSize++;
}

static inline void parseHeaderData(const byte data)
{
  header.u.data[header.currentSize++] = data;
}

enum {
  PSTATUS_DETECT = 0,
  PSTATUS_HEADER = 2,
  PSTATUS_STATICDATA = 3,
  PSTATUS_DYNAMICDATA = 4,
} parsingStatus = PSTATUS_DETECT;

void resetDynamicParsers();
void restartParser()
{
  resetDynamicParsers();
  parsingStatus = PSTATUS_DETECT;
  pkt.id = 0;
  pkt.crc = CRC_INIT;
  pkt.dynamicFieldPointer = 0;
}

uint16_t getDynamicCrc()
{
  return pkt.crc ^ CRC_XOR;
}

void finishRecieiving()
{
  if(pkt.currentLen != header.u.header.length ||
     pkt.currentSize != pkt.requiredSize) {
    reportError(PERR_LENGTH_MISMATCH);
    return;
  }

#ifndef DISABLE_CRC_CHECK
  if(getDynamicCrc() != header.u.header.dataChecksum) {
    reportError(PERR_DATA_CRC_MISMATCH);
    return;
  }
#endif

#ifndef DISABLE_ACK_SEQ_CHECK
  if(header.u.header.seqNumber != totalPacketsReceived) {
    reportError(PERR_SEQ_MISMATCH);
  }
  if(header.u.header.ackNumber != totalPacketsSent) {
    reportError(PERR_ACK_MISMATCH);
  }
#endif

  totalPacketsReceived++;
  pkt.newReady = 1;
  restartParser();
  if(pktPrsCallback != NULL) pktPrsCallback(header.u.header, pkt.data);
}

#ifdef __AVR__
inline uint16_t calculateCrc(byte *buffer, size_t size)
#else
uint16_t calculateCrc(byte *buffer, size_t size)
#endif
{
  uint16_t crc = CRC_INIT;
  while(size--) {
    crc = (crc >> 8) ^ crc16_table[(crc ^ *buffer++) & 0xff];
  }
  return crc ^ CRC_XOR;
}

const datatype *getFirstDynamicField()
{
  return parserTable[pkt.id] + packetStaticCount[pkt.id];
}

void finishHeaderParsing()
{
  const uint16_t prsHdrCrc = header.u.header.headerChecksum;
  header.u.header.headerChecksum = 0x0000;
  const uint16_t calculatedHdrCrc =
      calculateCrc(header.u.data, sizeof(PacketHeader));
  header.u.header.headerChecksum = prsHdrCrc;

  if(calculatedHdrCrc != prsHdrCrc) {
    reportError(PERR_HDR_CRC_MISMATCH);
    return;
  }

  pkt.id = header.u.header.id;
  if(pkt.id >= definedPacketCount) {
    reportError(PERR_UNKNOWN_ID);
    return;
  }

  pkt.requiredSize = packetStructSizes[pkt.id];

  if(pkt.requiredSize == 0) {
    finishRecieiving();
  } else {
    allocateMemoryForPacketData();

    if(packetDynamicCount[pkt.id] != 0) {
      pkt.dynamicFieldPointer = getFirstDynamicField();
    }
    if(packetStaticSizes[pkt.id] == 0) {
      parsingStatus = PSTATUS_DYNAMICDATA;
    } else {
      parsingStatus = PSTATUS_STATICDATA;
    }
  }
}

void finishStaticParsing()
{
  if(packetDynamicCount[pkt.id] == 0) {
    finishRecieiving();
  } else {
    pkt.dynamicFieldPointer = getFirstDynamicField();
    parsingStatus = PSTATUS_DYNAMICDATA;
  }
}

struct {
  size_t size;
  VARUINT value;
} vuinPrsData = {0, 0};
const size_t getVaruint(const byte data, VARUINT *out)
{
  const VARUINT value = data & 0x7F;
  const int marker = data & 0x80;

  vuinPrsData.value |= value << vuinPrsData.size;
  vuinPrsData.size += 7;

  if(marker == 0) {
    *out = vuinPrsData.value;
    vuinPrsData.value = 0;
    const size_t ret = vuinPrsData.size;
    vuinPrsData.size = 0;
    return ret;
  }

  return 0;
}

const size_t parseVaruint(const byte data)
{
  union {
    VARUINT value;
    byte buffer[sizeof(VARUINT)];
  } u;
  size_t size = getVaruint(data, &u.value);

  if(size == 0) {
    return 0;
  }

  writeBuffer(u.buffer, sizeof(VARUINT));

  return size;
}

struct {
  int lastMarkerBit;
  size_t size;
  union {
    VARINT value;
    byte buffer[sizeof(VARINT)];
  } u;
} vinPrsData = {1, 0};
const size_t parseVarint(const byte data)
{
  const int valueBits = data & 0x7F;
  const int markerBit = (data & 0x80) >> 7;

  vinPrsData.u.value |= valueBits << vinPrsData.size;
  vinPrsData.size += 7;

  if(vinPrsData.lastMarkerBit == 1) {
    vinPrsData.lastMarkerBit = markerBit;
    return 0;
  }

  if(markerBit == 1) {
    vinPrsData.u.value *= -1 * markerBit;
  }

  writeBuffer(vinPrsData.u.buffer, sizeof(VARINT));

  const size_t ret = vinPrsData.size;
  vinPrsData.size = 0;
  vinPrsData.u.value = 0;
  vinPrsData.lastMarkerBit = 1;

  return ret;
}

struct {
  VARUINT requiredLen;
  size_t len;
  int parsedCount;
  union {
    char *string;
    byte buffer[sizeof(char *)];
  } u;
#ifdef MALLOC_ALLOCATOR
  union {
    void *string;
    byte buffer[sizeof(void *)];
  } last;
#endif
#ifdef BUFFER_ALLOCATOR
  size_t stringBufferStart;
  char stringBuffer[STRING_BUFFER_SIZE];
#endif
} strPrsData = {0, 0};
const size_t parseString(const byte data)
{
  if(strPrsData.requiredLen == 0) {
    if(getVaruint(data, &strPrsData.requiredLen) == 0) {
      return 0;
    }
    strPrsData.requiredLen++;  // Null terminator
#ifdef MALLOC_ALLOCATOR
    strPrsData.u.string =
        malloc(sizeof(char) * strPrsData.requiredLen + sizeof(STRING));
    if(strPrsData.u.string == 0) {
      reportError(PERR_MALLOC_FAILED);
      return 0;
    }
#endif
#ifdef BUFFER_ALLOCATOR
    strPrsData.u.string =
        &strPrsData.stringBuffer[strPrsData.stringBufferStart];
    if(strPrsData.stringBufferStart + strPrsData.len > STRING_BUFFER_SIZE) {
      reportError(PERR_BUFFER_OVERFLOW);
      return 0;
    }
#endif
  } else {
    if(data == 0x00) {
      reportError(PERR_UNEXPECTED_NULL);
      return 0;
    }
    strPrsData.u.string[strPrsData.len++] = data;
  }

  if(strPrsData.requiredLen - 1 != strPrsData.len) {
    return 0;
  }

  strPrsData.u.string[strPrsData.len++] = 0x0;

#ifdef MALLOC_ALLOCATOR
  for(int i = 0; i < sizeof(STRING); i++) {
    strPrsData.u.string[strPrsData.len + i] = strPrsData.last.buffer[i];
  }
#endif

  writeBuffer(strPrsData.u.buffer, sizeof(char *));

#ifdef BUFFER_ALLOCATOR
  strPrsData.stringBufferStart += strPrsData.requiredLen;
#endif

#ifdef MALLOC_ALLOCATOR
  strPrsData.last.string = strPrsData.u.string;
#endif

  const size_t returnLen = strPrsData.requiredLen;
  strPrsData.requiredLen = 0;
  strPrsData.u.string = 0;
  strPrsData.len = 0;
  strPrsData.parsedCount++;

  return returnLen;
}

void resetDynamicParsers()
{
  strPrsData.len = 0;
#ifdef MALLOC_ALLOCATOR
  if(strPrsData.u.string != 0) {
    free(strPrsData.u.string);
    strPrsData.u.string = 0;
  }

  if(!pkt.newReady && strPrsData.parsedCount != 0) {
    STRING block = strPrsData.last.string;
    while(block != 0 && strPrsData.parsedCount-- > 0) {
      STRING head = block;
      while(*head++) {
      }

      head = (void *)(*(void **)head);
      free(block);
      block = head;
    }
  }
  strPrsData.last.string = 0;
  strPrsData.parsedCount = 0;
#endif
#ifdef BUFFER_ALLOCATOR
  strPrsData.stringBufferStart = 0;
#endif
  strPrsData.requiredLen = 0;

  vinPrsData.u.value = 0;
  vinPrsData.lastMarkerBit = 1;
  vinPrsData.size = 0;

  vuinPrsData.value = 0;
  vuinPrsData.size = 0;
}

void processDynamicData(const byte data)
{
  switch(*pkt.dynamicFieldPointer) {
    case TYPE_VARUINT:
      if(parseVaruint(data) != 0x0) {
        pkt.dynamicFieldPointer++;
      }
      break;
    case TYPE_VARINT:
      if(parseVarint(data) != 0x0) {
        pkt.dynamicFieldPointer++;
      }
      break;
    case TYPE_STRING:
      if(parseString(data) != 0x0) {
        pkt.dynamicFieldPointer++;
      }
      break;
    default:
      break;
  }
}

void calculateDynamicCrc(byte data)
{
  pkt.crc = (pkt.crc >> 8) ^ crc16_table[(pkt.crc ^ data) & 0xff];
}

void incrementMagicPointer(byte data)
{
  if((MagicBytes[header.magicPointer] ^ data) == 0x0)
    header.magicPointer++;
  else if((MagicBytes[0] ^ data) == 0x0)
    header.magicPointer = 1;
  else
    header.magicPointer = 0;
}

void processByte(const byte data)
{
  switch(parsingStatus) {
    case PSTATUS_DETECT:
      incrementMagicPointer(data);
      if(header.magicPointer >= MagicSize) {
        freeLastPacket();
        header.magicPointer = 0;
        parsingStatus = PSTATUS_HEADER;
      }
      break;
    case PSTATUS_HEADER:
      parseHeaderData(data);
      if(header.currentSize == sizeof(PacketHeader)) {
        finishHeaderParsing();
      }

      break;
    case PSTATUS_STATICDATA:
#ifndef DISABLE_CRC_CHECK
      calculateDynamicCrc(data);
#endif
      writeByte(data);
      if(lstErrCode) return;
      pkt.currentLen++;

      if(pkt.currentLen == packetStaticSizes[pkt.id]) {
        finishStaticParsing();
      }

      break;
    case PSTATUS_DYNAMICDATA:
#ifndef DISABLE_CRC_CHECK
      calculateDynamicCrc(data);
#endif
      processDynamicData(data);

      if(lstErrCode) return;
      pkt.currentLen++;
      if(*pkt.dynamicFieldPointer == 0x0 ||
         pkt.currentLen >= header.u.header.length) {
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
  byte *dynDataWriter = genPktData + header.totalLength + staticLength;
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

#ifdef BUFFER_ALLOCATOR
byte genPktData[BUFFER_SIZE];
#endif
byte *generatePacket(const uint32_t id, const void *data, size_t *size)
{
  if(id >= definedPacketCount) {
    reportError(PERR_UNKNOWN_ID);
    return 0;
  }

  const size_t staticLength = packetStaticSizes[id];
  const size_t dynamicLength = calculateDynamicSize(id, data);
  const size_t datalength = staticLength + dynamicLength;
  const size_t totalSize = datalength + header.totalLength;

#ifdef MALLOC_ALLOCATOR
  byte *const genPktData = (byte *)calloc(totalSize, sizeof(byte));
#endif
#ifdef BUFFER_ALLOCATOR
  if(totalSize > BUFFER_SIZE) {
    reportError(PERR_BUFFER_OVERFLOW);
    return NULL;
  }
  for(int i = 0; i < totalSize + 1; i++)
    genPktData[i] = 0x00;
#endif

  memcpy(genPktData, MagicBytes, MagicSize);
  memcpy(genPktData + header.totalLength, data, staticLength);
  if(packetDynamicCount[id] != 0) {
    generateDynamicData(id, data, staticLength, genPktData);
  }

  PacketHeader genHdr = {
      .headerChecksum = 0x0000,
      .length = datalength,
      .id = id,
#ifndef DISABLE_ACK_SEQ_CHECK
      .seqNumber = totalPacketsSent,
      .ackNumber = totalPacketsReceived,
#endif
#ifndef DISABLE_CRC_CHECK
      .dataChecksum = calculateCrc(genPktData + header.totalLength, datalength),
#endif
  };

  genHdr.headerChecksum = calculateCrc((void *)&genHdr, sizeof(PacketHeader));
  memcpy(genPktData + MagicSize, (void *)&genHdr, sizeof(PacketHeader));

  totalPacketsSent++;
  if(size != NULL) *size = totalSize;
#ifdef MALLOC_ALLOCATOR
  return genPktData;
#endif
#ifdef BUFFER_ALLOCATOR
  return &genPktData[0];
#endif
}
byte *getLastGeneratedPacket()
{
  return pkt.data;
}

int isNewPacketReady()
{
  return pkt.newReady;
}

const size_t getPacketLength()
{
  if(!pkt.newReady) return 0;
  return header.u.header.length;
}

const size_t getPacketStructureSize()
{
  if(!pkt.newReady) return 0;
  return packetStructSizes[header.u.header.id];
}

const uint32_t getPacket(PacketHeader *rheader, void *rpacketData)
{
  if(pkt.newReady == 0) return -1;
  if(rheader != NULL) {
    *rheader = header.u.header;
  }
  if(rpacketData != NULL) {
    memcpy(rpacketData, pkt.data, pkt.requiredSize);
  }
  pkt.newReady = 0;
  return header.u.header.id;
}

void setPacketCallback(PacketHandler handler)
{
  pktPrsCallback = handler;
}

void setErrorCallback(ErrorHandler handler)
{
  errCallback = handler;
}

void hardResetParser()
{
  totalPacketsSent = 0;
  totalPacketsReceived = 0;
  resetParsing();
}

void resetParsing()
{
  resetDynamicParsers();
  freeLastPacket();

  lstErrCode = 0;

  parsingStatus = PSTATUS_DETECT;
  header.currentSize = 0;
  header.magicPointer = 0;

  pkt.id = 0;
  pkt.currentLen = 0;
  pkt.currentSize = 0;
  pkt.dynamicFieldPointer = 0;
  pkt.requiredSize = 0;
  pkt.crc = CRC_INIT;

  pkt.newReady = 0;
}

int getLastErrorCode()
{
  protoErrorCode cpy = lstErrCode;
  lstErrCode = PERR_NOERR;
  return cpy;
}
#ifdef __cplusplus
}
#endif
