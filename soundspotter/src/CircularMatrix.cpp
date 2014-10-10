#include "CircularMatrix.h"
#include <iostream>
#include <assert.h>

extern void post(const char *fmt, ...);


SeriesOfVectors::SeriesOfVectors(float* buf, idxT m,idxT n):M(m),N(n){
  series = (ss_sample*)buf;
}

SeriesOfVectors::SeriesOfVectors(idxT m,idxT n):M(m),N(n){
  series = new ss_sample[M*N];
  for(unsigned long a=0;a<m;a++)
    for(unsigned long b=0; b<n;b++)
      *(series+(b*M)+a)=0.0;
}

SeriesOfVectors::~SeriesOfVectors(){
    delete[] series;
}


// Column 0 Accessor
 ss_sample* SeriesOfVectors::getSeries(){return series;}


// Element accessor 
 ss_sample SeriesOfVectors::get(idxT m,idxT n){return *(series+(n*M)+m);}


// Column accessor
 ss_sample* SeriesOfVectors::getCol(idxT n){return series+(n*M);}


// Element mutator
void SeriesOfVectors::set(idxT m, idxT n, ss_sample v){*(series+(n*M)+m)=v;}


// Column mutator
 void SeriesOfVectors::setCol(idxT n, ss_sample* v){
  ss_sample* p=series+n*M;
  idxT k=M;
  while(k--)*p++=*v++;
}


// Dimension accessor
 idxT SeriesOfVectors::getRows(){
	 return M;
 }
 
 idxT SeriesOfVectors::getCols(){
	 return N;
 }

void SeriesOfVectors::insert(ss_sample* v, idxT j){
  ss_sample* qp=getCol(j);
  ss_sample* fp=v;
  idxT l=M;
  while(l--)
    *qp++=*fp++; // copy feature extract output to inShingle current pos (muxi)
}

int SeriesOfVectors::copy(SeriesOfVectors* source){
	if( !( getCols()==source->getCols() && getRows()==source->getRows()) ){
		// NEED AN ERROR MESSAGE
		return 0;
	}
	ss_sample* dp = series;
	ss_sample* sp = source->getSeries();
	idxT n = getRows()*getCols();
	while(n--)
		*dp++ = *sp++;
	return 1;
}

float SeriesOfVectors::vectorSumSquares(idxT k, int lodim, int hidim){
	return vectorSumSquares(getCol(k)+lodim, hidim-lodim+1);
}

 
void SeriesOfVectors::seriesSum(idxT seqlen, idxT sz){
	if(!sz){
		sz=getCols();
	}
	seriesSum(getSeries(), seqlen, sz);
}

void SeriesOfVectors::seriesSqrt(idxT seqlen, idxT sz){
	if(!sz){
		sz=getCols();
	}
	seriesSqrt(getSeries(), seqlen, sz);
}

void SeriesOfVectors::seriesMean(idxT seqlen, idxT sz){
	if(!sz){
		sz=getCols();
	}
	seriesMean(getSeries(), seqlen, sz);
}

float SeriesOfVectors::vectorSumSquares(ss_sample* vec, int len){
	float sum1=0, v1;
	while( len-- ){
		v1 = *vec++;
		sum1 += v1 * v1;
	}
	return sum1;  
}

void SeriesOfVectors::seriesSqrt(float* v, idxT seqlen, idxT sz){
	seriesSum(v, seqlen, sz); // <<<<<****** INTERCEPT NANS or INFS here ?
	idxT	l = sz - seqlen + 1;
	while(l--){
	  *v = sqrtf( *v );  
		v++;
	}
}

void SeriesOfVectors::seriesMean(float* v, idxT seqlen, idxT sz){
	seriesSum(v, seqlen, sz);
	float oneOverSeqLen = 1.0f / seqlen;
	idxT l = sz - seqlen + 1;
	while(l--){
		*v *= oneOverSeqLen;
		v++;
	}
}

//<<<<<****** INTERCEPT NANS or INFS here ?
void SeriesOfVectors::seriesSum(float* v, idxT seqlen, idxT sz){
	float tmp1=0.0,tmp2=0.0;
	float* sp = v;
	float* spd = v+1;
	idxT l = seqlen - 1;
	tmp1 = *sp;
	// Initialize with first value
	while( l-- )
		*sp += *spd++;
	// Now walk the array subtracting first and adding last
	l = sz - seqlen; // +1 -1
	sp = v + 1;
	while( l-- ){
		tmp2 = *sp;
		*sp = *( sp - 1 ) - tmp1 + *( sp + seqlen - 1 );
		tmp1 = tmp2;
		sp++;
	} 
}



/******************* CircularMatrix (LILO) *********************************/
//  A circular buffered column-major Matrix class, matrix shape is immutable
//  CircularMatrix(Rows, Cols); // constructor, fixed rows and columns
//  pushBack(vector); // insert a column vector to back of matrix
//  getCol(0) returns the current 'front' column
//  front() is the same as getCol(0);
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
CircularMatrix::CircularMatrix(int m,int n):M(m),N(n){
  matrix = new ss_sample[M*N];
  for(int a=0;a<m;a++)
    for(int b=0; b<n;b++)
      *(matrix+(b*M)+a)=0.0;
  wCol = 0;
  wRow = 0;
}

CircularMatrix::~CircularMatrix(){
  delete[] matrix;
}

int CircularMatrix::circularCol(int c){
  c = ensurePositiveCol(c);
  return c % N;
}

int CircularMatrix::ensurePositiveCol(int c){
  while(c<0)
    c+=N;
  return c;
}

int CircularMatrix::circularRow(int r){
  r = ensurePositiveRow(r);
  return r % M;
}

int CircularMatrix::ensurePositiveRow(int r){
  while(r<0)
    r+=M;
  return r;
}

// Column 0 Accessor
ss_sample* CircularMatrix::getMatrix(){
  return matrix;
}

// Element accessor 
ss_sample CircularMatrix::get(int m,int n){
  n = circularCol(wCol+n);
  return *( matrix + n * M + m );
}

// Column accessor
ss_sample* CircularMatrix::getCol(int n){
  n = circularCol(wCol+n);
  return matrix + n * M;
}

// Column accessor
ss_sample* CircularMatrix::getRow(int m){
  int n = circularCol(wCol);
  m = circularRow(wRow+m);
  return matrix + n * M + m;
}

ss_sample* CircularMatrix::frontCol(){
  return getCol(-1); // Same as N-1
}

ss_sample* CircularMatrix::frontRow(){
  return getRow(-1); // Same as M-1
}

ss_sample* CircularMatrix::backCol(){
  return getCol(0);
}

ss_sample* CircularMatrix::backRow(){
  return getRow(0);
}

// Column mutator
void CircularMatrix::setCol(int n, ss_sample* v){
  n = circularCol(wCol+n);
  ss_sample* p = matrix + n * M;
  int k=M;
  while(k--)
    *p++ = *v++;
}

// Row mutator
void CircularMatrix::setRow(int m, ss_sample* v){
  int n = wCol;
  m = circularRow(wRow+m);
  ss_sample* p = matrix + n * M + m;
  int k = M;
  while(k--){
    *p = *v++;
    n = circularCol(n+1);
    p = matrix + n * M + m;
  }
}

// Dimension accessor
int CircularMatrix::getRows(){return M;}
int CircularMatrix::getCols(){return N;}

// Circular shifting column mutator
void CircularMatrix::pushBackCol(ss_sample* v){
  setCol(0,v);
  wCol = circularCol(wCol+1);
}

// Circular shifting row mutator
void CircularMatrix::pushBackRow(ss_sample* v){
  setRow(0,v);
  wRow = circularRow(wRow+1);
}

void CircularMatrix::dump(){
  for(int j=0; j<N; j++){
    ss_sample* col = getCol(j);
    int i = M;
    while(i--)
      std::cout << *col++ << " ";
    std::cout << std::endl;
  }
    
}

#ifdef TEST_CIRCULAR_MATRIX
// g++ -o cmt -DTEST_CIRCULAR_MATRIX CircularMatrix.cpp
int main(int argc, char* argv[]){
  CircularMatrix* C = new CircularMatrix(2,3); // rows=2, cols=3
  ss_sample* col = new ss_sample(C->getRows());
  int a=0, j, k;
  for(j=0; j<2*C->getCols(); j++){
    for(k=0; k<C->getRows(); k++)
      col[k]=a++; // test value to insert
    C->pushBackCol(col); // insert column
    C->dump(); // dump matrix
    ss_sample* frontCol = C->frontCol(); // get front column
    std::cout << "front:";
    for(k=0; k<C->getRows(); k++) // dump front column only
      std::cout << frontCol[k] << " ";
    ss_sample* backCol = C->backCol(); // get back column
    std::cout << std::endl << "back :";
    for(k=0; k<C->getRows(); k++) // dump front column only
      std::cout << backCol[k] << " ";
    std::cout << std::endl << "**" << std::endl;

  }
  delete C;
}

#endif

