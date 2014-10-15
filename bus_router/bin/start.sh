#!/bin/sh

if [ $# -gt 0 ]; then
	#start part of the operation
	if [ -f "$1" ]; then
		./"$1" -d
	else 
		echo "$1" not exist
	fi
else
	#start all of the operation
	./bus_proxy -d
	./bus_router -d
fi
