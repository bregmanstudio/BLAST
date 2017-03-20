#!/bin/bash
set q=0
while (( q<=8 ))
do qstr=`printf "%02d" $q`
    ../aCollage amen${qstr}.test.cqft12 amen${qstr}.test.power amen${qstr}.test.wav sfl.t spl.t sml.t 1 1 0 0 0 2.5 1 44100 44100 > amen${qstr}.imatsh.txt
    let q=q+1
done

