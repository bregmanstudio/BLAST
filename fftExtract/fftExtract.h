
#ifndef _FFTEXTRACT_H
#define _FFTEXTRACT_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <iostream>
#include <fstream>
#include <sndfile.h>
#include <math.h>
#include <fftw3.h>
#include <assert.h>


#if !defined(M_TWOPI)
#define M_TWOPI (M_PI * 2.0)
#endif

#define DEFAULT_FFT_SIZE 16384
#define DEFAULT_WIN_SIZE 8192
#define DEFAULT_HOP_SIZE 4410
#define DEFAULT_SLICE_SIZE 250 // length of hop in milliseconds
#define DEFAULT_FEAT_BANDS 12  // if no feature is given cqt of this many bands 
#define SILENCE_THRESH -6.0    // -60dB in Bels

//#define DUMP_CQT
//#define DUMP_DCT

#define _FFTEXTRACT_MAIN_

using namespace std;

inline double do_nothing(double d, double c){ return d;}
inline double log10_clip(double d, double c){ d = log10(d); if (d<c) d=c; return d;}

void print_usage(char* appname);

class fftExtract {
 private:
  SNDFILE *inFile;
  SF_INFO sfinfo;
  unsigned int readCount;  
  char *inFileName;
  char *outFileName;
  ofstream *outFile;
  ifstream *beatFile;
  char *fftwPlanFileName;
  FILE *fftwPlanFile;

  double *audioData;
  double *extractedData;
  double *beatSynchData;
  double *fftOut;
  double *powerOut;
  double *harmonicityOut;
  double *cqtOut;
  double *hammingWindow;
  double *logFreqMap;
  double *in;
  double loEdge;
  double hiEdge;
  fftw_complex *out;
  double *dp,*ip,*op,*wp;
  fftw_complex *cp;
  fftw_plan p;
  int useWisdom;
  int verbose;
  int power;
  int harmonicity;
  int magnitude;
  int using_log;
  int spectrogram;
  int constantQ;
  int chromagram;
  int cepstrum;
  int lowceps;
  int lcqft;
  int beats;
  int float_output;
  double* CQT;
  double* DCT;

  int fftN;
  int winN;
  int hopN;
  int outN;
  int cqtN;
  int dctN;
  int extractN;
  int hopTime;
  bool usingS;
  int extractChannel;
       
  double (*log_fn) (double, double);

  void open_output_file(void);
  void initialize(void);
  void extract(void);
  void makeHammingWindow(void);
  void makeLogFreqMap(void);
  void makeDCT(void);
  void windowAudioData(void);
  void extractFeatures(void);
  void magnitudeOutput(double*, int);
  void extractConstantQ(void);
  void iExtractMFCC(void);
  void updateBuffers(void);
  void zeroBuf(double*, unsigned int);
  void beatSum(double *, double *, double);
  void beatDiv(double *, double);
  void newBeat(double *, double *, double);
  int nextHigherPow2(int);
  void error(const char* s1, const char* s2="");
  void processArgs(int argc, char* argv[]);

public:
  fftExtract(int argc, char* argv[]);
  ~fftExtract(void);
}; /* class fftExtract */


#endif
