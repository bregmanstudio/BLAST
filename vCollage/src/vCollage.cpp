#include "vCollage.h"
// Possible titles:
//
// Time and Motion Pitcture Study
// Copyright Volation / Copyright Volition
// A Million Seconds

//-----------------------------------------------------
bool operator< (NNresult const &a, NNresult const &b) {
  return a.query_pos < b.query_pos;
}

//-----------------------------------------------------
bool operator> (NNresult const &a, NNresult const &b) {
  return a.query_pos > b.query_pos;
}

//-----------------------------------------------------------------------------------------------------------------------------------
VideoMashup::VideoMashup(double audFps, int hopSize, double vidFps=25.0, Size outSize=Size(800,600), int motionThresh=10, int decimationFactor=4):
  audFps(audFps),
  vidFps(vidFps),
  hopSize(hopSize),
  segmentDuration(2*hopSize/audFps),
  outSize(outSize),
  motionThresh(motionThresh),
  decimationFactor(decimationFactor),
  vidNumFramesCeil(ceil(vidFps*segmentDuration)),
  timeSyncLocator(0.0),
  blendFadeRate(0.75),
  report_level(1),
  event_counter(0),
  outFrame_count(0),
  displayWindow(false),
  query_pos(0)
{
  capture = VideoCapture();
  outImg.create(outSize, CV_8UC3);
  outImgF.create(outSize, CV_32F);
  poissonMaskImg.create(outSize, CV_8UC1);
  blendBuffer.create(outSize, CV_8UC3);
  outSequence = new Mat[vidNumFramesCeil];
  for(int k=0; k<vidNumFramesCeil; k++){
    outSequence[k].create(outSize, CV_8UC3);
  }
  clearOutputBuffers();
  report("audFps:", to_string(audFps), 1);
  report("hopSize:", to_string(hopSize), 1);
  report("segmentDuration:", to_string(segmentDuration), 1);  
  report("vidFps:", to_string(vidFps), 1);  
  report("outSize:", to_string(outSize.width)+" x "+to_string(outSize.height),1);
  report("motionThresh:", to_string(motionThresh),1);
  report("decimationFactor:",to_string(decimationFactor),1);
  report("vidNumFramesCeil:",to_string(vidNumFramesCeil),1);
  report("outSequenceSizes", to_string(vidNumFramesCeil)+" x "+to_string(outSize.width)+" x "+to_string(outSize.height), 1);
}

//--------------------------
VideoMashup::~VideoMashup(){

}

//------------------------------------------------------
void VideoMashup::error(const char* s1, const char* s2=""){
  report(s1,s2,0);
  throw 101;
}

//-------------------------------------------------
void VideoMashup::error(const char* s1, string s2){
  report(s1,s2,0);
  throw 101;
}

//-------------------------------------------------------
void VideoMashup::report(const char* s1, const char* s2="", int priority=0){
  if(priority<=report_level)
    cout << s1 << " " << s2 << endl;
}

//--------------------------------------------------
void VideoMashup::report(const char* s1, string s2, int priority=0){
  if(priority<=report_level)
    cout << s1 << " " << s2 << endl;
}

//-----------------------------------------------
void VideoMashup::setDisplayWindow(bool display){
  displayWindow = display;
  if(displayWindow)
    namedWindow( "video", WINDOW_AUTOSIZE );
}

//---------------------------------------------------------------------------
bool VideoMashup::initializeFiles(const char* movieListFileName="movieList.txt", const char* matshupFileName="movie.imatsh.txt"){
  if(!loadMovieList(movieListFileName)){
    report("Failed to open movieList:", movieListFileName,0);
    return false;
  }
  if(!loadMatshupSet(matshupFileName)){
    report("Failed to populate matshupSet from", matshupFileName,0);
    return false;
  }

  std::string str(matshupFileName);
  if ( str.find(".") == std::string::npos) 
    VideoMashup::error("Cannot get file extension from matshupFileName %s.", matshupFileName);  
  std::string outfileName(str.substr(0,str.find(".")));
  outfileName.append(".imatsh.avi");

  writer = VideoWriter(outfileName, CV_FOURCC('X','V','I','D'), vidFps, outSize, 1); // color

  return true;
}

//---------------------------------------------------------------------------
bool VideoMashup::loadMovieList(const char* movieListFileName = "movieList.txt"){
  // get ordered list of movie absolute file names from movieList.txt  
  movieListFile.open(movieListFileName);  
  string line;
  if (movieListFile.is_open() && movieListFile.good())
    {
      report("movieListFile:", movieListFileName, 2);
      while ( getline (movieListFile,line) ){
	report("found movie:", line, 3);
	movieList.push_back(line);
      }
      movieListFile.close();
    }
  else {
    cout << "Unable to open movieListFile." << endl; 
    return false;
  }
  
  if(movieList.size()<1){
    cout << "Could not find any movie files." << endl;
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------------
bool VideoMashup::getNumMatches(){
  nnResult = *matshup.begin();
  MitPair mp = matshup.equal_range(nnResult);
  Mit it = mp.first;
  query_pos = nnResult.query_pos;
  numMatches = 0;
  while(it++ != mp.second)
    numMatches++;
  report("numMatshes:", to_string(numMatches), 2);
  event_counter=0;
  return true;
}

//------------------------------------------------------------------------------------
void VideoMashup::loadMedia(string filename){
  capture.release();
  int retval = capture.open(filename);
  if(!retval)
    error("Could not open file:", filename+", retval="+to_string(retval));
  report("opened:", filename, 3);
  width = (int) capture.get(CV_CAP_PROP_FRAME_WIDTH);
  height = (int) capture.get(CV_CAP_PROP_FRAME_HEIGHT);
  movFps = capture.get(CV_CAP_PROP_FPS);
  totalFrameCount = (int) capture.get(CV_CAP_PROP_FRAME_COUNT);
  capture.set(CV_CAP_PROP_POS_FRAMES, startPosSeconds * movFps);
  frame.create(Size(width, height), CV_8UC3);
  procImg.create(Size(width, height), CV_8UC1);
  procImg2.create(Size(width/decimationFactor,height/decimationFactor), CV_8UC1);
  diffImg.create(Size(width/decimationFactor,height/decimationFactor), CV_8UC1);
  alphaImg.create(Size(width,height), CV_8UC1);
  prevImg.create(Size(width/decimationFactor,height/decimationFactor), CV_8UC1);
  zeroBuf.create(Size(width, height), CV_8UC3);
  movNumFrames = movFps * segmentDuration;
  report("totalFrameCount:",to_string(totalFrameCount),4);
  report("mov size:", to_string(width)+" x "+to_string(height),3);
  report("movFps:",to_string(movFps),3);
  report("movNumFrames:",to_string(movNumFrames),3);
  report("locator:", to_string(startPosSeconds),3);
  report("spos frame:", to_string(capture.get(CV_CAP_PROP_POS_FRAMES)),3);
}

//----------------------------------------------------------------------------------------------
bool VideoMashup::loadMatshupSet(const char *matshupFileName){
  NNresult r;
  matshupFile.open(matshupFileName);
  if(!matshupFile.is_open())
    error("Cannot open matshup file %s for reading.", matshupFileName);

  while(matshupFile.good()){
    matshupFile >> r.dist;
    if(!matshupFile.good())
      break;
    matshupFile >> r.prob;
    matshupFile >> r.amp;
    matshupFile >> r.query_pos;
    matshupFile >> r.track_pos;
    matshupFile >> r.media_idx;
    if(r.media_idx < movieList.size()){
      r.media = movieList[r.media_idx].c_str();
    }
    else{
      fprintf(stderr,"Error: source media index %d is out of range [%d]\n", r.media_idx,(int)movieList.size());
      return false;
    }
    matshup.insert(r);
  }
  if(matshupFile.eof()){
    matshupFile.close();
    fprintf(stdout, "MatshupSet: loaded %d events from %s\n", matshup.size(), matshupFileName);
    return getNumMatches();
  }
  else
    return false;
}

//------------------------
void VideoMashup::clearOutputBuffers(){
  // Clear output frames
  for(int k=0; k < vidNumFramesCeil; k++)
    outSequence[k] = Scalar(0,0,0);
}

//-------------------------------------------------
// Process a range of events for current 
// audio window (query position)
//-------------------------------------------------
void VideoMashup::update(int render_method=0){
  int MatchEventIsGood = processNextEvent(render_method);
  if(!MatchEventIsGood){
    if(!finished())
      error("Unexpected end of matshup set","vCollage::processNextEvent()");
  }
  else{
    writeOutputFrames();
    if(report_level>2)
      cout << "q=" << query_pos << " a=" << (query_pos+hopSize)/audFps << " v=" << outFrame_count / vidFps << endl;
    query_pos+=hopSize;
    event_counter++;
  }
}

//--------------------------------
bool VideoMashup::processNextEvent(int render_method){
  NNresult r;
  r.query_pos = query_pos;
  MitPair current_subset = matshup.equal_range(r);
  Mit it = current_subset.first;
  if(it==matshup.cend())
    error("query_pos has walked of end of matshup queue. Something is wrong (hopSize set incorrectly?). Terminating.");
  if(it==current_subset.second)
    cout << "Warning: no events found for query_pos=" << query_pos << endl;
  else while( it != current_subset.second ){
    nnResult = *it++;
    startPosSeconds = nnResult.track_pos / audFps;
    if(!isnan(nnResult.dist)){
      loadMedia(movieList[nnResult.media_idx]);
      processRange(render_method); 
    }
    else{
      cout << "Warning: Nan encountered in matshup file." << endl;
    }
  }
  matshup.erase(current_subset.first, current_subset.second);
  return true;
}

//-------------------------------------------------------------
void VideoMashup::setMovieProtectedFramePosition(double frameIdx){
  frameIdx = std::min((double)totalFrameCount-1.0, std::max(frameIdx, 0.0));
  if(!capture.set(CV_CAP_PROP_POS_FRAMES, frameIdx))
    error("Failed to set mov frame:", to_string(frameIdx));
}

//--------------------------------------------
// mov range: movFps x segmentDuration = movNumFrames
// aud range: audFps x segmentDuration = [audioNumFrames]
// vid range: vidFps x segmentDuration = vidNumFrames
//
// movFrame = startPosSeconds x movFps
// if fractional part of vidFrame >= 0.5, write additional output frame
//--------------------------------------------
void VideoMashup::processRange(int render_method=0){
  // Pre-roll one frame, for flicker / delta frame renderers
  if(render_method>3){
    setMovieProtectedFramePosition(startPosSeconds * movFps - 1); // Roll back 1 frame
    capture >> frame; 
    extractFlickerRegions(); // initialize prev frame
  }
  // process sequence for current movie
  for(int k=0; k<vidNumFramesCeil; k++){
    setMovieProtectedFramePosition( ( startPosSeconds + k/vidFps ) * movFps );
    capture >> frame;
    switch(render_method){
    case 0:
      extractMovieFrame();
      max(outImg, outSequence[k], outSequence[k]);
      break;
    case 1: // exponential distance (whole) image
      extractMovieFrame();
      exponentiateImage(-3.0, 0.0);
      max(outImg, outSequence[k], outSequence[k]);
      break;
    case 2: // exponential pixel color values
      extractMovieFrame();
      exponentiateColors(-3.0, 0.0);
      max(outImg, outSequence[k], outSequence[k]);
      break;
    case 3: // Poisson Blending
      extractAlphaThreshold(100);
      poissonBlend(outImg, outSequence[k], blendBuffer, poissonMaskImg);
      blendBuffer.copyTo(outSequence[k]);
      break;
    case 4: // flicker / motion
      extractFlickerRegions();
      //exponentiateImage(-2.0, 0.0);
      max(outImg, outSequence[k], outSequence[k]);      
      break;
    default:
      thru();
    }
  }
}

//------------------------------------
void VideoMashup::extractMovieFrame(){
  frame.copyTo(zeroBuf);
  resize(zeroBuf, outImg, outImg.size());
}

//-------------------------------------
void VideoMashup::exponentiateImage(double alpha=-3.0, double beta=0.0){
  outImg = exp(alpha*nnResult.dist) * outImg + beta;
}

//-------------------------------------
void VideoMashup::exponentiateColors(double alpha=-3.0, double beta=0.0){
  outImgF.create(outSize, CV_32F);
  outImg.convertTo(outImgF, CV_32F);
  outImg = -alpha * outImg;
  exp(outImgF,outImgF);
  outImgF = outImgF + beta;
  outImgF.convertTo(outImg,CV_8UC3);
}

//---------------------------------------
void VideoMashup::extractAlphaThreshold(int thresh){
  cvtColor(frame, alphaImg, CV_BGR2GRAY );
  threshold(alphaImg, alphaImg, thresh, 255, THRESH_TOZERO);
  resize(alphaImg, poissonMaskImg, poissonMaskImg.size());
}

//---------------------------------------
void VideoMashup::extractFlickerRegions(){
  zeroBuf = Scalar(0, 0, 0); 
  cvtColor(frame, procImg, CV_BGR2GRAY );
  resize(procImg, procImg2, procImg2.size());
  absdiff(procImg2, prevImg, diffImg);
  threshold(diffImg, diffImg, motionThresh, 255, THRESH_TOZERO);
  procImg2.copyTo(prevImg);
  resize(diffImg, alphaImg, alphaImg.size());
  frame.copyTo(zeroBuf, alphaImg > 0);
  resize(zeroBuf, outImg, outImg.size());
}

//---------------------------------------------------------------------
void VideoMashup::poissonBlend(Mat& src, Mat& tgt, Mat& dst, Mat& msk){  
  // Not yet, need opencv3
  //Blend::PoissonBlender pb = Blend::PoissonBlender(src, tgt, msk);
  //pb.seamlessClone(dst, 0, 0, 0);  
}

//-----------------------
void VideoMashup::thru(){
  resize(frame, outImg, outImg.size());
}

//------------------------------------
void VideoMashup::writeOutputFrames(){  
  double vidFrameDur = 1/vidFps;
  int numFramesWritten = 0;
  int j=0,k=0;

  while(timeSyncLocator < (query_pos+hopSize) / audFps - vidFrameDur){
    writer << outSequence[numFramesWritten++];
    timeSyncLocator += 1/vidFps;
  }
  if(displayWindow)
    for(k=0; k<numFramesWritten; k++){
      imshow("video",outSequence[k]);
      waitKey(1000/vidFps);
    }
  for(j=0, k=numFramesWritten; k<vidNumFramesCeil; j++, k++)
    outSequence[j] = outSequence[k] * blendFadeRate;
  for( ; j<vidNumFramesCeil; j++)
    outSequence[j] = Scalar(0,0,0); 
  outFrame_count += numFramesWritten;
  report("wrote to frame:", "["+to_string(outFrame_count)+"]", 2);
}

//--------------------------------
void VideoMashup::matInfo(Mat& M){
  double minVal, maxVal; 
  Point minLoc, maxLoc;
  minMaxLoc( M.reshape(1), &minVal, &maxVal, &minLoc, &maxLoc );
  std::cout << M.rows << " " << M.cols << " " << M.channels() << " " << M.depth() << " " << minVal << " " << maxVal << std::endl;
}

//---------------------------
bool VideoMashup::finished(){
  bool fin = matshup.empty();
  if(fin)
    cout << "MatshupSet empty, finished!" << endl;
  return fin;
}

//--------------------------
void VideoMashup::cleanUp(){

}

//---------------------------------------------------------------------
void _get_arg(int argc, char* argv[], int* c, int* a, const char* str){ // get int argument
  *c = (*c)+1;
  if(argc > *c){
    *a = atoi(argv[*c]);
    fprintf(stderr, "%s=%d\n", str, *a);
  }
}

//------------------------------------------------------------------------
void _get_arg(int argc, char* argv[], int* c, double* a, const char* str){ // get float argument
  *c = (*c)+1;
  if(argc > *c){
    *a = strtod(argv[*c],0);
    fprintf(stderr, "%s=%f\n", str, *a);
  }  
}

//------------------------------------------------------------------
#define AUD_SR 48000. // This is the TARGET sample rate, not sources
#define HOP_FP 8      // Shingle hop size
#define BUF_SZ 1024   // STFT hop samples
#define USAGE "Usage: %s movieListFile matshupFile [audioFps(%f) hopSize(%d) motionThresh(%d) decimationFactor(%d) sizeX(%d) sizeY(%d) videoFps(%f) display(%d) verbosity(%d)]\n"

//-------------------------------
int main(int argc, char* argv[]){
  char* movieListFile;
  char* matshupFile;
  double audioFps = AUD_SR / BUF_SZ ;
  int hopSize = HOP_FP;
  int motionThresh = 20;
  int decimationFactor = 4;
  int sizeX = 1280;
  int sizeY = 720;
  double videoFps = 25.0;  
  int display = 0;
  int verbosity = 2;
  int arg = 0;

  if(argc<3){
    fprintf(stderr, USAGE, argv[0], audioFps, hopSize, motionThresh, decimationFactor, sizeX, sizeY, videoFps, display, verbosity);
    return 101;
  }

  movieListFile = argv[++arg];
  matshupFile = argv[++arg];
  _get_arg(argc, argv, &arg, &audioFps, "audioFps");
  _get_arg(argc, argv, &arg, &hopSize, "hopSize");
  _get_arg(argc, argv, &arg, &motionThresh, "motionThresh");
  _get_arg(argc, argv, &arg, &decimationFactor, "decimationFactor");
  _get_arg(argc, argv, &arg, &sizeX, "sizeX");
  _get_arg(argc, argv, &arg, &sizeY, "sizeY");  
  _get_arg(argc, argv, &arg, &videoFps, "videoFps");
  _get_arg(argc, argv, &arg, &display, "display");
  _get_arg(argc, argv, &arg, &verbosity, "verbosity");
  
  VideoMashup VM(audioFps, hopSize, videoFps, Size(sizeX,sizeY), motionThresh, decimationFactor);
  VM.report_level = verbosity; 

  if(!VM.initializeFiles(movieListFile, matshupFile)){
    return 101;
  }

  VM.setDisplayWindow(display);
  int renderer=1;
  while(!VM.finished())
    VM.update(renderer);  
  return 0;
}
