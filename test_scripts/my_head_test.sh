#!/usr/bin/env bash

port=$(bash test_files/get_port.sh)

# Start up server.
./httpserver $port > /dev/null &
pid=$!

# Wait until we can connect.
while ! nc -z localhost $port; do
    sleep 0.1
done

for i in {1..5}; do
    # Make sure file doesn't exist and make it a directory.
    # It's... very unlikely students have a needed directory called oogabooga.

    # Expected status code.
    expected=403

    # The only thing that is should be printed is the status code. 
    diff <(curl -I  localhost:$port/Makefile | grep "Content-Length") <(curl -l -I localhost:$port/Makefile | grep "Content-Length")



    # Check the status code.
    if [[ $? -ne 0 ]]; then
        # Make sure the server is dead.
        kill -9 $pid
        wait $pid
        exit 1
    fi

    diff <(curl -I  localhost:$port/Makefile | head -n 1) <(curl -l -I localhost:$port/Makefile | head -n 1)



    # Check the status code.
    if [[ $? -ne 0 ]]; then
        # Make sure the server is dead.
        kill -9 $pid
        wait $pid
        exit 1
    fi



   # In case they create the file.
done

# Make sure the server is dead.
kill -9 $pid
wait $pid

# In case they create the file.

exit 0

