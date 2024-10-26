### Packet structure

| Header | Data |
| ------ | ---- |

# Header structure
| Magic Start | Length | ID | Squence Number | Acknowledgement number | Checksum (CRC16) |
| ----------- | ------ | -- | -------------- | ---------------------- | ---------------- |
| 0x575F | Varuint | Varuint | uint32 | uint32 | uint16 |
