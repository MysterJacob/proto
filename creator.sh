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
END {
  printf "#define PACKET_COUNT %s\n", count
}
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
BEGIN{
CONFIG_V["DataCrcCheck"]["value"] = "yes";
CONFIG_V["DataCrcCheck"]["match"] = "[yes|no]";
CONFIG_V["DataCrcCheck"]["name"] = "DATA_CRC_CHECK";

CONFIG_V["AckSeqCheck"]["value"] = "no";
CONFIG_V["AckSeqCheck"]["match"] = "[yes|no]";
CONFIG_V["AckSeqCheck"]["name"] = "ACK_SEQ_CHECK";

CONFIG_V["SaveStringSize"]["value"] = "no";
CONFIG_V["SaveStringSize"]["match"] = "[yes|no]";
CONFIG_V["SaveStringSize"]["name"] = "SAVE_STRING_SIZE";

CONFIG_V["SkipLen"]["value"] = "yes";
CONFIG_V["SkipLen"]["match"] = "[yes|no]";
CONFIG_V["SkipLen"]["name"] = "SKIP_PACKET_LEN";

CONFIG_V["AllocatorType"]["value"] = "dynamic";
CONFIG_V["AllocatorType"]["match"] = "[dynamic|buffer]";
CONFIG_V["AllocatorType"]["name"] = "ALLOCATOR_TYPE";

CONFIG_V["MaxPacketSize"]["value"] = "128";
CONFIG_V["MaxPacketSize"]["match"] = "[0-9]+";
CONFIG_V["MaxPacketSize"]["name"] = "MAX_PACKET_SIZE";

CONFIG_V["DataBufferSize"]["value"] = "128";
CONFIG_V["DataBufferSize"]["match"] = "[0-9]+";
CONFIG_V["DataBufferSize"]["name"] = "DATA_BUFFER_SIZE";

CONFIG_V["StringBufferSize"]["value"] = "32";
CONFIG_V["StringBufferSize"]["match"] = "[0-9]+";
CONFIG_V["StringBufferSize"]["name"] = "STRING_BUFFER_SIZE";

CONFIG_V["HeaderCrcAlgo"]["value"] = "CRC16_XMODEM";
CONFIG_V["HeaderCrcAlgo"]["match"] = ".*";
CONFIG_V["HeaderCrcAlgo"]["name"] = "HEADER_CRC_ALGO";

CONFIG_V["DataCrcType"]["value"] = "CRC16_XMODEM";
CONFIG_V["DataCrcType"]["match"] = ".*";
CONFIG_V["DataCrcType"]["name"] = "DATA_CRC_ALGO";

}

/^CONFIG/{
  if(!($2 in CONFIG_V)) {
    printf "#error Unknown config option %s\n", $2;
  }else{
    if(match($3, CONFIG_V[$2]["match"]) != 1) {
      printf "#error Unknown value %s for field %s\n", $3, $2;
    }else{
      CONFIG_V[$2]["value"] = $3;
    }
  }
}

END {
  for (configField in CONFIG_V){
    if(configField == "AllocatorType"){
      if(CONFIG_V[configField]["value"] == "dynamic") print "#define MALLOC_ALLOCATOR";
      if(CONFIG_V[configField]["value"] == "buffer") print "#define BUFFER_ALLOCATOR";
    }else{
      if(CONFIG_V[configField]["value"] == "no") {continue;}
      else if(CONFIG_V[configField]["value"] == "yes") {printf "#define %s\n", CONFIG_V[configField]["name"];}
      else {printf "#define %s %s\n", CONFIG_V[configField]["name"], CONFIG_V[configField]["value"]}
    }
  }
}
' >> $configh
