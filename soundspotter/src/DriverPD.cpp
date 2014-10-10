#include "DriverPD.h"

// Macro for unpacking strings from a t_atom object
#define GetSymbol(a) a.a_w.w_symbol
#define GetString(a) a.a_w.w_symbol->s_name

// Macro for working around deprecated conversion from string constant to 'char*'
#define GENSYM(a) gensym((char*)a)

// NEW INSTANCE
DECLSPEC void* soundspotter_tilde_new(void){
  t_soundspotter_tilde* x = (t_soundspotter_tilde*)pd_new(soundspotter_tilde_class);

  // ACTIVE INLETS (ALL)
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, GENSYM("setQueueSize"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, GENSYM("setShingleSize"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, GENSYM("setLoBasis"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, GENSYM("setBasisWidth"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, GENSYM("setMatchRadius"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, GENSYM("setLoDataLoc"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, GENSYM("setHiDataLoc"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, GENSYM("setEnvFollow"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, GENSYM("setBetaParameter"));
  // OUTLETS
  outlet_new(&x->x_obj, &s_signal);
  outlet_new(&x->x_obj, &s_signal);
  x->winningFrameOut = outlet_new(&x->x_obj, &s_float);
  x->winningDistanceOut = outlet_new(&x->x_obj, &s_float);
  x->SS = NULL;
  x->SS = ss_new(44100, SS_WINDOW_LENGTH, 2);
  if(x->SS)
    post("Hurray! We Have SoundSpotter...");
  else
    post("Problem allocating soundspotter~");
  x->aSound = 0;
  x->audioOutputBuffer = new t_sample[SS_WINDOW_LENGTH*2];
  return (void *) x;
}

// NEW CLASS DEFINITION
DECLSPEC void soundspotter_tilde_setup(void){
  soundspotter_tilde_class = class_new(GENSYM("soundspotter~"),
				(t_newmethod)soundspotter_tilde_new,
				(t_method)soundspotter_tilde_cleanup, 
				sizeof(t_soundspotter_tilde),CLASS_DEFAULT, A_DEFFLOAT,0);
  
  // sent to MSP objects when audio is turned on/off
  class_addmethod(soundspotter_tilde_class,
		  (t_method)soundspotter_tilde_dsp, GENSYM("dsp"), A_GIMME,0);

  // soundspotter message bindings inlet 1
  class_addmethod(soundspotter_tilde_class, (t_method)m_stop,GENSYM("stop"),A_GIMME,0); 
  class_addmethod(soundspotter_tilde_class, (t_method)m_liveSpot,GENSYM("livespot"),A_GIMME,0);
  class_addmethod(soundspotter_tilde_class, (t_method)m_liveSpot,GENSYM("liveSpot"),A_GIMME,0);
  class_addmethod(soundspotter_tilde_class, (t_method)m_liveSpot,GENSYM("LiveSpot"),A_GIMME,0);
  class_addmethod(soundspotter_tilde_class, (t_method)m_spot,GENSYM("spot"),A_GIMME,0);
  class_addmethod(soundspotter_tilde_class, (t_method)m_thru,GENSYM("thru"),A_GIMME,0);  
  class_addmethod(soundspotter_tilde_class, (t_method)m_master,GENSYM("master"),A_GIMME,0);
  class_addmethod(soundspotter_tilde_class, (t_method)m_slave,GENSYM("slave"),A_GIMME,0);  
  
  // Float argument bindings
  class_addmethod(soundspotter_tilde_class, (t_method)m_setQueueSize,GENSYM("setQueueSize"),A_DEFFLOAT, 0);  
  class_addmethod(soundspotter_tilde_class, (t_method)m_setShingleSize,GENSYM("setShingleSize"),A_DEFFLOAT, 0);  
  class_addmethod(soundspotter_tilde_class, (t_method)m_setLoBasis,GENSYM("setLoBasis"),A_DEFFLOAT, 0);  
  class_addmethod(soundspotter_tilde_class, (t_method)m_setBasisWidth,GENSYM("setBasisWidth"),A_DEFFLOAT, 0);    
  class_addmethod(soundspotter_tilde_class, (t_method)m_setMatchRadius,GENSYM("setMatchRadius"),A_DEFFLOAT, 0);    
  class_addmethod(soundspotter_tilde_class, (t_method)m_setLoDataLoc,GENSYM("setLoDataLoc"),A_DEFFLOAT, 0);    
  class_addmethod(soundspotter_tilde_class, (t_method)m_setHiDataLoc,GENSYM("setHiDataLoc"),A_DEFFLOAT, 0);    
  class_addmethod(soundspotter_tilde_class, (t_method)m_setEnvFollow,GENSYM("setEnvFollow"),A_DEFFLOAT, 0);    
  class_addmethod(soundspotter_tilde_class, (t_method)m_setBetaParameter,GENSYM("setBetaParameter"),A_DEFFLOAT, 0);

  // Feature and buffer messages bindings
  class_addmethod(soundspotter_tilde_class, (t_method)m_extract,GENSYM("extract"),A_GIMME,0);
  class_addmethod(soundspotter_tilde_class, (t_method)m_dump,GENSYM("dump"),A_GIMME,0);  
  class_addmethod(soundspotter_tilde_class, (t_method)m_load,GENSYM("load"),A_GIMME,0);    
  class_addmethod(soundspotter_tilde_class, (t_method)m_soundfile,GENSYM("soundfile"),A_GIMME,0);    

  class_addfloat(soundspotter_tilde_class, (t_method)m_setQueueSize);
  class_addfloat(soundspotter_tilde_class, (t_method)m_setShingleSize);
  class_addfloat(soundspotter_tilde_class, (t_method)m_setLoBasis);
  class_addfloat(soundspotter_tilde_class, (t_method)m_setBasisWidth);
  class_addfloat(soundspotter_tilde_class, (t_method)m_setMatchRadius);
  class_addfloat(soundspotter_tilde_class, (t_method)m_setLoDataLoc);
  class_addfloat(soundspotter_tilde_class, (t_method)m_setHiDataLoc);
  class_addfloat(soundspotter_tilde_class, (t_method)m_setEnvFollow);
  class_addfloat(soundspotter_tilde_class, (t_method)m_setBetaParameter);
    
  CLASS_MAINSIGNALIN(soundspotter_tilde_class, t_soundspotter_tilde, f);
}

DECLSPEC void soundspotter_tilde_dsp(t_soundspotter_tilde *x, t_signal **sp){
  dsp_add(soundspotter_tilde_perform, 6, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

DECLSPEC t_int *soundspotter_tilde_perform(t_int* w){
  t_soundspotter_tilde* x = (t_soundspotter_tilde*) w[1]; // Get the instance pointer from argument list
  if(w[2]!=SS_WINDOW_LENGTH){
    post("MAX Options->DSP_Status->Signal_Vector_Size must be set to 2048");
    ss_stop(x->SS);
  }
  else{
    if(ss_getStatus(x->SS)==EXTRACT){
      post("**READY**"); 
    }
    // WindowLength needs be 2048 samps at the moment
    // input buffer should be MONO
    // ouptput buffer should be STEREO INTERLEAVED
    t_sample* outBufPtr = x->audioOutputBuffer;
    if(ss_getStatus(x->SS)==THRU || ss_getStatus(x->SS)==STOP){
      outBufPtr =  (t_sample*)w[6];
    }
    ss_run(x->SS, w[2], (t_sample*)w[3],(t_sample*)w[4],(t_sample*)w[5], outBufPtr);

    // FIX ME: This is complicated because we haven't got a clean stereo implementation in
    // DriverPD yet. Coming soon. mkc 11/09
    if(ss_getStatus(x->SS)==SPOT || ss_getStatus(x->SS)==EXTRACTANDSPOT){
      t_float result = (t_float) ss_reportResult(x->SS);\
      t_float distance = (t_float) ss_reportDistance(x->SS);
      outlet_float(x->winningDistanceOut, expf( -x->SS->getBetaParameter() * distance ) ); // Post the winning distance to outlet 4
      if(result!=-1.){
	outlet_float(x->winningFrameOut, result); // Post the winning frame to outlet 3
	// Copy stereo output samples to MONO PD output buffer for now
	t_sample* oppd = (t_sample*)w[6];
	t_sample* opss = x->audioOutputBuffer;
	int nn = w[2];      
	int numChannels = (x->SS->getStatus()==EXTRACTANDSPOT)?1:x->SS->getAudioDatabaseNumChannels();
	while(nn--){
	  int c = numChannels - 1;
	  *oppd = *opss++; // Hook for multichannel output, mix for now
	  while(c--){
	    *oppd += *opss++;
	  }
	  *oppd++;
	}
      }
    }
  }
  return w+7; // Need to return w + numArgs + 1 
}

DECLSPEC void soundspotter_tilde_cleanup(void* z,...){
  t_soundspotter_tilde* x = (t_soundspotter_tilde*) z;
  ss_free(x->SS);
  delete x->aSound;
  delete x->audioOutputBuffer;
}

void m_stop(t_soundspotter_tilde* x){
  ss_stop(x->SS);
  post("STOP");
}


void m_spot(t_soundspotter_tilde* x){
  ss_spot(x->SS);
  post("SPOT");
}

void m_thru(t_soundspotter_tilde* x){
  ss_thru(x->SS);
  post("THRU");
}

void m_liveSpot(t_soundspotter_tilde* x){
  if( ss_liveSpot(x->SS) ){
    post("LIVESPOT");
  }
}

void m_master(t_soundspotter_tilde* x){
  ss_master(x->SS);
  post("MASTER");
}

void m_slave(t_soundspotter_tilde* x){
  ss_slave(x->SS);
  post("SLAVE");
}

// The SoundFile and corresponding AudioBuffer are the Applications responsibility
// Once an audio buffer is allocated and populated, ss_extract is called
// This makes database sound data allocation the responsibility of the application
// not the API. Hence, SoundFile lives in the Application data space, not the SoundSpotter API.
void m_extract(t_soundspotter_tilde* x, t_symbol *s, short argc,const t_atom *argv){
  t_symbol* sfName;  

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
      // Call ss_extract to initialize audio database with soundspotter
      ss_extract(x->SS, x->aSound->getSoundBuf(), x->aSound->getBufLen(), x->aSound->getNumChannels());
      delete[] x->audioOutputBuffer;
      x->audioOutputBuffer = new t_sample[SS_WINDOW_LENGTH*x->aSound->getNumChannels()];
      post("Loaded %s...", sfName->s_name);
      post("EXTRACTING %s...", sfName->s_name);
    }
  }
  else{
    post("m_extract: requires a valid soundfile name (wav/aiff)");
  }
}


void m_dump(t_soundspotter_tilde* x, t_symbol *s,short argc,const t_atom *argv)
{
  if(argc){
    ss_dump(x->SS, GetString(argv[0]));
    post("DumpVectors %s", GetString(argv[0]));
  }
}

void m_load(t_soundspotter_tilde* x, t_symbol *s, short argc,const t_atom *argv)
{
  if(argc){
    int result = ss_load(x->SS, GetString(argv[0]));
    post("LoadVectors %s %d", GetString(argv[0]), result);
  }
}

void m_soundfile(t_soundspotter_tilde* x, t_symbol *s, short argc,const t_atom *argv)
{
  t_symbol* sfName;  
  if(argc){
    delete x->aSound;
    x->aSound = new SoundFile();
    if(!x->aSound){
      post("Couldn't instantiate SoundFile class");
    }
    
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
      ss_setAudioBuf(x->SS, x->aSound->getSoundBuf(), x->aSound->getBufLen(), x->aSound->getNumChannels());
      delete[] x->audioOutputBuffer;
      x->audioOutputBuffer = new t_sample[SS_WINDOW_LENGTH*x->aSound->getNumChannels()];
      post("Read %d frames from %s", retval, sfName->s_name);
    }	
  }
  else{
    post("m_extract: requires a valid soundfile name (wav/aiff)");
  } 
}


void m_setQueueSize(t_soundspotter_tilde* x, t_floatarg i){
  ss_setQueueSize(x->SS, (int)i);
}

void m_setShingleSize(t_soundspotter_tilde* x, t_floatarg i){
  ss_setShingleSize(x->SS, (int)i);
}

void m_setLoBasis(t_soundspotter_tilde* x, t_floatarg i){
  ss_setLoBasis(x->SS, (int)i);
}

void m_setBasisWidth(t_soundspotter_tilde* x, t_floatarg i){
  ss_setBasisWidth(x->SS, (int)i);
}

void m_setMatchRadius(t_soundspotter_tilde* x, t_floatarg f){
  ss_setMatchRadius(x->SS, f);
}

void m_setLoDataLoc(t_soundspotter_tilde* x, t_floatarg f){
  ss_setLoDataLoc(x->SS, f);
} 

void m_setHiDataLoc(t_soundspotter_tilde* x, t_floatarg f){
  ss_setHiDataLoc(x->SS, f);
}

void m_setEnvFollow(t_soundspotter_tilde* x, t_floatarg f){
  ss_setEnvFollow(x->SS, f);
} 

void m_setBetaParameter(t_soundspotter_tilde* x, t_floatarg f){
  ss_setBetaParameter(x->SS, f);
} 


#ifdef _WIN32
// Windows DLLs need to export a main function as an entry point
int main(void){
  return 1;
}
#endif
