#include "MatchedFilter.h"

void err(const char* s){
  printf("%s\n",s);
  exit(1);
}


MatchedFilter::MatchedFilter()  :
  maxShingleSize(0),
  maxDBSize(0)
{
  ;
}

MatchedFilter::MatchedFilter(int maxShingleSize, int maxDBSize) :
  maxShingleSize(0),
  maxDBSize(0)
{
  resize(maxShingleSize, maxDBSize);
}

void MatchedFilter::resize(int maxShingleSize, int maxDBSize){  
  if(MatchedFilter::maxShingleSize)    
    clearMemory();

  MatchedFilter::maxShingleSize = maxShingleSize;
  MatchedFilter::maxDBSize = maxDBSize;
  
  int j;
  
  // Cross-correlation matrix
  D = new float*[maxShingleSize];
  if(!D)
    err("Could not allocate for D matrix rows.\n");
  for( j=0; j < maxShingleSize ; j++ ){
    D[j]=new float[ maxDBSize ];
    if(!D[j])
      err("Could not allocate for D matrix cols.\n");
  }
  
  // Matched filter matrix
  DD = new float[ maxDBSize ];
  if(!DD)
    err("Could not allocate for DD matrix rows.\n");
  
  // Allocate for L2 norm vectors
  qNorm = new float[ maxShingleSize ]; // query is one shingle of length W
  sNorm = new float[ maxDBSize ];        // source shingles of length W
  
}
  
void MatchedFilter::clearMemory(){
  int j;
  if(D !=0 ){
    for( j=0; j < maxShingleSize ; j++ )
      delete[] D[j];
    delete[] D;
    D=0;
  }
  
  if(DD!=0){
    delete[] DD;
    DD=0;
  }
  
  if(qNorm!=0)
    delete[] qNorm;
  
  if(sNorm!=0)
    delete[] sNorm;  
}

MatchedFilter::~MatchedFilter(){
  clearMemory();
}

float MatchedFilter::getQNorm(int i){
  if(i<maxShingleSize)
    return qNorm[i];
  else
    return 0;
}

float MatchedFilter::getSNorm(int i){
  if(i<maxDBSize)
    return sNorm[i];
  else
    return 0;
}

float MatchedFilter::getDD(int i){
  if(i<maxDBSize)
    return DD[i];
  else
    return 0;
}


// Incremental multidimensional time series insert
void MatchedFilter::insert(SeriesOfVectors* inShingle, int shingleSize, SeriesOfVectors* dbShingles, int dbSize, int muxi, 
			   int loFeature, int hiFeature, int loK, int hiK){
  
  // incrementally compute cross correlation matrix
  incrementalCrossCorrelation(inShingle, shingleSize, dbShingles, dbSize, muxi, loFeature, hiFeature, loK, hiK);
  
  // Keep input L2 norms for correct shingle norming at distance computation stage
  ss_sample* qPtr = inShingle->getCol(muxi)+loFeature;
  if(*qPtr > NEGINF)
    qNorm[muxi]=SeriesOfVectors::vectorSumSquares(qPtr, hiFeature-loFeature+1);
  else
    qNorm[muxi]=0.0f;
}

void MatchedFilter::incrementalCrossCorrelation(SeriesOfVectors* inShingle, int shingleSize, SeriesOfVectors* dbShingles, 
						int dbSize, int muxi, int loFeature, int hiFeature, int loK, int hiK){
  ss_sample* sp,*qp;
  float *dp;
  int l;
  int numFeatures = inShingle->getRows();
  long ioff = (muxi * numFeatures) + loFeature;
  long doff = ((muxi + loK) * numFeatures) + loFeature;
  float* isp = (float*) ( inShingle->getSeries() + ioff );
  float* dsp = (float*)dbShingles->getSeries();
  int totalLen = dbSize - shingleSize - hiK - loK + 1;
  int dim = hiFeature - loFeature + 1;
  float* dpp = *(D+muxi) + muxi;
  // Make Correlation matrix entry for this frame against entire source database
  while( totalLen-- ){
    qp = isp ; // input column pointer 
    sp = dsp + doff ; // dbcolumn pointer
    doff+=numFeatures;
    dp = dpp++;    // point to correlation cell j,k
    *dp = 0.0;      // initialize correlation cell
    l = dim; // Size of bounded feature vector
    while( l-- )
      *dp += *qp++ * *sp++;
  }  
}

void MatchedFilter::sumCrossCorrMatrixDiagonals(int shingleSize, int dbSize, int loK, int hiK){
  float *sp = 0;
  float **dp = 0;
  int k=0,l=0,w=0;  
  
  // Matched Filter length W hop H over N frames
  for( k=loK ; k < dbSize - hiK - shingleSize + 1; k++ ){
    sp = DD + k; // initialize matched filter output k
    *sp = D[0][k]; // DD+k <- q_0 . s_k 
    l = shingleSize - 1;
    dp= D + 1;
    w = k + 1;       // next diagonal element
    while( l-- )
      *sp += *( *dp++ + w++ ); // Sum rest of k's diagonal up to W elements
  }
}


void MatchedFilter::updateDatabaseNorms(SeriesOfVectors* dbShingles, int shingleSize, int dbSize, 
					int loFeature, int hiFeature, int loK, int hiK){
  ss_sample* sPtr;
  for( int k = loK; k < dbSize - hiK ; k++ ){
    sPtr = dbShingles->getCol(k)+loFeature;
    if(*sPtr > NEGINF)
      sNorm[k]=SeriesOfVectors::vectorSumSquares(sPtr, hiFeature-loFeature+1);
    else
      sNorm[k]=0.0f;
  }
  SeriesOfVectors::seriesSqrt(sNorm, shingleSize, dbSize);
}
  
// PRE-CONDITIONS:
// a complete input shingle (W feature frames)
// D (W x N) and DD (1 x N) are already allocated by Matcher constructor
// D (W x N) is a normed cross-correlation matrix between input features and sources
void MatchedFilter::execute(int shingleSize, int dbSize, int loK, int hiK){
  // sum diagonals of cross-correlation matrix
  sumCrossCorrMatrixDiagonals(shingleSize, dbSize, loK, hiK);
  // Perform query shingle norming
  SeriesOfVectors::seriesSqrt(qNorm, shingleSize, shingleSize);
}

