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
    # Test input file.
    file="test_files/wonderful.txt"
    infile="temp.txt"
    outfile="outtemp.txt"

    # Create the input file to overwrite and remove write permissions.
    echo "rickrolled?" > $infile
    chmod -w $infile

    # Expected status code changes depending on user.
    expected=403
    if [[ $EUID -eq 0 ]]; then
        expected=200
    fi

    # The only thing that is should be printed is the status code.
    actual=$(curl -s -w "%{http_code}" -o $outfile localhost:$port/$infile -T $file)

    # Check the status code.
    if [[ $actual -ne $expected ]]; then
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
