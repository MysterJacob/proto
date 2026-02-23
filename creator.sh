#!/bin/bash
configFile="$1"
parserTables="$2"
packetsh="$3"
configh="$4"

parsedConfig="$(sed "s/#.*//g" $configFile)"

echo -e "/*\nWARNING!!!\nThis file is generated automatically during build process!\n*/" > $parserTables

# parserTables.h
echo "
#ifndef protoh
#include <stdint.h>
#include <stddef.h>
#include \"datatypes.h\"
#include \"packets.h\"
#endif
" >> $parserTables

DYNAMIC_TYPE_REGEX="(VARUINT|VARINT|STRING)"

# Parser table entries
echo "$parsedConfig" | awk -v regex="$DYNAMIC_TYPE_REGEX" \
'
/^DEF/{
printf "const datatype %s_pte[] = {", $2
for(i=4;i<=NF;i+=2) if(!($i ~ regex)){ printf "TYPE_%s, ", $i; }
for(i=4;i<=NF;i+=2) if(($i ~ regex)){ printf "TYPE_%s, ", $i; }
print "0x00};"
}
' >> $parserTables

# Parser table
echo "$parsedConfig" | awk \
'
BEGIN {printf "const datatype *const parserTable[] = {"}
/^DEF/{printf "%s_pte, ", $2}
END {print "0};"}
' >> $parserTables

#definedPacketCount
echo "$parsedConfig" | awk \
'
BEGIN {count = 0}
/^DEF/{count += 1}
END {printf "const unsigned int definedPacketCount = %s;\n", count}
' >> $parserTables

#packetStaticSizes
echo "$parsedConfig" | awk -v regex=$DYNAMIC_TYPE_REGEX \
'
BEGIN {
  sizes["INT8"] = 1;
  sizes["INT16"] = 2;
  sizes["INT32"] = 4;
  sizes["INT64"] = 8;
  sizes["VARINT"] = 0;
  sizes["UINT8"] = 1;
  sizes["UINT16"] = 2;
  sizes["UINT32"] = 4;
  sizes["UINT64"] = 8;
  sizes["VARUINT"] = 0;
  sizes["STRING"] = 0;
  printf "const unsigned int packetStaticSizes[] = {"
}
/^DEF/{
  size = 0;
  for(i=4;i<=NF;i+=2) size+=sizes[$i];
  printf "%s, ", size
}
END { print "0x00};" }
' >> $parserTables

#packetStaticCount
echo "$parsedConfig" | awk -v regex=$DYNAMIC_TYPE_REGEX \
'
BEGIN{c=0; printf "const unsigned int packetStaticCount[] = {"}
/^DEF/{for(i=4;i<=NF;i+=2) if(!($i ~ regex)){c+=1}; printf "%s, ", c; c=0}
END {print "0x00};"}
' >> $parserTables

#packetDynamicCount
echo "$parsedConfig" | awk -v regex=$DYNAMIC_TYPE_REGEX \
'
BEGIN{c=0; printf "const unsigned int packetDynamicCount[] = {"}
/^DEF/{for(i=4;i<=NF;i+=2) if($i ~ regex){c+=1}; printf "%s, ", c; c=0}
END {print "0x00};"}
' >> $parserTables

#packetStructSizes
echo "$parsedConfig" | awk -v regex=$DYNAMIC_TYPE_REGEX \
'
BEGIN{c=0; printf "const size_t packetStructSizes[] = {"}
/^DEF/{printf "sizeof(%s), ", $2}
END {print "0x00};"}
' >> $parserTables


# packets.h
echo -e "/*\nWARNING!!!\nThis file is generated automatically during build process!\n*/" > $packetsh
echo "
#ifndef protoh
#include <stdint.h>
#include <stddef.h>
#include \"datatypes.h\"
#endif
" >> $packetsh

# Ids
echo "$parsedConfig" | awk -v regex="$DYNAMIC_TYPE_REGEX" \
'
BEGIN{print "enum {"; packet_id = 0;}
/^DEF/{
  printf "%s_ID = %d,\n", $2, packet_id;
  packet_id+=1;
}
END{print "};"}
' >> $packetsh

# Structs
echo "$parsedConfig" | awk -v regex="$DYNAMIC_TYPE_REGEX" \
'
/^DEF/{
printf "\ntypedef const volatile struct __attribute__((packed)) {\n"
for(i=3;i<NF;i+=2) if(!($(i + 1) ~ regex)){ printf "\t%s %s;\n", $(i + 1), $i }
for(i=3;i<NF;i+=2){
  if(($(i + 1) ~ regex)){
    printf "\t%s %s;\n", $(i + 1), $i, $i
  }
  if(($(i + 1) == "STRING")){ printf "#ifdef SAVE_STRING_SIZE\nsize_t %s_len;\n#endif\n", $i }
}
printf "} %s;\n", $2
}
' >> $packetsh

# Config
echo -e "/*\nWARNING!!!\nThis file is generated automatically during build process!\n*/" > $configh
echo "$parsedConfig" | awk \
'
/^CONFIG/{
  if(tolower($3) == "yes") {
    printf "#define "
    if($2 == "DisableCrc") print "DISABLE_CRC_CHECK"
    else if($2 == "DisableAckSeq") print "DISABLE_ACK_SEQ_CHECK"
    else if($2 == "DisableAckSeq") print "DISABLE_ACK_SEQ_CHECK"
    else if($2 == "SaveStringSize") print "SAVE_STRING_SIZE"
    else printf "\n#error Unknown config option %s\n", $2
  }
  if($2 == "HeaderCrcType") {
    printf "#define HEADER_CRC_ALGO %s\n", $3
  }else if($2 == "DataCrcType") {
    printf "#define DATA_CRC_ALGO %s\n", $3
  }else if($2 == "AllocatorType") {
    if(tolower($3) == "malloc") print "#define MALLOC_ALLOCATOR"
    else if(tolower($3) == "buffer") print "#define BUFFER_ALLOCATOR"
  }else if($2 == "BufferSize") {
    printf "#define BUFFER_SIZE %s\n", $3
  }else if($2 == "MaxPacketSize") {
    printf "#define MAX_PACKET_SIZE %s\n", $3
  }else if($2 == "StringBufferSize") {
    printf "#define STRING_BUFFER_SIZE %s\n", $3
  }else{
  }
}
' >> $configh
