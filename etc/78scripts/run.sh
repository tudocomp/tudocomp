#!/bin/zsh

infiles=(/scratch/data/78/dblp.xml /scratch/data/78/english.1024MB /scratch/data/78/est.fa  /scratch/data/78/proteins)
sed -i 's@^CMAKE_BUILD_TYPE:STRING=.*@CMAKE_BUILD_TYPE:STRING=Release@' CMakeCache.txt

algos=(
'binary' 'lz78(coder=binary,lz78trie=binary)'
'chain' 'lz78(coder=binary,lz78trie=chain)'
'compact'    'lz78(coder=binary,lz78trie=compact_sparse_hash(load_factor=95, compact_hash_strategy=sparse_cv))'
'compact_k'  'lz78(coder=binary,lz78trie=compact_sparse_hash(load_factor=95, compact_hash_strategy=no_k_grow(sparse_cv)))'
'compact_kv' 'lz78(coder=binary,lz78trie=compact_sparse_hash(load_factor=95, compact_hash_strategy=no_kv_grow(sparse_cv)))'
)

# for setting in algos; do
# 	./tdc -a 'lz78(coder=binary,lz78trie=compact_sparse_hash(load_factor=0.95, compact_hash_strategy=no_k_grow(sparse_cv)))' /scratch/data/78/english.1024MB  -o /dev/null -f --stats


#infiles=('/scratch/data/78/english.1024MB')

timePattern='^Wall Time:\s\+\([0-9]\+\)'

make
for infile in $infiles; do
	((algo_it=1))
	while [[ $algo_it -lt $#algos ]]; do
		algo_id=$algos[$algo_it]
		algo_cmd=$algos[$(expr $algo_it + 1)]
		# echo $algo_id $algo_cmd
		echo ''

		filename=$(basename $infile)
		comppressedFile=$(mktemp  -p /scratch/tmp --suffix .${filename}.comp) 
		uncompressedFile=$(mktemp -p /scratch/tmp --suffix .${filename}.dec )
		logCompFile=$(mktemp -p /scratch/tmp --suffix .${filename}.log.comp )
		logDecFile=$(mktemp -p /scratch/tmp --suffix .${filename}.log.dec )
		statsFile=$(mktemp -p /scratch/tmp --suffix .${filename}.log.json )
		prefix=$(stat --format="%s" $infile)
#		prefix=$(calc -p '200*1024*1024')
		stats="file=${filename} n=${prefix} algo=${algo_id} "
		set -x
		/usr/bin/time --format='Wall Time: %e'  ./tdc ${infile} -p $prefix -a "${algo_cmd}" -o ${comppressedFile} -f --stats --statfile "$statsFile" > "$logCompFile" 2>&1
		set +x
		echo -n "RESULT action=compression compressedsize=$(stat --format="%s" $comppressedFile) $stats "
		t=$(grep $timePattern $logCompFile | sed "s@${timePattern}@\1@")
		echo -n "time=$t "
		./readlog.sh "$statsFile"
		echo ''


		set -x
		/usr/bin/time --format='Wall Time: %e' ./tdc -d $comppressedFile -o ${uncompressedFile} -f > "$logDecFile"  2>&1
		set +x
		cmp -n ${prefix} --silent $uncompressedFile $infile; checkDecomp="$?"
		echo -n "RESULT action=decompression check=${checkDecomp} $stats "
		echo -n "time=$(grep $timePattern $logDecFile | sed "s@${timePattern}@\1@") "
		# ./readlog.sh "$logDecFile"
		echo ''
		if [[ $checkDecomp -ne 0 ]]; then
			echo "files $infile and $uncompressedFile are different. Compressed file: $comppressedFile" >&2
		else
			rm $comppressedFile $uncompressedFile $logCompFile $logDecFile $statsFile
		fi

		((algo_it+=2))
	done
done

	# sed -i "s@^#define BONSAI_HASH_TABLE 0@#define BONSAI_HASH_TABLE ${tabletype}@" ../include/SLZ78/defs.h
	# cat ../include/SLZ78/defs.h
	# make
	# for indextype in $indextypes; do 
	# 		for factor in $factors; do 
	# 		done #factor
	# 	done #infile
	# done #indextype
# done #tabletype
