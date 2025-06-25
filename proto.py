from ctypes import (
    CDLL,
    Structure,
    POINTER,
    byref,
    c_int,
    c_int8,
    c_int16,
    c_int32,
    c_int64,
    c_uint,
    c_uint8,
    c_uint16,
    c_uint32,
    c_uint64,
    c_size_t,
    c_void_p,
    c_ubyte,
    c_char_p,
    create_string_buffer,
)
from enum import IntEnum


class ErrorCode(IntEnum):
    PERR_NOERR = 0
    PERR_MALLOC_FAILED = 1
    PERR_BUFFER_OVERFLOW = 2
    PERR_UNKNOWN_ID = 3
    PERR_LENGTH_MISMATCH = 4
    PERR_ACK_MISMATCH = 5
    PERR_SEQ_MISMATCH = 6
    PERR_DATA_CRC_MISMATCH = 7
    PERR_HDR_CRC_MISMATCH = 8


class PacketHeader(Structure):
    _pack_ = 1  # Disable padding (matches `__attribute__((packed))`)
    _fields_ = [
        ("checksum", c_uint16),  # uint16_t
        ("length", c_uint32),  # uint32_t
        ("id", c_uint32),  # uint32_t
        ("seqNumber", c_uint32),  # uint32_t
        ("ackNumber", c_uint32),  # uint32_t
    ]


class Packet(Structure):
    _id: int
    _pack_ = 1


ctype = {
    "INT8": c_int8,
    "INT16": c_int16,
    "INT32": c_int32,
    "INT64": c_int64,
    "VARINT": c_int64,
    "UINT8": c_uint8,
    "UINT16": c_uint16,
    "UINT32": c_uint32,
    "UINT64": c_uint64,
    "VARUINT": c_uint64,
    "STRING": c_char_p,
}

# CREATOR INSERT PACKETS
packets = [Packet]
# CREATOR INSERT PACKETS


class Proto:
    def __init__(self, libpath: str) -> None:
        self.__proto = CDLL(libpath)

        self.__proto.generatePacket.argtypes = [
            c_uint,
            c_void_p,
            POINTER(c_size_t),
        ]
        self.__proto.generatePacket.restype = POINTER(c_ubyte)

    # void processByte(const byte data);
    def processByte(self, data):
        return self.__proto.processByte(data)

    # byte* generatePacket(const unsigned int id, const void* data, size_t* size);
    def generatePacket(self, data: Packet) -> tuple[int, bytes]:
        size = c_size_t(0)
        buffer = self.__proto.generatePacket(data._id, byref(data), byref(size))
        return int(size.value), buffer

    # int isNewPacketReady();
    def isNewPacketReady(self) -> bool:
        return self.__proto.isNewPacketReady() == 1

    # const size_t getPacketLength();
    def getPacketLength(self) -> int:
        return self.__proto.getPacketLength()

    # const size_t getPacketStructureSize();
    def getPacketStructureSize(self) -> int:
        return self.__proto.getPacketStructureSize()


    # const uint32_t getPacket(PacketHeader* header, void* packetData);
    def getPacket(self) -> tuple[int, PacketHeader, Packet]:
        header = PacketHeader()
        data = create_string_buffer(self.getPacketStructureSize())
        id = self.__proto.getPacket(byref(header), data)

        packetType = packets[id]
        packet = packetType.from_buffer(data)

        return id, header, packet

    # void resetParsing();
    def resetParsing(self):
        self.__proto.resetParsing()

    # void hardResetParser();
    def hardResetParser(self):
        self.__proto.hardResetParser()

    # int getLastErrorCode();
    def getLastErrorCode(self) -> ErrorCode:
        return ErrorCode(self.__proto.getLastErrorCode())
