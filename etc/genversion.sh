#!/bin/bash
die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -eq 1 ] || die "usage: $0 [version.hpp]"

WORKINGSETCHANGES=$(git status --porcelain | grep -Pv "^\?\?")

DATE=$(date "+%Y.%m")
COMMITNO=$(git rev-list --count HEAD)
COMMITREV=$(git show HEAD | head -n 1 | cut -c8-)

VERSION=`if [ -z "$WORKINGSETCHANGES" ]; then
    printf "0.%s.%s (%s)" $DATE $COMMITNO $COMMITREV
else
    printf "0.%s.%s-modified (uncommited changes based on %s)" $DATE $COMMITNO $COMMITREV
fi`

TMP_VERSION=$(mktemp -t "$(basename "$1").XXXXXX")

trap "rm -f $TMP_VERSION" EXIT

cat << EOF > $TMP_VERSION
#pragma once
#include <string>

namespace tdc {
    const std::string VERSION = "$VERSION";
}
EOF

if cmp --silent $1 $TMP_VERSION; then
    rm $TMP_VERSION
else
    cp $TMP_VERSION $1
fi
