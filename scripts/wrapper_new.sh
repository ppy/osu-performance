#!/bin/bash

cd "$(dirname ${BASH_SOURCE[0]})/../Bin"
while : ; do
	./osu-performance new "$@"
	echo "osu-performance terminated. Restarting in 5 seconds..."
	sleep 5s
done
