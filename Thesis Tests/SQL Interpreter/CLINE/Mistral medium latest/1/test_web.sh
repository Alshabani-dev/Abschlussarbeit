#!/bin/bash

# Test the web interface
echo "Testing web interface..."

# Test the HTML page
echo "Testing HTML page..."
curl -s http://localhost:8080 > /dev/null
if [ $? -eq 0 ]; then
    echo "HTML page loaded successfully"
else
    echo "Failed to load HTML page"
    exit 1
fi

# Test SQL execution
echo "Testing SQL execution..."
response=$(curl -s -X POST --data "sql=SELECT * FROM test;" http://localhost:8080/execute)
if [[ "$response" == *"ID | NAME | AGE"* ]]; then
    echo "SQL execution successful"
    echo "Response:"
    echo "$response"
else
    echo "SQL execution failed"
    echo "Response:"
    echo "$response"
    exit 1
fi

echo "All tests passed!"
