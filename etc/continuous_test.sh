#!/bin/zsh
# Run continous random tests on the compressor specified in the function check
# Aborts when the decompression does not restore the original input
# change kSize to the number of generated characters

##### EDIT
local -r kSize=1000
function check {
#./tdc -a 'lcpcomp(ascii,comp=plcppeaks,dec=compact)' dna.random -o dna.random.tdc --force && 
#./tdc -a 'lcpcomp(ascii,comp=heap,dec=compact)' dna.random -o dna.random.tdc --force && 
#./tdc -a 'lcpcomp(ascii,comp=plcp,dec=compact)' dna.random -o dna.random.tdc --force && 
#./tdc -a 'lz78(ascii,monte(hash_roller = rk))' dna.random -o dna.random.tdc --force && 
./tdc -a 'lz78(ascii,ternary)' dna.random -o dna.random.tdc --force && 
#./tdc -a 'lz78(ascii,myhash)' dna.random -o dna.random.tdc --force && 
#./tdc -a 'lz78u(streaming(ascii),ascii)' dna.random -o dna.random.tdc --force && 
./tdc --decompress dna.random.tdc --output dna.random.orig --force && 
diff -q dna.random dna.random.orig ||
die "Sizes differ"
#echo "terminate called after throwing an instance"
}
##### EDIT END


function die {
echo "$1"
echo -n $'\a'
exit 1
}

function genDNA {
	chars=acgt
	num="$1"
	for i in $(seq 1 "$num") ; do
		echo -n ${chars:RANDOM%${#chars}:1}
	done
	echo
}




wheel="_/\\|"
wheeli=1
it=1


while [ 1 ]; do 
make -s tudocomp_driver >/dev/null&&
./genDNA.sh $kSize > dna.random &&
timeA=$(date +%s%N | cut -b1-13)
check dna.random 
timeB=$(date +%s%N | cut -b1-13)
echo -e -n "\r    \r$t "
if [[ $? -ne 0 ]]; then
	it=0
echo -n $'\a'
	echo -n ':-( '
fi
echo -n $wheel[$wheeli] $it $(expr $timeB - $timeA)ms 
 ((wheeli = (wheeli % 4) + 1))
 ((++it))
 done

