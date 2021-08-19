#!/bin/zsh

function die {
	echo "$1" >&2
	exit 1
}

[[ $# -eq 1 ]] || die "Usage: $0 [dataset-folder]"

typeset -rx scriptDir=$(dirname $(readlink -f "$0"))

# [[ -r $tudocompFolder/CMakeLists.txt ]] || die "$tudocompFolder seems not to be the root folder of tudocomp"

local -r datasetFolder=$(readlink -f "$1")
mkdir -p $datasetFolder || die "Cannot create $datasetFolder"

# local -r tudocompFolder=$2

local -r kInFileNames=(
english.1
wikipedia
xml
commoncrawl 
fibonacci
gutenberg 
proteins
english
dna
)

local -r kDownloadFiles=(
dna est.fa.xz
wikipedia all_vital.txt.xz
xml pc/dblp.xml.xz
english pc/english.xz
commoncrawl commoncrawl_10240.ascii.xz
fibonacci pc-artificial/fib46.xz
gutenberg gutenberg-201209.24090588160.xz
proteins pc/proteins.xz
)

set -x
set -e

oldpwd=$(pwd)
cd "$datasetFolder"
needDownload=0
for filename in $kInFileNames; do
	if [[ ! -r $filename ]]; then
		needDownload=1
		break
	fi
done
if [[ $needDownload -eq 1 ]]; then
	((downloadCounter=-1))
	while
		((downloadCounter+=2))
		[[ $downloadCounter -gt $#kDownloadFiles ]] && break
		[[ -r "$datasetFolder/"$kDownloadFiles[$downloadCounter] ]] && continue
		downloadfile=$kDownloadFiles[$downloadCounter+1]
		wget --continue "http://dolomit.cs.tu-dortmund.de/tudocomp/$downloadfile"
		unxz -k $(basename $downloadfile)
	do :; done
	truncate -s 1G english
	dd if=english of=english.1 bs=1M count=1
	[[ ! -e dna ]] && ln -sv est.fa dna
	[[ ! -e wikipedia ]] && ln -sv all_vital.txt wikipedia
	dd if=gutenberg-201209.24090588160 of=gutenberg bs=1000000000 count=1
	[[ ! -e commoncrawl ]] && ln -sv commoncrawl.ascii commoncrawl
	[[ ! -e fibonacci ]] && ln -sv fib46 fibonacci
	[[ ! -e xml ]] && ln -sv dblp.xml xml 
	# ./tdc -g 'fib(46)' --usestdout >! fib
fi
cd "$oldpwd"


useDecompression=1

cd $scriptDir/../..
mkdir -p build
cd build
cmake .. -DSTATS_DISABLED=0 -DTDC_REGISTRY=../etc/registries/lz78tries.py
sed -i 's@^CMAKE_BUILD_TYPE:STRING=.*@CMAKE_BUILD_TYPE:STRING=Release@' CMakeCache.txt
cmake .. -DSTATS_DISABLED=0 -DTDC_REGISTRY=../etc/registries/lz78tries.py

# either with or without stats
#cmake .. -DTDC_REGISTRY=../etc/registries/lz78tries.py
#cmake .. -DSTATS_DISABLED=0 -DTDC_REGISTRY=../etc/registries/lz78tries.py

#cmake .. -DSTATS_DISABLED=1 -DTDC_REGISTRY=../etc/registries/lzpacked.py

# cmake .. -DSTATS_DISABLED=0 -DTDC_REGISTRY=../etc/registries/lzpacked.py


# 'binaryPN' 'lz78packednode(coder=binary,lz78trie=binary)'
# 'binarykPN' 'lz78packednode(coder=binary,lz78trie=binaryk)'
# 'chain0PN' 'lz78packednode(coder=binary,lz78trie=chain0)'
# 'binaryP' 'lz78packed(coder=binary,lz78trie=binary)'
# 'binarykP' 'lz78packed(coder=binary,lz78trie=binaryk)'
#'cedar' 'lz78(coder=binary,lz78trie=cedar)'

algos=(
'binaryMTF' 'lz78(coder=binary,lz78trie=binaryMTF)'
'wbinaryMTF' 'lzw(coder=binary,lz78trie=binaryMTF)'
#  'plainchain' 'lz78(coder=binary,lz78trie=plainchain)'
#  'wplainchain' 'lzw(coder=binary,lz78trie=plainchain)'
 'binary' 'lz78(coder=binary,lz78trie=binary)'
 'wbinary' 'lzw(coder=binary,lz78trie=binary)'
 'binarysorted' 'lz78(coder=binary,lz78trie=binarysorted)'
 'wbinarysorted' 'lzw(coder=binary,lz78trie=binarysorted)'
 'wrolling128' 'lzw(coder=binary,lz78trie=rolling(hash_roller=rk0))'
 'wrolling128plus' 'lzw(coder=binary,lz78trie=rolling_plus(hash_roller=rk0))'
 'wchain30' 'lzw(coder=binary,lz78trie=chain30)'
 'wexthash' 'lzw(coder=binary,lz78trie=exthash)'
 'rolling128' 'lz78(coder=binary,lz78trie=rolling(hash_roller=rk0))'
 'rolling128plus' 'lz78(coder=binary,lz78trie=rolling_plus(hash_roller=rk0))'
 'chain30' 'lz78(coder=binary,lz78trie=chain30)'
 'exthash' 'lz78(coder=binary,lz78trie=exthash)'
 # LZ packed
 'chain0P' 'lz78packed(coder=binary,lz78trie=chain0)'
 'compact_kvP' 'lz78packed(coder=binary,lz78trie=compact_sparse_hash(load_factor=95, compact_hash_strategy=no_kv_grow(sparse_cv)))'
 'binarykP' 'lz78packed(coder=binary,lz78trie=binaryk)'
 #chain
 'chain0' 'lz78(coder=binary,lz78trie=chain0)'
 'chain10' 'lz78(coder=binary,lz78trie=chain10)'
 'chain20' 'lz78(coder=binary,lz78trie=chain20)'
 #compact
 'compact'    'lz78(coder=binary,lz78trie=compact_sparse_hash(load_factor=95, compact_hash_strategy=sparse_cv))'
 'compact_k'  'lz78(coder=binary,lz78trie=compact_sparse_hash(load_factor=95, compact_hash_strategy=no_k_grow(sparse_cv)))'
 'compact_kv' 'lz78(coder=binary,lz78trie=compact_sparse_hash(load_factor=95, compact_hash_strategy=no_kv_grow(sparse_cv)))'
 'binaryk' 'lz78(coder=binary,lz78trie=binaryk)'
 'ternary' 'lz78(coder=binary,lz78trie=ternary)'
 'hash' 'lz78(coder=binary,lz78trie=hash)'
 'hashplus' 'lz78(coder=binary,lz78trie=hash_plus)'
 'rolling' 'lz78(coder=binary,lz78trie=rolling)'
 'rollingplus' 'lz78(coder=binary,lz78trie=rolling_plus)'
 ##LZW
 #chain
 'wchain0' 'lzw(coder=binary,lz78trie=chain0)'
 'wchain10' 'lzw(coder=binary,lz78trie=chain10)'
 'wchain20' 'lzw(coder=binary,lz78trie=chain20)'
 #compact
 'wcompact'    'lzw(coder=binary,lz78trie=compact_sparse_hash(load_factor=95, compact_hash_strategy=sparse_cv))'
 'wcompact_k'  'lzw(coder=binary,lz78trie=compact_sparse_hash(load_factor=95, compact_hash_strategy=no_k_grow(sparse_cv)))'
 'wcompact_kv' 'lzw(coder=binary,lz78trie=compact_sparse_hash(load_factor=95, compact_hash_strategy=no_kv_grow(sparse_cv)))'
 'wternary' 'lzw(coder=binary,lz78trie=ternary)'
 'wbinaryk' 'lzw(coder=binary,lz78trie=binaryk)'
 'whash' 'lzw(coder=binary,lz78trie=hash)'
 'whashplus' 'lzw(coder=binary,lz78trie=hash_plus)'
 'wrolling' 'lzw(coder=binary,lz78trie=rolling)'
 'wrollingplus' 'lzw(coder=binary,lz78trie=rolling_plus)'
)

[[ -r /usr/include/Judy.h ]] &&
algos+=(
 'judy' 'lz78(coder=binary,lz78trie=judy)'
 'wjudy' 'lzw(coder=binary,lz78trie=judy)'
)


# for setting in algos; do
# 	./tdc -a 'lz78(coder=binary,lz78trie=compact_sparse_hash(load_factor=0.95, compact_hash_strategy=no_k_grow(sparse_cv)))' /scratch/data/78/english.1024MB  -o /dev/null -f --stats


#infiles=('/scratch/data/78/english.1MB')

timePattern='^Wall Time:\s\+\([0-9]\+\)'


make
for filename in $kInFileNames; do
	infile=$(readlink -f "$datasetFolder/$filename")

	[[ -r $infile ]] || die "cannot read $infile!"

	((algo_it=1))
	while [[ $algo_it -lt $#algos ]]; do
		algo_id=$algos[$algo_it]
		algo_cmd=$algos[$(expr $algo_it + 1)]
		# echo $algo_id $algo_cmd
		echo ''

		comppressedFile=/dev/null
		[[ $useDecompression -eq 1 ]] && comppressedFile=$(mktemp  -p /scratch/tmp --suffix .${filename}.comp)  ##TODO: this is a hack
		logCompFile=$(mktemp -p /scratch/tmp --suffix .${filename}.comp.log )
		statsCompFile=$(mktemp -p /scratch/tmp --suffix .${filename}.comp.json )
		prefix=$(stat --format="%s" "$infile")
		# prefix=$(calc -p '200*1024*1024')
		stats="file=${filename} n=${prefix} algo=${algo_id} "
		set -x
		/usr/bin/time --format='Wall Time: %e'  ./tdc ${infile} -p $prefix -a "${algo_cmd}" -o ${comppressedFile} -f --stats --statfile "$statsCompFile" > "$logCompFile" 2>&1
		set +x
		echo -n "RESULT action=compression compressedsize=$(stat --format="%s" $comppressedFile) $stats "
		t=$(grep $timePattern $logCompFile | sed "s@${timePattern}@\1@")
		echo -n "time=$t "
		"$scriptDir"/readlog.sh "$statsCompFile"
		echo ''

		if [[ $useDecompression -eq 1 ]]; then
			logDecFile=$(mktemp -p /scratch/tmp --suffix .${filename}.dec.log )
			uncompressedFile=$(mktemp -p /scratch/tmp --suffix .${filename}.dec )
			statsDecFile=$(mktemp -p /scratch/tmp --suffix .${filename}.dec.json )

			set -x
			/usr/bin/time --format='Wall Time: %e' ./tdc -d $comppressedFile -o ${uncompressedFile} -f --stats --statfile "$statsDecFile" > "$logDecFile"  2>&1
			set +x
			cmp -n ${prefix} --silent $uncompressedFile $infile; checkDecomp="$?"
			echo -n "RESULT action=decompression check=${checkDecomp} $stats "
			echo -n "time=$(grep $timePattern $logDecFile | sed "s@${timePattern}@\1@") "
			"$scriptDir"/readlog.sh "${statsDecFile}"
			echo ''
			if [[ $checkDecomp -ne 0 ]]; then
				echo "files $infile and $uncompressedFile are different. Compressed file: $comppressedFile" >&2
			else
				rm $comppressedFile $uncompressedFile $logCompFile $logDecFile $statsCompFile $statsDecFile
			fi
		else
			rm $logCompFile $statsCompFile
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
