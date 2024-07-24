#!/bin/sh

source common_cflags.sh
source common_csynthflags.sh

CFLAGS=$CFLAGS CSYNTHFLAGS=$CSYNTHFLAGS vitis_hls -f csynth.tcl
