configFile="$1"
inputTemplate="$2"
output="$3"

# proto.py
pythonstructs=$(awk -v regex="$DYNAMIC_TYPE_REGEX" -v ind="   " \
'
BEGIN{packet_id = 0;}
/^DEF/{
printf "class %s(Packet):\n", $2

printf "%s_id=%d\n", ind, packet_id;
packet_id+=1;

printf "%s_fields_=[", ind
for(i=3;i<NF;i+=2) if(!($(i + 1) ~ regex)){ printf "(\"%s\", ctype[\"%s\"]),", $i , $(i + 1) }
for(i=3;i<NF;i+=2) if(($(i + 1) ~ regex)){ printf "(\"%s\", ctype[\"%s\"]),", $i , $(i + 1) }

printf "]\n", ind
}
' $configFile)
packetslist=$(awk -v regex="$DYNAMIC_TYPE_REGEX" -v ind="   " \
'
BEGIN {printf "packets=["}
/^DEF/ {printf "%s, ", $2}
END {print "]\n"}
' $configFile)

echo -e "# WARNING!!!\nThis file is generated automatically during build process!" > $output
awk -v pythonstructs="$pythonstructs" -v packetslist="$packetslist" \
'
BEGIN {marker=0;}
/^# CREATOR INSERT PACKETS/{ marker+=1; if(marker == 1) printf "%s\n%s\n%s\n", $0, pythonstructs, packetslist}
{if(marker != 1) print $0}
' $inputTemplate > $output
