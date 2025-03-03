#!/bin/bash

# Check if delay flag is provided
DELAY=""
if [ "$1" = "delay" ]; then
    DELAY="delay"
fi

# Function to check if process is already running
function is_running() {
    local host=$1
    ssh $host "ps aux | grep causal_broadcast | grep -v grep" > /dev/null
    return $?
}

# Compile
make

# Copy executable to each machine
echo "Copying executable to machines..."
for i in {1..4}; do
    host="dc0$i"
    ssh $host "mkdir -p ~/project1" || echo "Failed to create directory on $host"
    scp causal_broadcast $host:~/project1/ || echo "Failed to copy to $host"
done

# Kill any existing processes
echo "Killing any existing processes..."
for i in {1..4}; do
    host="dc0$i"
    ssh $host "pkill -f causal_broadcast" 2>/dev/null
done

# Start processes on each machine
echo "Starting processes..."
for i in {1..4}; do
    host="dc0$i"
    process_id=$((i-1))
    
    if is_running $host; then
        echo "Process already running on $host. Skipping."
    else
        ssh $host "cd ~/project1 && ./causal_broadcast $process_id $DELAY > log$process_id.txt 2>&1 &" || echo "Failed to start on $host"
        echo "Started process $process_id on $host"
    fi
done

echo "All processes started. Logs will be saved to log*.txt on each machine."