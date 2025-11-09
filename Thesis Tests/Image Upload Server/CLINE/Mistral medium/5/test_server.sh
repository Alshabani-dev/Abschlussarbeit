#!/bin/bash

# Test script for the C++ web server
# Usage: ./test_server.sh [port]

PORT=${1:-8080}
SERVER_PID=""

# Function to cleanup server process
cleanup() {
    if [ -n "$SERVER_PID" ]; then
        echo "Stopping server (PID: $SERVER_PID)..."
        kill $SERVER_PID 2>/dev/null
    fi
}

# Trap signals to ensure cleanup
trap cleanup EXIT INT TERM

# Start the server in the background
echo "Starting server on port $PORT..."
nix-shell shell.nix --run "mkdir -p build && cd build && cmake .. && make && cd .. && ./build/webserver --port $PORT" &
SERVER_PID=$!
sleep 2  # Give server time to start

# Test 1: GET root page
echo -e "\nTest 1: GET /"
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:$PORT/

# Test 2: Upload a test image (if available)
echo -e "\nTest 2: Upload test image"
if [ -f "test.jpg" ]; then
    curl -s -o /dev/null -w "%{http_code}\n" -F "file=@test.jpg" http://localhost:$PORT/
else
    echo "test.jpg not found, skipping upload test"
fi

# Test 3: Invalid upload (wrong field name)
echo -e "\nTest 3: Invalid upload (wrong field name)"
if [ -f "test.jpg" ]; then
    curl -s -o /dev/null -w "%{http_code}\n" -F "wrong=@test.jpg" http://localhost:$PORT/
else
    echo "test.jpg not found, skipping invalid upload test"
fi

# Test 4: GET non-existent file
echo -e "\nTest 4: GET non-existent file"
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:$PORT/nonexistent.html

echo -e "\nTests completed. Server is still running on port $PORT."
echo "You can test manually in your browser at http://localhost:$PORT/"
echo "Press Ctrl+C to stop the server."
