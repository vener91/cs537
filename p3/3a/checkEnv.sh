#!/bin/bash
PWD=`pwd`
if [ $(echo $LD_LIBRARY_PATH | grep $PWD | wc -l) -eq 0 ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:`pwd`"
fi
