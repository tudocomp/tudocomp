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
    printf "0.%s.%s" $DATE $COMMITNO
else
    printf "0.%s.%s-modified" $DATE $COMMITNO
fi`

VERSION_LONG=`if [ -z "$WORKINGSETCHANGES" ]; then
    printf "%s (%s)" $VERSION $COMMITREV
else
    printf "%s (uncommited changes based on %s)" $VERSION $COMMITREV
fi`

TMP_VERSION=$(mktemp -t "$(basename "$1").XXXXXX")

trap "rm -f $TMP_VERSION" EXIT

cat << EOF > $TMP_VERSION
#pragma once
#include <string>

namespace tdc {
    const std::string VERSION = "$VERSION_LONG";
}
EOF

if cmp --silent $1 $TMP_VERSION; then
    rm $TMP_VERSION
else
    cp $TMP_VERSION $1
fi
