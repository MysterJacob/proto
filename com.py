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
    PERR_MALLOC_FAILED = 1
    PERR_BUFFER_OVERFLOW = 2
    PERR_UNKNOWN_ID = 3
    PERR_LENGTH_MISMATCH = 4
    PERR_ACK_MISMATCH = 5
    PERR_SEQ_MISMATCH = 6
    PERR_CRC_MISMATCH = 7


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
class TestPacket0(Packet):
   _id=0
   _fields_=[]
class TestPacket1(Packet):
   _id=1
   _fields_=[("sample8", ctype["INT8"]),("sample16", ctype["INT16"]),("sample32", ctype["INT32"]),]
class TestPacket2(Packet):
   _id=2
   _fields_=[("sampleu8", ctype["UINT8"]),("sample16", ctype["INT16"]),("sampleU32", ctype["UINT32"]),("sample32", ctype["INT32"]),]
class TestPacket3(Packet):
   _id=3
   _fields_=[("test1", ctype["UINT8"]),("message", ctype["STRING"]),("test2", ctype["UINT8"]),]
class TestPacket4(Packet):
   _id=4
   _fields_=[("test1", ctype["UINT8"]),("varuint", ctype["VARUINT"]),("test2", ctype["UINT8"]),]
class TestPacket5(Packet):
   _id=5
   _fields_=[("test1", ctype["UINT8"]),("varint", ctype["VARINT"]),("test2", ctype["UINT8"]),]
class MultipleDynamicPacket(Packet):
   _id=6
   _fields_=[("s", ctype["STRING"]),("vu", ctype["VARUINT"]),("vi", ctype["VARINT"]),]
class MemoryTestPacket(Packet):
   _id=7
   _fields_=[("t1", ctype["STRING"]),("t2", ctype["STRING"]),]
packets=[TestPacket0, TestPacket1, TestPacket2, TestPacket3, TestPacket4, TestPacket5, MultipleDynamicPacket, MemoryTestPacket, ]
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

    # const uint32_t getPacket(PacketHeader* header, void* packetData);
    def getPacket(self) -> tuple[int, PacketHeader, Packet]:
        header = PacketHeader()
        data = create_string_buffer(self.getPacketLength())
        id = self.__proto.getPacket(byref(header), data)

        packetType = packets[id]
        packet = packetType.from_buffer(data)

        return id, header, packet

    # void resetParsing();
    def resetParsing(self):
        self.__proto.resetParsing()

    # void hardResetParser();
    def hardResetParser(self):
        self.__proto.hardresetParser()

    # int getLastErrorCode();
    def getLastErrorCode(self) -> ErrorCode:
        return ErrorCode(self.__proto.getLastErrorCode())
