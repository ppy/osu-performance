#!/bin/bash

cd "$(dirname ${BASH_SOURCE[0]})/../Bin"
./osu-performance all "$@"
while : ; do
	echo "osu-performance terminated. Restarting in 5 seconds..."
	sleep 5s
    ./osu-performance all "$@" -c
done
