#!/bin/sh

if [ $# -gt 0 ]; then
	#kill part of the operation
	busID=`pgrep "$1"`
	if [ -n "$busID" ]; then
		kill -61 "$busID"	
		echo "$1" killed
	else
		echo "$1" not exist.
	fi
else
	#kill all of the operation
	routerID=`pgrep "bus_router"`
	proxyID=`pgrep "bus_proxy"`

	if [ -n "$routerID" ]; then 
		kill -61 "$routerID"
		echo bus_router killed.
	fi

	if [ -n "$proxyID" ]; then
		kill -61 "$proxyID"
		echo bus_proxy killed.
	fi
fi
