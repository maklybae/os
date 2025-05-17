#!/bin/bash

# Compile the C files
echo "Compiling watchman and visitor..."
gcc -o watchman watchman.c || { echo "Failed to compile watchman.c"; exit 1; }
gcc -o visitor visitor.c || { echo "Failed to compile visitor.c"; exit 1; }
echo "Compilation successful."

# Run the watchman in the background
echo "Starting watchman..."
./watchman &
WATCHMAN_PID=$!
echo "Watchman started with PID: $WATCHMAN_PID"

# Give watchman time to initialize
sleep 1

# Start 50 visitor processes
echo "Launching 50 visitors..."
for i in {1..50}; do
    ./visitor &
    sleep 0.1
done

echo "All visitors launched."
wait $WATCHMAN_PID
