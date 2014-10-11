g++ -o soundspotterVSTmain.o -c -g -Ofast -fPIC -I /home/mkc/src/vstsdk2.4/ -I /home/mkc/src/soundspotterVST/src soundspotterVSTmain.cpp
g++ -o soundspotterVST.o -c -g -Ofast -fPIC -I /home/mkc/src/vstsdk2.4/ -I /home/mkc/src/soundspotterVST/src soundspotterVST.cpp
g++ -o SoundSpotter.so -shared -Ofast -g -fPIC \
    -I /home/mkc/src/vstsdk2.4/ -I ../src \
    -I ../../vstsdk2.4/public.sdk/source/vst2.x \
    ../../vstsdk2.4/public.sdk/source/vst2.x/audioeffect.o \
    ../../vstsdk2.4/public.sdk/source/vst2.x/audioeffectx.o \
    ../../vstsdk2.4/public.sdk/source/vst2.x/vstplugmain.o \
    ../src/CircularMatrix.o ../src/FeatureExtractor.o \
    ../src/MatchedFilter.o ../src/Matcher.o \
    ../src/SoundFile.o ../src/SoundSpotter.o \
    soundspotterVSTmain.o soundspotterVST.o -lsndfile -lfftw3
