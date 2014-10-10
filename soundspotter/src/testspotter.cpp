#include <stdio.h>
#include <time.h>
#include <string.h>

#include "SoundSpotter.h"
#include "SoundFile.h"


#define TEST_MEMORY(a) try {  \
    a;			      \
  }			      \
  catch (const char *str){    \
    printf("Exception thrown: %s\n", str); \
  }

#define TEST_FILE "bell.wav"

#define ITER_MAX (1000)

#define MAXSTR (1024)

unsigned int N = 2048;

const float MAXRANDINT = 32767.0f;

char strbuf[MAXSTR];

// test0001: open sound file, set SoundSpotter buffer to SoundFile buffer
int test0001(SoundSpotter* SS, SoundFile* SF){
  int retval = SF->sfOpen(strbuf);
  if(retval<0){
    printf("Could not open %s\n", strbuf);
    fflush(stdout);
    return 1;
  }  
  SS->setAudioDatabaseBuf(SF->getSoundBuf(), SF->getBufLen(), SF->getNumChannels());
  return 0;
}

// test0002: perform feature extraction on a test sound
int test0002(SoundSpotter* SS){
  SS->setStatus(EXTRACT);
  printf("EXTRACTING %s...", strbuf);
  fflush(stdout);
  SS->run(N,NULL,NULL,NULL,NULL);
  SS->setStatus(SPOT);
  printf("\nDONE.\n");
  fflush(stdout);
  return 0;
}

// test0003: spot mode test
int runSpotter(SoundSpotter* SS){
  float inputFeatures[N], inputSamps[N], outputFeatures[N], outputSamps[N];  
  unsigned int iter = 0, nn = 0;
  unsigned int iterMax = ITER_MAX;

  iter = 0;
  while(iter++<iterMax){
    nn=0;
    while(nn<N){
      inputFeatures[nn]=0.0f;
      inputSamps[nn]=  2.0f * rand() / MAXRANDINT - 1.0f ; //(nn%512)/512.0f;
      //printf("%4.3f ", inputSamps[nn]);
      outputFeatures[nn]=0.0f;
      outputSamps[nn]=0.0f;
      nn++;
    }
    SS->run(N, inputFeatures, inputSamps, outputFeatures, outputSamps);    
    printf("%d ", SS->reportResult());
    fflush(stdout);
  }
  printf("\n\n");
  fflush(stdout);
  return 0;
}


// test0003: spot mode test
int test0003(SoundSpotter* SS){
  printf("SPOT...");
  fflush(stdout);
  SS->setStatus(SPOT);
  return runSpotter(SS);
}

// test0004: livespot mode test
int test0004(SoundSpotter* SS){
  SS->setStatus(EXTRACTANDSPOT);
  printf("EXTRACTANDSPOT...");
  fflush(stdout);
  return runSpotter(SS);
}


int main(int argc, char* argv[]){
  SoundSpotter * SS = 0;
  SoundFile * SF = 0;

  if(argc<2){
    strncpy(strbuf, TEST_FILE, MAXSTR);
  }
  else{
    strncpy(strbuf, argv[1], MAXSTR);
  }
  srand( (unsigned int) time(NULL) ); // seed the random number generator

  TEST_MEMORY(SS = new SoundSpotter(44100, N, 2))
  TEST_MEMORY(SF = new SoundFile())

  // open sound file, set SoundSpotter buffer
  int ret = test0001(SS,SF);
  
  // feature extraction
  ret += test0002(SS);

  // SPOT mode test
  ret += test0003(SS);

  // EXTRACTANDSPOT mode test
  ret += test0004(SS);

  if(ret)
    printf("%d tests had errors.\n", ret);
  fflush(stdout);  

  TEST_MEMORY(delete SF)
  TEST_MEMORY(delete SS)


  return 0;
}

