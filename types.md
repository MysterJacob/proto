| Type Name | Byte Size | Description |
| --------- | --------- | ----------- |
| int8 | 1 | |
| int16 | 2 | |
| int32 | 4 | |
| int64 | 8 | |
| varint | $\Big\lceil\frac{\log_{2}\left(x+2\right)}{7}\Big\rceil$ | |
| uint8 | 1 | |
| uint16 | 2 | |
| uint32 | 4 | |
| uint64 | 8 | |
| varuint | $\Big\lceil\frac{\log_{2}\left(x+1\right)}{7}\Big\rceil$ | |
| string | length + varuint of length | |
