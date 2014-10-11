#ifndef __MATCHEDFITLER_H
#define __MATCHEDFITLER_H

#include <stdio.h>
#include <limits>
#include <deque>
#include <vector>
#include <algorithm>
#include <math.h>
#include "CircularMatrix.h"

using namespace std;

#ifdef _MSC_VER
static const float NEGINF = - numeric_limits<float>::infinity(); // machine-specific representation of Negative Infinity
#define ISNAN _isnan
#else
static const float NEGINF = - numeric_limits<float>::infinity(); //(double)-LONG_MAX;
#define ISNAN isnan
#endif
class MatchedFilter {

 protected:
  float** D;            // cross-correlation matrix
  float* DD;            // matched filter result vector
  float* qNorm;         // query L2 norm vector
  float* sNorm;         // database L2 norm vector

  int maxShingleSize;    // largest shingle to allocate
  int maxDBSize;         // largest database to allocate
  
  void incrementalCrossCorrelation(SeriesOfVectors* inShingle, int shingleSize, 
				   SeriesOfVectors* dbShingles, int dbSize, int muxi, int loFeature, int hiFeature, int loK=0, int hiK=0);
  void sumCrossCorrMatrixDiagonals(int shingleSize, int dbSize, int loK, int hiK);
public:
  MatchedFilter();
  MatchedFilter(int maxShingleSize, int maxDBSize); // shingleSize x databaseSize
  virtual ~MatchedFilter();
  void clearMemory();
  virtual void resize(int maxShingleSize, int maxDBSize);
  void insert(SeriesOfVectors* inShingle, int shingleSize, SeriesOfVectors* dbShingles, int dbSize, int muxi, int loFeature, int hiFeature, int loK=0, int hiK=0);
  void updateDatabaseNorms(SeriesOfVectors* dbShingles, int shingleSize, int dbSize, int loFeature, int hiFeature, int loK=0, int hiK=0);
  void execute(int shingleSize, int dbSize, int loK=0, int hiK=0);
  
  float getQNorm(int i);
  float getSNorm(int i);
  float getDD(int i);
};


#endif
