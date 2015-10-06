#! /bin/bash

kPath="./download"

artificial_files=(\
'http://pizzachili.dcc.uchile.cl/repcorpus/artificial/fib41.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/artificial/rs.13.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/artificial/tm29.gz' \
)
real_files=(\
'http://pizzachili.dcc.uchile.cl/texts/code/sources.200MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/code/sources.50MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/xml/dblp.xml.50MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/music/pitches.50MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/protein/proteins.200MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/protein/proteins.50MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/dna/dna.200MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/dna/dna.50MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/nlang/english.1024MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/xml/dblp.xml.200MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/nlang/english.50MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/nlang/english.100MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/nlang/english.200MB.gz' \
'http://pizzachili.dcc.uchile.cl/texts/nlang/english.1024MB.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/real/Escherichia_Coli.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/real/cere.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/real/coreutils.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/real/einstein.de.txt.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/real/einstein.en.txt.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/real/influenza.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/real/kernel.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/real/para.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/real/world_leaders.gz' \
)
pseudoreal_files=(\
'http://pizzachili.dcc.uchile.cl/repcorpus/pseudo-real/proteins.001.1.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/pseudo-real/sources.001.2.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/pseudo-real/english.001.2.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/pseudo-real/dna.001.1.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/pseudo-real/dblp.xml.0001.2.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/pseudo-real/dblp.xml.0001.1.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/pseudo-real/dblp.xml.00001.2.gz' \
'http://pizzachili.dcc.uchile.cl/repcorpus/pseudo-real/dblp.xml.00001.1.gz' \
)
xml_files=(\
'http://dblp.uni-trier.de/xml/dblp.xml.gz' \
)

function download {
    kSubPath="$kPath/$1"
    mkdir -p "$kSubPath"
    kArrayName="${1}_files[@]"
    fileArray=${!kArrayName}
    for url in ${fileArray}; do
        current_file="$kSubPath/"$(basename "$url")
        current_file_extracted="$kSubPath/"$(basename "$url" ".gz")
        [[ -e ${current_file_extracted} ]] || wget -P "$kSubPath" "$url" --continue
        [[ -e ${current_file_extracted} ]] || gunzip -k ${current_file}&
    done
}

for t in 'real' 'artificial' 'pseudoreal' 'xml'; do
    download "$t"
done
