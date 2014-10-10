#include "SoundFile.h"

SoundFile::SoundFile() : 
  inFile(0),
  soundBuf(0),
  numFrames(0){}

SoundFile::~SoundFile(){
  sfCleanUp();
}

void SoundFile::sfCleanUp(){
  if (inFile){
    delete[] soundBuf;
    soundBuf = 0;
	if(inFile){
		sf_close(inFile);
	    inFile = 0;
	}
  }
}

// Attempt to open sound file
// return -1 if cannot open soundFile
// else return number of frames read
int SoundFile::sfOpen(const char* inFileName){
  sfCleanUp();
  // Open sound file
  sfinfo.format = 0;
  if (! (inFile = sf_open (inFileName, SFM_READ, &sfinfo))){   
    return -1;
  } 
  /*  if(!sf_format_check (&sfinfo)){
      printf("SoundFile format not supported: %0X\n", sfinfo.format);
     return -1;
    }
  */
  printf("sfinfo.format = %0X, %d\n", sfinfo.format, sfinfo.channels);
  fflush(stdout);
  numFrames = (unsigned long) sfinfo.frames;
  if(numFrames > SF_MAX_NUM_FRAMES){
    numFrames = SF_MAX_NUM_FRAMES;
  }
  int result = loadSound();
  return result;
}

// Read the entire soundfile into a new float buffer
unsigned long SoundFile::loadSound(){
  unsigned long numRead;
  delete[] soundBuf;
  soundBuf = new float[numFrames*sfinfo.channels];
  numRead = (unsigned long) sf_readf_float (inFile, soundBuf, numFrames);
  return numRead;
}

float* SoundFile::getSoundBuf(){
  return soundBuf;
}

int SoundFile::getNumChannels(){
  return sfinfo.channels;
}

unsigned long SoundFile::getBufLen(){
  return (unsigned long)numFrames; // this should be ulong
}

