#!/bin/bash
echo "#include \"proto.h\""
echo ""

# Parser table entries
awk '
{printf "const byte %s_pte[] = {", $1}
{for(i=3;i<=NF;i+=2) printf "TYPE_%s, ", $i;}
{print "0xFF};"}' $1
echo ""

# Parser table
awk '
BEGIN {printf "const byte *const parserTable[] = {"} 
{printf "%s_pte, ", $1} 
END {print "0};"}' $1
echo ""

# Structs
awk '
{printf "typedef volatile const struct {\n"}
{for(i=2;i<NF;i+=2) printf "\t%s %s;\n", $(i + 1), $i}
{printf "} %s;\n\n", $1}' $1
