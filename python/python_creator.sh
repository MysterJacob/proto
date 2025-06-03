dataclasses=$(awk \
'
BEGIN {
id=0
types["INT8"] = "int";
types["INT16"] = "int";
types["INT32"] = "int";
types["INT64"] = "int";
types["VARINT"] = "int";
types["UINT8"] = "int";
types["UINT16"] = "int";
types["UINT32"] = "int";
types["UINT64"] = "int";
types["VARUINT"] = "int";
types["STRING"] = "str";
}
/^DEF/{

print "@dataclass";
printf "class %s:\n", $2;
printf "    id=%d\n", id
printf "    fields=["
for(i=3;i<NF;i+=2){printf "(\"%s\", \"%s\")", $i, $(i + 1); if(i!=NF-1) printf ", "}
print "]"
for(i=3;i<NF;i+=2) printf "    %s: %s\n", $i, types[$(i + 1)]
print ""
id+=1;
}
' $2)
# Packets Flag
awk -v dc="$dataclasses" \
'
{print $0}
/# Packets Flag/{printf "%s", dc}
' $1 > proto.py

