// fftExtract : libsndfile implementation
//
// Extracts STFT(fftN, winN, hopN)
//
// Michael Casey
// Goldsmiths, University of London

#include "fftExtract.h"

fftExtract::fftExtract(int argc, char* argv[]):
  fftwPlanFile(0),
  beatSynchData(0),
  fftOut(0),    
  powerOut(0),
  harmonicityOut(0),
  cqtOut(0),
  hammingWindow(0),
  in(0),
  loEdge(0.0),
  hiEdge(0.0),
  out(0),
  p(0),
  useWisdom(0),
  verbose(0),
  power(0),
  harmonicity(0),
  magnitude(0),
  using_log(1),
  spectrogram(0),
  constantQ(0),
  chromagram(0),
  cepstrum(0),
  lowceps(3),
  lcqft(0),
  beats(0),
  float_output(0),
  CQT(0),
  DCT(0),
  fftN(0),
  winN(0),
  hopN(0),
  outN(0),
  cqtN(0),
  dctN(0),
  hopTime(0),
  usingS(false),
  extractChannel(0),
  log_fn(log10_clip)
{
  // Command line options
  processArgs(argc, argv);

  // Handle the feature output
  open_output_file();

  // Initialize extractor parameters and memory
  initialize();

  // Perform the extraction
  extract();      
}

void fftExtract::initialize(void){
  //if using the semantic hop size, since we now know the sample rate, set dependant params
  if (usingS || (beats && hopTime)){
    if (verbose){
      printf("Sample Rate: %i\n", sfinfo.samplerate);
    }
    hopN = (int)(0.001*hopTime*sfinfo.samplerate);
    winN = nextHigherPow2(hopN*2);
    fftN = winN*2;
  }
  else{ // Check and audio-set FFT params
    if(!fftN){
      if(hopN){
	fftN = hopN*4;
      }
      else{
	fftN = DEFAULT_FFT_SIZE;
      }
    }    
    if(!winN){
      if(hopN){
	winN = hopN*2;
      }
      else{
	winN=fftN/2;
      }
    }
    if(!hopN){
      hopN=winN/2;
    }
  }

  if(fftN<winN || winN<hopN){
    error("FFT params are not well formed.");
  }
  else if (verbose) {
    printf("fftN=%d, winN=%d, hopN=%d\n", fftN, winN, hopN);
  }

  // Import wisdom
  if(useWisdom) {
    if (!power || !harmonicity) {
      if(!(fftw_import_wisdom_from_file(fftwPlanFile)))
	printf("Warning: failed to import wisdom\n");
    }
  }

  // Output size
  outN = fftN/2+1;

  // Memory allocation
  audioData = new double[sfinfo.channels*winN];
  extractN = outN;
  fftOut = new double[outN]; // Largest possible output is FFT
  extractedData = fftOut;    // Point to FFT output
  makeHammingWindow();    
  if (power) {
    extractN = 1;
    powerOut = new double[1];
    extractedData = powerOut;
  } else if (harmonicity) {
    extractN = 1;
    harmonicityOut = new double[1];
    extractedData = harmonicityOut;
  } else if(constantQ) {
    makeLogFreqMap(); 
    extractN = cqtN; // Modify feature output size here
    cqtOut = new double[cqtN]; // Largest possible output is FFT
    extractedData = cqtOut;  // Point to CQT output
    if(chromagram)
      extractN = constantQ;    // Fold output into one octave
    else if(cepstrum){ // LFCC coefficients
      makeDCT();
      if(!lcqft)
	extractN = dctN; // Modify feature output size
      else
	extractN = cqtN; // inverse MFCC size
    }
  }
  // FFTW memory allocation
  in = (double*) fftw_malloc(sizeof(double)*fftN);
  if (!power && !harmonicity) {
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*outN);
    // FFTW plan
    p = fftw_plan_dft_r2c_1d(fftN, in, out, FFTW_PATIENT);  
    if(!useWisdom && fftwPlanFile)
      fftw_export_wisdom_to_file(fftwPlanFile);  
  }
  // If beat-synchronus output then make beat-synchronus data buffer
  if(beats){
    beatSynchData = new double[extractN];
    zeroBuf(beatSynchData,extractN);
  }
}

// Clean up
fftExtract::~fftExtract(){
  // Close input and output files
  if(inFile)
    sf_close(inFile) ;
  if(fftwPlanFile)
    fclose(fftwPlanFile);
  if(p)
    fftw_destroy_plan(p);
  if(beats && beatFile){
    if(beatFile->is_open())
      beatFile->close();
    delete beatFile;
  }    
  if(in)
    fftw_free(in); 
  if(out)
    fftw_free(out);
  if(audioData)
    delete[] audioData;
  if(fftOut)
    delete[] fftOut;
  if(powerOut)
    delete[] powerOut;
  if(harmonicityOut)
    delete[] harmonicityOut;
  if(cqtOut)
    delete[] cqtOut;
  if(hammingWindow)
    delete[] hammingWindow;
  if(CQT)
    delete[] CQT;
  if(DCT)
    delete[] DCT;
  if(outFile){
    if(outFile->is_open())
      outFile->close();
    delete outFile;
  }
  if(beatSynchData)
    delete[] beatSynchData;
}

void fftExtract::open_output_file(){
  // Open output file
  {
    struct stat buf;
    int code = stat(outFileName, &buf);
    if (code == 0) {
      fprintf(stderr, "Output file exists: %s\n", outFileName);
      exit(1);
    }
  }
  outFile = new ofstream(outFileName, ios::binary);
  if(!outFile){
    printf("Could not open %s for writing\n",outFileName);
    exit(1);
  }
  // Open sound file
  sfinfo.format = 0;
  if (! (inFile = sf_open (inFileName, SFM_READ, &sfinfo))){   
    printf ("Not able to open input file %s.\n", inFileName) ;
    puts (sf_strerror (NULL)) ;
    exit(1);
  } 

  if(extractChannel>sfinfo.channels){
    printf("Extract channel exceeds available channels: defaulting to left\n");
    extractChannel=0;
  }

}

void fftExtract::extract(){ // beat synchronus feature extraction
  double beati,beatj,t=0.0;
  double fileLength = sfinfo.frames/(double)sfinfo.samplerate;
  // Initialize extraction by reading a window's worth of audio
  if(verbose){
    cout << "extracting..." << endl;
    cout.flush();
  }    
  outFile->write( (char*) &extractN, sizeof(int) ); // Feature dimension
  if(beats){
    *beatFile >> beati;
    *beatFile >> beatj;
    if(beatj<=beati)
      error("First two beats are not a sequence.\n");
    if(beati > fileLength)
      error("First beat is past end of file\n");
    if(beatj > fileLength){
      beatj=fileLength;
      std::cerr << "Setting second beat to file length: " << fileLength << std::endl;
    }
    readCount=0;
    while(readCount<(unsigned int)floor(beati*sfinfo.samplerate))
      readCount += sf_readf_double (inFile, audioData, min((unsigned int)winN,(unsigned int)floor(beati*sfinfo.samplerate)-readCount));
    if(readCount<(unsigned int)floor(beati*sfinfo.samplerate)){
      printf("Error reading audioData from file : read %d/%d\n", readCount, winN);
      exit(1);
    }
    t = beati;
  }
  readCount = sf_readf_double (inFile, audioData, winN);
  if(readCount<(unsigned int) winN){
    printf("Error reading audioData from file : read %d/%d\n", readCount, winN);
    exit(1);
  }
  // Extractor loop
  do{
    // process audio frames to FFT buffer
    windowAudioData();
    if (power) {
      powerOut[0] = 0;
      for(int k = 0; k < winN; k++) {
	powerOut[0] += in[k] * in[k];
      }
      powerOut[0] = log_fn(powerOut[0] / winN, SILENCE_THRESH);
    } else if (harmonicity) {
      /* A quick comment about implementation choices here: we are
	 looking for a "fundamental" frequency within a certain
	 range.  We arbitrarily decide that that range is between
	 the loEdge or twice the window frequency, whichever is
	 greater, and one quarter of the hiEdge frequency. */
      int loN = (int) ceil(sfinfo.samplerate / loEdge);
      if (loN > winN / 2) {
	loN = winN / 2;
      }
      int hiN = (int)floor(sfinfo.samplerate / (hiEdge/4));
      double max = 0.0;
      for(int lag = hiN; lag < loN; lag++) {
	double current = 0.0;
	for(int k = 0; k < winN - lag; k++) {
	  current += in[k] * in[k+lag];
	}
	current = current / (winN - lag);
	if(current > max) {
	  max = current;
	}
      }
      harmonicityOut[0] = log_fn(max, SILENCE_THRESH);
    } else {
      // Compute FFT
      fftw_execute(p);
      // Extract power spectrum
      extractFeatures();
    }
    if(beats){
      if(t + hopN/(double)sfinfo.samplerate >=beatj || t >= fileLength - winN/(double)sfinfo.samplerate){
	double interval = (beatj - t) / (beatj - beati);
	beatSum(extractedData, beatSynchData, interval);       // Sum
	beatDiv(beatSynchData, (beatj-beati)*sfinfo.samplerate/(double)hopN); // Average
	if (float_output){
	  float tmp[extractN];
	  int l = extractN;
	  float* p = tmp;
	  while(l--)
	    *p++ = (float)(*beatSynchData++);
	  outFile->write( (const char*) tmp, extractN * sizeof(float) ); 	    	      
	}
	else{
	  outFile->write( (const char*) beatSynchData, extractN * sizeof(double) ); 
	}
	beati = beatj;
	*beatFile >> beatj;
	newBeat(extractedData, beatSynchData, 1 - interval);
      }
      else
	beatSum(extractedData, beatSynchData, 1.0); // Sum
    } else{
      if (float_output){
	float tmp[extractN];
	int l = extractN;
	float* p = tmp;
	double* q = extractedData;
	while(l--)
	  *p++ = (float)(*q++);
	outFile->write( (const char*) tmp, extractN * sizeof(float) ); 	    	      
      }
      else{
	// Write output
	outFile->write( (const char*) extractedData, extractN * sizeof(double) ); 
      }
    }
    // Perform buffer update
    updateBuffers();
    t+=hopN/(double)sfinfo.samplerate; // current frame time
    // Read next window of audioData
    readCount = sf_readf_double(inFile, audioData+(winN-hopN)*sfinfo.channels, hopN);   
  } while( readCount>0 &&  ! ( beats && beatFile->eof() ) );
  outFile->flush();
  outFile->close();      
}

void fftExtract::makeHammingWindow(){
  hammingWindow = new double[winN];
  for(int i=0; i < winN; i++){
    hammingWindow[i] = 0.54 - 0.46*cos(M_TWOPI*i/(winN-1));
  }
}

// Linear to log frequency map for FFT
void fftExtract::makeLogFreqMap(){
  int i,j;
  double BPO = (double) constantQ; // set BPO from command line argument
  double fratio = pow(2.0,1.0/BPO);// Constant-Q bandwidth
  cqtN = (int) floor(log(hiEdge/loEdge)/log(fratio));
  if(cqtN<1)
    printf("warning: cqtN not positive definite\n");
  double fftfrqs[outN]; // Actual number of real FFT coefficients
  double logfrqs[cqtN];// Number of constant-Q spectral bins
  double logfbws[cqtN];// Bandwidths of constant-Q bins
  CQT = new double[cqtN*outN];// The transformation matrix
  double mxnorm[cqtN];  // Normalization coefficients
  double N = (double)fftN;
  for(i=0; i < outN; i++)
    fftfrqs[i] = i * sfinfo.samplerate / N;
  for(i=0; i<cqtN; i++){
    logfrqs[i] = loEdge * exp(log(2.0)*i/BPO);
    logfbws[i] = max(logfrqs[i] * (fratio - 1.0), sfinfo.samplerate / N);
  }
  double ovfctr = 0.5475; // Norm constant so CQT'*CQT close to 1.0
  double tmp,tmp2;
  double* ptr;
  // Build the constant-Q transform (CQT)
  ptr = CQT;
  for(i=0; i < cqtN; i++){
    mxnorm[i]=0.0;
    tmp2=1.0/(ovfctr*logfbws[i]);
    for(j=0 ; j < outN; j++, ptr++){
      tmp=(logfrqs[i] - fftfrqs[j])*tmp2;
      tmp=exp(-0.5*tmp*tmp);
      *ptr=tmp; // row major transform
      mxnorm[i]+=tmp*tmp;
    }      
    mxnorm[i]=2.0*sqrt(mxnorm[i]);
  }

  // Normalize transform matrix for identity inverse
  ptr = CQT;    
  for(i=0; i < cqtN; i++){
    tmp = 1.0/mxnorm[i];
    for(j=0; j < outN; j++, ptr++)
      *ptr*=tmp;
  }
  // 
#ifdef DUMP_CQT
  ptr = CQT;
  for(i=0; i < cqtN; i++){
    for(j=0; j < outN; j++, ptr++)
      cout << *ptr << " ";
    cout << "\n";
  }
#endif
}

// We are going to ignore the first 3 coefficients which are flat
void fftExtract::makeDCT(){
  int i,j;
  double nm = 1 / sqrt( cqtN / 2.0 );
  dctN = cepstrum; // Number of cepstral coefficients
  DCT = new double[ cqtN * ( dctN + lowceps ) ];
  assert( DCT );
  for( i = 0 ; i < dctN + lowceps ; i++ )
    for ( j = 0 ; j < cqtN ; j++ )
      DCT[ i * cqtN + j ] = nm * cos( i * (2 * j + 1) * M_PI / 2 / cqtN  );
  for ( j = 0 ; j < cqtN ; j++ )
    DCT[ j ] *= sqrt(2) / 2;
#ifdef DUMP_DCT
  for(i=lowceps; i < dctN+lowceps; i++){
    for(j=0; j < cqtN; j++)
      cout << DCT[i*cqtN + j] << " ";
    cout << "\n";
  }
#endif
}

void fftExtract::windowAudioData(){
  // Copy [default=left] channel of each frame to input buffer
  if(extractChannel<sfinfo.channels){
    int c=winN;
    ip=in;
    dp=audioData+extractChannel;
    wp=hammingWindow;
    while(c--){
      *ip++=*dp**wp++;
      dp+=sfinfo.channels;
    }    
    // Zero pad if needed
    while(ip<in+fftN)
      *ip++=0.0;
  }else{ // Mix all channels together
    int c=winN;
    int d;
    ip=in;
    dp=audioData;
    wp=hammingWindow;
    double oneOverChannels=1.0/sfinfo.channels;
    while(c--){
      d=sfinfo.channels;
      *ip=0.0;
      while(d--)
	*ip+=*dp++**wp*oneOverChannels;
      wp++;
      ip++;
    }
    // Zero pad if needed
    while(ip<in+fftN)
      *ip++=0.0;      
  }      
}

void fftExtract::extractFeatures(){
  double a,b;
  int c=outN;
  cp=out;
  op=fftOut;
  // Compute linear power spectrum
  while(c--){
    a=*(((double *)cp) + 0); // Real
    b=*(((double *)cp) + 1); // Imaginary
    *op++=a*a+b*b; // Power
    cp++;
  }
  if(constantQ)
    extractConstantQ(); // CQ, CHROM, LFCC

  if(magnitude && !(cepstrum || using_log)){
    magnitudeOutput(extractedData, extractN);
  }
}

void fftExtract::magnitudeOutput(double* op, int c){
  while(c--){
    *op = sqrt( *op );
    op++;
  }
}

void fftExtract::extractConstantQ(){
  int a,b,i;
  double *ptr1, *ptr2, *ptr3;
  // matrix product of CQT * FFT
  a=cqtN;
  i=0;
  while( a-- ){
    ptr1=cqtOut+i;
    *ptr1=0.0;
    ptr2 = CQT+i++*outN;
    ptr3 = fftOut;
    b=outN;
    while(b--)
      *ptr1+=*ptr2++**ptr3++;
    ptr1++;
  }    

  // Chromagram and Cepstrum are mututally exclusive
  // CHROM ( in-place )
  if( chromagram ){
    a=constantQ; // Number of CQ bands in one octave
    ptr1 = cqtOut;
    i=0;
    while(a--){
      int c = 1;
      for(b=constantQ+i; b<cqtN; b+=constantQ){
	*ptr1+=cqtOut[b];   
	c++;
      }   
      *ptr1 /= c; // Normalize by number of bands summed
      *ptr1 = log_fn( *ptr1, SILENCE_THRESH );
      ptr1++;
      i++;
    }
    // LFCC ( out-of-place )
    // Want coefficients 2 to cqtN+1 as 1 to cqtN
  } else if( cepstrum ){
    // copy CQT values to tmp array
    double* tmp = new double[cqtN];
    assert( tmp && dctN && ( dctN <= cqtN ) );
    a = cqtN;
    ptr1 = cqtOut;
    ptr2 = tmp;
    while( a-- )
      *ptr2++ = log10_clip( *ptr1++, -10. ); // this must be log10
    a = dctN;
    ptr2 = DCT+cqtN*lowceps; // point to 4th column of DCT
    ptr3 = cqtOut;
    while( a-- ){
      ptr1 = tmp;  // point to cqt vector copy
      *ptr3 = 0.0; 
      b = cqtN;
      while( b-- )
	*ptr3 += *ptr1++ * *ptr2++;
      ptr3++; 
    }
    delete[] tmp; // free up temporary memory
    if(lcqft){
      iExtractMFCC(); // inverse MFCC transform
    }
  }
  if(!chromagram || lcqft) { // either cqft or lcqft
    a=cqtN;
    ptr1 = cqtOut;
    while( a-- ){
      // PUT CQT ON LINEAR POWER OR LOG POWER SCALE
      *ptr1 = log_fn ( *ptr1, SILENCE_THRESH );
      ptr1++;
    }
  }
}

void fftExtract::iExtractMFCC(){ // inverse MFCC transform, back to cqft
  int a,b,i;
  double *ptr1, *ptr2, *ptr3;
  // matrix product of MFCC*DCT^(-1)
  double* tmp = new double[dctN];
  assert( tmp && dctN && ( dctN <= cqtN ) );
  a = dctN;
  ptr1 = cqtOut;
  ptr2 = tmp;
  while(a--)
    *ptr2++=*ptr1++; // make tmp copy
  a = cqtN;
  ptr3 = cqtOut;
  i = 0;
  while( a-- ){
    ptr2 = DCT+cqtN*lowceps+i; // point to 4th column of DCT
    ptr1 = tmp;  // point to cqt vector copy
    *ptr3 = 0.0; 
    b = dctN;
    while( b-- ){
      *ptr3 += *ptr1++ * *ptr2;
      ptr2+=cqtN;
    }
    *ptr3 = pow(10.0, *ptr3); // invert log transform
    ptr3++; 
    i++;
  }
  delete[] tmp;
}


void fftExtract::updateBuffers(){
  if(hopN<winN)
    memmove(audioData,audioData+hopN*sfinfo.channels,(winN-hopN)*sfinfo.channels*sizeof(double));
}
  
void fftExtract::zeroBuf(double* d, unsigned int n){
  if(!d)
    error("Attempt to zero non-allocated buffer","");
  while(n--)
    *d++=0.0;
}

void fftExtract::beatSum(double *src, double *dst, double prop){
  int c = extractN;
  while(c--)
    *dst++ += *src++*prop;    
}

void fftExtract::beatDiv(double *src, double d){
  int c = extractN;
  double n = 1.0/d;
  while(c--){
    *src = *src*n;
    src++;
  }
}

void fftExtract::newBeat(double *src, double *dst, double prop){
  zeroBuf(dst,(unsigned int)extractN);
  beatSum(src,dst,prop);
}

int fftExtract::nextHigherPow2(int k) {
  k--;
  for (int i=1; i<(int)(sizeof(int)*CHAR_BIT); i<<= 1)
    k = k | k >> i;
  return k+1;
}

void fftExtract::error(const char* s1, const char* s2){
  fprintf(stderr,"%s %s\n", s1, s2);
  exit(1);
} 

void fftExtract::processArgs(int argc, char* argv[]){
  // get file names
  inFileName = argv[argc-2];  
  outFileName= argv[argc-1];  

  // get command-line options
  for(int k=1; k<argc-2; k++){
    if(strcmp(argv[k], "-P") == 0) {
      power = 1;
    }
    if(strcmp(argv[k], "-H") == 0) {
      harmonicity = 1;
    }
    if(strcmp(argv[k],"-p")==0){
      fftwPlanFileName = argv[k+1];
      fftwPlanFile = fopen(fftwPlanFileName, "r");
      if(fftwPlanFile==0){
	printf("Creating new wisdom\n");
	fftwPlanFile = fopen(fftwPlanFileName, "w");
      }
      else
	useWisdom=1;
      if(fftwPlanFile==0)
	printf("Error creating wisdom file %s\n", fftwPlanFileName);
      //	if(useWisdom)
      //	  printf("fftw wisdom file=%s\n", fftwPlanFileName);
    }

    if(strcmp(argv[k],"-b")==0){
      if(!(beatFile = new ifstream(argv[k+1])) || beatFile->fail()){
	printf("Cannot open beat file %s\n", argv[k+1]);
	exit(1);
      }
      else
	beats=1;
    }				

    if(strcmp(argv[k],"-l")==0){
      loEdge=atof(argv[k+1]);
      if(loEdge<0 || loEdge>8000.0)
	error("loEdge is out of range (0 - 8kHz)");
    }
    if(strcmp(argv[k],"-i")==0){
      hiEdge=atof(argv[k+1]);
      if(hiEdge<loEdge || hiEdge>22050.0)
	error("hiEdge is out of range (0 - 22.05kHz)");
    }
    if(strcmp(argv[k],"-n")==0 and !(usingS)){
      fftN = atoi(argv[k+1]);
    }
    if(strcmp(argv[k],"-w")==0 and !(usingS)){
      winN = atoi(argv[k+1]);
    }
    if(strcmp(argv[k],"-h")==0 and !(usingS)){
      hopN = atoi(argv[k+1]);
    }
    if(strcmp(argv[k], "-s")==0){
      usingS = true;
      hopTime = atoi(argv[k+1]);
    }
    if(strcmp(argv[k],"-v")==0){
      verbose = atoi(argv[k+1]);
    }
    if(strcmp(argv[k],"-C")==0){
      extractChannel = atoi(argv[k+1]);
      if(extractChannel<0 || extractChannel>128){
	printf("Badly formed extract channel: %d", extractChannel);
	exit(1);
      }      
    }
    if(strcmp(argv[k],"-q")==0){
      if(!constantQ)
	constantQ = atoi(argv[k+1]);
    }
    if(strcmp(argv[k],"-f")==0){
      spectrogram = atoi(argv[k+1]);
    }
    if(strcmp(argv[k],"-c")==0){
      chromagram = atoi(argv[k+1]);
      if(chromagram){ // Default Chromagram
	if(constantQ && verbose){
	  printf("Warning: overriding %d constantQ bands for %d chromagram bins\n", constantQ, chromagram);
	  fflush(stdout);
	}
	constantQ=chromagram;
      }
    }

    if(strcmp(argv[k],"-m")==0){
      cepstrum = atoi(argv[k+1]);
      if(cepstrum < 0){
	cepstrum = -cepstrum;
	lcqft = 1;
      }
      if(cepstrum && !constantQ)
	constantQ=12;	
    }

    if(strcmp(argv[k],"-M")==0){
      lowceps = atoi(argv[k+1]);
    }

    if(strcmp(argv[k],"-g")==0){
      if(atoi(argv[k+1])){
	using_log = 1;
	log_fn = log10_clip;
      }
      else{
	using_log = 0;
	log_fn = do_nothing;		      
      }
    }

    if(strcmp(argv[k], "-a")==0){
      if(atoi(argv[k+1])){
	magnitude=1; // output values in magnitudes instead of powers
      }
    }

    if(strcmp(argv[k], "-F")==0){
      if(atoi(argv[k+1])){
	float_output=1; // output values in floats instead of doubles
      }
    }
  }

  if(loEdge==0.0){
    loEdge = 55.0 * pow(2.0, 2.5/12.0); // low C minus quater tone
  }
  if(hiEdge==0.0){
    hiEdge=8000.0;
  }
  if (!(constantQ || cepstrum || chromagram || spectrogram || power || harmonicity)) {
    //if no feature is given, do a stft
    //constantQ = DEFAULT_FEAT_BANDS;
    spectrogram = 1;
  }
  if ((!(winN || hopN || fftN) && !hopTime) && !beats){
    //either specify one of (winN, hopN, fftN) or hopTime or use beats
    //if neither of these three, use the default hopTime
    hopTime = DEFAULT_SLICE_SIZE;
    usingS = true;
  } else if ((!(winN || hopN || fftN) || !hopTime) && beats){
    //for beat features use a hop of 100ms unless overridden
    hopTime = 100;
  }
  if(verbose) {
    if (power){
      if(!using_log)
	printf("Overall linear-power\n");
      else
	printf("Overall log-power\n");
    }
    if (harmonicity){
      printf("Overall harmonicity\n");
    }
    if((spectrogram || constantQ || chromagram) && !cepstrum){
      if (magnitude && !using_log ){
	printf("magnitude-");
      }
      else if(using_log){
	printf("log-power-");
      }
      else{
	printf("power-");
      }
    }	
    if (spectrogram){
      printf("short-time Fourier transform\n");
    }
    if(constantQ && !(chromagram || cepstrum)){
      printf("Constant Q transform : %d bands per octave\n", constantQ);
    }
    if(chromagram){
      printf("Chromagram transform : %d chroma bins\n", chromagram );
    }
    if(cepstrum){
      printf("Cepstral transform : %d cepstral coefficients\n", cepstrum );
    }
    if(chromagram || constantQ || cepstrum){
      if(loEdge!=0.0){
	printf("loEdge: %f\n", loEdge);
      }
      if(hiEdge!=0.0){
	printf("hiEdge: %f\n", hiEdge);
      }
    }
    if (usingS)
      printf("creating sample rate invariant feature slices of length %ims.\n", hopTime);

    if(beats){
      printf("Beat synchronus features\n");
    }
    fflush(stdout);
  }
}


#ifdef _FFTEXTRACT_MAIN_

/* ENTRY POINT */
int main (int argc, char* argv[]){
  // Usage
  if(argc<3){
    print_usage(argv[0]);
    exit(1);
  }
  else{
    fftExtract(argc,argv);
    exit(0);
  }
}

void print_usage(char* appname){
  printf("%s [options] inFile outFile\n", appname);
  printf("where options are:\n\n");

  printf("feature options (mutually exclusive):\n");
  printf("-f N : STFT (short-time Fourier transform, N=0, N=1) \n");
  printf("-m M : MFCC (Mel-frequency cepstral coefficients) -M=invert MFCC\n");
  printf("-M l : Low coefficient for MFCC (0..bands-1) [3] \n");
  printf("-q B : constant-Q spectrum with B bands per octave (0=linear freq.)\n");
  printf("-c C : chromagram folded into C bins\n");
  printf("-P   : overall log power\n");
  printf("-H   : overall log harmonicity\n\n");

  printf("FFT options:\n");
  printf("-l L : low edge of constant-Q transform (63.5444Hz = low C - 50cents)\n");
  printf("-i I : hi edge of constant-Q transform (8kHz)\n");
  printf("-s S : hop size in ms.\n");
  printf("\tA 50%% overlap is used to derive window length and FFT length,\n");
  printf("\twhich will both be the smallest power of 2 number of samples\n");
  printf("\tabove or equal to 2 * the hop size and window length respectively.\n");
  printf("\tGiven in place of -[nwh] (if -s is present they will be ignored.\n");
  printf("-n N : length of FFT (in samples)\n");
  printf("-w W : length of window (in samples) within FFT\n");
  printf("-h H : hop size (in samples)\n\n");

  printf("Extraction options:\n");
  printf("-C c : channel 0=left, 1=right, 2=mix [0=left]\n");
  printf("-p file : fftw plan file (generates new if does not exist)\n");
  printf("-b file : beat timing file (secs)\n");
  printf("-g 1/0 : output constant-Q spectrum/chroma/harmonicity/powers as log10 power bands [1] (true)\n");
  printf("-a 1/0 : absolute value (magnitude) instead of power output for stft, constant-Q, or chromagram [0] (false)\n");
  printf("-F 1/0 : output as (float) instead of (double) [0] (false)\n");

  printf("\nDebug options:\n");
  printf("-v V : verbosity [0 - 10]\n");
}

#endif
