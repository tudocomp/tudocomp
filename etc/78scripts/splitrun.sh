#!/bin/zsh
# LZ78 compression into multiple files, where each compressed file can store a specified maximum limit of factors.
# needs  a patch for /include/tudocomp/compressors/LZ78Compressor.hpp such that it terminates instead of clearing the dictionary and continuing
# standard decompression won't work

function die {
    echo "$1" >&2
    exit 1
}

infiles=(
# /scratch/data/78/english.1MB 100000
# /scratch/data/78/wikipedia 17438145
# /scratch/data/78/dblp.xml 10505674
/scratch/data/78/commoncrawl 401297922
# /scratch/data/78/gutenberg 37105835
# /scratch/data/78/english.1024MB 68072997
/scratch/data/78/est.fa 126079783
# /scratch/data/78/proteins 109823857
	)

cmake .. -DSTATS_DISABLED=0 -DTDC_REGISTRY=../etc/registries/lz78tries.py
make

function readlog() {
	logFile=$1
	[[ ! -r $logFile ]] && die "cannot read log file $1"
	statlength=$(jq '.["data"]["sub"][0]["stats"] | length' ${logFile})
	for i in $(seq 0 $statlength); do
		key=$(jq '.["data"]["sub"][0]["stats"]['$i']["key"]' $logFile)
		value=$(jq '.["data"]["sub"][0]["stats"]['$i']["value"]' $logFile)
		case $key in
			"\"remaining_characters\"")
				echo -n "remaining=$value "
				remaining_characters=$value
				;;
			"\"factor_count\"")
				echo -n "z=$value "
				;;
		esac
	done
	echo ''
}

((infile_it=1))
while [[ $infile_it -lt $#infiles ]]; do
#
# for _infile in $infiles; do
	infile=$(readlink -f $infiles[$infile_it])
	filename=$(basename $infile)
	dict_size=$infiles[$(expr $infile_it + 1)]


	comppressedFile=$(mktemp  -p /scratch/tmp --suffix .${filename}.comp)
	logCompFile=$(mktemp -p /scratch/tmp --suffix .${filename}.comp.log )
	statsCompFile=$(mktemp -p /scratch/tmp --suffix .${filename}.comp.json )
	remainingFile=$(mktemp -p /scratch/tmp --suffix .${filename}.dat )
	fileSize=$(stat --format="%s" "$infile")


	offset=0
	while [ 1 ]; do 
		./a.out "$infile" "$remainingFile" $offset
		prefix=$(stat --format="%s" "$remainingFile")
		stats="file=${filename} n=${prefix} offset=${offset} algo=splitbinary "
		set -x 
		/usr/bin/time --format='Wall Time: %e'  ./tdc ${remainingFile} -p $prefix -a "lz78(coder=binary,lz78trie=binary,dict_size=$dict_size)" -o ${comppressedFile} -f --stats --statfile "$statsCompFile" > "$logCompFile" 2>&1
		set +x
		echo -n "RESULT action=compression compressedsize=$(stat --format="%s" $comppressedFile) $stats "
		remaining_characters=0
		readlog "$statsCompFile"
		echo "Remaining characters: $remaining_characters"
		[[ $remaining_characters -eq 0 ]] && break
		((offset=fileSize-remaining_characters))
		echo "Setting offset to $offset"
	done
	rm $comppressedFile $logCompFile $statsCompFile $remainingFile
	((infile_it+=2))
done




