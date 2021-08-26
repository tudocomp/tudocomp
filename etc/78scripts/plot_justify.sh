#!/bin/zsh
pattern='\(RESULT.*mem=\)\([^ ]\+\),\([^ ]\+\)'
for txtfile in *.log; do
	while grep -q "$pattern" "$txtfile"; do
		sed -i "s!$pattern!\1\2\3!g" $txtfile
	done
	sed -i "s!sigma+2: !sigma+2=!g" $txtfile
done

for txtfile in tudocomp_memory.log tudocomp_time.log; do
	sed -i 's@^RESULT \([^t].*algo=[^w]\)@RESULT type=78 \1@' "$txtfile"
	sed -i 's@^RESULT \([^t].*algo=\)w@RESULT type=w \1@' "$txtfile"

done



