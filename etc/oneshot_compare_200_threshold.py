#!/usr/bin/python3

files = [
    "../etc/datasets/pizza_chili/dblp.xml.200MB",
    "../etc/datasets/pizza_chili/dna.200MB",
    "../etc/datasets/pizza_chili/english.200MB",
    "../etc/datasets/pizza_chili/proteins.200MB",
    "../etc/datasets/pizza_chili/sources.200MB",
    "../etc/datasets/tagme/wiki-disamb30.200MB",
    "../etc/datasets/wiki/all_vital.txt.200MB",
]

cmd = "esacomp(code2, threshold=%1);.tdc;tdc;--algorithm=esacomp(coder=code2,threshold='%1');"

for i in range(1, 41):
    s = cmd.replace("%1", str(i))
    print(s)
