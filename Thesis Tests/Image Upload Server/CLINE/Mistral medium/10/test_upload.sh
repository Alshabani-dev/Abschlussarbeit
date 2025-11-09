#!/bin/bash

# Test GET request
echo "Testing GET request..."
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8080/

# Test file upload with a small image file
echo "Creating test image..."
echo "P1 10 10 255" > test.pgm
for i in {1..100}; do echo 0; done > test.pgm

echo "Testing file upload..."
curl -s -o /dev/null -w "%{http_code}\n" -F "file=@test.pgm" http://localhost:8080/

# Clean up
rm -f test.pgm
