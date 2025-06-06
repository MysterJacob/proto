#ifndef DATATYPES_H
#define DATATYPES_H
typedef enum {
  TYPE_INT8 = 1,
  TYPE_INT16 = 2,
  TYPE_INT32 = 3,
  TYPE_INT64 = 4,
  TYPE_VARINT = 5,
  TYPE_UINT8 = 6,
  TYPE_UINT16 = 7,
  TYPE_UINT32 = 8,
  TYPE_UINT64 = 9,
  TYPE_VARUINT = 11,
  TYPE_STRING = 12
} datatype;

#define INT8 int8_t
#define INT16 int16_t
#define INT32 int32_t
#define INT64 int64_t
#define VARINT int64_t
#define UINT8 uint8_t
#define UINT16 uint16_t
#define UINT32 uint32_t
#define UINT64 uint64_t
#define VARUINT uint64_t
#define STRING char *
#endif
