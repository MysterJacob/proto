from ctypes import CDLL, Structure, c_uint8, c_uint16, c_uint32
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


class Packet:
    pass


class Proto:
    def __init__(self, libpath: str) -> None:
        self.__proto = CDLL(libpath)

    def processByte(self):
        pass

    def genereatePacket(self, id: int, data: Packet) -> bytes:
        pass

    def isNewPacketReady(self) -> bool:
        pass

    def getPacketLength(self) -> int:
        pass

    def getPacket(self) -> tuple[int, PacketHeader, Packet]:
        pass

    def resetParsing(self):
        pass

    def hardResetParser(self):
        pass

    def getLastErrorCode(self) -> ErrorCode:
        pass
