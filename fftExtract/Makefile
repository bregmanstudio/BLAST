DEBUGFLAGS=
CFLAGS=-O3 -g
INCLUDE=-I/usr/include -I/usr/local/include -I/opt/local/include
LIBS=-L/usr/lib -L/usr/local/lib -L/opt/local/lib

EXECUTABLE=fftExtract

.PHONY: all clean

all: ${EXECUTABLE}

${EXECUTABLE}: fftExtract.o Makefile
	g++ -o fftExtract ${CFLAGS} ${DEBUGFLAGS} fftExtract.o ${LIBS} ${INCLUDE} -lsndfile -lfftw3 -lm

fftExtract.o: fftExtract.cpp Makefile
	g++ -c ${CFLAGS} ${LIBS} ${INCLUDE} -Wall -Werror ${DEBUGFLAGS} fftExtract.cpp 

clean:
	-rm fftExtract.o fftExtract
