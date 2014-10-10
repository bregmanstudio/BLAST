#include "DriverMax.h"
#include <string.h>

// Macro for unpacking strings from a t_atom object
#define GetSymbol(a) a.a_w.w_sym
#define GetString(a) a.a_w.w_sym->s_name

void* soundspotter_new(void){
  t_soundspotter* x = (t_soundspotter*)newobject(soundspotter_class);
  // new non-signal inlets *must* preceed dsp_setup
  floatin(x,9);
  intin(x,8);
  intin(x,7);
  floatin(x,6);
  intin(x,5);
  intin(x,4);
  intin(x,3);
  intin(x,2);
  dsp_setup((t_pxobject*) x, 2);
  // new non-signal outlets *must* preceed signal outlets
  x->winningFrameOut = intout((t_object*)x);
  outlet_new((t_object*)x, (char*) "signal");
  outlet_new((t_object*)x, (char*) "signal"); 
  outlet_new((t_object*)x, (char*) "signal"); 
  x->x_obj.z_misc = Z_NO_INPLACE; // Do not re-use the buffers from input to output
  x->SS = ss_new(44100,2048,2);
  x->windowLength = SS_WINDOW_LENGTH;
  x->audioOutputBuffer = new t_sample[x->windowLength*2]; // Room for Stereo Output if needed
  x->audioInputBuffer = new t_sample[x->windowLength]; 
  x->featureInputBuffer = new t_sample[x->windowLength]; 
  x->featureOutputBuffer = new t_sample[x->windowLength]; 
  x->insamp_pos = 0;
  x->outsamp_pos = 0;
  x->aSound = 0;
  post((char*)"Hurray! We Have SoundSpotter...");
  return x;
}

void soundspotter_cleanup(void* z,...){
  t_soundspotter* x = (t_soundspotter*) z;
  ss_free(x->SS);
  delete x->aSound;
  delete[] x->audioOutputBuffer;
  delete[] x->audioInputBuffer;
  delete[] x->featureInputBuffer;
  delete[] x->featureOutputBuffer;

  dsp_free((t_pxobject*)x);
}

void m_stop(t_soundspotter* x){
  ss_stop(x->SS);
  post("STOP");
}


void m_spot(t_soundspotter* x){
  ss_spot(x->SS);
  post("SPOT");
}

void m_thru(t_soundspotter* x){
  ss_thru(x->SS);
  post("THRU");
}

void m_liveSpot(t_soundspotter* x){
  if(ss_liveSpot(x->SS)){
    post("LIVESPOT");
  }
}

void m_extract(t_soundspotter* x, t_symbol *s, short argc,const t_atom *argv){
  t_symbol* sfName;  
  if(argc)
    post("%d, %s", argc, GetString(argv[0]));
  
  if(argc){
    delete x->aSound;
    x->aSound = new SoundFile();
    if(!x->aSound){
      post("Couldn't instantiate SoundFile class");
    }
    else{
      sfName = GetSymbol(argv[0]);
      char buf[1024];
      char* cptr = strchr(sfName->s_name,':');
      int retval=-1;
      if(cptr!=NULL){
	// remove leading disk path up to colon (how to fix this libsndfile issue ?)
	strcpy(buf,cptr+1);
	retval=x->aSound->sfOpen(buf);
      }
      else{
	retval=x->aSound->sfOpen(sfName->s_name);
      }
      if(retval<0){
	post("Couldn't Open SoundFile %s",sfName->s_name);
	return;
      }
      else{
	post("Read %d frames from %s", retval, sfName->s_name);
      }	
      ss_extract(x->SS, x->aSound->getSoundBuf(), x->aSound->getBufLen(), x->aSound->getNumChannels());
      delete[] x->audioOutputBuffer;
      x->audioOutputBuffer = new t_sample[x->windowLength*x->aSound->getNumChannels()];
      post("EXTRACTING %s...", sfName->s_name);
    }
  }
  else
    post("m_extract: requires a valid soundfile name (wav/aiff)");
  
}


void m_dump(t_soundspotter* x, t_symbol *s,short argc,const t_atom *argv)
{
  if(argc){
    ss_dump(x->SS, GetString(argv[0]));
    post("DumpVectors %s", GetString(argv[0]));
  }
}

void m_load(t_soundspotter* x, t_symbol *s, short argc,const t_atom *argv)
{
  if(argc) {
    ss_load(x->SS, GetString(argv[0]));
    post("LoadVectors %s", GetString(argv[0]));
  }
}

void m_intQueueSize(t_soundspotter* x, long i){
  ss_setQueueSize(x->SS, (int)i);
}

void m_intShingleSize(t_soundspotter* x, long i){
  ss_setShingleSize(x->SS, (int)i);
}

void m_intLoBasis(t_soundspotter* x, long i){
  ss_setLoBasis(x->SS, (int)i);
}

void m_intBasisWidth(t_soundspotter* x, long i){
  ss_setBasisWidth(x->SS, (int)i);
}

void m_floatRadius(t_soundspotter* x, double r){
  ss_setMatchRadius(x->SS, (float)r);
}

void m_intLoK(t_soundspotter* x, long i){
  ss_setLoDataLoc(x->SS, (float)i);
} 

void m_intHiK(t_soundspotter* x, long i){
  ss_setHiDataLoc(x->SS, (float)i);
}

void m_floatEnvFollow(t_soundspotter* x,double f){
  ss_setEnvFollow(x->SS, (float)f);
} 

void soundspotter_dsp (t_soundspotter *x, t_signal **sp, short *count){
  dsp_add(soundspotter_perform, 7, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
}

t_int *soundspotter_perform(t_int* w){
  t_soundspotter* x = (t_soundspotter*) w[1]; // Get the instance pointer from argument list
  if(ss_getStatus(x->SS)==EXTRACT){
    ss_run(x->SS, x->windowLength, 0, 0, 0, 0); // Feature extraction on new soundFile
    post("**READY**"); // soundspotter is now in STOP mode
  }
  else{
    soundspotter_copy_samples_to_output(w);
    soundspotter_copy_samples_from_input(w);
    if( x->insamp_pos == x->windowLength ){
      soundspotter_do_match(w);
    }
  }
  return w + 8; // Need to return w + numArgs + 1, pointer to next step in DSP chain 
}

void soundspotter_copy_samples_to_output(t_int* w){
  t_soundspotter* x = (t_soundspotter*) w[1]; // Get the instance pointer from argument list
  int maxMspBufLen = w[2]; // MAXMSP current signal vector size
  t_sample* oppdL = (t_sample*)w[6];
  t_sample* oppdR = (t_sample*)w[7];
  int sampleCount = 0;
  int numChannels = (x->SS->getStatus()!=SPOT)?1:x->SS->getAudioDatabaseNumChannels();
  t_sample* outBufPtr = x->audioOutputBuffer + (x->outsamp_pos*numChannels);
  while(sampleCount < maxMspBufLen && x->outsamp_pos < x->windowLength){
    *oppdL++ = *outBufPtr;
    *oppdR++ = *(outBufPtr + numChannels - 1);
    outBufPtr+=numChannels;
    x->outsamp_pos++;
    sampleCount++;
  }
  x->outsamp_pos = x->outsamp_pos % x->windowLength;
}

void soundspotter_copy_samples_from_input(t_int* w){
  t_soundspotter* x = (t_soundspotter*) w[1]; // Get the instance pointer from argument list
  int maxMspBufLen = w[2]; // MAXMSP current signal vector size
  t_sample* maxmspBufPtr = (t_sample*) w[4];
  t_sample* inputBufPtr = x->audioInputBuffer + x->insamp_pos;
  int sampleCount = 0;
  while(sampleCount < maxMspBufLen && x->insamp_pos < x->windowLength){
    *inputBufPtr++ = *maxmspBufPtr++;
    sampleCount++;
    x->insamp_pos++;
  }  
}

void soundspotter_do_match(t_int* w){
  t_soundspotter* x = (t_soundspotter*) w[1]; // Get the instance pointer from argument list
  // input buffer is MONO  ouptput buffer is STEREO INTERLEAVED
  ss_run(x->SS, x->windowLength, x->featureInputBuffer, x->audioInputBuffer, x->featureOutputBuffer, x->audioOutputBuffer);
  if(ss_getStatus(x->SS)==SPOT || ss_getStatus(x->SS)==EXTRACTANDSPOT){
    long result =  ss_reportResult(x->SS);
    if(result!=-1.){
      outlet_int(x->winningFrameOut, result); // Post the winning frame to outlet 3	
    }
  }
  x->insamp_pos = 0; // reset the input-buffer sample counter
}

int main(void){
  // setup method needs to register new, free and creation argument list
  setup((t_messlist **)&soundspotter_class, (method)soundspotter_new, (method)soundspotter_cleanup, (short)sizeof(t_soundspotter), 0L, 0);  
    
  // soundspotter method bindings
  // addmess((method)soundspotters_assist,"assist",A_CANT,0);

  //(sent to MSP objects when audio is turned on/off)
  addmess((method)soundspotter_dsp, "dsp", A_CANT, 0);        // respond to the dsp message
  
  // soundspotter message bindings inlet 1
  addmess((method)m_stop,"stop",0); 
  addmess((method)m_liveSpot,"livespot",0);
  addmess((method)m_liveSpot,"liveSpot",0);
  addmess((method)m_liveSpot,"LiveSpot",0);
  addmess((method)m_spot,"spot",0);
  addmess((method)m_thru,"thru",0);  
  
  // Feature and buffer messages bindings
  addmess((method)m_extract,"extract",A_GIMME,0);
  addmess((method)m_dump,"dump",A_GIMME,0);  
  addmess((method)m_load,"load",A_GIMME,0);    
    
  // "float" parameters
  addftx((method)m_floatEnvFollow,9);// callback for method m_floatEnvFollow (with one double argument)
  addinx((method)m_intHiK,8);// callback for method m_intHiK (with one long argument)
  addinx((method)m_intLoK,7);// callback for method m_intLoK (with one long argument)
  addftx((method)m_floatRadius,6);// callback for method m_floatRadius with one double argument
  addinx((method)m_intBasisWidth,5);// callback for method m_intBasisWidth with one long argument
  addinx((method)m_intLoBasis,4);// callback for method m_intLoBasis with one long argument
  addinx((method)m_intShingleSize,3);// callback for method m_intShingleSize with one long argument
  addinx((method)m_intQueueSize,2);// callback for method m_intQueueSize with one long argument

  dsp_initclass();// must call this function for MSP object classes 

}

