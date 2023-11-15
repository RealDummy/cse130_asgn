#!/usr/bin/env python3

# perform some calculations on data
# data location defaults to perf_log.txt but if an argument is supplied, uses arg instead
# intended for use with perf.sh

import math
import sys
def lat(r):
    return r[2] - r[1]

def avg_lat(data):
    return sum(map(lat, data)) / len(data)
    
def nth_percentile_lat(data, n):
    d2 = sorted(map(lat, data))
    k = n * len(d2)
    if int(k) >= len(d2):
        return d2[-1]
    l = math.floor(k)
    h = math.ceil(k)
    if l==h:
        return d2[int(k)]
    return d2[int(l)] * (k-l) + d2[int(h)] * (h-k)


def up_time(data):
    d2 = list(map(lambda r: tuple((r[1], r[2])), data))
    d2.sort(key=lambda r:r[0])
    dt = []

    for begin,end in d2:
        if dt and dt[-1][1] >= begin - 1:
            dt[-1][1] = max(dt[-1][1], end)
        else:
            dt.append([begin, end])
    
    return dt
        

def main():
    file_path = "perf_log.txt"
    if len(sys.argv) > 1:
        file_path = sys.argv[1]

    with open(file_path) as log:
        data = [[int(n) for n in r.split(' ')] for r in log.read().split('\n') if r != '']
    if any(map(lambda r: r[3] != 0, data)):
        print("diff check failed on one or more requests, get back to work!")
        #return
    start = min(data, key=lambda r: r[1])
    end = max(data, key=lambda r: r[2])
    total_time = end[2] - start[1]
    nreqs = len(data)
    print("total time(ms):\t", total_time / 10)
    print("reqs done:\t", nreqs)
    print("reqs /sec:\t", nreqs/(total_time / 10000))
    print("avg lat(ms):\t", avg_lat(data) / 10)
    print("99% lat(ms):\t", nth_percentile_lat(data, .99) / 10)
    ut = up_time(data)
    down_time = total_time - sum(map(lambda r: r[1] - r[0], ut))
    print("idle time(ms):\t", down_time / 10)

    


if __name__ == "__main__":
    main()