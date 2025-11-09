#!/bin/bash

# Test script for WAV Streaming Server

# Start the server in the background
echo "Starting WAV streaming server..."
nix-shell shell.nix --run "mkdir -p build && cd build && cmake .. && make && cd .. && ./build/wavserver --port 8080" &

# Wait for server to start
sleep 2

# Test endpoints
echo -e "\nTesting endpoints:"
curl -s -o /dev/null -w "Home: %{http_code}\n" http://localhost:8080/
curl -s -o /dev/null -w "CSS: %{http_code}\n" http://localhost:8080/styles.css
curl -s -o /dev/null -w "JS: %{http_code}\n" http://localhost:8080/app.js
curl -s -o /dev/null -w "Audio: %{http_code}\n" http://localhost:8080/audio

# Verify binary integrity
echo -e "\nVerifying binary integrity:"
curl -s -o /tmp/out.wav http://localhost:8080/audio && diff -q /tmp/out.wav data/track.wav && echo "✓ Binary integrity verified" || echo "✗ Binary integrity check failed"

# Kill the server
pkill -f wavserver

echo -e "\nTest completed."
