#include "SoundSpotter.h"


// multi-channel soundspotter
// expects MONO input and possibly multi-channel DATABASE audio output
SoundSpotter::SoundSpotter(int sampleRate, int WindowLength, int numChannels) : 
sampleRate(sampleRate),
WindowLength(WindowLength),
numChannels(numChannels),
SOUNDSPOTTER_INITIALIZERS
{
	DEBUGINFO("feature extractor...")
		featureExtractor = new FeatureExtractor(sampleRate, WindowLength, SS_FFT_LENGTH);
	NASB = featureExtractor->dctN;
	DEBUGINFO("maxF...")
		maxF = (int)( ( (float)sampleRate / (float)WindowLength ) * SS_MAX_DATABASE_SECS );
	DEBUGINFO("makeHammingWin2...")
		makeHammingWin2();
	printf("inShingle...");
	inShingle = new SeriesOfVectors(NASB, MAX_SHINGLE_SIZE);
	inPowers = new SeriesOfVectors(MAX_SHINGLE_SIZE, 1);

	DEBUGINFO("audioOutputBuffer...")
		audioOutputBuffer = new ss_sample[WindowLength*MAX_SHINGLE_SIZE*numChannels]; // fix size at constructor ?    
	assert(inShingle && audioOutputBuffer);
	resetShingles(maxF); // Some default number of shingles
#ifdef SS_DEBUG
	reporter = new ofstream("C:\\winners.txt");
#endif
}


SoundSpotter::~SoundSpotter(){  
	delete[] hammingWin2;
	delete[] audioOutputBuffer;  
	delete inShingle;
	delete inPowers;
	delete sourceShingles;
	delete sourcePowers;
	delete sourcePowersCurrent;
	delete matcher;
	delete featureExtractor;
#ifdef SS_DEBUG
	if(reporter){
		reporter->close();
		delete reporter;
	}
#endif
}


SoundSpotterStatus SoundSpotter::getStatus(){
	return soundSpotterStatus;
}

int SoundSpotter::getShingleSize(){
	return shingleSize;
}

int SoundSpotter::getQueueSize(){
	return queueSize;
}

int SoundSpotter::getHop(){
	return shingleHop;
}

int SoundSpotter::getNASB(){
	return NASB;
}

int SoundSpotter::getMinASB(){
	return minASB;
}

int SoundSpotter::getMaxASB(){
	return maxASB;
}

int SoundSpotter::getBasisWidth(){
	return basisWidth;
}

ss_sample SoundSpotter::getMatchRadius(){
	return Radius;
}

int SoundSpotter::getLoDataLoc(){
	return LoK;
}

int SoundSpotter::getHiDataLoc(){
	return HiK;
}

ss_sample SoundSpotter::getEnvFollow(){
	return envFollow;
}

float SoundSpotter::getBetaParameter(){
	return betaParameter;
}

// FIXME: source sample buffer (possibly externally allocated)
// Require different semantics for externally and internally 
// allocated audio buffers
ss_sample* SoundSpotter::getAudioDatabaseBuf(){
	return x;    
}
// samples not frames (i.e. includes channels)
unsigned long SoundSpotter::getAudioDatabaseBufLen(){
	return bufLen;
}

// samples not frames (i.e. includes channels)
unsigned long SoundSpotter::getAudioDatabaseFrames(){
	return bufLen / numChannels;
}

int SoundSpotter::getAudioDatabaseNumChannels(){
	return numChannels;
}

int SoundSpotter::getLengthSourceShingles(){
	return (int)ceil(bufLen/(double)(numChannels*WindowLength));
}

int SoundSpotter::getXPtr(){
	return XPtr;
}

int SoundSpotter::getxPtr(){
	return xPtr;
}

int SoundSpotter::getMaxF(){
	return maxF;
}

// Implement triggers based on state transitions
// Status changes to EXTRACT or EXTRACTANDSPOT
// require memory checking and possible allocation
//
// FIXME: Memory allocation discipline is suspicious
// Sometimes the audio buffer is user allocated, other times it is auto allocated
// EXTRACT: requires a source buffer (x) to be provided, allocates database
// EXTRACTANDSPOT: allocates its own source buffer and database
int SoundSpotter::setStatus(SoundSpotterStatus stat){

	if( stat==soundSpotterStatus ){
		printf("failed no new state\n");
		return 0; // fail if no new state
	}  

	switch (stat) {

  case SPOT:
	  break;

  case EXTRACT: 
  case EXTRACTANDSPOT:
#ifdef SS_DEBUG
	  *reporter << "extract : bufLen=" << getAudioDatabaseBufLen() << " c=" << getAudioDatabaseNumChannels() << " f=" << getAudioDatabaseFrames() << endl;
#endif
	  if ( stat==EXTRACT && !getAudioDatabaseBuf() ) {
		  printf("failed no audio\n");
		  return 0; // fail if no audio to extract (user allocated audio buffer)
	  }
	  if( stat==EXTRACTANDSPOT && !getAudioDatabaseBuf() ) {
		  setAudioDatabaseBuf(new ss_sample[maxF*WindowLength], maxF*WindowLength, 1);  // allocate audio buffer
	  }
	  break;

  case STOP: 
  case THRU: 
  case DUMP:
	  break;

  default :
	  return 0;
	}

	soundSpotterStatus=stat;

	return 1;

}

void SoundSpotter::setMaster(){
  isMaster=1;
}

void SoundSpotter::disableAudio(){
  audioDisabled=1;
}

void SoundSpotter::enableAudio(){
  audioDisabled=0;
}

void SoundSpotter::setSlave(){
  isMaster=0;
}

void SoundSpotter::setShingleSize(int s){
	if(s!=shingleSize && s>0 && s<=MAX_SHINGLE_SIZE)
		ifaceShingleSize=s;
}

void SoundSpotter::setQueueSize(int s){
	if(s<MAX_Queued && s >=0) 
		queueSize=s;
}

void SoundSpotter::setMinASB(int m){
	if(m<0){
		m = 0;
	}
	else if (m>NASB){
		m = NASB;
	}
	ifaceLoFeature=m;
	basisRange();
}

void SoundSpotter::basisRange(){
	ifaceHiFeature=(ifaceLoFeature+basisWidth>NASB)?NASB:ifaceLoFeature+basisWidth;
}


void SoundSpotter::setBasisWidth(int w){
	if(w<1){
		w = 1;
	}
	else if (w>NASB){
		w = NASB;
	}
	basisWidth=w;
	basisRange();
}

void SoundSpotter::setMatchRadius(float r){
	if(r>=0 && r<=4)
		Radius=r;
}

void SoundSpotter::setLoDataLoc(float l){
	l*= ((float)sampleRate/WindowLength);
	if(l<0){
		l=0;
	}
	else if( l > getLengthSourceShingles()){
		l = (float)getLengthSourceShingles();
	}
	ifaceLoK=(int)l;
}

void SoundSpotter::setHiDataLoc(float h){
	h*= ((float)sampleRate/WindowLength);
	if(h<0){
		h=0;
	}
	else if( h > getLengthSourceShingles()){
		h = (float)getLengthSourceShingles();
	}
	ifaceHiK=(int)h;
}

void SoundSpotter::setEnvFollow(float e){
	if(e>=0 && e<=1)
		envFollow=e;
}

void SoundSpotter::setBetaParameter(float b){
	if(b>=0 && b<=10)
		betaParameter=b;
}

void SoundSpotter::setAudioDatabaseBuf(ss_sample* b, unsigned long l, int channels){
#ifdef SS_DEBUG
	*reporter << "setAudioDatabaseBuf : len=" << l << " c=" << channels << endl;
#endif

	numChannels = channels;
	x=b;
	l = l*channels;
	if (l > (unsigned long)maxF*(unsigned)WindowLength*channels)
		l = maxF*WindowLength*channels;
	bufLen=l;
	xPtr=0;
}

// Buffer reset for EXTRACT and EXTRACTANDSPOT
void SoundSpotter::resetBufPtrs(){
	XPtr=0;
	xPtr=0;
	/* #ifdef SS_DEBUG
	*reporter << "resetBufPtrs : XPtr=0, xptr=0" << endl;
	#endif   */
	if(inShingle && sourceShingles){
		int numCols = sourceShingles->getCols();
		int numRows = sourceShingles->getRows();
		int k = 0;

		// zero-out source shingles
		for(k = 0; k < numCols ; k++){
			zeroBuf(sourceShingles->getCol(k), numRows);
		}
		numCols = inShingle->getCols();
		// zero-out inShingle
		for(k = 0; k < numCols ; k++){
			zeroBuf(inShingle->getCol(k), numRows);
		}

		// zero-out power sequences
		zeroBuf( inPowers->getCol(0), inPowers->getRows() );
		zeroBuf( sourcePowers->getCol(0), sourcePowers->getRows() );
	}

	// audio output buffer
	zeroBuf(audioOutputBuffer, SS_MAX_SHINGLE_SZ*WindowLength*numChannels);
}

// This half hamming window is used for cross fading output buffers
void SoundSpotter::makeHammingWin2(){
	if(hammingWin2)
		delete[] hammingWin2;
	hammingWin2 = new ss_sample[WindowLength];
	float TWO_PI = 2*3.14159265358979f;
	float oneOverWinLen2m1 = 1.0f/(WindowLength*2-1);
	for(int k=0; k<WindowLength;k++)
		hammingWin2[k]=0.54f - 0.46f*cosf(TWO_PI*k*oneOverWinLen2m1);
}


int SoundSpotter::DumpVectors(const char* filename){
	std::ofstream dumper(filename, ofstream::binary);
	if(!dumper){
		//post("SoundSpotter: Cannot open output file: %s",filename);
		return 0;
	}
	if(XPtr==0){
		//post("SoundSpotter: No Vectors to dump!");
	}
	else{
		//m_stop(0,0);
		dumper.write((char*)&NASB, sizeof(int));
		for(int k=0; k<XPtr;k++)
			dumper.write((char*)sourceShingles->getCol(k), NASB * sizeof(ss_sample));
	}
	//post("Features saved: %d", XPtr);
	return 1;
}

int SoundSpotter::LoadVectors(const char* filename){
	std::ifstream loader(filename, ifstream::binary);
	int k=0;

	if(!loader){
		//post("SoundSpotter: Cannot open input file: %s", filename);
		return 0;
	}
	else{
		//m_stop(0,0);
		// Assume new shingles will load for existing audio buffer
		int numVectors = getLengthSourceShingles();
		numVectors = resetShingles(numVectors);
		int dummy;
		if(!loader.eof())
			loader.read((char*)&dummy, sizeof(int));
		if(dummy!=NASB)
			printf("Loaded feature set dimension mismatch %d!=%d\n", dummy, NASB);
		while(!loader.eof() && k<numVectors){
			loader.read((char*)sourceShingles->getCol(k), NASB * sizeof(ss_sample));
			if(!loader.eof())
				k++;
		}
		XPtr=k;
		xPtr=k*WindowLength*getAudioDatabaseNumChannels();
		setAudioDatabaseBuf(getAudioDatabaseBuf(), xPtr/getAudioDatabaseNumChannels(), getAudioDatabaseNumChannels());
	}
	return k;
}

void SoundSpotter::run(int n, ss_sample* ins1, ss_sample* ins2, ss_sample* outs1, ss_sample* outs2){
	int m;
#ifdef SS_DEBUG
	*reporter << " run : status=" << soundSpotterStatus << endl;
#endif

	switch (soundSpotterStatus) {
  case STOP:
	  while(n--)
		  *outs2++=0;
	  return;    

  case THRU:
	  m=0;
	  while(m<WindowLength){
		  outs2[m]=ins2[m];
		  m++;
	  }
	  return;

  case EXTRACT:
	  DEBUGINFO("featureExtractor...")
		  resetShingles( getLengthSourceShingles() );
	  resetMatchBuffer();
	  XPtr = featureExtractor->extractSeriesOfVectors(getAudioDatabaseBuf(), 
		  numChannels, getAudioDatabaseBufLen(), 
		  sourceShingles->getSeries(), sourcePowers->getSeries(), 
		  sourceShingles->getRows(), getLengthSourceShingles());
	  xPtr = getAudioDatabaseBufLen();
	  normsNeedUpdate=1;
#ifdef SS_DEBUG
	  *reporter << " run(EXTRACT) : xPtr=" << xPtr << ", XPtr=" << XPtr << endl;
#endif
	  setStatus(SPOT);
	  return;

  case SPOT:
	  spot(n,ins1,ins2,outs1,outs2);
	  return;

  case EXTRACTANDSPOT:
	  liveSpot(n,ins1,ins2,outs1,outs2);
	  return;

  default:
	  ;  
	}
}

void SoundSpotter::spot(int n,ss_sample *ins1,ss_sample *ins2, ss_sample*outs1, ss_sample*outs2){
	if(!checkExtracted())
		return;// features extracted?
	assert(ins1&&ins2&&outs1&&outs2); // FAIL if any pointers are NULL
	if(!muxi){
		synchOnShingleStart(); // update parameters at shingleStart
	}
	// ins2 holds the audio samples, convert ins2 to outs1 (FFT buffer) 
	featureExtractor->extractVector(n,ins1,ins2,outs1, inPowers->getSeries() + muxi, isMaster ); 
	// insert MFCC into SeriesOfVectors
	inShingle->insert(outs1, muxi);
	// insert shingles into Matcher
	matcher->insert(inShingle, shingleSize, sourceShingles, XPtr, muxi, minASB, maxASB, LoK, HiK);
	// post-insert buffer multiplex incremenet
	muxi = incrementMultiplexer(muxi, shingleSize);
	// Do the matching at shingle end
	if( !muxi ){
		match();
	}
	// generate current frame's output sample and update everything
	updateAudioOutputBuffers(outs2); 
}

int SoundSpotter::incrementMultiplexer(int multiplex, int sz){
	return ++multiplex%sz;
}

// liveSpot() the entry point for streaming seriesOfVector matching
void SoundSpotter::liveSpot(int n,ss_sample *ins1,ss_sample *ins2, ss_sample*outs1, ss_sample*outs2){
	assert(ins1&&ins2&&outs1&&outs2);
	if(!muxi){
		synchOnShingleStart();	// update parameters at shingleStart
	}
	ss_sample* p=x+xPtr; // xPtr is current audio end point in buffer
	ss_sample* q=ins2; // ins2 is the real-time audio input buffer

	// Take input first due to input pointer outs1=ins2 re-assignment in PD
	if(xPtr<=getAudioDatabaseBufLen()-n){
		int nn=WindowLength;
		while(nn--)
			*p++=*q++;
		xPtr+=WindowLength;
	}
	featureExtractor->extractVector(WindowLength,ins1,ins2,outs1,inPowers->getSeries() + muxi, isMaster); 
	inShingle->insert(outs1, muxi); // insert ASP into SeriesOfVectors
	matcher->insert(inShingle, shingleSize, sourceShingles, XPtr, muxi, minASB, maxASB, LoK, HiK);
	if(XPtr<maxF && xPtr<getAudioDatabaseBufLen()-n){
		sourceShingles->setCol( XPtr, outs1 ); // insert input ASP into Source Features
		sourcePowers->getSeries()[XPtr++]=inPowers->getSeries()[muxi]; // insert power into database
	}
	else{
		setStatus(SPOT); // We can no longer extract from live input, there's no more room
	}
	muxi = incrementMultiplexer(muxi, shingleSize);   // post-insert multiplexer increment
	if(!muxi){
		match();   // Do the matching at shingle end
	}
	// generate current frame's output sample and update everything
	updateAudioOutputBuffers(outs2);
}


int SoundSpotter::checkExtracted(){
	if(XPtr==0||getAudioDatabaseBufLen()==0){ 
		printf("You must extract some features from source material first! XPtr=%d,xPtr=%lu",XPtr,getAudioDatabaseBufLen());
		fflush(stdout);
		setStatus(STOP); 
		return 0;
	}
	else
		return 1;
}

// Perform matching on shingle boundary
void SoundSpotter::match(){
	// zero output buffer in case we don't match anything
	zeroBuf(audioOutputBuffer,WindowLength*shingleSize*numChannels); 
	// calculate powers for detecting silence and balancing output with input
	SeriesOfVectors::seriesMean(inPowers->getSeries(),shingleSize,shingleSize);
//	SeriesOfVectors::seriesSqrt(inPowers->getSeries(),shingleSize,shingleSize);
	float inPwMn = inPowers->getSeries()[0];
#ifdef SS_DEBUG
	*reporter << " match : lastWinner=" << lastWinner 
		<< ", winner=" << winner << ", inPwMn=" << inPwMn << endl;
#endif
	if( inPwMn > pwr_abs_thresh ){
		lastWinner=winner; // preserve previous state for cross-fading audio output
		// matched filter matching to get winning database shingle
		winner = matcher->match(Radius, shingleSize, XPtr, LoK, HiK, queueSize, 
			inPwMn, sourcePowersCurrent->getSeries(), pwr_abs_thresh); 
		if(winner>-1){
			sampleBuf(); // fill output audioOutputBuffer with matched source frames
		}
	}
}


// update() take care of the current DSP frame output buffer
//
// pre-conditions
//      multiplexer index (muxi) must be an integer (integral type)
// post-conditions
//      n samples ready in outs2 buffer
void SoundSpotter::updateAudioOutputBuffers(ss_sample* outSamps){
	ss_sample* p2=audioOutputBuffer+muxi*WindowLength*numChannels; // so that this is synchronized on match boundaries
	ss_sample* p1=outSamps;
	int nn=WindowLength*numChannels;
	while(nn--){
		*p1++=*p2++; // multi-cannel output
	}
}

int SoundSpotter::reportResult(){
	return winner;
}

float SoundSpotter::reportDistance(){
	return matcher->getDist();
}

// sampleBuf() copy n*shingleHop to audioOutputBuffer from best matching segment in source buffer x[]
void SoundSpotter::sampleBuf(){ 
	ss_sample* p = audioOutputBuffer; // MULTI-CHANNEL OUTPUT
	ss_sample* q = x+winner*WindowLength*numChannels; 
	float env1 = inPowers->getSeries()[0];
	float env2 = sourcePowers->getSeries()[winner];
	// Envelope follow factor is alpha * sqrt(env1/env2) + (1-alpha)
	// sqrt(env2) has already been calculated, only take sqrt(env1) here
	float alpha = envFollow * sqrtf(env1/env2) + (1-envFollow); 
	if(winner>-1){
		// Copy winning samples to output buffer, these could be further processed
		int nn=WindowLength*shingleSize*numChannels;
		while(nn--){
			*p++=alpha**q++;
		}
		// Cross-fade between current output shingle and one frame past end 
		// of last winning shingle added to beginning of current
		if(lastWinner>-1 && lastWinner<XPtr-shingleSize-1){
			p=audioOutputBuffer;            // first scanning pointer
			q=x+(lastWinner+shingleSize)*WindowLength*numChannels; // one past end of last output buffer
			ss_sample* p1 = x+winner*WindowLength*numChannels; // this winner (first frame)
			ss_sample* w1 = hammingWin2;    // forwards half-hamming pointer
			ss_sample* w2 = hammingWin2+WindowLength-1; // backwards half-hamming pointer
			nn=WindowLength; // first audio output buffer only
			int c;
			while(nn--){
				c = numChannels;
				while(c--){
					*p++= alpha**p1++**w1 + lastAlpha**q++**w2;
				}
				w1++;
				w2--;
			}
		}
	}
	lastAlpha=alpha;
}


int SoundSpotter::resetShingles(int newSize){
	DEBUGINFO("resetShingles()...")

		if(!inShingle){
			fprintf(stderr, "error: inShingle not allocated in SoundSpotter::resetShingles()");
			return 0;
		}

		if(newSize > maxF){
			newSize = maxF;
		}

		if(!sourceShingles){
			DEBUGINFO("allocate new sourceShingles...")
				sourceShingles = new SeriesOfVectors( NASB, newSize);
			sourcePowers = new SeriesOfVectors( newSize, 1);
			sourcePowersCurrent = new SeriesOfVectors( newSize, 1);
			DEBUGINFO("allocate matcher...")
				matcher = new Matcher(MAX_SHINGLE_SIZE, newSize);

			if(!sourceShingles){
				fprintf(stderr, "error: source shingles allocation failed in SoundSpotter::resetShingles()");
				return 0;
			}
			if(!matcher){
				fprintf(stderr, "error: matcher allocation failed in SoundSpotter::resetShingles()");
				return 0;
			}
		}
		resetBufPtrs(); // fill shingles with zeros and set buffer pointers to zero
		return newSize;
}

// perform reset on new soundfile load or extract
void SoundSpotter::resetMatchBuffer(){
	muxi = 0;     // multiplexer index
	lastAlpha = 0.0;       // crossfade coefficient
	winner = -1;
	lastWinner = -1;
	setLoDataLoc(0); 
	setHiDataLoc(0);
	lastShingleSize = -1;
	if(matcher)
		matcher->clearFrameQueue();
	normsNeedUpdate=1;
#ifdef SS_DEBUG
	*reporter << "len: " << getAudioDatabaseBufLen() << endl;
	*reporter << "frm: " << getAudioDatabaseFrames() << endl;
	*reporter << "dbSz: " << getLengthSourceShingles() << endl;
	*reporter << "stat: " << getStatus() << endl;
	*reporter << "quSz: " << queueSize << endl;
	*reporter << "shSz: " << shingleSize << endl;
#endif
}

void SoundSpotter::synchOnShingleStart(){
	if( muxi==0 ){ // parameters and statistics must only change on match boundaries
		LoK = ifaceLoK;
		HiK = ifaceHiK;
		minASB = ifaceLoFeature;
		maxASB = ifaceHiFeature;
		shingleSize = ifaceShingleSize;
		normsNeedUpdate = normsNeedUpdate || !( lastLoFeature == minASB && 
			lastHiFeature==maxASB && 
			lastShingleSize==shingleSize && 
			lastLoK == LoK &&
			lastHiK == HiK);
		// update database statistics based on current search parameters
		if(normsNeedUpdate){ 
			// update the power threshold data
			if(shingleSize!=lastShingleSize){
				sourcePowersCurrent->copy(sourcePowers);
				SeriesOfVectors::seriesMean(sourcePowersCurrent->getSeries(),shingleSize,
					getLengthSourceShingles());
//				SeriesOfVectors::seriesSqrt(sourcePowersCurrent->getSeries(),shingleSize,
//					getLengthSourceShingles());
				lastShingleSize=shingleSize;
				matcher->clearFrameQueue();
			}
			// update the database norms for new parameters
			matcher->updateDatabaseNorms(sourceShingles, shingleSize, getLengthSourceShingles(), 
				minASB, maxASB, LoK, HiK);
			// set the change indicators
			normsNeedUpdate = 0;
			lastLoFeature = minASB;
			lastHiFeature = maxASB;
			lastLoK = LoK;
			lastHiK = HiK;
		}
	}
}

void SoundSpotter::zeroBuf(ss_sample* buf,unsigned long length){
	while(length--)
		*buf++=0.0;
}

void SoundSpotter::zeroBuf(int* buf, unsigned long length){
	while(length--)
		*buf++=0;
}
