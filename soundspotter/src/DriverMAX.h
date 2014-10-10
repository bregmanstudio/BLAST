/* Max/Msp driver layer for SoundSpotter */

/* The driver layer handles all the UI, State, I/O and buffer storage */
#include "ext.h"
#include "z_dsp.h"

/* SoundFileLib and SoundSpotter interfaces */
#include "SoundFile.h"
#include "SoundSpotter.h"
#include "DriverCommon.h"

void *soundspotter_class;

typedef struct _soundspotter
{
  t_pxobject x_obj;
  SoundSpotter* SS;         /* pointer to SoundSpotter C++ instance */
  SoundFile * aSound;       /* pointer to SoundFile instance        */
  void* winningFrameOut;    /* pointer to float outlet (3)          */
  int windowLength;         /* soundspotter internal sample buffer size */
  t_sample* audioOutputBuffer; /* array for soundspotter interleaved multichannel output */ 
  t_sample* audioInputBuffer;  /* array for soundspotter single-channel (MONO) input     */ 
  t_sample* featureInputBuffer;  /* array for soundspotter feature vector input (optional)    */ 
  t_sample* featureOutputBuffer;  /* array for soundspotter feature vector output (optional)  */ 
  int insamp_pos;              /* current soundspotter input array position              */
  int outsamp_pos;             /* current soundspotter output array position             */
} t_soundspotter;

void * soundspotter_new(void);
void soundspotter_cleanup(void*,...);
t_int * soundspotter_perform(t_int *w);

/* Signal routine */
void soundspotter_dsp(t_soundspotter* x, t_signal **sp, short *count);
/* void m_signal(int n, t_sample *const *in, t_sample *const *out); */

void soundspotter_assist(t_soundspotter* x, void *b, long m, long a, char *s);

/* soundspotter MAX helper methods */
void soundspotter_copy_samples_to_output(t_int* w);
void soundspotter_copy_samples_from_input(t_int* w);
void soundspotter_do_match(t_int* w);

// Messages
void m_stop(t_soundspotter*);
void m_spot(t_soundspotter*);
void m_thru(t_soundspotter*);
void m_liveSpot(t_soundspotter*);


// Messages with arguments (filename)
void m_extract(t_soundspotter*, t_symbol *s, short int argc,const t_atom *argv);
void m_dump(t_soundspotter*, t_symbol *s, short argc,const t_atom *argv);
void m_load(t_soundspotter*, t_symbol *s, short argc,const t_atom *argv);

// Numerical inlets
void m_intQueueSize(t_soundspotter*,long);
void m_intShingleSize(t_soundspotter*,long);
void m_intLoBasis(t_soundspotter*,long);
void m_intBasisWidth(t_soundspotter*,long);
void m_floatRadius(t_soundspotter*,double);
void m_intLoK(t_soundspotter*,long);
void m_intHiK(t_soundspotter*,long);
void m_floatEnvFollow(t_soundspotter*,double f);  



