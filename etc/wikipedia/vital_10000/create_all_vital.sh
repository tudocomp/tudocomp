#!/bin/bash

for f in *.html; do
    ./extract.py $f
done | ./dedup_and_randomize.py > $1
wc -l $1
