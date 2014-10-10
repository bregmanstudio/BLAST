#include <stdio.h>

#ifdef FFMPEG
#include "ffmpeginterface.h"
#else
#include <sndfile.h>
#endif

#define SF_MAX_NUM_FRAMES 7200*44100

class SoundFile {
 protected:
  SNDFILE *inFile;    // If Extracting a soundFile
  SF_INFO sfinfo;     // If extracting from an external soundFile
  float* soundBuf;    // If extracting from an external soundFile
  unsigned long loadSound();
  unsigned long numFrames; // total number of frames not samples
  void sfCleanUp();
 public:
  SoundFile();
  ~SoundFile();
  int sfOpen(const char* inFileName);  
  float* getSoundBuf();
  int getNumChannels();
  unsigned long getBufLen();
};
