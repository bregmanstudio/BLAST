# iMatsh
# Makefile for iMatsh Linux
# usage: make
#
# Copyright (C) Michael Casey 2010, Dartmouth College, All Rights Reserved

INCDIR=
CFLAGS=-O3
LIBDIR=
LIBS=-lsndfile

EXECUTABLE=iMatsh

OBJS=iMatsh.o MatchedFilter.o Matcher.o CircularMatrix.o Compiler.o

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS) 
	g++ -o $(EXECUTABLE) $(CFLAGS) $(INCDIR) -Wall $(OBJS) $(LIBDIR) $(LIBS) 

$(OBJS): %.o: %.cpp *.h
	g++ -c $(CFLAGS) $(INCDIR) -Wall $<

clean:
	-rm $(OBJS)
	-rm $(EXECUTABLE)

