#!/usr/bin/env bash

port=$(test_files/port.py)

# Start up server.
./httpserver $port > /dev/null &
pid=$!

# Wait until we can connect.
while ! nc -z localhost $port; do
    sleep 0.01
done

tempfile=hangmepls

for i in {1..10}; do
    # This PUT should hang until we kill nc.
    # To test that the server is resilient, we'll just issue a GET request afterwards.
    printf "PUT /$tmpfile HTTP/1.1\r\nContent-Length: 12\r\n\r\n" | nc localhost $port &
    cpid=$!
    sleep 1
    kill -9 $cpid

    # Test input file.
    file="test_files/wonderful.txt"
    infile="temp.txt"
    outfile="outtemp.txt"

    # Copy the input file.
    cp $file $infile

    # Expected status code.
    expected=200

    # The only thing that is should be printed is the status code.
    actual=$(curl -s -w "%{http_code}" -o $outfile localhost:$port/$infile)

    # Check the status code.
    if [[ $actual -ne $expected ]]; then
        # Make sure the server is dead.
        kill -9 $pid
        wait $pid
        rm -f $infile $outfile $tmpfile
        exit 1
    fi

    # Check the diff.
    diff $file $outfile
    if [[ $? -ne 0 ]]; then
        # Make sure the server is dead.
        kill -9 $pid
        wait $pid
        rm -f $infile $outfile $tmpfile
        exit 1
    fi

    # Clean up.
    rm -f $infile $outfile $tmpfile
done

# Make sure the server is dead.
kill -9 $pid
wait $pid

# Clean up.
rm -f $infile $outfile $tmpfile

exit 0
