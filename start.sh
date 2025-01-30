#!/bin/bash
chmod +x start.sh

echo "Starting FastAPI server..."
python -m uvicorn src.python.create_dfa:app --host 127.0.0.1 --port 8000 &
SERVER_PID=$!
echo "FastAPI server started with PID $SERVER_PID"
echo "Waiting for FastAPI server to be ready..."
timeout=30

while ! nc -z 127.0.0.1 8000; do
    if [ $timeout -le 0 ]; then
        echo "Timeout while waiting for FastAPI server."
        kill $SERVER_PID  
        exit 1
    fi
    timeout=$((timeout - 1))
    sleep 1
done
echo "FastAPI server is ready"

echo "Starting C++ program..."
make
./main

echo "Stopping FastAPI server..."
kill $SERVER_PID
echo "FastAPI server stopped."
