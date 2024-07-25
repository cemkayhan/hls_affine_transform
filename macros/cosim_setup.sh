#!/bin/sh

source common_cflags.sh
source common_csimflags.sh
source common_cosimflags.sh
source ldflags.sh

CFLAGS=$CFLAGS CSIMFLAGS=$CSIMFLAGS COSIMFLAGS=$COSIMFLAGS LDFLAGS=$LDFLAGS LD_LIBRARY_PATH=$(cat ../LD_LIBRARY_PATH) vitis_hls -f cosim_setup.tcl
