#!/bin/bash
set -e
./tdc -a "$1" --stats -f "$2" -o roundtrip.tdc
./tdc -d -a "$1" --stats -f roundtrip.tdc -o "$2.decomp"
diff -s "$2" "$2.decomp"
