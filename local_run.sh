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
PIDS=()
for i in {0..3}; do
    ./causal_broadcast $i $DELAY $DEBUG > logs/log$i.txt 2>&1 &
    PIDS+=($!)
    echo "Started process $i (PID: $!)"
done

echo "All processes started. Logs are being saved to logs/log*.txt"
echo "Monitoring processes. Script will terminate when all processes complete."

# Optional: Show logs in real-time in the background
tail -f logs/log*.txt &
TAIL_PID=$!

# Wait for all processes to complete
while true; do
    all_done=true
    for pid in ${PIDS[@]}; do
        if kill -0 $pid 2>/dev/null; then
            # Process is still running
            all_done=false
            break
        fi
    done
    
    if $all_done; then
        # All processes have completed
        echo "All processes have completed."
        # Kill the tail process
        kill $TAIL_PID 2>/dev/null
        break
    fi
    
    # Sleep for a short time before checking again
    sleep 1
done

echo "Execution complete. Check logs/log*.txt for details."