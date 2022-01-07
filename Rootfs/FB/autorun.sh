#!/bin/sh

cd /FB

source ./board.sh

# Set stdout to /dev/ttyS0


# Initialize retry counter variable
retry=0
# Try to run FBrowser
while [ $retry -lt 30 ]
do
    ./FBrowser
    echo "FBrowser exited with code $?."
    sleep 15
    retry=$((retry+1))
done

echo "Rebooting..."
reboot -f