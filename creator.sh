#!/bin/bash
configFile="$1"
parserTables="$2"
packetsh="$3"
configh="$4"
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
awk -v regex="$DYNAMIC_TYPE_REGEX" \
'
/^DEF/{
printf "const datatype %s_pte[] = {", $2
for(i=4;i<=NF;i+=2) if(!($i ~ regex)){ printf "TYPE_%s, ", $i; }
for(i=4;i<=NF;i+=2) if(($i ~ regex)){ printf "TYPE_%s, ", $i; }
print "0x00};"
}
' $configFile >> $parserTables

# Parser table
awk \
'
BEGIN {printf "const datatype *const parserTable[] = {"}
/^DEF/{printf "%s_pte, ", $2}
END {print "0};"}
' $configFile >> $parserTables

#definedPacketCount
awk \
'
BEGIN {count = 0}
/^DEF/{count += 1}
END {printf "const unsigned int definedPacketCount = %s;\n", count}
' $configFile >> $parserTables

#packetStaticSizes
awk -v regex=$DYNAMIC_TYPE_REGEX \
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
' $configFile >> $parserTables

#packetStaticCount
awk -v regex=$DYNAMIC_TYPE_REGEX \
'
BEGIN{c=0; printf "const unsigned int packetStaticCount[] = {"}
/^DEF/{for(i=4;i<=NF;i+=2) if(!($i ~ regex)){c+=1}; printf "%s, ", c; c=0}
END {print "0x00};"}
' $configFile >> $parserTables

#packetDynamicCount
awk -v regex=$DYNAMIC_TYPE_REGEX \
'
BEGIN{c=0; printf "const unsigned int packetDynamicCount[] = {"}
/^DEF/{for(i=4;i<=NF;i+=2) if($i ~ regex){c+=1}; printf "%s, ", c; c=0}
END {print "0x00};"}
' $configFile >> $parserTables

#packetStructSizes
awk -v regex=$DYNAMIC_TYPE_REGEX \
'
BEGIN{c=0; printf "const size_t packetStructSizes[] = {"}
/^DEF/{printf "sizeof(%s), ", $2}
END {print "0x00};"}
' $configFile >> $parserTables


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
awk -v regex="$DYNAMIC_TYPE_REGEX" \
'
BEGIN{print "enum {"; packet_id = 0;}
/^DEF/{
  printf "%s_ID = %d,\n", $2, packet_id;
  packet_id+=1;
}
END{print "};"}
' $configFile >> $packetsh
# Structs
awk -v regex="$DYNAMIC_TYPE_REGEX" \
'
/^DEF/{
printf "\ntypedef const volatile struct __attribute__((packed)) {\n"
for(i=3;i<NF;i+=2) if(!($(i + 1) ~ regex)){ printf "\t%s %s;\n", $(i + 1), $i }
for(i=3;i<NF;i+=2) if(($(i + 1) ~ regex)){ printf "\t%s %s;\n", $(i + 1), $i }
printf "} %s;\n", $2
}
' $configFile >> $packetsh

# Config
echo -e "/*\nWARNING!!!\nThis file is generated automatically during build process!\n*/" > $configh
awk \
'
/^CONFIG/{
  if(tolower($3) == "yes") {
    printf "#define "
    if($2 == "DisableCrc") print "DISABLE_CRC_CHECK"
    else if($2 == "DisableAckSeq") print "DISABLE_ACK_SEQ_CHECK"
    else printf "\n#error Unknown config option %s\n", $2
  }
}
' $configFile >> $configh

