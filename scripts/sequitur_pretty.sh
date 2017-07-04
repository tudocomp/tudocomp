#!/bin/bash
base="/media/sf_Corpus/"
_dfiles="english.1MB
"

	for f in $_dfiles
	do
        echo "Processing $f file"
		#echo "output /media/sf_Corpus/auto/$f.reapir "

		START=$(date +%s.%N)
		sequitur -c < $base$f > $base$f.seq 2> /dev/null
		END=$(date +%s.%N)
		DIFF=$(echo "$END - $START" | bc | awk '{printf("%.1f\n", $1)}' | sed 's/\./,/g')

		in_size=$(stat --printf="%s" $base$f)
		out_size=$(stat --printf="%s" $base$f.seq)

		ratio=$(echo "scale=10; $out_size / $in_size * 100 " | bc | awk '{printf("%.2f\n", $1)}' | sed 's/\./,/g')
	

		valgrind -q --tool=massif --pages-as-heap=yes --massif-out-file=$base$f.massif sequitur -c < $base$f  &> /dev/null

		mems=$(grep -oP '(?<=mem_heap_B=).*' < $base$f.massif 2>&1)

		max=0
		for m in $mems
		do
			((m > max)) && max=$m
		done

		START2=$(date +%s.%N)
		sequitur -u < $base$f.seq > /dev/null &> /dev/null
		END2=$(date +%s.%N)
		DIFF2=$(echo "$END2 - $START2" | bc | awk '{printf("%.1f\n", $1)}' | sed 's/\./,/g')


		valgrind -q --tool=massif --pages-as-heap=yes --massif-out-file=$base$f.massif sequitur -u < $base$f.seq &> /dev/null

		mems=$(grep -oP '(?<=mem_heap_B=).*' < $base$f.massif 2>&1)

		max2=0
		for m in $mems
		do
			((m > max2)) && max2=$m
		done

		#cleanup
		rm $base$f.massif
		rm $base$f.seq

	

		echo "sequitur &  ${DIFF}s & $(( ${max} / 1024 / 1024))MiB  &  $ratio \\% &  ${DIFF2}s & $(( ${max2} / 1024 / 1024))MiB \\\\"
	done
