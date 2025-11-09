#!/bin/bash

# Build and run the server
echo "Building and running the server..."
nix-shell shell.nix --run "mkdir -p build && cd build && cmake .. && make && cd .. && ./build/webserver --port 8080" &

# Wait for server to start
sleep 2

# Test GET request
echo -e "\nTesting GET request..."
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8080/

# Test file upload
echo -e "\nTesting file upload..."
curl -s -o /dev/null -w "%{http_code}\n" -F "file=@/dev/null" http://localhost:8080/

# Test invalid file upload
echo -e "\nTesting invalid file upload..."
curl -s -o /dev/null -w "%{http_code}\n" -F "wrong=@/dev/null" http://localhost:8080/

# Kill the server
pkill -f webserver
