#!/bin/bash
die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -eq 1 ] || die "usage: $0  (build | check | website)"

# Exit if error, and print all commands
set -e
set -o xtrace

mkdir build -p
cd build

export TDC_ALLWAYS_DOWNLOAD=1
export CCACHE_SLOPPINESS=file_macro,time_macros,include_file_mtime,include_file_ctime,pch_defines
export CCACHE_MAXSIZE=10G

# Log ccache usage for debugging reasons
ccache -s > ccache.pre.txt

if [[ "$1" == "website" ]]; then
    make website
else
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
fi

# Log ccache usage for debugging reasons
ccache -s > ccache.post.txt
diff -y -W 100 ccache.pre.txt ccache.post.txt || true
