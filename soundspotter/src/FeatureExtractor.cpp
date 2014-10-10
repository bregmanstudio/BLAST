#include <stdarg.h>
#include <stdlib.h>
#include "FeatureExtractor.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#define FFTW_WIS_TEMPLATE "%s/.fftwis"
#define WISLEN 8


FeatureExtractor::FeatureExtractor(int samplerate, int windowlength, int fftn) :
  fftPowerSpectrum(0),
  fftN(fftn),
  CQT(0),             // constant-Q transform coefficients
  DCT(0),             // discrete cosine transform coefficients
  cqtOut(0),          // constant-Q coefficient output storage
  dctOut(0),          // mfcc coefficients (feature output) storage
  logFreqMap(0),
  loEdge(0),
  hiEdge(0),
  hammingWin(0),
  winNorm(0.0f),
  sampleRate(samplerate),
  WindowLength(windowlength)
{
  initializeFeatureExtractor();
}

FeatureExtractor::~FeatureExtractor(){
	delete[] hammingWin;
	delete[] DCT;
	delete[] cqStart;
	delete[] cqStop;
	delete[] CQT;
	delete[] fftPowerSpectrum; 
	delete[] cqtOut; 
	delete[] dctOut; 
	free_fftw();
}

void FeatureExtractor::initializeFeatureExtractor(){
	fftwPlanFile = 0;
	fftIn = 0;
	fftComplex = 0;
	fftPowerSpectrum = 0;
	fftOutN = fftN/2+1;
	bpoN = 12;
	cqtN = 0;
	dctN = 0;
	CQT = 0;
	DCT = 0;
	cqtOut = 0;
	dctOut = 0;
	logFreqMap = 0;

	// Construct transform coefficients
	DEBUGINFO("makeHammingWin...");
	makeHammingWin();
	DEBUGINFO("makeLogFreqMap...");
	makeLogFreqMap();
	DEBUGINFO("DCT...");
	makeDCT(); 

	// FFTW memory allocation
	DEBUGINFO("FFTW...");
	fftIn = (double*) fftw_malloc(sizeof(double)*fftN);
	fftComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*fftOutN);
	fftPowerSpectrum = new double[fftOutN];
	cqtOut = new double[cqtN];
	dctOut = new ss_sample[dctN];

	assert(fftIn && fftComplex && fftPowerSpectrum && cqtOut && dctOut);    
	// FFTW plan cacheing
	DEBUGINFO("FFTWplan...");
	initializeFFTWplan(); // cannot write from VST plugins ?
}

void FeatureExtractor::initializeFFTWplan(){
	fftwPlan = fftw_plan_dft_r2c_1d(fftN, fftIn, fftComplex, FFTW_ESTIMATE);
}

void FeatureExtractor::free_fftw(){
	if(fftIn)
		fftw_free(fftIn);
	if(fftComplex)
		fftw_free(fftComplex);
	if(fftwPlan)
		fftw_destroy_plan(fftwPlan);
}

// FFT Hamming window
void FeatureExtractor::makeHammingWin(){
  float TWO_PI = 2*3.14159265358979f;
  float oneOverWinLenm1 = 1.0f/(WindowLength-1);
  if(hammingWin)
	  delete[] hammingWin;
  hammingWin = new ss_sample[WindowLength];
  for(int k=0; k<WindowLength;k++)
    hammingWin[k]=0.54f - 0.46f*cosf(TWO_PI*k*oneOverWinLenm1);
  float sum=0.0;
  int n=WindowLength;
  ss_sample* w=hammingWin;
  while(n--){
    sum+=*w**w; // Make a global value, compute only once
    w++;
  }
  winNorm = 1.f/(sqrtf(sum*WindowLength));
}

void FeatureExtractor::makeLogFreqMap(){
	int i,j;
	if(loEdge==0.0)
		loEdge = 55.0 * pow(2.0, 2.5/12.0); // low C minus quater tone
	if(hiEdge==0.0)
		hiEdge=8000.0;
	double fratio = pow(2.0,1.0/bpoN);// Constant-Q bandwidth
	cqtN = (int) floor(log(hiEdge/loEdge)/log(fratio));
	if(cqtN<1)
		printf("warning: cqtN not positive definite\n");
	double *fftfrqs = new double[fftOutN]; // Actual number of real FFT coefficients
	double *logfrqs = new double[cqtN];// Number of constant-Q spectral bins
	double *logfbws = new double[cqtN];// Bandwidths of constant-Q bins
	CQT = new double[cqtN*fftOutN];// The transformation matrix
	cqStart = new int[cqtN]; // Sparse matrix coding indices
	cqStop = new int[cqtN];  // Sparse matrix coding indices
	double * mxnorm = new double[cqtN];  // CQ matrix normalization coefficients
	double N = (double)fftN;
	for(i=0; i < fftOutN; i++)
		fftfrqs[i] = i * sampleRate / N;
	for(i=0; i<cqtN; i++){
	  logfrqs[i] = loEdge * pow(2.0,(double)i/bpoN);
	  logfbws[i] = max(logfrqs[i] * (fratio - 1.0), sampleRate / N);
	}
	double ovfctr = 0.5475; // Norm constant so CQT'*CQT close to 1.0
	double tmp,tmp2;
	double* ptr;
	double cqEnvThresh = CQ_ENV_THRESH; //0.001; // Sparse matrix threshold (for efficient matrix multiplicaton)	
	// Build the constant-Q transform (CQT)
	ptr = CQT;
	for(i=0; i < cqtN; i++){
		mxnorm[i]=0.0;
		tmp2=1.0/(ovfctr*logfbws[i]);
		for(j=0 ; j < fftOutN; j++, ptr++){
			tmp=(logfrqs[i] - fftfrqs[j])*tmp2;
			tmp=exp(-0.5*tmp*tmp);
			*ptr=tmp; // row major transform
			mxnorm[i]+=tmp*tmp;
		}      
		mxnorm[i]=2.0*sqrt(mxnorm[i]);
	}

#ifdef DUMP_CQT // manual inspection of CQT to make transform more efficient
	ofstream dumper ("CQT.txt");
	if(!dumper){
	  fprintf(stderr, "Cannot open file CQT.txt");
	  fflush(stderr);
	  exit(1);
	}
	  
#endif
	// Normalize transform matrix for identity inverse
	ptr = CQT;    
	for(i=0; i < cqtN; i++){
	  cqStart[i] = 0;
	  cqStop[i] = 0;
	  tmp = 1.0/mxnorm[i];
	  for(j=0; j < fftOutN; j++, ptr++){
	    *ptr *= tmp;
	    if( ( ! cqStart[i] ) && ( cqEnvThresh < *ptr ) ){
	      cqStart[i] = j;
	    }
	    else if( ( ! cqStop[i] ) && cqStart[i] && ( *ptr < cqEnvThresh ) ){
	      cqStop[i] = j;
	    }
	  }
#ifdef DUMP_CQT
	    dumper << cqStart[i] << " " << cqStop[i] << endl;
#endif
	}
#ifdef DUMP_CQT
	dumper.close();
#endif
	// cleanup local dynamic memory
	delete[] fftfrqs;
	delete[] logfrqs;
	delete[] logfbws;
	delete[] mxnorm;
}

// discrete cosine transform
void FeatureExtractor::makeDCT(){
	int i,j;
	double nm = 1 / sqrt( cqtN / 2.0 );
	dctN = cqtN; // Full spectrum DCT matrix
	DCT = new double[ cqtN * dctN ];
	assert( DCT );
	for( i = 0 ; i < dctN ; i++ )
		for ( j = 0 ; j < cqtN ; j++ )
			DCT[ i * cqtN + j ] = nm * cos( i * (2 * j + 1) * M_PI / 2 / cqtN  );
	for ( j = 0 ; j < cqtN ; j++ )
		DCT[ j ] *= sqrt(2.0) / 2.0;
}

void FeatureExtractor::computeMFCC(ss_sample* outs1){
	double x = 0 ,y = 0;

	fftw_execute(fftwPlan);

	fftw_complex* cp=fftComplex; // the FFTW output
	double* op=fftPowerSpectrum; // the MFCC output
	// Compute linear power spectrum
	int c=fftOutN;
	while(c--){
		x=*(((double *)cp) + 0); // Real
		y=*(((double *)cp) + 1); // Imaginary
		*op++=x*x+y*y; // Power
		cp++;
	}

	int a = 0,b = 0;
	double *ptr1 = 0, *ptr2 = 0, *ptr3 = 0;
	ss_sample* mfccPtr = 0;

	// sparse matrix product of CQT * FFT
	assert( CQT && DCT );
	for( a = 0; a<cqtN ; a++ ){
		ptr1 = cqtOut + a; // constant-Q transform vector
		*ptr1=0.0;
		ptr2 = CQT + a * fftOutN + cqStart[a];
		ptr3 = fftPowerSpectrum + cqStart[a];
		b = cqStop[a] - cqStart[a];
		while(b--){
			*ptr1 += *ptr2++ * *ptr3++;
		}
	}

	// LFCC ( in-place )
	assert( dctN && ( dctN <= cqtN ) );
	a = cqtN;
	ptr1 = cqtOut;
	while( a-- ){
		*ptr1 = log10( *ptr1 );
		ptr1++;
	}
	a = dctN;
	ptr2 = DCT; // point to column of DCT
	mfccPtr = outs1;
	while( a-- ){
		ptr1 = cqtOut;  // point to cqt vector
		*mfccPtr = 0.0; 
		b = cqtN;
		while( b-- )
			*mfccPtr += (ss_sample)(*ptr1++ * *ptr2++);
		mfccPtr++;	
	}
}

// extract feature vectors from multichannel audio float buffer (allocate new vector memory)
int FeatureExtractor::extractSeriesOfVectors(float *databuf, int numChannels, int buflen, float* vecs, float* powers, int dim, int numvecs){
  ss_sample *w, *ptr1, *ptr2;  // moving pointer to hamming window
  double* o;     // moving pointer to fftIn
  ss_sample* in; // moving pointer to input audio buffer (possibly multi-channel)
  float oneOverWindowLength = 1.0f / WindowLength;  // power normalization
  int xPtr, XPtr;
  for(xPtr = 0, XPtr=0; xPtr < buflen - WindowLength*numChannels && XPtr < numvecs; xPtr+=WindowLength*numChannels, XPtr++){
    o = fftIn;
    in = databuf + xPtr;
    w = hammingWin;    
    int n2 = WindowLength;
	float val, sum = 0;
    while( n2-- ){ 
	  val = *in;
      *o++ = val * *w++ * winNorm;
	  sum += val * val;
      in += numChannels; // extract from left channel only
    }
	*powers++ = sum * oneOverWindowLength; // database powers calculation in Bels
    n2 = fftN-WindowLength; // Zero pad the rest of the FFT window
    while(n2--) 
      *o++=0.0;
    computeMFCC(dctOut); 
    ptr1 = vecs + XPtr * dctN;
    ptr2 = dctOut;
    n2 = dctN;
    while(n2-- ) // Copy to series of vectors
      *ptr1++ = *ptr2++;
  } 	
  return XPtr;
}

// extract feature vectors from MONO input buffer
void FeatureExtractor::extractVector(int n, ss_sample* in1, ss_sample* in2, ss_sample * outs1, float* power, int doMFCC){
  ss_sample *w=hammingWin;
  double *o=fftIn;
  ss_sample *in=in2; // the MONO input buffer
  float val, sum = 0; 
  float oneOverWindowLength = 1.0f / WindowLength;  
  // window input samples
  while(n--){ 
	val = *in++;
    sum += val * val;
    *o++ = val * *w++ * winNorm;
  }
  *power = sum * oneOverWindowLength; // power calculation in Bels   
  // zero pad the rest of the FFT window
  n=fftN-WindowLength; 
  while(n--) *o++=0.0;  // <====== FIX ME: This is redundant (in theory, but pointers might get re-used in SP chain)
  // extract MFCC and place result in outs1
  if(doMFCC){
    computeMFCC(outs1);
  }
  else{
    n=dctN;
    while(n--){
      *outs1++ = *in2++;
    }
  }
}
