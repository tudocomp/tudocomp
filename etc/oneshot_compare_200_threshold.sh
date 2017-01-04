#!/bin/bash

exec > >(tee -i log_`date -Is`.txt)
exec 2>&1

trap "exit" INT

TDC=./tudocomp_driver
TIME=/usr/bin/time
SIZE=1MB

shopt -s globstar
for dataset in ../etc/datasets/**; do
    if [[ $dataset == *.$SIZE ]]; then
        echo
        echo $dataset
        for i in `seq 1 40`; do
            algo="esacomp(coder=code2,threshold='$i')"

            mkdir -p /tmp/$USER
            OUT=/tmp/$USER/`basename $dataset`_${i}.tdc
            CMD="${TDC} --force --output=${OUT} --algorithm=${algo} $dataset"

            if [[ $i == 1 ]]; then
                echo $CMD
                echo "threshold;in_size;out_size;ratio;time;mem;cmd"
            fi

            TIME_S=`$TIME -f "%U;%M" $CMD 2>&1`

            IN_SIZE=`stat --printf="%s" $dataset`
            OUT_SIZE=`stat --printf="%s" $OUT`
            RATIO=`bc <<< "scale=5; $OUT_SIZE * 100 / $IN_SIZE"`

            #echo $CMD
            printf "%s;%s;%s;%s;%s;%s\n" $i $IN_SIZE $OUT_SIZE $RATIO $TIME_S "$CMD"
        done
    fi
done
