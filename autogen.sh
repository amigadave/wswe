#!/bin/sh -e
 
autoreconf --install
./configure "$@"
