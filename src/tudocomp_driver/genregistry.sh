#!/bin/bash



online_coder=(
    ASCIICoder
    BitCoder
    EliasGammaCoder
    EliasDeltaCoder
)
offline_coder=(
    Code2Coder
    HuffmanCoder
)
all_coder=(
	${online_coder[@]}
	${offline_coder[@]}
)
lz78_trie=(
    lz78::BinarySortedTrie
    lz78::BinaryTrie
    lz78::HashTrie
    lz78::MyHashTrie
    lz78::TernaryTrie
    lz78::CedarTrie
)
grep -q '^#define JUDY_H_AVAILABLE 1' ../../../include/tudocomp/config.h && lz78_trie+=(lz78::JudyTrie)

esa_strat=(
    esacomp::MaxHeapStrategy
    esacomp::MaxLCPStrategy
    esacomp::LazyListStrategy
)
esa_buffer=(
    esacomp::MyMapBuffer
    esacomp::MultimapBuffer
    esacomp::SuccinctListBuffer
    esacomp::DecodeForwardQueueListBuffer
)
esa_coder=(
    ASCIICoder
    Code2Coder
)

function ex {
	local -a 'arr=("${!'"$1"'[@]}")'
	local ins="$1"'[arg]'
	for arg in ${arr[@]}; do
		echo "${!ins}"
	done
}

function regr {
local prefix="$1"
shift
local -a 'args'
if [[ -n "$1" ]]; then
	param="$1"
	shift
	for p in $(ex "$param"); do
		regr "$prefix,$p" $@
	done
else
	echo "$prefix"
fi
}


function reg {
local compressor="$1"
shift
if [[ $# -eq 0 ]]; then
	echo "  r.register_compressor<$compressor>();"
elif [[ "$@" == "_" ]]; then
    echo "  r.register_compressor<$compressor<>>();"
else
	regr '' $@ |
	while read line; do
		echo "  r.register_compressor<$compressor<${line:1}>>();"
	done
fi
echo ''
}


cat <<EOF
#include <tudocomp/tudocomp.hpp>
#include <tudocomp_driver/Registry.hpp>

namespace tdc_algorithms {

using namespace tdc;

void register_algorithms(Registry& r);

// One global instance for the registry
Registry REGISTRY = Registry::with_all_from(register_algorithms);

void register_algorithms(Registry& r) {

EOF



reg LiteralEncoder              all_coder
reg LZ78Compressor              online_coder lz78_trie
reg LZWCompressor               online_coder lz78_trie
reg RePairCompressor            all_coder
reg LZSSLCPCompressor           all_coder
reg ESACompressor               esa_coder esa_strat esa_buffer
reg LZSSSlidingWindowCompressor online_coder
reg RunLengthEncoder            online_coder
reg EasyRLECompressor
reg MTFCompressor
reg BWTCompressor               _
reg ChainCompressor
reg NoopCompressor



cat <<EOF
}//function register_algorithms

}//ns

EOF

