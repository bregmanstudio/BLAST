#ifndef __CIRCULARMATRIX_H
#define __CIRCULARMATRIX_H

#include "SignalDefs.h"
#include <stdlib.h>

typedef unsigned long long idxT;

// REPLACE with CircularMatrix class
class SeriesOfVectors{
protected:
  ss_sample* series; // sample type
  idxT M; // Length of feature vectors
  idxT N; // Length of vector series

public:
  SeriesOfVectors(float* buf, idxT m, idxT n);
  SeriesOfVectors(idxT m,idxT n);
  ~SeriesOfVectors();
  // accessors
  ss_sample* getSeries();    // Column 0 Accessor
  ss_sample get(idxT,idxT);// Element accessor 
  ss_sample* getCol(idxT);  // Column accessor
  idxT getRows();
  idxT getCols();
  // mutators
  void set(idxT, idxT, ss_sample); // Element mutator
  void setCol(idxT, ss_sample*); // Column mutator
  void insert(ss_sample*,idxT);
  // vector series operations
  float vectorSumSquares(idxT k, int lodim, int hidim);
  void seriesSum(idxT seqlen, idxT sz=0);
  void seriesSqrt(idxT seqlen, idxT sz=0);
  void seriesMean(idxT seqlen, idxT sz=0);
  // static utility methods for vectors and series
  static float vectorSumSquares(ss_sample* vec, int dim);
  static void seriesSum(float* v, idxT seqlen, idxT sz);
  static void seriesSqrt(float* v, idxT seqlen, idxT sz);
  static void seriesMean(float* v, idxT seqlen, idxT sz);
  // other utilities
  int copy(SeriesOfVectors* source);
};


/******************* CircularMatrix (LILO) *********************************/
//  A circular buffered column-major Matrix class, matrix shape is immutable
//  CircularMatrix(Rows, Cols); // constructor, fixed rows and columns
//  pushBack(vector); // insert a column vector to back of matrix
//  getCol(0) returns the current 'front' column
//  top() is the same as getCol(0);
//
//  The advantage of a circular matrix is that memory does not have to be moved
//  when a new column is inserted into a fixed-size matrix.
//  The circular shift is implemented efficiently by pointer arithmetic.
//
//  The disadvantage is that there is a small overhead in accessing matrix elements
//  due to the modulo pointer arithmetic
//
//  A circular matrix is written newest-first (back)
//  and is read oldest-first (front)
//
//  Write operations mean last-in, last-out (LILO)
//
//  mkc Nov. 25th 2008
//
// More efficient implementation of the SeriesOfVectors
// We maintain a circular list of vectors as columns of a matrix
// Write operations are destructive, overwriting back vector
// Read operations are non-destructive, reading from front
class CircularMatrix{  
 protected:
  ss_sample* matrix; // FLEXT's sample type
  int M; // Length of feature vectors
  int N; // Length of vector series
  int wCol; // write index (column)
  int wRow; // write index (column)
  int circularCol(int c); // actual index of circular column
  int circularRow(int r); // actual index of circular row
  int ensurePositiveCol(int c); // non-negative index constraint
  int ensurePositiveRow(int r); // non-negative index constraint

 public:
  CircularMatrix(int nrows,int ncols);
  ~CircularMatrix();
  ss_sample* getMatrix(); // Circular column 0 accessor (relative to read position)
  ss_sample get(int,int); // Circular element accessor (relative to read position)
  ss_sample* getCol(int); // Cicular column accessor (relative to read position)
  ss_sample* getRow(int); // Cicular column accessor (relative to read position)
  ss_sample* frontCol(void); // Cicular column accessor (relative to read position)
  ss_sample* frontRow(void); // Cicular column accessor (relative to read position)
  ss_sample* backCol(void); // Cicular column accessor (relative to read position)
  ss_sample* backRow(void); // Cicular column accessor (relative to read position)
  void set(int, int, ss_sample); // Element mutator (relative to write position)
  void setCol(int, ss_sample*); // Column mutator (relative to write position)
  void setRow(int, ss_sample*); // Column mutator (relative to write position)
  int getRows();
  int getCols();
  void pushBackCol(ss_sample*); // col circular insertion
  void pushBackRow(ss_sample*); // row circular insertion
  void dump();
};


#endif
