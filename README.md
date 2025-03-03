# Causal Broadcast Implementation

This repository implements a causally ordered broadcasting system for CS 6378: Advanced Operating Systems (Spring 2025).

## Overview

The system simulates communication between four processes using socket connections, with each process broadcasting 100 messages. Messages are delivered according to causal ordering principles using vector clocks.

## Setup and Execution

### Requirements
- C++ compiler with C++11 support
- UNIX/Linux environment (tested on UTD lab machines dc01-dc45)

### Compilation
```bash
make
```

### Running the System

```bash
./local_run.sh [delay] [debug]
```
This script starts all four processes and redirects output to log files in the `logs/` directory.

## Analyzing Results

### Key Log Files
- `logs/log0.txt` - `logs/log3.txt`: Process execution logs
- Each log contains connection information, message broadcasts, deliveries, and final statistics
