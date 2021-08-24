#!/bin/zsh

function die {
    echo "$1" >&2
    exit 1
}

[[ $# -eq 2 ]] || die "Usage: $0 [dataset-folder] [tmp-folder]"

typeset -rx kScriptDir=$(dirname $(readlink -f "$0"))
typeset -rx kDatasetFolder=$(readlink -f "$1")
typeset -rx kTmpFolder=$(readlink -f "$2")

[[ -r $kDatasetFolder ]] || die "Cannot read $kDatasetFolder"

set -x
set -e

for _infile in $kDatasetFolder/*; do
	filename=$(basename $_infile)
	infile=$(readlink -f "$_infile")
	comppressedFile=$(mktemp  -p $kTmpFolder --suffix .${filename}.comp) 
	uncompressedFile=$(mktemp -p $kTmpFolder --suffix .${filename}.dec )
	prefix=$(stat --format="%s" $infile)
	stats="file=${filename} tabletype=${tabletype} indextype=${indextype} factor=${factor} n=${prefix}"
	set -x
	/usr/bin/time --format='Wall Time: %e' compress -c ${infile} > ${comppressedFile}
	set +x
	echo -n "RESULT action=compression program=compress compressedsize=$(stat --format="%s" $comppressedFile) $stats "

	set -x
	/usr/bin/time --format='Wall Time: %e' uncompress -c $comppressedFile > ${uncompressedFile} 
	set +x
	cmp --silent $uncompressedFile $infile; checkDecomp="$?"
	echo -n "RESULT action=decompression program=compress check=${checkDecomp} $stats "
	if [[ $checkDecomp -ne 0 ]]; then
		echo "files $infile and $uncompressedFile are different. Compressed file: $comppressedFile"
	else
		rm $comppressedFile $uncompressedFile 
	fi
done
