#!/bin/bash

valgrind --tool=massif --pages-as-heap=yes --massif-out-file=massif.out ./test.sh;
grep mem_heap_B massif.out | sed -e 's/mem_heap_B=\(.*\)/\1/' | sort -g | tail -n 1

# --pages-as-heap=yes
# --detailed-freq=1
