#!/bin/bash

# local_run.sh - Script to run all processes on a single Mac machine

# Parse arguments
DELAY=""
DEBUG=""

for arg in "$@"; do
    if [ "$arg" = "delay" ]; then
        DELAY="delay"
    elif [ "$arg" = "debug" ]; then
        DEBUG="debug"
    fi
done

# Compile the program
echo "Compiling..."
make

# Create log directory
mkdir -p logs

# Kill any existing processes
echo "Killing any existing processes..."
pkill -f causal_broadcast

# Start all four processes locally
echo "Starting processes..."
for i in {0..3}; do
    ./causal_broadcast $i $DELAY $DEBUG > logs/log$i.txt 2>&1 &
    echo "Started process $i (PID: $!)"
done

echo "All processes started. Logs are being saved to logs/log*.txt"
echo "Use 'tail -f logs/log*.txt' to monitor all logs"
echo "Press Ctrl+C to stop monitoring (processes will continue running)"
echo ""
echo "To check if processes are still running: ps aux | grep causal_broadcast"
echo "To stop all processes: pkill -f causal_broadcast"

# Optional: Monitor logs in real-time
tail -f logs/log*.txt