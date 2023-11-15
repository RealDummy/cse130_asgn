#!/usr/bin/env bash

port=$1
filename=$2
out=$3
while true; do
    t1=$(./time)
    curl http://localhost:$port/$filename 2> /dev/null | diff - $filename > /dev/null
    ec=$?
    t2=$(./time)
    echo "$$ $t1 $t2 $ec" >> $3
done



