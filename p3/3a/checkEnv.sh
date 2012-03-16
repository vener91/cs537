#!/bin/bash
PWD=`pwd`
IFST=`echo $LD_LIBRARY_PATH | grep $PWD | wc -l`
if [ "$IFST" != "1" ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:`pwd`"
fi
