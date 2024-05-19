#!/bin/sh

LDFLAGS=""

while read LINE; do
  LDFLAGS="$LDFLAGS $LINE"
done <../LDFLAGS

LINE=""
