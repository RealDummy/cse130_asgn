#!/usr/bin/env bash

#### usable port range 32768 -> 65535

ss --tcp state CLOSE-WAIT --kill > /dev/null
ss --tcp state TIME-WAIT --kill > /dev/null
getports=`netstat -antu | tail -n +3 | awk '{split($4, parts,":"); print parts[length(parts)]}' | uniq`

declare -A portmap
for port in $getports; do
    portmap[$port]=1
done

return_port=32768
while [[ $return_port -le 65535 ]]; do
    if [[ ! -v portmap[$return_port] ]]; then
        echo $return_port
        exit 0
    fi
    ((return_port+=1))
done

echo "couldn't find port"
exit 1
