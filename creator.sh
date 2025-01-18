#!/bin/bash
echo "#include \"proto.h\"" > $2

# Parser table entries
awk '
{printf "const byte %s_pte[] = {", $1}
{for(i=3;i<=NF;i+=2) printf "TYPE_%s, ", $i;}
{print "0xFF};"}' $1 >> $2
# Parser table
awk '
BEGIN {printf "const byte *const parserTable[] = {"}
{printf "%s_pte, ", $1}
END {print "0};"}' $1 >> $2

# Structs
echo "#include \"proto.h\"" > $3
awk '
{printf "\ntypedef const volatile struct __attribute__((packed)) {\n"}
{for(i=2;i<NF;i+=2) printf "\t%s %s;\n", $(i + 1), $i}
{printf "} %s;\n", $1}' $1 >> $3
