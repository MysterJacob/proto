# Simple proto parser and generator

### Config template
```
# Disable data CRC check, header check is always enabled
CONFIG DisableCrc no
# Disable Sequence and Acknowledgment number check
CONFIG DisableAckSeq yes
# Type of allocator, use malloc or known size buffer
CONFIG AllocatorType malloc
# Size of buffer allocator (only needed when buffer allocator is enabled)
CONFIG BufferSize 4096
# Size of string buffer
CONFIG StringBufferSize 2048

#Define packet with name <SimplePacket> and two fields <Field1> and <Field2> with types INT8 and STRING (char*)
DEF SimplePacket Field1 INT8 Field2 STRING
```
### Compilation
Create file named &ltconfig> with config values and run ``make``.
Compiled files are placed in bin/lib:
- proto&#183;ar static .ar packed liblary
- proto&#183;h header file containing all the definitions
- proto&#183;so dynamic .so liblary

### Usage

#### Serialization of packet
```c
SimplePacket packet = {.Field1 = 70, .Field2 = "ABCD1234"};

size_t outsize;
byte *serialized = generatePacket(SimplePacket_ID, &packet, &outsize);
```

#### Parsing of packet
```c
for(int i = 0; i < outsize; i++) {
    // processByte requires received bytes of serialized data as input
    processByte(*input++);
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

#### Parsing of packet (Using callback)
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

void parse() {
  setErrorCallback(errorHandler);
  setPacketCallback(packetHandler);

  for(int i = 0; i < outsize; i++) {
    processByte(*serialized++);
  }
}
```

### Python API
Python creator is still W.I.P.

Run ``./pythoncreator.sh configFile proto.py output.py``

<output&#183;py> will contain python API wrapper

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
- [ ] Refactor makefile
