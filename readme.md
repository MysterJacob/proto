# Proto (parser/generator)

Used to serialize and deserialize packets for communicating between devices. Highly configurable to suit different project.
Configuration is done via a makefile at compile time; the result is a ready to use communication protocol


# Usage
### Config template
```
CONFIG DataCrcCheck yes #Calculate packet data CRC sum (yes/no)
CONFIG AckSeqCheck no #Check ACK/SEQ numbers (yes/no)
CONFIG SaveStringSize no #Save string size to packet struct while parsing (yes/no)
CONFIG SkipLen yes #Skip packet len for known size packets (yes/no)
CONFIG AllocatorType dynamic #Select memory allocator (dynamic/buffer)
CONFIG MaxPacketSize 128 #Maximal packet size
CONFIG DataBufferSize 128 #Buffer size (buffer allocator)
CONFIG StringBufferSize 128 #Buffer size for string (buffer allocator)
CONFIG HeaderCrcAlgo CRC16_XMODEM #Crc algorithm type for header checksum
CONFIG DataCrcType CRC16_XMODEM # Crc alogrithm type for data checksum

# Define packet with name <SimplePacket> 
# and two fields <Field1> and <Field2> with types INT8 and STRING (char*)
DEF SimplePacket Field1 INT8 Field2 STRING

# Types of data supported by proto
# INT8, INT16, INT32, INT64, VARINT, UINT8, UINT16, UINT32, UINT64, VARUINT, STRING
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
- [ ] Preamble change from config level
- [ ] Choose to calculate CRC or do LUT
- [ ] Final refactoring

### Planned features
- Transferring blobs of binary data
- zlib compression
- packet encryption
- Multiple crc8 sum inside of packet (segmentation)

### External libs
- CuTest by Asim Jalis (Used only for testing)
