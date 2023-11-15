#!/usr/bin/env bash

if [ $# -lt 2 ]; then
    echo "usage: $0 <n-clients> <n secs> [file] [port]"
    exit 1
fi

ncli=$1
nsecs=$2
port=$4
file=$3

if [ ! -f "time" ]; then
    clang "time.c" -o "time"
fi

if [ "$ncli" == "" ]; then
    ncli=1
fi

if [ "$file" == "" ]; then
    file="perf_readme.txt"
fi

if [ ! -f "$file" ]; then
    echo "$file not found!"
    exit 1
fi


if [ "$port" == "" ]; then
    port=$(test_files/port.py)
    ./httpserver -t 10 -l server.log $port & pid=$!
fi

out="perf_log$ncli-$nsecs.txt"

while ! nc -z localhost $port 2> /dev/null; do
    printf "."
    sleep 0.1
done

rm -f "$out"


clients=()
for i in $(seq $ncli); do
    (./closed_loop.sh $port $file $out) & clients+=($!)
done
sleep $nsecs

kill -9 ${clients[@]}
wait ${clients[@]}


if [ "$pid" != "" ]; then
    kill -2 $pid
    wait $pid
fi

wait
s=s
echo "$ncli clients ran for about $nsecs$s"
./perf_analyze.py $out

