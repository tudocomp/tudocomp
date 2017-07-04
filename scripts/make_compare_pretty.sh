#!/bin/bash
base="/media/sf_Corpus/auto/compare/"


_dfiles=$(ls $base | grep -v '.pretty')


for f in $_dfiles
do

	echo "Processing file $base$f "
	tr '.' ',' < $base$f | tr '|' '&' | sed -r  's/%/ \\%/g' | sed -r  's/_/\\_/g' | sed -r  's/---[\-]+/\\hline/g' | sed -r  's/&   OK &/\\\\/g' > $base$f.pretty

done
	