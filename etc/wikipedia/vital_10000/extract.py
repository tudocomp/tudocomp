#!/usr/bin/env python3

# Given a source file of wiki html language for a
# vital article list page, extract the names of all referenced articles

import sys
import re
import string
import urllib.parse as ul

filename = sys.argv[1]

#print(filename)
regex = r"\"/wiki/(.*?)\""

textfile = open(filename, 'r')
filetext = textfile.read()
textfile.close()
matches = re.findall(regex, filetext)

filter = re.compile(r"[a-zA-Z_]*?:")

names = set()

for match in matches:
    if not filter.match(match):
        #print(match)
        names.add(match)

for name in names:
    print(name)
