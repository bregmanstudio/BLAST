#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <set>
#include <algorithm>

#include <opencv2/opencv.hpp>

#define EXIT_GOOD 104
#define EXIT_BAD 101

using namespace cv;
using namespace std;

typedef struct nnresult {
  float dist;
  float prob;
  float amp;
  uint32_t query_pos;
  uint32_t track_pos;
  uint32_t media_idx; 
  string media;
} NNresult;

bool operator< (NNresult const &a, NNresult const &b);
bool operator> (NNresult const &a, NNresult const &b);

typedef std::multiset<NNresult, std::less<NNresult> > MatshupSet;
typedef MatshupSet::iterator Mit;
typedef std::pair<Mit, Mit> MitPair;

class VideoMashup{
  // private:
 public:

  VideoCapture capture;
  VideoWriter writer;

  Mat frame;      // input frame buffer
  Mat outImg;     // output frame buffer
  Mat outImgF;    // output frame buffer, float (for math funs)
  Mat procImg;    // processing buffer (grayscale)
  Mat procImg2;   // processing buffer (downsample)
  Mat diffImg;    // processing buffer (absdiff)
  Mat prevImg;    // memory buffer (t-1)
  Mat alphaImg;   // processing buffer (alpha mask)
  Mat poissonMaskImg; // Dummy alpha mask image
  Mat zeroBuf;    // processing buffer (blending)
  Mat* outSequence;// sequence of output frames for blending / writing
  Mat blendBuffer; // output buffer for blending frames

  double audFps; // audio frame rate (fftHop)
  double vidFps; // output frames per second
  int    hopSize;// shingle hop size
  double segmentDuration; // time extent of audio hop 1/audFps
  Size outSize; // size of output frame
  uchar motionThresh; // set to detect motion
  int decimationFactor; // set to downsample detection  
  int vidNumFramesCeil; // max number of output frames in segment (ceil)
  double timeSyncLocator;  // A-V time synchronization locator
  double blendFadeRate;
  unsigned int event_counter; // keep track of processed events
  unsigned int outFrame_count; // keep track of written video frames

  vector<string> movieList; // the list of movies (absolute paths)
  ifstream movieListFile;  // the file containing the list of movie paths
  ifstream matshupFile;    // the matshup control file
  MatshupSet matshup;      // memory container for matshup file contents
  int numMatches;      // how many matches (nearest neighbors) per match event
  NNresult nnResult;   // nearest neighbor result structure
  int totalFrameCount; // total number of frames in media
  int width;           // current movie width
  int height;          // current movie height
  double movFps;       // current movie fps
  double movNumFrames; // number of movie frames in a segment
  double startPosSeconds; // current movie locator
  bool displayWindow;  // whether to show progress as display window
  int query_pos;       // current query position (in FFT frames)
  int report_level;    // verbosity of reporting output

  VideoMashup(double audFps, int hopSize, double vidFps, Size outSize, int motionThresh, int decimationFactor);
  ~VideoMashup();

  void error(const char* s1, const char* s2);
  void error(const char* s1, string s2);
  void report(const char* s1, const char* s2, int);
  void report(const char* s1, string s2, int);

  bool initializeFiles(const char* movieListFileName, const char* matshupFileName);
  bool loadMovieList(const char* movieListFileName); // returns true if loaded, false otherwise
  bool getNumMatches();
  bool loadMatshupSet(const char *matshupFileName);
  void setDisplayWindow(bool display);
  void clearOutputBuffers();
  void update(int);
  bool processNextEvent(int); // returns true if OK, false if end/error
  void cleanUp();
  void loadMedia(string filename);
  void setMovieProtectedFramePosition(double frameIdx);
  void processRange(int);
  void writeOutputFrames();
  void extractMovieFrame();
  void exponentiateImage(double alpha, double beta);
  void exponentiateColors(double alpha, double beta);
  void extractAlphaThreshold(int thresh);
  void extractFlickerRegions();
  void poissonBlend(Mat& srcA, Mat& srcB, Mat& dst, Mat& msk);
  void thru();
  void matInfo(Mat& M);
  bool finished();

};


// Implement c++11 to_string overloads when __cplusplus is c++0x
# if __cplusplus < 201103L
string to_string(int val)
{
  return std::to_string(static_cast<long long>(val));
}

string to_string(unsigned val)
{
  return std::to_string(static_cast<unsigned long long>(val));
}

string to_string(long val)
{
  return std::to_string(static_cast<long long>(val));
}

string to_string(unsigned long val)
{
  return std::to_string(static_cast<unsigned long long>(val));
}

string to_string(float val)
{
  return std::to_string(static_cast<long double>(val));
}

string to_string(double val)
{
  return std::to_string(static_cast<long double>(val));
}
#endif


