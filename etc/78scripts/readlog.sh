#!/bin/zsh

function die {
    echo "$1" >&2
    exit 1
}
[[ ! -r $1 ]] && die "cannot read log file $1"

logFile=$1


echo -n "tdctime=$(jq '.["data"]["timeRun"]' ${logFile}) "
echo -n "mem=$(jq '.["data"]["memPeak"]' ${logFile}) "


statlength=$(jq '.["data"]["sub"][0]["stats"] | length' ${logFile})
for i in $(seq 0 $statlength); do
	key=$(jq '.["data"]["sub"][0]["stats"]['$i']["key"]' $logFile)
	value=$(jq '.["data"]["sub"][0]["stats"]['$i']["value"]' $logFile)
case $key in
"\"factor_count\"")
	echo -n "z=$value "
;;
esac
done
echo ''

