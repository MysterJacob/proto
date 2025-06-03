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


# Packets Flag
