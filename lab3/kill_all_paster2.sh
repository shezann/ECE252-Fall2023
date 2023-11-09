#!/bin/bash

# Name of the process to kill
PROCESS_NAME="paster2"

# Get the PIDs of all instances of the process
PIDS=$(pgrep ${PROCESS_NAME})

# Check if PIDS is not empty
if [ -z "$PIDS" ]; then
    echo "No processes found with name ${PROCESS_NAME}"
    exit 0
fi

# Iterate over the PIDs and kill each process
for PID in $PIDS; do
    kill -9 $PID
    if [ $? -eq 0 ]; then
        echo "Successfully killed process with PID $PID"
    else
        echo "Failed to kill process with PID $PID. May require higher privileges."
    fi
done