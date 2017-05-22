#!/bin/bash
[[ $# -eq 0 ]] &&
chromium "file:///bighome/workspace/compreSuite/tudocomp/www/charter/index.html?$(base64 <&0)" &
for i in $@; do
	chromium "file:///bighome/workspace/compreSuite/tudocomp/www/charter/index.html?$(cat $i | base64)" &
done

