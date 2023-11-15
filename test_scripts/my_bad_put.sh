#!/usr/bin/env bash

port=$(bash test_files/get_port.sh)

 Start up server.
./httpserver $port > /dev/null &
pid=$!

# Wait until we can connect.
while ! nc -zv localhost $port; do
    sleep 0.01
done


# The only thing that should be printed is the status code.
res=$(cat test_files/bad_put1 | test_files/my_nc.py localhost $port | head -n 1 | awk '{ print $2 }')

# Check the status code.
if [[ $res -ne 400 ]]; then
    # Make sure the server is dead.
    kill -9 $pid
    wait $pid
    exit 1
fi

res=$(cat test_files/bad_put2 | test_files/my_nc.py localhost $port | head -n 1 | awk '{ print $2 }')

# Check the status code.
if [[ $res -ne 400 ]]; then
    # Make sure the server is dead.
    kill -9 $pid
    wait $pid
    exit 1
fi

# Make sure the server is dead.
kill -9 $pid
wait $pid

exit 0
