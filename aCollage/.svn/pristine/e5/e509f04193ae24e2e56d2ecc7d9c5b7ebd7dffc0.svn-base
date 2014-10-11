#include "Matcher.h"
// CLASS Matcher
// 
// match multi-dimensional time-series using a matched filter
//
// Pre-conditions:
//        We have extracted features from some source material first.
//        input buffer length is an integral divisor of shingleSize
// Post-conditions:
//		  The best segment is picked and the corresponding samples are output
// Each windowed target segment is spotted with its best match from source segment array.
//

bool operator< (MatchResult const &a, MatchResult const &b){
  return a.dist < b.dist;
}

bool operator> (MatchResult const &a, MatchResult const &b){
  return a.dist > b.dist;
}

Matcher::Matcher() : MatchedFilter(){

}

Matcher::Matcher(int maxShingleSize, int maxDBSize) : 
MatchedFilter(maxShingleSize, maxDBSize)
{
  frameHashTable = new int[ maxDBSize ];
  clearFrameQueue();
  dist = 0.0f;
  useRelativeThreshold = 0;
}

Matcher::~Matcher(){ 
  delete[] frameHashTable;
  frameHashTable=0;
}

void Matcher::resize(int maxShingleSize, int maxDBSize){
  MatchedFilter::resize(maxShingleSize, maxDBSize);
  frameHashTable = new int[ maxDBSize ];
  clearFrameQueue();
  dist = 0.0f;
  useRelativeThreshold = 0;
}

// Matching algorithm using recursive matched filter algorithm
// This algorithm is based on factoring the multi-dimensional convolution 
// between the current input shingle and the database shingles
//
// The sum-of-products re-factoring reduces the number of multiplications 
// required by an order of magnitude or more.
//
// Author: Michael A. Casey, April 24th - November 12th 2006
// Substantially Modified: Michael A. Casey, August 24th - 27th 2007
// Factored out dependency on SoundSpotter class, August 8th - 9th 2009
// Added power features for threshold tests
int Matcher::match(float matchRadius, int shingleSize, int dbSize, int loDataLoc, int hiDataLoc, 
		   int queueSize, float inPwMn, float* powers, float pwr_abs_thresh, float pwr_rel_thresh,
		   std::vector<off_t>* track_offsets){
  dist=0.0f;
  float minD=1e6f;
  float dRadius=0.0f;
  float iRadius=matchRadius*matchRadius; // squared search radius
  float minDist=10.0f;
  int winner=-1;
  // Perform the recursive Matched Filtering (core match algorithm)
  execute(shingleSize, dbSize, loDataLoc, hiDataLoc);
  float qN0 = getQNorm(0); // pre-calculate denominator coefficient
  // DD now contains (1 x N) multi-dimensional matched filter output
  ss_sample oneOverW = 1.0f / shingleSize;  
  for(int k= loDataLoc ; k < dbSize - shingleSize - hiDataLoc + 1 ; k++ ){
    // Test frame Queue
    if( ! frameHashTable[ (int)( k * oneOverW ) ] ){
      float sk = getSNorm(k);
      float pk = powers[k];      
      if( !ISNAN(pk) &&  !(sk==NEGINF) && pk > pwr_abs_thresh && 
	  ( !useRelativeThreshold || inPwMn/pk < pwr_rel_thresh ) ){
	// The norm matched filter distance  is the Euclidean distance between the vectors
	dist = 2 - 2 / ( qN0 * sk ) * getDD( k ); // squared Euclidean distance
	dRadius = fabs( dist - iRadius ); // Distance from search radius      
	// Perform min-dist search
	if( dRadius < minD ){ // prefer matches at front
	  minD = dRadius;
	  minDist = dist;
	  winner = k;
	}
      }
    }
  }
  if( frameQueue.size() > (unsigned int)queueSize ){
    // New size is smaller
    // Reset frames beyond queueSize
    unsigned int sz = (unsigned int) frameQueue.size();
    unsigned int qs = (unsigned int) queueSize;
    for( unsigned int k = 0 ; k < sz - qs ; k++ ){
      frameHashTable[ frameQueue.at(qs + k) ] = 0;
    }
  }
  else 
    if( frameQueue.size() < (unsigned int) queueSize ){
      // New size is larger, set remainder to 0
      frameQueue.resize(queueSize, 0);
    }
  // FIX ME: the frame queue hash table logic is a bit off when queue sizes (or window sizes) change
  if(winner>-1)
    pushFrameQueue((int)(winner*oneOverW),queueSize); // Hash down frame to hop boundary and queue
  else if( !frameQueue.empty() ){
    frameHashTable[ frameQueue.at(0) ]=0;
    frameQueue.pop_front();
  }
  dist = minDist;
  return winner;
}

int Matcher::multiMatch(ResultQueue* resultQueue, uint32_t numHits, float matchRadius, int shingleSize, int dbSize, 
			int loDataLoc, int hiDataLoc, int queueSize, float inPwMn, 
			float* powers, float pwr_abs_thresh, float pwr_rel_thresh,
			std::vector<off_t>* track_offsets){
  dist=0.0f;
  //float dRadius=0.0f;
  //float iRadius=matchRadius*matchRadius; // squared search radius
  MatchResult matchResult;
  // Perform the recursive Matched Filtering (core match algorithm)
  execute(shingleSize, dbSize, loDataLoc, hiDataLoc);
  float qN0 = getQNorm(0); // pre-calculate denominator coefficient
  // DD now contains (1 x N) multi-dimensional matched filter output
  ss_sample oneOverW = 1.0f / shingleSize;  
  for(int k= loDataLoc ; k < dbSize - shingleSize - hiDataLoc + 1 ; k++ ){
    // Test frame Queue
    if( ! frameHashTable[ (int)( k * oneOverW ) ] ){
      float sk = getSNorm(k);
      float pk = powers[k];      
      if( !ISNAN(pk) &&  !(sk==NEGINF) && pk > pwr_abs_thresh && 
	  ( !useRelativeThreshold || inPwMn/pk < pwr_rel_thresh ) ){
	// The norm matched filter distance  is the Euclidean distance between the vectors
	dist = 2 - 2 / ( qN0 * sk ) * getDD( k ); // squared Euclidean distance
	//dRadius = fabs( dist - iRadius ); // Distance from search radius      
	// Perform min-dist search
	matchResult.track_pos = k;
	matchResult.dist = dist;
	resultQueue->push(matchResult);
	//	if(resultQueue->size() > numHits)
	//	  resultQueue->pop_back(resultQueue->end()-1);
      }
    }
  }
  return 104;
}

float Matcher::getDist(){
  return dist;
}

// Push a frame onto the frameQueue 
// and pop the last frame from the queue
void Matcher::pushFrameQueue(int slot, int queueSize){  
  while(!frameQueue.empty() && frameQueue.size()>=(unsigned)queueSize){
    frameHashTable[frameQueue.at(0)]=0;
    frameQueue.pop_front();
  }
  
  if(queueSize){
    frameHashTable[slot]=1;
    frameQueue.push_back(slot);
  }
}

void Matcher::clearFrameQueue(){
  frameQueue.clear();
  int* p = frameHashTable;
  int mx = maxDBSize;
  while(mx--){
    *p++ = 0;
  }
}
