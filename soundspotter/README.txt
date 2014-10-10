Soundspotter - real-time concatenative audio matching and retrieval

Soundspotter is a library for feature extraction and similarity/dissimilarity-based matching designed
for real-time use in music performance.

Written by Michael Casey, Dartmouth College, 
Copyright Michael A. Casey 2001-2009 All Rights Reserved

Repository assistance by David Casal and Davide Morelli

License: Gnu Public License version 2.0 or any later version


SOURCE DOWNLOAD
svn checkout https://mp7c.svn.sourceforge.net/svnroot/mp7c/trunk/soundspotter

COMPILATION

Dependencies:
fftw3
libsndfile or ffmpeg

Compilation is for one of two host environments, Max/MSP and PD.
This requires either Max/MSP SDK or the PD source code available from http://crca.ucsd.edu/~msp/software.html.

Set the include and library paths to point to the Max SDK or PD directory, fftw and libsndfile or ffmpeg libs and headers.

Linux (PD Only)


make -f Makefile.PD.Linux

Max (Max or PD)

Max: Requires XCode project
PD: make -f Makefile.PD.OSX


Windows (Cygwin/Mingw)

Max: make -f Makefile.Max.OSX
PD: make -f Makefile.PD.OSX


General notes (Michael Casey)

testspotter:
make -f Makefile.HOST.OS test creates a test exectuable.

In Linux / OSX run with
./testspotter

In Windows run with
./testspotter.exe

This loads the sound file bell.wav, using the sound-file library that you compiled with, extracts features and performs matching using soundspotter's two matching algorithms: SPOT and LIVESPOT (EXTRACTANDSPOT).

Host wrappers:
DriverPD.{h,cpp} - the pure data host wrapper used by Makefile.PD.*
DriverMax.{h,cpp} - the Max/MSP host wrapper used by Makefile.Max.*


OSX notes (David Plans Casal)

FFMPEG is not available currently on fink. It is however available on port, using:

sudo port install ffmpeg +gpl +lame +x264 +xvid

this assumes you have macports installed...

If you come up against erros on installing FFMPEG, make sure you do 'sudo pot selfupdate', and then:

sudo port -v upgrade outdated
sudo port install ffmpeg

you can install the variants above if you need them later on 
