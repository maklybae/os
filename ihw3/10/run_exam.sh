#!/bin/bash

# === CONFIGURATION ===
SERVER_EXEC=./server
CLIENT_EXEC=./client
WATCHER_EXEC=./watcher
SERVER_PORT=8080
WATCHER_PORT=8081
LOG_DIR=logs
SERVER_LOG=$LOG_DIR/server.log
CLIENT_LOG_PREFIX=$LOG_DIR/client_
WATCHER_LOG=$LOG_DIR/watcher.log
CLIENT_COUNT=10
CLIENT_IP=127.0.0.1
WATCHER_COUNT=5
WATCHER_PIDS=()

# === BUILD ===
make || { echo "Build failed"; exit 1; }

# === LOG DIR ===
mkdir -p "$LOG_DIR"
rm -f $LOG_DIR/*

# === START SERVER ===
echo "Starting server on port $SERVER_PORT (watcher port $WATCHER_PORT)..."
$SERVER_EXEC $SERVER_PORT $WATCHER_PORT > $SERVER_LOG 2>&1 &
SERVER_PID=$!

# Wait for server to start
sleep 1

# === START FIRST HALF OF WATCHERS ===
FIRST_WATCHERS=$((WATCHER_COUNT / 2))
echo "Starting $FIRST_WATCHERS watcher(s) before clients..."
for i in $(seq 1 $FIRST_WATCHERS); do
  $WATCHER_EXEC $CLIENT_IP $WATCHER_PORT > $LOG_DIR/watcher_${i}.log 2>&1 &
  WATCHER_PIDS+=("$!")
done

# Wait for watchers to connect
sleep 1

# === START CLIENTS ===
echo "Starting $CLIENT_COUNT clients..."
PIDS=()
for i in $(seq 1 $CLIENT_COUNT); do
  $CLIENT_EXEC $CLIENT_IP $SERVER_PORT > ${CLIENT_LOG_PREFIX}${i}.log 2>&1 &
  PIDS+=("$!")
done

# === START SECOND HALF OF WATCHERS ===
SECOND_WATCHERS=$((WATCHER_COUNT - FIRST_WATCHERS))
echo "Starting $SECOND_WATCHERS watcher(s) after clients..."
for i in $(seq $((FIRST_WATCHERS + 1)) $WATCHER_COUNT); do
  $WATCHER_EXEC $CLIENT_IP $WATCHER_PORT > $LOG_DIR/watcher_${i}.log 2>&1 &
  WATCHER_PIDS+=("$!")
done

# Wait for all clients to finish

echo "Waiting for clients to finish..."
for pid in "${PIDS[@]}"; do
  wait $pid
done

echo "All clients finished. Stopping server and watchers."
kill -2 $SERVER_PID
wait $SERVER_PID 2>/dev/null
echo "Stopping watchers..."
for pid in "${WATCHER_PIDS[@]}"; do
  kill -2 $pid
  wait $pid 2>/dev/null
done

echo "Done. Logs in $LOG_DIR/"
