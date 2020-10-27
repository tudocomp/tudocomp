#!/usr/bin/env python3
import json
import sys

# read specific stats from ./tdc --stats by piping the output into this script
# change here which stats you want to output:
stats = ['num_factors', 'num_unreplaced', 'num_replaced']

#
#

l = sys.stdin.read()
j = json.loads(l)
dic = dict()
for i in j["data"]["stats"]:
	dic[i['key']] = i['value']


for i in stats:
	if i in dic:
		print(str(i) + "=" + str(dic[i]) + " ", end='')
print()



