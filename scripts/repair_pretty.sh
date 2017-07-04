#!/bin/bash
base="/media/sf_Corpus/"
_dfiles="dblp.xml.50MB
dna.50MB
english.50MB
proteins.50MB
sources.50MB
pitches"

#$(ls $b | grep -e '.50MB' | grep -v -e  '\.[tsRCMmc]')

	for f in $_dfiles
	do
        echo "Processing $f file"
		#echo "output /media/sf_Corpus/auto/$f.reapir "

		START=$(date +%s.%N)
		output=$(repair $base$f 2>&1)
		END=$(date +%s.%N)
		DIFF=$(echo "$END - $START" | bc | awk '{printf("%.1f\n", $1)}' | sed 's/\./,/g')

		
		ratio=$(echo $output | grep -oP '(?<=ratio: ).*' | sed 's/\./,/g' | sed 's/\%/ \\%/g')

		valgrind -q --tool=massif --pages-as-heap=yes --massif-out-file=$base$f.massif repair $base$f &> /dev/null

		mems=$(grep -oP '(?<=mem_heap_B=).*' < $base$f.massif 2>&1)

		max=0
		for m in $mems
		do
			((m > max)) && max=$m
		done

		START2=$(date +%s.%N)
		output2=$(repair $base$f 2>&1)
		END2=$(date +%s.%N)
		DIFF2=$(echo "$END2 - $START2" | bc | awk '{printf("%.1f\n", $1)}' | sed 's/\./,/g')


		valgrind -q --tool=massif --pages-as-heap=yes --massif-out-file=$base$f.massif despair $base$f &> /dev/null

		mems=$(grep -oP '(?<=mem_heap_B=).*' < $base$f.massif 2>&1)

		max2=0
		for m in $mems
		do
			((m > max2)) && max2=$m
		done

		#cleanup
		rm $base$f.massif
		rm $base$f.C
		rm $base$f.R

		echo "repair(navarro) &  ${DIFF}s & $(( ${max} / 1024 / 1024))MiB  &  $ratio &  ${DIFF2}s & $(( ${max2} / 1024 / 1024)) MiB \\\\"
	done
