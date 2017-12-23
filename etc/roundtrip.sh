#!/bin/bash
set -e
rm -f roundtrip.tdc
./tdc -a "$1" --stats "$2" -o roundtrip.tdc
rm -f "$2.decomp"
./tdc -d -a "$1" --stats roundtrip.tdc -o "$2.decomp"
diff -s "$2" "$2.decomp"
rm -f "$2.decomp"
