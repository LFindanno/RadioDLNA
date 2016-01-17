#! /bin/sh -
# Luigi Findanno 
# Data: 22/12/2015
# File: display.sh
# Description: get the information about the media played by mpd and show it on LCD.
# It send commands to arduino by serial port every 0,8 s.
# If the line to view is too long for the display, the line scroll automatically.

# Set the number of columns - 1 of the display
LIMIT=15
riga1="Luigi Findanno - Internet Radio/DLNA Renderer"
riga2="Powerd by Arduino and OpenWrt"
r1=0
r2=0
newrig1=""
newrig2=""
DELAY=800000

add_spaces ()
{
    add_spaces="                $@              "
}


set_row_spaces ()
{
    add_spaces $@
    set_row_spaces="$add_spaces"
}

update_info ()
{
    DISPLAY_MODE=`cat /root/DISPLAY`
    if [ "$DISPLAY_MODE" == "RADIO" ];
    then
        newrig1="$(echo "currentsong" | nc localhost 6600 | grep -e "^Name: " | cut -d " " -f2-)"       # web radio
        if [ "$newrig1" == "" ];
        then
            newrig1="$(echo "currentsong" | nc localhost 6600 | grep -e "^Artist: " | cut -d " " -f2-)" # DLNA
        fi;
        newrig2="$(echo "currentsong" | nc localhost 6600 | grep -e "^Title: " | cut -d " " -f2-)"
        DELAY=800000
    fi;

    if [ "$DISPLAY_MODE" == "MUTE" ];
    then
        newrig1="      MUTE"
        newrig2=""
        DELAY=800000
    fi;

    if [ "$DISPLAY_MODE" == "VOL" ];
    then
        VOLUME=`cat /root/PARAM`
        newrig1="VOLUME $VOLUME"
        newrig2=""
        DELAY=100000
    fi;
#    echo "$newrig1"
#    echo "$newrig2"

    if [ "$newrig1" != "$riga1" ];
    then
        riga1="$newrig1"
        if [ "${#riga1}" -gt $LIMIT ];
        then
            set_row_spaces $riga1
            rig1="$set_row_spaces"
        else
            rig1="$riga1"
        fi;
        r1=0
    fi;
    if [ "$newrig2" != "$riga2" ];
    then
        riga2="$newrig2"
        if [ "${#riga2}" -gt $LIMIT ];
        then
            set_row_spaces $riga2
            rig2="$set_row_spaces"
        else
            rig2="$riga2"
        fi;
        r2=0
    fi;
    
}

trap 'exit 1' SIGINT	# exit on ctrl-c

# Set the serial port baud rate and options
stty 9600 -echo -onlcr < /dev/ttyUSB0

SCROLLNESS=4

# Add spaces if the line is too long for the display
set_row_spaces $riga1
rig1="$set_row_spaces"

set_row_spaces $riga2
rig2="$set_row_spaces"


while true		# loop forever
do

   # Set substrings to view
   lcd1="${rig1:$r1:16}"
   lcd2="${rig2:$r2:16}"

#   echo "$lcd1"   # for debug
#   echo "$lcd2"

   # Send data to serial port
   echo -e "RIG1$lcd1\nRIG2$lcd2\n" > /dev/ttyUSB0;
   usleep $DELAY

   #echo -e "RIG1$lcd1\n" > /dev/ttyUSB0;
   #usleep 500000
   #echo -e "RIG2$lcd2\n" > /dev/ttyUSB0;
   #usleep 500000
  
   let "r1 = $r1 + $SCROLLNESS"
   let "r2 = $r2 + $SCROLLNESS"

   let "len1 = ${#rig1} - 16"
   let "len2 = ${#rig2} - 16"

   # Check/set to zero the counters
   if [ "$r1" -gt "$len1" ];
   then 
      r1=0
   fi;

   if [ "$r2" -gt "$len2" ];
   then
      r2=0
   fi;

   update_info

done

