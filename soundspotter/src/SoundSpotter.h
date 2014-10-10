#ifndef __SOUNDSPOTTER_H
#define __SOUNDSPOTTER_H

#include <fstream>
#include <stdio.h>
#include <limits>
#include <deque>
#include <math.h>
#include <assert.h>

#include "CircularMatrix.h"
#include "Matcher.h"
#include "FeatureExtractor.h"

//#define SS_DEBUG


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SS_MAX_DATABASE_SECS 7200
#define SS_MAX_SHINGLE_SZ 32
#define SS_MAX_QUEUE_SZ 10000
#define SS_NUM_BASIS 83
#define SS_MAX_RADIUS 4
#define SS_FFT_LENGTH 4096
#define SS_WINDOW_LENGTH 2048


typedef enum {STOP,EXTRACTANDSPOT,EXTRACT,SPOT,THRU,DUMP} SoundSpotterStatus;


class SoundSpotter {
 public:

  SoundSpotter(int sampleRate, int WindowLength, int numChannels=1);
  ~SoundSpotter();

  SoundSpotterStatus getStatus();
  int getShingleSize();
  int getQueueSize();
  int getNASB();
  int getMinASB();
  int getMaxASB();
  int getBasisWidth();
  ss_sample getMatchRadius();
  int getHop();
  int getLoDataLoc();
  int getHiDataLoc();
  ss_sample getEnvFollow();
  float getBetaParameter();
  ss_sample* getAudioDatabaseBuf();   // source sample buffer (external)
  unsigned long getAudioDatabaseBufLen();
  unsigned long getAudioDatabaseFrames();
  int getAudioDatabaseNumChannels();
  int getLengthSourceShingles();
  int getXPtr();
  int getxPtr();
  int getMaxF();

  int setStatus(SoundSpotterStatus stat);
  void setShingleSize(int);
  void setQueueSize(int);
  void setMinASB(int);
  void setBasisWidth(int);
  void setMatchRadius(float);
  void setLoDataLoc(float);
  void setHiDataLoc(float);
  void setEnvFollow(float);
  void setBetaParameter(float);
  void setAudioDatabaseBuf(ss_sample*, unsigned long, int channels=1); // source sample buffer (external)
  void disableAudio();
  void enableAudio();
  void setMaster();
  void setSlave();

  void run(int,ss_sample*,ss_sample*,ss_sample*,ss_sample*);
  int reportResult();  
  float reportDistance();  

  int DumpVectors(const char* filename);
  int LoadVectors(const char* filename);
  void resetBufPtrs();
  int resetShingles(int newSize);
  void resetMatchBuffer();

  // Public static methods
  static void zeroBuf(ss_sample* a, unsigned long len);
  static void zeroBuf(int* a, unsigned long len);

  
protected:
  friend class Matcher;    // allow matcher classes to access SoundSpotter protected fields and methods
  friend class MatchedFilter;
  friend class FeatureExtractor;

  int sampleRate;
  int WindowLength;
  int numChannels;
  int MAX_SHINGLE_SIZE; // longest sequence of vectors
  SeriesOfVectors *sourceShingles;
  SeriesOfVectors *inShingle;
  SeriesOfVectors *sourcePowers;
  SeriesOfVectors *sourcePowersCurrent;
  SeriesOfVectors *inPowers;

  int MAX_Queued;  // maximum length of segment priority queue
  int maxF; // Maximum number of source frames to extract (initial size of database)
  
  int minASB;
  int maxASB;
  int NASB;
  int lastLoFeature;     // persist feature parameters for database norming
  int lastHiFeature;     // persist feature parameters for database norming
  int ifaceLoFeature;    // store interface values to change on shingle boundary
  int ifaceHiFeature;
  int normsNeedUpdate;   // flag to indicate that database norms are dirty
  int isMaster;          // flag to indicate master soundspotter, extracts features
  int audioDisabled;     // whether to process audio
  int basisWidth;
  ss_sample* x;             // SoundSpotter pointer to PD internal buf
  unsigned long bufLen;


  ss_sample *audioOutputBuffer; // Matchers buffer
  int muxi;                // SHINGLEing overlap-add buffer multiplexer index
  ss_sample lastAlpha;    // envelope follow of previous output
  ss_sample* hammingWin2;

  int queueSize;
  int shingleSize;
  int lastShingleSize;
  int ifaceShingleSize;

  int shingleHop;
  unsigned long xPtr;
  int XPtr;
  int lastWinner;          // previous winning frame
  int winner;              // winning frame/shingle in seriesOfVectors match
  Matcher* matcher;        // shingle matching algorithm
  FeatureExtractor* featureExtractor;

  ss_sample pwr_abs_thresh;     // don't match below this threshold
  ss_sample pwr_rel_thresh;     // don't match below this threshold
  ss_sample Radius;         // Search Radius

  int LoK;                 // Database start point marker
  int HiK;                 // Database end point marker
  int lastLoK;
  int lastHiK;
  int ifaceLoK;
  int ifaceHiK;

  ss_sample envFollow;       // amount to follow the input audio's energy envelope
  float betaParameter;       // parameter for probability function (-1.0f * beta)

  SoundSpotterStatus soundSpotterStatus;
#ifdef SS_DEBUG
  ofstream * reporter;
#endif
  void makeHammingWin2();
  void resizeShingles(float newSize);
  void basisRange();
  void spot(int n,ss_sample *ins1,ss_sample *ins2, ss_sample *outs1, ss_sample*outs2); // pre-computed database
  void liveSpot(int n,ss_sample *ins1,ss_sample *ins2, ss_sample *outs1, ss_sample*outs2); // on-the-fly database
  void synchOnShingleStart();
  int checkExtracted(); // check() if source vectors have been extracted  
  void match();  // check if there is sufficient data and energy above thresh
  void sampleBuf(); // re-synthesis buffer (overlap-add previous and current output)
  int incrementMultiplexer(int multiplex, int sz); // counter for output buffer train
  void updateAudioOutputBuffers(ss_sample* out);     // multiplex buffer to output
}; // end of class declaration for SoundSpotter

#define SOUNDSPOTTER_INITIALIZERS			\
  MAX_SHINGLE_SIZE(SS_MAX_SHINGLE_SZ),	\
    sourceShingles(0),					\
    inShingle(0),					\
    MAX_Queued(SS_MAX_QUEUE_SZ),	\
    minASB(0),						\
    maxASB(0),						\
    NASB(0),						\
    lastLoFeature(-1),					\
    lastHiFeature(-1),					\
    ifaceLoFeature(3),					\
    ifaceHiFeature(20),					\
    isMaster(1),                                        \
    audioDisabled(0),                                   \
    basisWidth(0),					\
    x(0),						\
    bufLen(0),						\
    audioOutputBuffer(0),				\
    muxi(0),						\
    lastAlpha(0),					\
    hammingWin2(0),					\
    queueSize(0),					\
    shingleSize(0),					\
    lastShingleSize(-1),				\
    ifaceShingleSize(4),				\
    shingleHop(1),					\
    xPtr(0),						\
    XPtr(0),						\
    lastWinner(-1),					\
    winner(-1),						\
    matcher(0),						\
    pwr_abs_thresh(0.000001f),				\
    pwr_rel_thresh(0.1f),				\
    Radius(0.0f),					\
    LoK(-1),						\
    HiK(-1),						\
    lastLoK(-1),					\
    lastHiK(-1),					\
    ifaceLoK(0),					\
    ifaceHiK(0),					\
    envFollow(0.0f),					\
    betaParameter(1.0f),				\
    soundSpotterStatus(STOP)
#endif


