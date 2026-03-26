#if !defined(DATA_CRC_CHECK) && !defined(JOIN_DATA_CRC)
#warning Data checksum validation disabled!
#endif

#if !defined(MALLOC_ALLOCATOR) && !defined(BUFFER_ALLOCATOR)
#error No allocator type selected define MALLOC_ALLOCATOR or BUFFER_ALLOCATOR
#endif

#if defined(MALLOC_ALLOCATOR) && defined(BUFFER_ALLOCATOR)
#error Can not use two allocator types at the same time
#endif

#if defined(BUFFER_ALLOCATOR) && \
    (!defined(DATA_BUFFER_SIZE) || !defined(STRING_BUFFER_SIZE))
#error BUFFER_SIZE AND STRING_BUFFER_SIZE needed to use BUFFER_ALLOCATOR
#endif

#if defined(BUFFER_ALLOCATOR) && MAX_PACKET_SIZE > DATA_BUFFER_SIZE
#error Buffer size is smaller than maximum packet size
#endif

#if defined(DATA_CRC_CHECK) && defined(JOIN_DATA_CRC)
#error Can not calculate data crc and join crc with header
#endif
