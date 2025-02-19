### Packet structure

| Header | Data |
| ------ | ---- |

# Header structure
| Magic Start | Length | ID | Squence Number | Acknowledgement number | Checksum (CRC16) |
| ----------- | ------ | -- | -------------- | ---------------------- | ---------------- |
| 0x575F | uint32 | uint32 | uint32 | uint32 | uint16 |

### Header Size
| Field | Size |
| ----- | ---- |
| Magic Start | 2 |
| Length | 4 |
| ID | 4 |
| Sequence| 4 |
| Ack | 4 |
| Checksum | 2 |
| $\Sigma$ | 20 |
