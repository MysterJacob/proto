#!/bin/bash
echo "#include \"proto.h\"" > $2

DYNAMIC_TYPE_REGEX="(VARUINT|VARINT|STRING)"

# Parser table entries
awk -v regex="$DYNAMIC_TYPE_REGEX" \
'
{printf "const byte %s_pte[] = {", $1}
{for(i=3;i<=NF;i+=2) if(!($i ~ regex)){ printf "TYPE_%s, ", $i; }}
{for(i=3;i<=NF;i+=2) if(($i ~ regex)){ printf "TYPE_%s, ", $i; }}
{print "0xFF};"}
' $1 >> $2

# Parser table
awk \
'
BEGIN {printf "const byte *const parserTable[] = {"}
{printf "%s_pte, ", $1}
END {print "0};"}
' $1 >> $2
#definedPacketCount
awk \
'
END {printf "const unsigned int definedPacketCount = %s;\n", NR}
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
{
  size = 0;
  for(i=3;i<=NF;i+=2) size+=sizes[$i];
  printf "%s, ", size
}
END {
  print "0x00};"
}
' $1 >> $2

#packetStaticCount
awk -v regex=$DYNAMIC_TYPE_REGEX \
'
BEGIN{c=0; printf "const unsigned int packetStaticCount[] = {"}
{for(i=3;i<=NF;i+=2) if(!($i ~ regex)){c+=1}; printf "%s, ", c; c=0}
END {printf "0x00};"}
' $1 >> $2

# Structs
echo "#include \"proto.h\"" > $3
awk -v regex="$DYNAMIC_TYPE_REGEX" \
'
{printf "\ntypedef const volatile struct __attribute__((packed)) {\n"}
{for(i=2;i<NF;i+=2) if(!($(i + 1) ~ regex)){ printf "\t%s %s;\n", $(i + 1), $i }}
{for(i=2;i<NF;i+=2) if(($(i + 1) ~ regex)){ printf "\t%s %s;\n", $(i + 1), $i }}
{printf "} %s;\n", $1}
' $1 >> $3
