#!/bin/bash
ls *.wav | while read l 
do 
    echo "Extracting $l..."
    fftExtract -p wis.fftw -n 44100 -w 44100 -h 44100 -P -F 1 "$l" "${l/.wav/.power_f}"
    fftExtract -p wis.fftw -n 44100 -w 44100 -h 44100 -q 12 -g 1 -a 1 -F 1 "$l" "${l/.wav/.cqft12_f}"
done
echo "Done."

