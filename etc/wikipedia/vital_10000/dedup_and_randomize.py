#!/usr/bin/env python3

import fileinput
import random

lines_set = set()

for line in fileinput.input():
    lines_set.add(line[:-1])

lines_list = []

for line in lines_set:
    lines_list.append(line)

random.shuffle(lines_list)

for line in lines_list:
    print(line)
