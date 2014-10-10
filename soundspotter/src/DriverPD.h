/* PD driver layer for SoundSpotter */
/* The driver layer handles all the UI, State, I/O and buffer storage */

/* SoundFileLib and SoundSpotter interfaces are in C++ */
#include "SoundFile.h"
#include "SoundSpotter.h"
#include "DriverCommon.h"
#include <string.h>

/* The remainder of the PD interface is in C */

extern "C" {
  // Include the PD environment 
#include "m_pd.h"
  
  // Typed pointer to the t_soundspotter class
  static t_class* soundspotter_tilde_class;
  
  // Declaration of the t_soundspotter class
  typedef struct _soundspotter_tilde
  {
    t_object x_obj;
    t_float f;                /* A dummy float arg */
    SoundSpotter* SS;         /* pointer to SoundSpotter C++ instance */
    SoundFile * aSound;       /* pointer to SoundFile instance */
    t_outlet* winningFrameOut;    /* pointer to float outlet (3) */
    t_outlet* winningDistanceOut;    /* pointer to float outlet (4) */
    t_sample* audioOutputBuffer;  /* array for soundspotter interleaved multichannel output */ 
  } t_soundspotter_tilde;
  
#ifdef WIN_VERSION
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC
#endif
  
  DECLSPEC void*  soundspotter_tilde_new(void);
  DECLSPEC void soundspotter_tilde_setup(void);
  DECLSPEC void   soundspotter_tilde_cleanup(void*,...);
  DECLSPEC t_int* soundspotter_tilde_perform(t_int *w);
  /* Signal routine */
  DECLSPEC void soundspotter_tilde_dsp(t_soundspotter_tilde* x, t_signal **sp);
  DECLSPEC void soundspotter_tilde_assist(t_soundspotter_tilde* x, void *b, long m, long a, char *s);
  
  // Messages
  void m_stop(t_soundspotter_tilde*);
  void m_spot(t_soundspotter_tilde*);
  void m_thru(t_soundspotter_tilde*);
  void m_liveSpot(t_soundspotter_tilde*);
  void m_master(t_soundspotter_tilde*); // set soundspotter to be a master spotter
  void m_slave(t_soundspotter_tilde*); // soundspotters can be slaved to a master spotter
  
  
  // Messages with arguments (filename)
  void m_extract(t_soundspotter_tilde*, t_symbol *s, short argc,const t_atom *argv);
  void m_dump(t_soundspotter_tilde*, t_symbol *s, short argc,const t_atom *argv);
  void m_load(t_soundspotter_tilde*, t_symbol *s, short argc,const t_atom *argv);
  void m_soundfile(t_soundspotter_tilde* x, t_symbol *s, short argc,const t_atom *argv);
  
  // Numerical inlets
  void m_setQueueSize(t_soundspotter_tilde*,t_floatarg);
  void m_setShingleSize(t_soundspotter_tilde*,t_floatarg);
  void m_setLoBasis(t_soundspotter_tilde*,t_floatarg);
  void m_setBasisWidth(t_soundspotter_tilde*,t_floatarg);
  void m_setMatchRadius(t_soundspotter_tilde*,t_floatarg);
  void m_setLoDataLoc(t_soundspotter_tilde*,t_floatarg);
  void m_setHiDataLoc(t_soundspotter_tilde*,t_floatarg);
  void m_setEnvFollow(t_soundspotter_tilde*,t_floatarg);  
  void m_setBetaParameter(t_soundspotter_tilde*,t_floatarg);    
  
  
};
