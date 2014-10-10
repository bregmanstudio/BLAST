#include "DriverFlext.h"

soundspotter::soundspotter() : aSound(0) {
  // The constructor of your class is responsible for
  // setting up inlets and outlets and for registering
  // inlet-methods:
  // The descriptions of the inlets and outlets are output
  // via the Max/MSP assist method (when mousing over them in edit mode).
  // PD will hopefully provide such a feature as well soon
    
  AddInSignal("Messages in");          // 0 Power Spectrum in, messages in
  AddInSignal("Audio in");             // 1 audio in, messages in
  AddInFloat("FrameQueue Length");     // Leave out n frames from spotment choice
  AddInFloat("Shingle Size");	       // Number of frames per match
  AddInFloat("Low Basis");	       // Quefrency cutoff
  AddInFloat("Basis Width");	       // Quefrency bandwidth
  AddInFloat("Radius");                // Search Radius 0..1
  AddInFloat("LoK");		       // Database start time
  AddInFloat("HiK");		       // Database end time
  AddInFloat("EnvFollow");             // Envelope Follow parameter 0..1
  AddOutSignal("features out");        // 2 audio out1
  AddOutSignal("audio out");           // 3 audio out2
  AddOutFloat("Matching Frame");       // Matched frame index out

  FLEXT_ADDMETHOD_(0,"stop",m_stop);
  FLEXT_ADDMETHOD_(0,"liveSpot",m_liveSpot);
  FLEXT_ADDMETHOD_(0,"livespot",m_liveSpot);
  FLEXT_ADDMETHOD_(0,"extract",m_extract);
  FLEXT_ADDMETHOD_(0,"milt",m_milt);
  FLEXT_ADDMETHOD_(0,"spot",m_spot);
  FLEXT_ADDMETHOD_(0,"thru",m_thru);
  FLEXT_ADDMETHOD_(0,"dump",m_dump);
  FLEXT_ADDMETHOD_(0,"load",m_load);
  FLEXT_ADDMETHOD_(0,"set",m_set);
  FLEXT_ADDMETHOD_(0,"sound",m_sound);
  FLEXT_ADDMETHOD(2,m_float1);
  FLEXT_ADDMETHOD(3,m_float2);
  FLEXT_ADDMETHOD(4,m_floatLoBasis);
  FLEXT_ADDMETHOD(5,m_floatBasisWidth);
  FLEXT_ADDMETHOD(6,m_floatRadius);  
  FLEXT_ADDMETHOD(7,m_floatLoK);
  FLEXT_ADDMETHOD(8,m_floatHiK);
  FLEXT_ADDMETHOD(9,m_floatEnvFollow);

  post("");
  post("***********************************************");
  post("***   soundspotter~ PD/MSP MPEG-7 external  ***");
  post("***      (C) Michael Casey, 2003-May 2009   ***");
  post("***********************************************");
  post("");

  SS = new SoundSpotter();

}

soundspotter::~soundspotter(){
  delete SS;
  delete aSound;
}

void soundspotter::m_stop(int argc,const t_atom *argv){
  SS->setStatus(STOP);
  post("STOP");
}


void soundspotter::m_spot(int argc,const t_atom *argv){
  SS->setStatus(SPOT);
  post("SPOT");
}

void soundspotter::m_thru(int argc,const t_atom *argv){
  SS->setStatus(THRU);
  post("THRU");
}

void soundspotter::m_liveSpot(int argc,const t_atom *argv){
  if(SS->setStatus(EXTRACTANDSPOT)){
    post("LIVESPOT");
  }
}


void soundspotter::m_extract(int argc,const t_atom *argv){
  if(buf!=0&&buf->Valid()){
    SS->setStatus(EXTRACT);
    post("EXTRACTING...");
    post("buf=%0x", buf);
    post("Valid=%d", (!buf)?(int)buf:buf->Valid());
  }else
    post("Source buffer empty");
}

void soundspotter::m_sound(int argc,const t_atom *argv){
  t_symbol* sfName;  
  if(argc == 1 && IsSymbol(argv[0])) {
    Clear();
    delete aSound;
    aSound = new SoundFile();
    if(!aSound)
      post("Couldn't find SoundFile classes");
    else{
      sfName = GetSymbol(argv[0]);
      int retval=aSound->sfOpen(sfName->s_name);
      if(retval<0)
	post("Couldn't Open SoundFile %s",sfName->s_name);
      else{
	post("Read %d frames from %s", retval, sfName->s_name);
      }	
      SS->setBuf(aSound->getSoundBuf(), aSound->getBufLen());
    }
  }
  else
    post("m_sound: requires a valid soundfile name (wav/aiff)");
}

void soundspotter::m_milt(int argc,const t_atom *argv){

  m_sound(argc, argv);
  if(SS->getBuf()){
    SS->setStatus(EXTRACT);
    post("EXTRACTING %s...", sfName->s_name);
  }
  else
    post("m_milt: requires a valid soundfile name (wav/aiff)");
}

// SOUNDSPOTTER method definitions
void soundspotter::Clear()
{
  if(buf) {
    delete buf;
    buf = 0; 
    if(0){
      //      delete bufname;
      bufname = 0;
    }
  }
}

bool soundspotter::Check()
{
  if(!buf || !buf->Valid()) {
    post("%s (%s) - no valid buffer defined",thisName(),GetString(thisTag()));
    // return zero length
    return false;
  }
  else {
    if(buf->Update()) {
      // buffer parameters have been updated
      if(buf->Valid()) {
	post("%s (%s) - updated buffer reference",thisName(),GetString(thisTag()));
	return true;
      }
      else {
	post("%s (%s) - buffer has become invalid",thisName(),GetString(thisTag()));
	return false;
      }
    }
    else
      return true;
  }
}


void soundspotter::m_set(int argc,const t_atom *argv)
{
  if(argc == 0) {
    // argument list is empty
    // clear existing buffer
    // Clear();
  }
  else if(argc == 1 && IsSymbol(argv[0])) {
    // one symbol given as argument
    // clear existing buffer
    Clear();
    delete aSound;
    // save buffer name
    bufname = GetSymbol(argv[0]);
    // make new reference to system buffer object
    buf = new buffer(bufname);
    if(!buf->Ok()) {
      post("%s (%s) - warning: buffer is currently not valid!",thisName(),GetString(thisTag()));
    }
    else{
      SS->setBuf(buf->Data(), buf->Frames());
      post("Source Database: Name=%s, Channels=%d, Frames=%d",buf->Name(),buf->Channels(),buf->Frames());
    }
  }
  else {
    // invalid argument list, leave buffer as is but issue error message to console
    post("%s (%s) - message argument must be a symbol (or left blank)",thisName(),GetString(thisTag()));
  }
}


void soundspotter::m_dump(int argc,const t_atom *argv)
{
  if(argc == 0) {
    SS->DumpVectors("SoundSpotterVectors.bin");
  }
  else if(argc == 1 && IsSymbol(argv[0])) {
    SS->DumpVectors(GetString(argv[0]));
  }
  else {
    // invalid argument list, leave buffer as is but issue error message to console
    post("%s (%s) - message argument must be a symbol (or left blank)",thisName(),GetString(thisTag()));
  }

}

// Livespot buffer flush and sourceShingle sizing
void soundspotter::flushBufs(){
  SS->flushBufs();
}

void soundspotter::m_load(int argc,const t_atom *argv)
{
  if(argc == 0) {
    SS->LoadVectors("SoundSpotterVectors.bin");
    m_stop(0,0);
  }
  else if(argc == 1 && IsSymbol(argv[0])) {
    SS->LoadVectors(GetString(argv[0]));
  }
  else {
    // invalid argument list, leave buffer as is but issue error message to console
    post("%s (%s) - message argument must be a symbol (or left blank)",thisName(),GetString(thisTag()));
  }
  post("Features loaded: %d", SS->getXPtr());
}


void soundspotter::m_float1(float f){
    SS->setQueueSize((int)f);
}

void soundspotter::m_float2(float f){
  SS->setShingleSize((int)f);
}

void soundspotter::m_floatLoBasis(float f){
  SS->setMinASB((int)f);
}

void soundspotter::m_floatBasisWidth(float f){
  SS->setBasisWidth((int)f);
}

void soundspotter::m_floatRadius(float r){
  SS->setRadius(r);
}

void soundspotter::m_floatLoK(float f){
  SS->setLoK(f);
} 

void soundspotter::m_floatHiK(float f){
  SS->setHiK(f);
}

void soundspotter::m_floatEnvFollow(float f){
  SS->setEnvFollow(f);
} 


FLEXT_NEW_DSP("soundspotter~", soundspotter)

void soundspotter::m_signal(int n, t_sample *const *in, t_sample *const *out)
{

  if(SS->getStatus()==EXTRACT)
    post("**READY**"); 

  SS->run(n, in[0], in[1], out[0], out[1]);
  
  float result=(float)SS->reportResult();

  if(result!=-1)
    ToOutFloat(2, result); // Post the winning frame to a Float outlet 3
}




