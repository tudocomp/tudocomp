#!/bin/bash
base="/media/sf_Corpus/"
_dfiles="pitches
english.50MB
dblp.xml.50MB
dna.50MB
pitches
sources.50MB
"

#english.50MB
#dblp.xml.50MB
#dna.50MB
#pitches
#sources.50MB

_algorithms="lfs_comp(sim_st)
lfs_comp(bst)
lfs2bst
lfs2
lfs_comp(esa)
"

for a in $_algorithms
do
	for f in $_dfiles
	do
        echo "Processing $f file"
		echo "tdc $a  auf $base$f"
		echo "output /media/sf_Corpus/auto/$a$f.json "
		tdc -s -f -a "$a" "$base$f" > /media/sf_Corpus/auto/$a$f.json 
	done
done