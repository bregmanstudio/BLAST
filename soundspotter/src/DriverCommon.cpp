// These methods serve to rationalize SoundSpotter drivers through common interface
// The SoundSpotter C-API

#include "DriverCommon.h"

// Skeleton C-API constructor
// We will allow better parameterization here soon
// i.e. setting sample rate and window length
// plus ability to dynamically adjust these
SoundSpotter* ss_new(int sr, int windowLength, int numChannels){
  return new SoundSpotter(sr, windowLength, numChannels);
}

int ss_stop(SoundSpotter* x){
  return x->setStatus(STOP);
}


int ss_spot(SoundSpotter* x){
  return x->setStatus(SPOT);
}

int ss_thru(SoundSpotter* x){
  return x->setStatus(THRU);
}

void ss_master(SoundSpotter* x){
  x->setMaster();
}

void ss_slave(SoundSpotter* x){
  x->setSlave();
}

int ss_liveSpot(SoundSpotter* x){
  x->setAudioDatabaseBuf(x->getAudioDatabaseBuf(), x->getAudioDatabaseBufLen(), 1);
  return x->setStatus(EXTRACTANDSPOT);
}

int ss_extract(SoundSpotter* x, ss_sample* soundBuf, unsigned long len, int numChannels){
    x->setAudioDatabaseBuf(soundBuf, len, numChannels);
    x->setStatus(EXTRACT);    
    return 1;
}

int ss_setAudioBuf(SoundSpotter* x, ss_sample* soundBuf, unsigned long len, int numChannels){
    x->setAudioDatabaseBuf(soundBuf, len, numChannels);
    return 1;
}

int ss_dump(SoundSpotter* x, const char *sfName)
{
  if ( ! x->DumpVectors(sfName) ){
    return 0;
  }

  return 1;
}

int ss_load(SoundSpotter* x, const char *sfName)
{
  int result = 0;
  result =  x->LoadVectors(sfName);
  return result;
}


void ss_setQueueSize(SoundSpotter* x, int i){
  x->setQueueSize(i);
}

void ss_setShingleSize(SoundSpotter* x, int i){
  x->setShingleSize(i);
}

void ss_setLoBasis(SoundSpotter* x, int i){
  x->setMinASB(i);
}

void ss_setBasisWidth(SoundSpotter* x, int i){
  x->setBasisWidth(i);
}

void ss_setMatchRadius(SoundSpotter* x, float r){
  x->setMatchRadius(r);
}

void ss_setLoDataLoc(SoundSpotter* x,  float f){
  x->setLoDataLoc(f);
} 

void ss_setHiDataLoc(SoundSpotter* x, float f){
  x->setHiDataLoc(f);
}

void ss_setEnvFollow(SoundSpotter* x, float f){
  x->setEnvFollow(f);
} 

void ss_setBetaParameter(SoundSpotter* x, float f){
  x->setBetaParameter(f);
} 

void ss_run(SoundSpotter* x,int n,ss_sample* in1,ss_sample* in2,ss_sample* out1,ss_sample* out2){
  x->run(n, in1, in2, out1, out2);
}

int ss_reportResult(SoundSpotter* x){
  return x->reportResult();
}

float ss_reportDistance(SoundSpotter* x){
  return x->reportDistance();
}

void ss_free(SoundSpotter* x){
  delete x;
}


SoundSpotterStatus ss_getStatus(SoundSpotter* x){
  return x->getStatus();
}
