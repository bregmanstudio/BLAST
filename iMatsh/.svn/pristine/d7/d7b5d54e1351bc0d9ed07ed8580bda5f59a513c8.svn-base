#ifndef __MATCHER_H
#define __MATCHER_H

#include "MatchedFilter.h"
#include <stdarg.h>
#include <string.h>
#include <fstream>
#include <queue>
#include <set>
#include <algorithm>

using namespace std;

typedef unsigned int uint32_t;

typedef struct matchResult {
  float dist;
  uint32_t track_pos;
} MatchResult;

bool operator< (MatchResult const &a, MatchResult const &b);
bool operator> (MatchResult const &a, MatchResult const &b);

typedef std::priority_queue< MatchResult, std::vector<MatchResult>, std::greater<MatchResult> > ResultQueue;

typedef deque<int > INTDEQUE; // STL implementation of frameQueue uses an INTDEQUE

class Matcher : public MatchedFilter { // declaration and definition of Matcher class for segment matching
 private:
  float dist;       // a similarity value in the range 0..4
  INTDEQUE frameQueue;     // frame queue
  int* frameHashTable;     // indicator vector for hashed frames  
  void pushFrameQueue(int slot, int queueSize); // hash table for previously selected frames
  int useRelativeThreshold;  // flag for relative power thresholding
public:  
  Matcher(); // default constructor
  Matcher(int maxShingleSize, int maxDBSize); // parameterized constructor
  virtual ~Matcher();                 // destructor
  void resize(int maxShingleSize, int maxDBSize);
  int match(float matchRadius, int shingleSize, int dbSize, int loK, int hiK, int queueSize, float inPwMn, float* powers, float pwr_abs_thresh, float pwr_rel_thresh=0.0f, std::vector<off_t>* track_offsets=0);
  int multiMatch(ResultQueue* queue, uint32_t numHits, float matchRadius, int shingleSize, int dbSize, int loK, int hiK, int queueSize, float inPwMn, float* powers, float pwr_abs_thresh, float pwr_rel_thresh=0.0f, std::vector<off_t>* track_offsets=0);
  void clearFrameQueue();
  float getDist();
};

#endif
