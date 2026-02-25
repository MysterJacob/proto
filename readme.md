


﻿# proto (parser/generator)

Used to serialize and deserialize packets for communicating between devices. Highly configurable to suit different project.
Configuration is done via a makefile at compile time; the result is a ready to use communication protocol


# Usage
### Config template
```
# Disable data CRC check, header check is always enabled
CONFIG DisableCrc no
# Disable Sequence and Acknowledgment number check
CONFIG DisableAckSeq yes
# Skip sending packet len for known size packets
CONFIG SkipLen yes
# Type of allocator, use malloc or known size buffer (malloc/buffer)
CONFIG AllocatorType malloc
# Size of buffer allocator (only needed when buffer allocator is enabled)
CONFIG BufferSize 4096
# Size of string buffer
CONFIG StringBufferSize 2048
# Maximum packet size
CONFIG MaxPacketSize 32
# Use CRC8-Techo3250 for header
CONFIG HeaderCrcType CRC8_TECH3250
# Use Crc16-Xmodem for data
CONFIG DataCrcType CRC16_XMODEM
# Crc algorithms: CRC32_POSIX, CRC16_XMODEM, CRC16_IBM3740, CRC8_TECH3250

# Define packet with name <SimplePacket> 
# and two fields <Field1> and <Field2> with types INT8 and STRING (char*)
DEF SimplePacket Field1 INT8 Field2 STRING
```
### Compilation
Create file named `config.cfg` with config values and run ``make``.

Compiled files are placed in `bin/lib`:
- `proto.ar` static .ar packed library
- `proto.h` header file containing all the definitions
- `proto.so` dynamic .so library

### Usage

#### Serialization of packet
```c
SimplePacket packet = {.Field1 = 70, .Field2 = "ABCD1234"};

size_t outsize;
byte *serialized = generatePacket(SimplePacket_ID, (void*)&packet, &outsize);
```

#### Parsing of packet
```c
for(int i = 0; i < outsize; i++) {
    // processByte requires received bytes of serialized data as input
    processByte(*serialized++);
}

if(getLastErrorCode() != PERR_NOERR) {
    // ERROR!
    puts("Error while receiving");
}
if(isNewPacketReady()) {
    // New packet is ready
    PacketHeader header;
    void *receivedData = malloc(getPacketLength());
    const int id = getPacket(&header, receivedData);

    if(id == SimplePacket_ID) {
      puts("Received SimplePacket");
      SimplePacket *received = (SimplePacket *)receivedData;
      printf("Field1: %d\nField2: %s\n", received->Field1, received->Field2);
    }

    free(receivedData);
}
```

#### Parsing of packet (Using callbacks)
```c
void packetHandler(const PacketHeader header, void *packetData)
{
  if(header.id == SimplePacket_ID) {
    puts("Received SimplePacket");
    SimplePacket *received = (SimplePacket *)packetData;
    printf("Field1: %d\nField2: %s\n", received->Field1, received->Field2);
  }
}
void errorHandler(const protoErrorCode errorCode)
{
  // ERROR!
  puts("Error while receiving");
}

int main() {
  setErrorCallback(errorHandler);
  setPacketCallback(packetHandler);

  for(int i = 0; i < outsize; i++) {
    processByte(*serialized++);
  }
}
```

### Python API (Experimental)

Run ``./pythoncreator.sh configFile proto.py output.py``

`output.py` will contain python API wrapper

### Data types
Defined in include/datatypes.h
| Type Name | Byte Size |
| --------- | --------- |
| int8 | 1 |
| int16 | 2 |
| int32 | 4 |
| int64 | 8 |
| uint8 | 1 |
| uint16 | 2 |
| uint32 | 4 |
| uint64 | 8 |
| varint | $\Big\lceil\frac{\log_{2}\left(x+2\right)}{7}\Big\rceil$ |
| varuint | $\Big\lceil\frac{\log_{2}\left(x+1\right)}{7}\Big\rceil$ |
| string | length + varuint of length |

### TODO
- [x] Parsing static packets
- [x] Generating static packets
- [x] Generating dynamic packets
- [x] Parsing dynamic packets
- [x] CRC16
- [x] ~Fix~ Multi endianness
- [x] Readme documentation
- [x] String of length 0 crashes
- [x] Refactor makefile
- [x] Fix Ack/Seq numbers not working correctly
- [x] Memory leaks when transmission error occurs while receiving a string
- [x] String buffer of size 0
- [x] String size in packet struct
- [x] Skiping length for constant size packets
- [ ] More tests
- [ ] Choose to calculate CRC or do LUT
- [ ] Preamble change from config level
- [ ] Final refactoring

### Planned features
- Transferring blobs of binary data
- zlib compression
- packet encryption
- Multiple crc8 sum inside of packet (segmentation)

### External libs
- CuTest (Only used for testing)
