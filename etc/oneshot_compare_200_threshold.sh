#!/bin/bash

exec > >(tee -i logfile.txt)
exec 2>&1

TMP_SUITE=tmp.suite
COMPARE_CMD=./compare

for i in `seq 1 40`; do
    cmd="esacomp(code2, threshold=$i);.tdc;tdc;--algorithm=esacomp(coder=code2,threshold='$i');"
    echo $cmd
done > $TMP_SUITE

cat $TMP_SUITE

shopt -s globstar

ln -s ../build/tudocomp_driver ./tdc
export PATH=$PATH:`pwd`
echo $PATH

for dataset in ../etc/datasets/**; do
    if [[ $dataset == *.200MB ]]; then
        echo $dataset
    fi
done | xargs $COMPARE_CMD $TMP_SUITE

rm ./tdc
rm $TMP_SUITE
