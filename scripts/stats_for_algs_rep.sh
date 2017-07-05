#!/bin/bash
base="/media/sf_Corpus/repetetiv_real/"
_dfiles="cere.50MB
einstein.de.txt.50MB
einstein.en.txt.50MB
Escherichia_Coli.50MB
influenza.50MB
kernel.50MB
para.50MB
world_leaders.50MB
"

_algorithms="lfs_comp(sim_st)
lfs_comp(bst)
lfs2bst
lfs2
lfs_comp(esa)"

for a in $_algorithms
do
	for f in $_dfiles
	do
        echo "Processing $f file"
		echo "tdc $a  auf $base$f"
		echo "output /media/sf_Corpus/auto/$a$f.json "
		tdc -s -f -a "$a" "$base$f" > /media/sf_Corpus/auto_real/$a$f.json 
	done
done