#!/usr/bin/env bash
# language: bash
# File: `test_server.sh`
# Starts the local mock server (mock_server.py) and runs simple tests against it.
# Logs server output to mock_server.log, waits for the server to accept connections,
# and prints the log on failure.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WEB_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PORT=8000
HOST="127.0.0.1"
ENDPOINT="http://$HOST:$PORT/MOTOR_STATUS"
LOG_FILE="$SCRIPT_DIR/mock_server.log"

if [ ! -f "$SCRIPT_DIR/mock_server.py" ]; then
  echo "Error: mock_server.py not found in $SCRIPT_DIR"
  exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo "Error: python3 not found. Install Python 3 to run the mock server."
  exit 1
fi

# truncate old log
: > "$LOG_FILE"

# start server in background, redirect stdout/stderr to log
python3 "$SCRIPT_DIR/mock_server.py" --port "$PORT" --root "$WEB_DIR" >"$LOG_FILE" 2>&1 &
SERVER_PID=$!

cleanup() {
  echo
  echo "Stopping mock server (pid $SERVER_PID)..."
  kill "$SERVER_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# wait until server accepts connections or until process dies
MAX_WAIT_SEC=5
SLEEP_STEP=0.2
elapsed=0
started=0

while (( $(echo "$elapsed < $MAX_WAIT_SEC" | bc -l) )); do
  # try a quick HEAD request with small timeout
  if curl -s --head --max-time 1 "$ENDPOINT" >/dev/null 2>&1; then
    started=1
    break
  fi
  # if server process died, break and show log
  if ! kill -0 "$SERVER_PID" >/dev/null 2>&1; then
    echo "Server process exited prematurely. Last entries from $LOG_FILE:"
    tail -n 200 "$LOG_FILE" || true
    exit 1
  fi
  sleep "$SLEEP_STEP"
  elapsed=$(echo "$elapsed + $SLEEP_STEP" | bc -l)
done

if [ "$started" -ne 1 ]; then
  echo "Server did not start within $MAX_WAIT_SEC seconds. Last entries from $LOG_FILE:"
  tail -n 200 "$LOG_FILE" || true
  exit 1
fi

echo "Server is running and listening on $HOST:$PORT (pid $SERVER_PID)."
echo

echo "1) single request (show headers + body):"
curl -i "$ENDPOINT" || echo "curl failed"
echo

echo "2) quick loop (prints values every 200 ms, Ctrl+C to stop):"
while true; do
  curl -s "$ENDPOINT"
  echo
  sleep 0.2
done
