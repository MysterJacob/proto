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

extern const datatype *const parserTable[];

protoErrorCode lstErrCode = 0;

struct {
  union {
    PacketHeader hdr;
    uint8_t data[sizeof(PacketHeader)];
  } u;
  size_t size;
  size_t preamblePointer;
} hdr = {{}, 0, 0};

struct {
  uint8_t newReady;
  packetId_t id;
  size_t requiredSize;
  // FIX
  size_t currentSize;
  size_t currentLen;
#ifdef DATA_CRC_CHECK
  crcData_t crc;
#endif
  const datatype *dynamicFieldPointer;
  uint8_t *writer;
#ifdef BUFFER_ALLOCATOR
  uint8_t data[DATA_BUFFER_SIZE];
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
  pkt.writer = (uint8_t *)pkt.data;
#endif
#ifdef BUFFER_ALLOCATOR
  for(int i = 0; i < DATA_BUFFER_SIZE; i++)
    pkt.data[i] = 0;
  pkt.writer = &pkt.data[0];
#endif
}

void resetDynamicParsers();
void freeLastPacket()
{
  resetDynamicParsers();
  hdr.size = 0;

#ifdef MALLOC_ALLOCATOR
  if(pkt.data != 0) free((void *)pkt.data);
  pkt.data = 0;
#endif
#ifdef BUFFER_ALLOCATOR
  for(int i = 0; i < DATA_BUFFER_SIZE; i++)
    pkt.data[i] = 0;
#endif

  pkt.newReady = 0;
  pkt.currentLen = 0;
  pkt.currentSize = 0;
  pkt.requiredSize = 0;

  for(int i = 0; i < sizeof(PacketHeader); i++) {
    hdr.u.data[i] = 0x00;
  }
}
void writeBuffer(const uint8_t *data, size_t size)
{
  if(pkt.currentSize + size > pkt.requiredSize) {
    reportError(PERR_BUFFER_OVERFLOW);
    return;
  }
#ifdef BUFFER_ALLOCATOR
  if(pkt.currentSize + size > DATA_BUFFER_SIZE) {
    reportError(PERR_BUFFER_OVERFLOW);
    return;
  }
#endif
  pkt.currentSize += size;
  for(int i = 0; i < size; i++) {
    *(pkt.writer++) = *data++;
  }
}
void writeByte(const uint8_t data)
{
  if(pkt.currentSize >= pkt.requiredSize) {
    reportError(PERR_BUFFER_OVERFLOW);
    return;
  }
#ifdef BUFFER_ALLOCATOR
  if(pkt.currentSize >= DATA_BUFFER_SIZE) {
    reportError(PERR_BUFFER_OVERFLOW);
    return;
  }
#endif
  *(pkt.writer++) = data;
  pkt.currentSize++;
}

static inline void parseHeaderData(const uint8_t data)
{
  hdr.u.data[hdr.size++] = data;
#ifdef SKIP_LEN
  if(hdr.size == sizeof(PacketHeader) - sizeof(packetSize_t) &&
     hdr.u.hdr.id <= definedPacketCount &&
     packetDynamicCount[hdr.u.hdr.id] == 0) {
    hdr.u.hdr.length = packetStaticSizes[hdr.u.hdr.id];
    hdr.size = sizeof(PacketHeader);
    return;
  }
#endif
}

enum {
#if PREAMBLE_SIZE != 0
  PSTATUS_DETECT = 0,
#endif
  PSTATUS_HEADER = 2,
  PSTATUS_STATICDATA = 3,
  PSTATUS_DYNAMICDATA = 4,
#if PREAMBLE_SIZE == 0
} parsingStatus = PSTATUS_HEADER;
#else
} parsingStatus = PSTATUS_DETECT;
#endif

void resetDynamicParsers();
void restartParser()
{
  resetDynamicParsers();
#if PREAMBLE_SIZE == 0
  parsingStatus = PSTATUS_HEADER;
#else

  parsingStatus = PSTATUS_DETECT;
#endif
  pkt.id = 0;
#ifdef DATA_CRC_CHECK
  resetDataCrc(&pkt.crc);
#endif
  pkt.dynamicFieldPointer = 0;
}

void finishRecieiving()
{
  if(pkt.currentLen != hdr.u.hdr.length ||
     pkt.currentSize != pkt.requiredSize) {
    reportError(PERR_LENGTH_MISMATCH);
    return;
  }

#ifdef DATA_CRC_CHECK
  if(getDataCrc(pkt.crc) != hdr.u.hdr.dataChecksum) {
    reportError(PERR_DATA_CRC_MISMATCH);
    return;
  }
#endif

#ifdef ACK_SEQ_CHECK
  if(hdr.u.hdr.seqNumber != totalPacketsReceived) {
    reportError(PERR_SEQ_MISMATCH);
  }
  if(hdr.u.hdr.ackNumber != totalPacketsSent) {
    reportError(PERR_ACK_MISMATCH);
  }
#endif

  totalPacketsReceived++;
  pkt.newReady = 1;
  restartParser();
  if(pktPrsCallback != NULL) pktPrsCallback(hdr.u.hdr, pkt.data);
}

const datatype *getFirstDynamicField()
{
  return parserTable[pkt.id] + packetStaticCount[pkt.id];
}

void finishHeaderParsing()
{
  const uint16_t prsHdrCrc = hdr.u.hdr.headerChecksum;
  hdr.u.hdr.headerChecksum = 0x0000;
  const uint16_t calculatedHdrCrc =
      calculateHeaderCrc(hdr.u.data, sizeof(PacketHeader));
  hdr.u.hdr.headerChecksum = prsHdrCrc;

  if(calculatedHdrCrc != prsHdrCrc) {
    reportError(PERR_HDR_CRC_MISMATCH);
    return;
  }

  pkt.id = hdr.u.hdr.id;
  if(pkt.id >= PACKET_COUNT) {
    reportError(PERR_UNKNOWN_ID);
    return;
  }
  if(hdr.u.hdr.length > MAX_PACKET_SIZE) {
    reportError(PERR_PACKET_TOO_LARGE);
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
const size_t getVaruint(const uint8_t data, VARUINT *out)
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

const size_t parseVaruint(const uint8_t data)
{
  union {
    VARUINT value;
    uint8_t buffer[sizeof(VARUINT)];
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
    uint8_t buffer[sizeof(VARINT)];
  } u;
} vinPrsData = {1, 0};
const size_t parseVarint(const uint8_t data)
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
    uint8_t buffer[sizeof(char *)];
  } u;
#ifdef MALLOC_ALLOCATOR
  union {
    void *string;
    uint8_t buffer[sizeof(void *)];
  } last;
#endif
#if defined(BUFFER_ALLOCATOR) && STRING_BUFFER_SIZE != 0
  size_t stringBufferStart;
  char stringBuffer[STRING_BUFFER_SIZE];
#endif
} strPrsData = {0, 0};
const size_t parseString(const uint8_t data)
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
#if defined(BUFFER_ALLOCATOR) && STRING_BUFFER_SIZE != 0
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

#ifdef SAVE_STRING_SIZE
  union {
    size_t len;
    uint8_t buffer[sizeof(size_t)];
  } saveStringSize = {strPrsData.len - 1};
  writeBuffer(saveStringSize.buffer, sizeof(size_t));
#endif

#if defined(BUFFER_ALLOCATOR) && STRING_BUFFER_SIZE != 0
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
#if defined(BUFFER_ALLOCATOR) && STRING_BUFFER_SIZE != 0
  strPrsData.stringBufferStart = 0;
#endif
  strPrsData.requiredLen = 0;

  vinPrsData.u.value = 0;
  vinPrsData.lastMarkerBit = 1;
  vinPrsData.size = 0;

  vuinPrsData.value = 0;
  vuinPrsData.size = 0;
}

void processDynamicData(const uint8_t data)
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

#if PREAMBLE_SIZE != 0
void incrementPreamblePointer(uint8_t data)
{
  if((((PREAMBLE_BYTES >> 8 * hdr.preamblePointer) ^ data) & 0xFF) == 0x0)
    hdr.preamblePointer++;
  else if(((PREAMBLE_BYTES & 0xFF) ^ data) == 0x0)
    hdr.preamblePointer = 1;
  else
    hdr.preamblePointer = 0;
}
#endif

void processByte(const uint8_t data)
{
  switch(parsingStatus) {
#if PREAMBLE_SIZE != 0
    case PSTATUS_DETECT:
      incrementPreamblePointer(data);
      if(hdr.preamblePointer >= PREAMBLE_SIZE) {
        freeLastPacket();
        hdr.preamblePointer = 0;
        parsingStatus = PSTATUS_HEADER;
      }
      break;
#endif
    case PSTATUS_HEADER:
      parseHeaderData(data);
      if(hdr.size == sizeof(PacketHeader)) {
        finishHeaderParsing();
      }

      break;
    case PSTATUS_STATICDATA:
#ifdef DATA_CRC_CHECK
      updateDataCrc(&pkt.crc, data);
#endif
      writeByte(data);
      if(lstErrCode) return;
      pkt.currentLen++;

      if(pkt.currentLen == packetStaticSizes[pkt.id]) {
        finishStaticParsing();
      }

      break;
    case PSTATUS_DYNAMICDATA:
#ifdef DATA_CRC_CHECK
      updateDataCrc(&pkt.crc, data);
#endif
      processDynamicData(data);

      if(lstErrCode) return;
      pkt.currentLen++;
      if(*pkt.dynamicFieldPointer == 0x0 ||
         pkt.currentLen >= hdr.u.hdr.length) {
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
#ifdef SAVE_STRING_SIZE
        dataPointer += sizeof(size_t);
#endif
      } break;

      default:
        break;
    }
  }

  return totalSize;
}

const size_t generateVaruint(VARUINT varuint, uint8_t *packetData)
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

const size_t generateVarint(VARINT varint, uint8_t *packetData)
{
  uint8_t signbit = 0x0;
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

const size_t generateString(const char *str, uint8_t *packetData,
                            size_t *strLen)
{
  size_t len = strlen(str);
  *strLen = len;
  size_t varuintSize = generateVaruint(len, packetData);
  packetData += varuintSize;
  memcpy(packetData, str, len);
  return len + varuintSize;
}

void generateDynamicData(const uint32_t id, const void *data,
                         const size_t staticLength, uint8_t *dataPtr)
{
  const uint8_t *pktDynData = data + staticLength;
  uint8_t *dynDataWriter = dataPtr + staticLength;
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
#ifdef SAVE_STRING_SIZE
        pktDynData += sizeof(size_t);
#endif
      } break;

      default:
        break;
    }
    dynDataWriter += datasize;
  }
}

#ifdef BUFFER_ALLOCATOR
uint8_t genPkt[DATA_BUFFER_SIZE];
#endif
uint8_t *generatePacket(const packetId_t id, const void *data, size_t *size)
{
  if(id >= PACKET_COUNT) {
    reportError(PERR_UNKNOWN_ID);
    return 0;
  }

  const size_t staticLength = packetStaticSizes[id];
  const size_t dynamicLength = calculateDynamicSize(id, data);

#ifdef SKIP_LEN
  const size_t headerSize =
      sizeof(PacketHeader) - (dynamicLength == 0 ? sizeof(packetSize_t) : 0);
#else
  const size_t headerSize = sizeof(PacketHeader);
#endif

  const size_t datalength = staticLength + dynamicLength;
  const size_t totalSize = datalength + headerSize + PREAMBLE_SIZE;

  if(totalSize > MAX_PACKET_SIZE) {
    reportError(PERR_PACKET_TOO_LARGE);
    return 0;
  }

#ifdef MALLOC_ALLOCATOR
  uint8_t *const genPkt = (uint8_t *)calloc(totalSize, sizeof(uint8_t));
#endif

#ifdef BUFFER_ALLOCATOR
  if(totalSize > DATA_BUFFER_SIZE) {
    reportError(PERR_BUFFER_OVERFLOW);
    return NULL;
  }
  for(int i = 0; i < totalSize + 1; i++)
    genPkt[i] = 0x00;
#endif
  uint8_t *const dataPtr = genPkt + headerSize + PREAMBLE_SIZE;

#if PREAMBLE_SIZE != 0
  for(int i = 0; i < PREAMBLE_SIZE; i++) {
    genPkt[i] = (PREAMBLE_BYTES >> (8 * i)) & 0xFF;
  }
#endif

  //   memcpy(genPkt, PREAMBLE_BYTES, PREAMBLE_SIZE);

  memcpy(dataPtr, data, staticLength);

  if(dynamicLength != 0) {
    generateDynamicData(id, data, staticLength, dataPtr);
  }

  PacketHeader genHdr = {
      .id = id,
      .headerChecksum = 0x0000,
#ifdef ACK_SEQ_CHECK
      .seqNumber = totalPacketsSent,
      .ackNumber = totalPacketsReceived,
#endif
#ifdef DATA_CRC_CHECK
      .dataChecksum = calculateDataCrc(dataPtr, datalength),
#endif
      .length = datalength,
  };

  genHdr.headerChecksum =
      calculateHeaderCrc((void *)&genHdr, sizeof(PacketHeader));
  memcpy(genPkt + PREAMBLE_SIZE, (void *)&genHdr, headerSize);

  totalPacketsSent++;

  if(size != NULL) *size = totalSize;

#ifdef MALLOC_ALLOCATOR
  return genPkt;
#endif
#ifdef BUFFER_ALLOCATOR
  return &genPkt[0];
#endif
}

uint8_t *getLastGeneratedPacket()
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
  return hdr.u.hdr.length;
}

const size_t getPacketStructureSize()
{
  if(!pkt.newReady) return 0;
  return packetStructSizes[hdr.u.hdr.id];
}

const uint32_t getPacket(PacketHeader *rheader, void *rpacketData)
{
  if(pkt.newReady == 0) return -1;
  if(rheader != NULL) {
    *rheader = hdr.u.hdr;
  }
  if(rpacketData != NULL) {
    memcpy(rpacketData, pkt.data, pkt.requiredSize);
  }
  pkt.newReady = 0;
  packetId_t id = hdr.u.hdr.id;
  freeLastPacket();
  return id;
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

#if PREAMBLE_SIZE == 0
  parsingStatus = PSTATUS_HEADER;
#else
  parsingStatus = PSTATUS_DETECT;
#endif

  hdr.size = 0;
  hdr.preamblePointer = 0;

  pkt.id = 0;
  pkt.currentLen = 0;
  pkt.currentSize = 0;
  pkt.dynamicFieldPointer = 0;
  pkt.requiredSize = 0;
#ifdef DATA_CRC_CHECK
  resetDataCrc(&pkt.crc);
#endif

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
