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

	for f in $_dfiles
	do
        echo "Processing $f file"
		echo "output /media/sf_Corpus/auto/$a$f.compare "
		python3 ../etc/compare.py "$base$f" > /media/sf_Corpus/auto_real/$f.compare
	done
