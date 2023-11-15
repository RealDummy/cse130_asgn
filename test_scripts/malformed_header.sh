#!/usr/bin/env bash

port=$(bash test_files/get_port.sh)

# Start up server.
./httpserver $port > /dev/null &
pid=$!

# Wait until we can connect.
while ! nc -zv localhost $port; do
    sleep 0.01
done

for i in {1..10}; do
    # Test input file.
    file="test_files/dogs.jpg"
    infile="temp.txt"
    outfile="outtemp.txt"
    size=$(wc -c $file | awk '{ print $1 }')

    # Create the input file to overwrite.
    cp $file $infile

    # Expected status code.
    expected=400

    printf "GET /$infile HTTP/1.1\r\nMalformed-Header; meow\r\n\r\n" > requestfile

    # The only thing that should be printed is the status code.
    actual=$(cat requestfile | test_files/my_nc.py localhost $port | head -n 1 | awk '{ print $2 }')

    # Check the status code.
    if [[ $actual -ne $expected ]]; then
        # Make sure the server is dead.
        kill -9 $pid
        wait $pid
        rm -f $infile $outfile requestfile
        exit 1
    fi

    # Clean up.
    rm -f $infile $outfile requestfile
done

# Make sure the server is dead.
kill -9 $pid
wait $pid

# Clean up.
rm -f $infile $outfile requestfile

exit 0
