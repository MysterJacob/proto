#!/bin/bash
echo -e "/*\nWARNING!!!\nThis file is generated automatically during build process!\n*/" > $2
echo "#include \"proto.h\"" >> $2

DYNAMIC_TYPE_REGEX="(VARUINT|VARINT|STRING)"

# Parser table entries
awk -v regex="$DYNAMIC_TYPE_REGEX" \
'
/^DEF/{
printf "const enum datatype %s_pte[] = {", $2
for(i=4;i<=NF;i+=2) if(!($i ~ regex)){ printf "TYPE_%s, ", $i; }
for(i=4;i<=NF;i+=2) if(($i ~ regex)){ printf "TYPE_%s, ", $i; }
print "0xFF};"
}
' $1 >> $2

# Parser table
awk \
'
BEGIN {printf "const enum datatype *const parserTable[] = {"}
/^DEF/{printf "%s_pte, ", $2}
END {print "0};"}
' $1 >> $2

#definedPacketCount
awk \
'
BEGIN {count = 0}
/^DEF/{count += 1}
END {printf "const unsigned int definedPacketCount = %s;\n", count}
' $1 >> $2
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
' $1 >> $2

#packetStaticCount
awk -v regex=$DYNAMIC_TYPE_REGEX \
'
BEGIN{c=0; printf "const unsigned int packetStaticCount[] = {"}
/^DEF/{for(i=4;i<=NF;i+=2) if(!($i ~ regex)){c+=1}; printf "%s, ", c; c=0}
END {print "0x00};"}
' $1 >> $2

#packetDynamicCount
awk -v regex=$DYNAMIC_TYPE_REGEX \
'
BEGIN{c=0; printf "const unsigned int packetDynamicCount[] = {"}
/^DEF/{for(i=4;i<=NF;i+=2) if($i ~ regex){c+=1}; printf "%s, ", c; c=0}
END {print "0x00};"}
' $1 >> $2

# Structs
echo -e "/*\nWARNING!!!\nThis file is generated automatically during build process!\n*/" > $3
echo "#include \"proto.h\"" >> $3
awk -v regex="$DYNAMIC_TYPE_REGEX" \
'
/^DEF/{
printf "\ntypedef const volatile struct __attribute__((packed)) {\n"
for(i=3;i<NF;i+=2) if(!($(i + 1) ~ regex)){ printf "\t%s %s;\n", $(i + 1), $i }
for(i=3;i<NF;i+=2) if(($(i + 1) ~ regex)){ printf "\t%s %s;\n", $(i + 1), $i }
printf "} %s;\n", $2
}
' $1 >> $3

# Config
echo -e "/*\nWARNING!!!\nThis file is generated automatically during build process!\n*/" > $4
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
' $1 >> $4
