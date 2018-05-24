#!/bin/bash

cd "$(dirname ${BASH_SOURCE[0]})/../Bin"
./osu-performance all "$@"
while [ $? -ne 0 ]; do
	echo "osu-performance terminated. Restarting in 5 seconds..."
	sleep 5s
    ./osu-performance all "$@" -c
done
