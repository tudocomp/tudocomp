#!/bin/bash
base="/media/sf_Corpus/"
_dfiles="proteins.50MB
"
#english.50MB
#dblp.xml.50MB
#dna.50MB
#pitches
#sources.50MB

	for f in $_dfiles
	do
        echo "Processing $f file"
		echo "output /media/sf_Corpus/auto/$f.compare "
		python3 ../etc/compare.py "$base$f" > /media/sf_Corpus/auto/compare/$f.compare
	done
