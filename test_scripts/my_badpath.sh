#!/usr/bin/env bash

port=$(bash test_files/get_port.sh)

# Start up server.
./httpserver $port > /dev/null &
pid=$!

# Wait until we can connect.
while ! nc -zv localhost $port; do
    sleep 0.01
done

for i in {1..5}; do

    # Expected status code.
    expected=400

    # The only thing that is should be printed is the status code.
    actual=$(curl -s -w "%{http_code}" -X PUT localhost:$port/test_files/pwnd.txt -d "pwnd")

    # Check the status code.
    if [[ $actual -ne $expected ]]; then
        # Make sure the server is dead.
        kill -9 $pid
        wait $pid
        exit 1
    fi
done

# Make sure the server is dead.
kill -9 $pid
wait $pid


exit 0
