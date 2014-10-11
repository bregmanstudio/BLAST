#!/bin/bash
set q=0
while (( q<=8 ))
do qstr=`printf "%02d" $q`
    ../iMatsh amen${qstr}.test.cqft12_f amen${qstr}.test.power_f amen${qstr}.test.wav sfl_f.t spl_f.t sml.t 1 1 0 0 0 2.5 1 44100 44100 > amen${qstr}.imatsh_f.txt
    let q=q+1
done

cat *.imatsh_f.txt

