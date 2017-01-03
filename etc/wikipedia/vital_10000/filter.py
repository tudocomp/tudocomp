#!/usr/bin/python3

# Given a source file of wiki markup language for a
# vital article list page, extract the names of all referenced articles
#
# this assumes them to be properly tagged with icons that give metadat about the articles quality

import sys
import re
import string
import urllib.parse as ul

filename = sys.argv[1]

#print(filename)
icon_regex = r"\{\{Icon\|(.*?)\}\}"
regex = r"(("+icon_regex+r"\W)+)(?:'''|)\[\[(.*?)\]\](?:'''|)"

textfile = open(filename, 'r')
filetext = textfile.read()
textfile.close()
matches = re.findall(regex, filetext)

for match in matches:
    icons = re.findall(icon_regex, match[0])
    name = match[3]
    name = name.rsplit('|', 1)[0]
    name = ul.quote(name)

    #print(icons)
    print(name)
