#ifdef DISABLE_ACK_SEQ_CHECK
#warning Ack and Seq validation disabled!
#endif

#ifdef DISABLE_CRC_CHECK
#warning Data checksum validation disabled!
#endif

#if !defined(MALLOC_ALLOCATOR) && !defined(BUFFER_ALLOCATOR)
#error No allocator type selected define MALLOC_ALLOCATOR or BUFFER_ALLOCATOR
#endif

#if defined(MALLOC_ALLOCATOR) && defined(BUFFER_ALLOCATOR)
#error Can not use two allocator types at the same time
#endif

#if defined(BUFFER_ALLOCATOR) && \
    (!defined(BUFFER_SIZE) || !defined(STRING_BUFFER_SIZE))
#error BUFFER_SIZE AND STRING_BUFFER_SIZE needed to use BUFFER_ALLOCATOR
#endif

#if defined(BUFFER_ALLOCATOR) && MAX_PACKET_SIZE > BUFFER_SIZE
#error Buffer size is smaller than maximum packet size
#endif
