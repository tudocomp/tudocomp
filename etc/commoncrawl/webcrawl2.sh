#!/bin/bash

# Script to download at least 200MiB of ASCII-only
# plain text extract from commoncrawls.org

target_size=10240
ascii_extract="commoncrawl_${target_size}.ascii"
non_ascii_extract="commoncrawl_${target_size}.non_ascii"
both_extract="commoncrawl_${target_size}.both"
all_extract="commoncrawl_${target_size}.all"

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
> $non_ascii_extract
> $both_extract
> $all_extract

while read -r line; do
    wet_file_gz=`basename $line`
    wet_file=`basename $wet_file_gz .gz`

    wget -c $line -O $wet_file_gz
    gzip -dc $wet_file_gz > $wet_file

    ascii_extract_fragment="${ascii_extract}.${wet_file}"
    non_ascii_extract_fragment="${non_ascii_extract}.${wet_file}"
    both_extract_fragment="${both_extract}.${wet_file}"
    all_extract_fragment="${all_extract}.${wet_file}"

    do_break=true

    ascii_extract_size=$((`stat -c %s $ascii_extract`/1024/1024))
    if [ $ascii_extract_size -lt $target_size ]; then
        ./warc_extract.py "$wet_file" --ascii > $ascii_extract_fragment || exit 1
        cat $ascii_extract_fragment >> $ascii_extract
        rm $ascii_extract_fragment
        do_break=false
    fi

    non_ascii_extract_size=$((`stat -c %s $non_ascii_extract`/1024/1024))
    if [ $non_ascii_extract_size -lt $target_size ]; then
        ./warc_extract.py "$wet_file" --non_ascii > $non_ascii_extract_fragment || exit 1
        cat $non_ascii_extract_fragment >> $non_ascii_extract
        rm $non_ascii_extract_fragment
        do_break=false
    fi

    both_extract_size=$((`stat -c %s $both_extract`/1024/1024))
    if [ $both_extract_size -lt $target_size ]; then
        ./warc_extract.py "$wet_file" --ascii --non_ascii > $both_extract_fragment || exit 1
        cat $both_extract_fragment >> $both_extract
        rm $both_extract_fragment
        do_break=false
    fi

    all_extract_size=$((`stat -c %s $all_extract`/1024/1024))
    if [ $all_extract_size -lt $target_size ]; then
        ./warc_extract.py "$wet_file" --ascii --non_ascii --nulls> $all_extract_fragment || exit 1
        cat $all_extract_fragment >> $all_extract
        rm $all_extract_fragment
        do_break=false
    fi

    rm $wet_file
    rm $wet_file_gz

    if [ "$do_break" = true ] ; then
        break
    fi
done <$random_path_file

rm $wet_paths
rm $wet_paths_gz
rm $random_path_file

#if [ -f $F ]; then
#fi
