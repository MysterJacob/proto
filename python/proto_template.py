import struct
from enum import IntEnum
from dataclasses import dataclass


class Datatypes(IntEnum):
    TYPE_INT8 = 1
    TYPE_INT16 = 2
    TYPE_INT32 = 3
    TYPE_INT64 = 4
    TYPE_VARINT = 5
    TYPE_UINT8 = 6
    TYPE_UINT16 = 7
    TYPE_UINT32 = 8
    TYPE_UINT64 = 9
    TYPE_VARUINT = 11
    TYPE_STRING = 12


class Packet:
    _checksum: int
    _length: int
    _id: int
    _seqNumber: int
    _ackNumber: int
    _fields: list[tuple[str, str]]


# Packets Flag Start
@dataclass
class TestPacket(Packet):
    _id = 5
    _fields = [("test1", "UINT8"), ("varint", "VARINT"), ("test2", "UINT8")]
    test1: int
    varint: int
    test2: int


crc16_table = []
packets = [TestPacket]
# Packets Flag End

totalPacketSent = 0
totalPacketReceived = 0


def serializeField(fieldtype: str, value) -> bytes:
    match fieldtype:
        case "INT8":
            return struct.pack("b", value)
        case "INT16":
            return struct.pack("h", value)
        case "INT32":
            return struct.pack("i", value)
        case "INT64":
            return struct.pack("q", value)
        case "VARINT":
            return b"\xfe\xba"
        case "UINT8":
            return struct.pack("B", value)
        case "UINT16":
            return struct.pack("H", value)
        case "UINT32":
            return struct.pack("I", value)
        case "UINT64":
            return struct.pack("Q", value)
        case "VARUINT":
            return b"\xbe\xfa"
        case "STRING":
            return value.encode("ascii") + b"\x00"

    return b""


def calculateStaticCrc(buffer):
    crc = 0x0000
    for i in range(len(buffer)):
        crc = (crc >> 8) ^ crc16_table[(crc ^ buffer[i]) & 0xff]
    return crc

def createPacket(packet: Packet) -> bytes:
    global totalPacketSent

    data = bytes()

    for fieldname, fieldtype in packet._fields:
        data += serializeField(fieldtype, getattr(packet, fieldname))

    data = (
        struct.pack(
            "IIII", len(data), packet._id, totalPacketSent, totalPacketReceived
        )
        + data
    )

    checksum =calculateStaticCrc(data) 
    data = b"\x57\x5f" + struct.pack("H", checksum) + data

    totalPacketSent += 1
    return bytes(data)
