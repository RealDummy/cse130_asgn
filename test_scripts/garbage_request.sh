#!/usr/bin/env bash

port=$(test_files/port.py)

 Start up server.
./httpserver $port &
pid=$!

# Wait until we can connect.
while ! nc -z localhost $port; do
    sleep 0.01
done

for i in {1..10}; do
    expected=400

    head -c 2048 test_files/dogs.jpg > requestfile
    printf "\r\n\r\n" >> requestfile

    # The only thing that should be printed is the status code.
    actual=$(cat requestfile | test_files/my_nc.py localhost $port | head -n 1 | awk '{ print $2 }')

    # Check the status code.
    if [[ $actual -ne $expected ]]; then
        echo "actual = $actual"
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
