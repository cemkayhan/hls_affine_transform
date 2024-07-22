#!/bin/sh

source common_cflags.sh
source common_csimflags.sh
source ldflags.sh

CFLAGS=$CFLAGS CSIMFLAGS=$CSIMFLAGS LDFLAGS=$LDFLAGS LD_LIBRARY_PATH=$(cat ../LD_LIBRARY_PATH) vitis_hls -f csim_debug.tcl
