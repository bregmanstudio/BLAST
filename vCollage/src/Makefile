IDIR=-I/usr/local/include/
CC=g++
CFLAGS=$(IDIR) -O3 -std=c++11

ODIR=.
SDIR=
LDIR=-L/opt/opencv/2.4.7/lib/

LIBS=-lopencv_calib3d -lopencv_objdetect -lopencv_features2d	\
-lopencv_imgproc -lopencv_video -lopencv_highgui -lopencv_ml	\
-lopencv_core -lopencv_flann -lopencv_contrib
LIBS+=-lstdc++ -lm

DEPS=vCollage.h
OBJ=vCollage.o

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

vCollage: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDIR) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o
