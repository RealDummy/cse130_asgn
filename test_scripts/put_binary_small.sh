#!/usr/bin/env bash

port=$(test_files/port.py)

# Start up server.
./httpserver $port > /dev/null &
pid=$!

# Wait until we can connect.
while ! nc -z localhost $port; do
    sleep 0.01
done

for i in {1..5}; do
    # Test input file.
    file="test_files/smoldogs.jpg"
    infile="temp.txt"
    outfile="outtemp.txt"

    # Expected status code.
    expected=201

    # The only thing that is should be printed is the status code.
    actual=$(curl -s -w "%{http_code}" -o $outfile localhost:$port/$infile -T $file)

    # Check the status code.
    if [[ $actual -ne $expected ]]; then
        echo "actual = $actual"
        # Make sure the server is dead.
        kill -9 $pid
        wait $pid
        rm -f $infile $outfile
        exit 1
    fi

    # Check the diff.
    diff $file $infile
    if [[ $? -ne 0 ]]; then
        # Make sure the server is dead.
        kill -9 $pid
        wait $pid
        rm -f $infile $outfile
        exit 1
    fi

    # Clean up.
    rm -f $infile $outfile
done

# Make sure the server is dead.
kill -9 $pid
wait $pid

# Clean up.
rm -f $infile $outfile

exit 0
