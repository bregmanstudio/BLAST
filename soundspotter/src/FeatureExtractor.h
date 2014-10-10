#ifndef __FEATUREEXTRACTOR_H
#define __FEATUREEXTRACTOR_H

#include <stdio.h>
#include <math.h>
#include <limits>
#include <fftw3.h>
#include <assert.h>
#include <stdlib.h>
#include<iostream>
#include <fstream>


#include "SignalDefs.h"

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEBUGINFO(a) printf(a);fflush(stdout);
//#define DEBUGINFO(a)
//#define DUMP_CQT     // only use this for debugging feature extractor

#define CQ_ENV_THRESH 0.001   // Sparse matrix threshold (for efficient matrix multiplicaton)	

using namespace std;

class FeatureExtractor {
friend class SoundSpotter;

private:
	fftw_plan fftwPlan;
	FILE* fftwPlanFile;
	double* fftIn;           // storage for FFT input
	fftw_complex *fftComplex;    // storage for FFT output
	double* fftPowerSpectrum; // storage for FFT power spectrum
	int fftN;                // linear frequency resolution (FFT) (user)
	int fftOutN;             // linear frequency power spectrum values (automatic)
	int bpoN;                // constant-Q bands per octave (user)
	int cqtN;                // number of constant-Q coefficients (automatic)
	int dctN;                // number of discrete cosine transform coefficients (automatic)
	double* CQT;             // constant-Q transform coefficients
	int* cqStart;            // sparse constant-Q matrix coding indices
	int* cqStop;            // sparse constant-Q matrix coding indices
	double* DCT;             // discrete cosine transform coefficients
	double* cqtOut;          // constant-Q coefficient output storage
	ss_sample* dctOut;          // mfcc coefficients (feature output) storage
	double *logFreqMap;
	double loEdge;
	double hiEdge;
	ss_sample* hammingWin;
	ss_sample winNorm;        // Hamming window normalization factor
 
	void initializeFFTWplan();
    void free_fftw();
	void makeLogFreqMap();
	void makeDCT();
	void computeMFCC(ss_sample* outs1);
	void makeHammingWin();

protected:
	int sampleRate;
	int WindowLength;  // Assumed patch window size PD : [block~ NFFT NFFT/WindowLength 1]

public:
  FeatureExtractor(int samplerate, int windowlength, int fftn);
  ~FeatureExtractor();
  void initializeFeatureExtractor();
  void extractVector(int n2, ss_sample* in1, ss_sample* in2, ss_sample * outs1, float* power, int doMFCC);
  int extractSeriesOfVectors(float *databuf, int numChannels, int buflen, float* vecs, float* powers, int dim, int numvecs);
};


#endif
