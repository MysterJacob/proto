packetlist=$(awk \
'
BEGIN{printf "packets=["}
/^DEF/{printf "%s,", $2}
END{print "]"}
' $2)
crc_table=$(cat crc_table)
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
printf "class %s(Packet):\n", $2;
printf "    _id=%d\n", id
printf "    _fields=["
for(i=3;i<NF;i+=2){printf "(\"%s\", \"%s\")", $i, $(i + 1); if(i!=NF-1) printf ", "}
print "]"
for(i=3;i<NF;i+=2) printf "    %s: %s\n", $i, types[$(i + 1)]
print ""
id+=1;
}
' $2)
# Packets Flag
awk -v pl="$packetlist" -v dc="$dataclasses" -v ct="$crc_table" \
'
BEGIN {printdefault=1}
/^# Packets Flag Start$/{printf "%s\n\n%s\n\n%s\n\n", dc, pl, ct; printdefault=0;}
{if(printdefault==1) print $0;}
/^# Packets Flag End$/{printdefault=1;}
' $1 > proto.py

