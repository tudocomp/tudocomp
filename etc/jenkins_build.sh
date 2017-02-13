#!/bin/bash
die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -eq 1 ] || die "usage: $0  (build | check)"

# Exit if error, and print all commands
set -e
set -o xtrace

mkdir build -p
cd build

export TDC_ALLWAYS_DOWNLOAD=1

if [[ "$optimization_target" == "Release" ]]; then
    BUILD_TYPE_FLAG=-DCMAKE_BUILD_TYPE=Release
    PARANOID_FLAG=
elif [[ "$optimization_target" == "Debug" ]]; then
    BUILD_TYPE_FLAG=-DCMAKE_BUILD_TYPE=Debug
    PARANOID_FLAG=
elif [[ "$optimization_target" == "Paranoid" ]]; then
    BUILD_TYPE_FLAG=-DCMAKE_BUILD_TYPE=Debug
    PARANOID_FLAG=-DPARANOID=1
else
    exit 1
fi

if [[ "$stats_enabled" == "StatsEnabled" ]]; then
    STATS_FLAG=
elif [[ "$stats_enabled" == "StatsDisabled" ]]; then
    STATS_FLAG=-DSTATS_DISABLED=1
else
    exit 1
fi

cmake -DVERSION_SUFFIX=-$BUILD_NUMBER $BUILD_TYPE_FLAG $PARANOID_FLAG $STATS_FLAG ..

if [[ "$1" == "build" ]]; then
    if [[ "$make_target" == "Make" ]]; then
        make
    elif [[ "$make_target" == "MakeCheck" ]]; then
        make build_check
    else
        exit 1
    fi
elif [[ "$1" == "check" ]]; then
    make check
else
    exit 1
fi
