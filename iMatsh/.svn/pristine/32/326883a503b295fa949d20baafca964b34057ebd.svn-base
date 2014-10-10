#!/bin/bash
ls *.mp3 | while read l 
do 
    mplayer -vc null -vo null -ao pcm:waveheader:fast:file="${l/.mp3/.wav}" "$l"&
    wait
    #fftExtract -p wis.fftw -n 4096 -w 4096 -h 1024 -m 45 "${l/.mp3/.wav}" "${l/.mp3/.mfcc45}"
    #fftExtract -p wis.fftw -n 4096 -w 4096 -h 1024 -P "${l/.mp3/.wav}" "${l/.mp3/.power}"
    #fftExtract -p wis.fftw -n 16384 -w 8192 -h 2205 -c 12 -g 0 -a 1 "${l/.mp3/.wav}" "${l/.mp3/_16384_8192_2205.chrom12}"
    #fftExtract -p wis.fftw -n 16384 -w 8192 -h 2205 -P "${l/.mp3/.wav}" "${l/.mp3/_16384_8192_2205.power}"
    fftExtract -p wis.fftw -n 16384 -w 8192 -h 2205 -q 12 -g 1 -a 1 "${l/.mp3/.wav}" "${l/.mp3/_16384_8192_2205.cqft12}"
done

