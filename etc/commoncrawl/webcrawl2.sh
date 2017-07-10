#!/bin/bash

# Script to download at least 200MiB of ASCII-only
# plain text extract from commoncrawls.org

target_size=10000
ascii_extract="commoncrawl_${target_size}.ascii"

wet_paths_url="https://commoncrawl.s3.amazonaws.com/crawl-data/CC-MAIN-2016-50/wet.paths.gz"
wet_paths_gz="wet.paths.gz"
wet_paths="wet.paths"
random_path_file="wet.paths.random"

wget -c $wet_paths_url -O $wet_paths_gz
gzip -dc $wet_paths_gz > $wet_paths

sort -R --random-source /dev/zero wet.paths | while read -r line; do
    echo "https://commoncrawl.s3.amazonaws.com/$line"
done > $random_path_file

> $ascii_extract

while read -r line; do
    wet_file_gz=`basename $line`
    wet_file=`basename $wet_file_gz .gz`

    wget -c $line -O $wet_file_gz
    gzip -dc $wet_file_gz > $wet_file

    extract_fragment="${ascii_extract}.${wet_file}"

    ./warc_extract.py $wet_file > $extract_fragment
    cat $extract_fragment >> $ascii_extract

    rm $wet_file
    rm $wet_file_gz
    rm $extract_fragment

    file_size=$((`stat -c %s $ascii_extract`/1024/1024))

    if [ $file_size -ge $target_size ]; then
        break
    fi
done <$random_path_file

rm $wet_paths
rm $wet_paths_gz
rm $random_path_file

#if [ -f $F ]; then
#fi
