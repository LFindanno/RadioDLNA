#! /bin/sh 
# Luigi Findanno 
# Data: 22/12/2015
# File: interface.sh
# Description: receive and execute the commands received by the serial port
# change volume, change radio station and shutdown the system

VOLUME=60
RADIO_COUNTER=0
CURRENT_RADIO=1
VOL_INC=2

loadradio ()
{
   mpc clear
   RADIO_COUNTER=0
   while IFS='' read -r line || [[ -n "$line" ]]; do
       if [ "$line" != "" ];
       then
           mpc add $line
           let "RADIO_COUNTER = $RADIO_COUNTER + 1"
       fi;
   done < "/root/radiolist"
}

check_DLNA ()
{
   test_DLNA=$(echo "currentsong" | nc localhost 6600 | grep -e "^Name: ")
   if [ "$test_DLNA" == "" ];
   then
       return 0; # DLNA
   else
       return 1; # web radio
   fi;
}


trap 'kill $! ; exit 1' SIGINT	# exit on ctrl-c, useful for debugging

stty 9600 -echo -onlcr < /dev/ttyUSB0

loadradio

mpc volume $VOLUME

mpc play $CURRENT_RADIO

/root/display.sh &

while true	# loop forever
do
   inputline="" # clear input
  
   until [ "$inputline" != "" ]
   do
      inputline="$(head -n1 < /dev/ttyUSB0)"
   done

   #echo "$inputline" # ONLY FOR DEBUG!

   cmd="${inputline:0:4}"

   if [ "$cmd" == "SHTD" ]; # shutdown OpenWrt
   then
       halt
   fi;
   
   if [ "$cmd" == "VOL+" ]; # increment volume
   then
       if [ $VOLUME -lt 100 ];
       then
           let "VOLUME = $VOLUME + $VOL_INC"
           mpc volume $VOLUME
       fi;
   fi;

   if [ "$cmd" == "VOL-" ]; # decrement volume
   then
       if [ $VOLUME -gt 0 ];
       then
           let "VOLUME = $VOLUME - $VOL_INC"
           mpc volume $VOLUME
       fi;
   fi;

   if [ "$cmd" == "TUN+" ]; # next radio
   then
       check_DLNA
       if [ $? -eq 0 ];
       then
           # if DLNA mode is active switch to radio mode
           loadradio
       else
           # if radio mode is active play next radio station  
           if [ $CURRENT_RADIO -eq $RADIO_COUNTER ];
           then
               CURRENT_RADIO=1
           else
               let "CURRENT_RADIO = $CURRENT_RADIO + 1"
           fi;
       fi;
       mpc play $CURRENT_RADIO   
   fi;

   if [ "$cmd" == "TUN-" ]; # next radio
   then
       check_DLNA
       if [ $? -eq 0 ];
       then
           loadradio
       else
           if [ $CURRENT_RADIO -eq 1 ];
           then
               CURRENT_RADIO=$RADIO_COUNTER
           else
               let "CURRENT_RADIO = $CURRENT_RADIO - 1"
           fi;
       fi;
       mpc play $CURRENT_RADIO
   fi;

done

