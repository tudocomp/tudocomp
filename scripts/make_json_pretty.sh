#!/bin/bash
base="/media/sf_Corpus/auto/"


_dfiles=$(ls $base | grep "lfs2")

scale="scale = 5;"


for f in $_dfiles
do
	
  #  echo "Processing file $base$f "

	file=$(grep -oP '(?<="input": "/media/sf_Corpus/).*' $base$f | sed 's/.50MB",//g' | sed 's!.*/!!g' | sed 's!_!\\_!g')
	in_size=$(grep -oP '(?<="inputSize": ).*' $base$f | grep -oP '[0-9]*')
	out_size=$(grep -oP '(?<="outputSize": ).*' $base$f | grep -oP '[0-9]*') 

	rules=$(grep -A1 'CFG rules' $base$f | grep -oP '(?<="value": ).*' | grep -oP '[0-9]*')

	len=$(grep -A1 '"Bytes Length Encoding",' $base$f | grep -oP '(?<="value": ).*' | grep -oP '[0-9]*')
	nts=$(grep -A1 '"Bytes Non-Terminal Symbol Encoding",' $base$f | grep -oP '(?<="value": ).*' | grep -oP '[0-9]*')
	start=$(grep -A1 '"Bytes Start Symbol Encoding",' $base$f | grep -oP '(?<="value": ).*' | grep -oP '[0-9]*')

	lit_dict=$(grep -A1 '"Literals in Dictionary",' $base$f | grep -oP '(?<="value": ).*' | grep -oP '[0-9]*')
	lit_start=$(grep -A1 '"Literals in Start Symbol",' $base$f | grep -oP '(?<="value": ).*' | grep -oP '[0-9]*')

#	echo "Size $in_size  out $out_size rules $rules"
#	echo "Length $len  $nts  $start"
#	echo "dict $lit_dict start $lit_start"

	len_rel=$(echo "$scale $len / $out_size  * 100"  | bc | awk '{printf("%.2f\n", $1)}' | sed 's/\./,/g')
	#len_rel_com=&(echo "$len_rel" )

	#echo "$len_rel"
	#echo "$len_rel_com"

	nts_rel=$(echo "$scale $nts / $out_size  * 100"  | bc | awk '{printf("%.2f\n", $1)}' | sed 's/\./,/g')
	start_rel=$(echo "$scale $start / $out_size  * 100"  | bc | awk '{printf("%.2f\n", $1)}' | sed 's/\./,/g')


	lit_dict_rel=$(echo "$scale $lit_dict / $in_size  * 100"  | bc | awk '{printf("%.2f\n", $1)}' | sed 's/\./,/g')
	lit_start_rel=$(echo "$scale $lit_start / $in_size  * 100"  | bc | awk '{printf("%.2f\n", $1)}' | sed 's/\./,/g')

	rate=$(echo "$scale $out_size / $in_size  * 100"  | bc | awk '{printf("%.2f\n", $1)}' | sed 's/\./,/g')

	#echo "file: $file"
	#echo "relative len $len_rel"

	echo "\\texttt{$file} & $rate \\% & $rules & $lit_dict_rel \\% & $lit_start_rel \\% & $len_rel \\% & $nts_rel \\% & $start_rel \\% \\\\"
done
