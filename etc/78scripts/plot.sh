#!/bin/zsh
function die {
	echo "$1" >&2
	exit 1
}

set -x
set -e

[[ -d log ]] || die "cannot read directory log"
mkdir -p plot

oldpwd=$(pwd)

cd $oldpwd/log 
$oldpwd/plot_justify.sh

cd $oldpwd
[[ -d sqlplot ]] || git clone https://github.com/koeppl/sqlplot

./sqlplot/sqlplot.py plot.tex
cd $oldpwd/plot
./justify.py
cd $oldpwd
if which latexmk; then 
	latexmk -pdf plot.tex
else
	zip -r plot.zip plot.tex plot
fi
