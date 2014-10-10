IDIR=-I/usr/local/include/
CFLAGS=$(IDIR) -O3 -std=c++11
CC=g++
ODIR=.
SDIR=
LDIR=-L/usr/local/lib/

LIBS=-lopencv_calib3d -lopencv_objdetect -lopencv_features2d	\
-lopencv_imgproc -lopencv_video -lopencv_highgui -lopencv_ml	\
-lopencv_core -lopencv_flann -lopencv_contrib
LIBS+=-lstdc++ -lm

DEPS=videoMashup.h
OBJ=videoMashup.o

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

videoMashup: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDIR) $(LIBS) 

.PHONY: clean

clean:
	rm -f *.o
