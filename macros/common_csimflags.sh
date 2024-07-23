#!/bin/sh

CSIMFLAGS=""

while read LINE; do
  if [[ "$LINE" =~ ^-I/ ]]; then
    CSIMFLAGS="$CSIMFLAGS $LINE"
  fi
done <../CSIMFLAGS

LINE=""
