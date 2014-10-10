// Rationalize SoundSpotter drivers through common interface
// The SoundSpotter C-API
#include "SoundSpotter.h"

extern "C" {

  // Constructor
  SoundSpotter* ss_new(int sr, int windowLength, int numChannels);

  // Destructor
  void ss_free(SoundSpotter* x);


  // Action messages (return 1 if successful, 0 if fail)
  int ss_stop(SoundSpotter*);  /* Stop spotting */
  int ss_spot(SoundSpotter*);  /* Start spotting */
  int ss_thru(SoundSpotter*);  /* Pass audio through (no spotting) */
  int ss_liveSpot(SoundSpotter*); /* Start spotting to accumulating input audio */
  void ss_master(SoundSpotter*); /* Set soundspotter to Master mode */
  void ss_slave(SoundSpotter*); /* Set soundspotter to Master mode */
  
  
  // Messages with arguments (filename) (return 1 if successful, 0 if fail)
  int ss_extract(SoundSpotter*, ss_sample*, unsigned long, int); /* Extract features from soundfile sfName */
  int ss_setAudioBuf(SoundSpotter* , ss_sample* , unsigned long, int);
  int ss_dump(SoundSpotter*, const char *fName);    /* Dump feature cache fName */
  int ss_load(SoundSpotter*, const char *fName);    /* Load feature cahce by fName */
  
  // Setters (mutators)
  void ss_setAudioInputBuffer(SoundSpotter*, const ss_sample*, unsigned long); /* user pointer to ss_sample data */
  void ss_setQueueSize(SoundSpotter*, int);    /* leave frame out queue size [4]-10000 */
  void ss_setShingleSize(SoundSpotter*, int);  /* number of stacked frames in a shingle [4]-1000 */
  void ss_setLoBasis(SoundSpotter*, int);      /* start dimension of MFCC [7]-83 */
  void ss_setBasisWidth(SoundSpotter*, int);   /* number of MFCC coefficients to use [20]-83 */
  void ss_setMatchRadius(SoundSpotter*, float);/* search radius: [0.0]-4.0 */
  void ss_setLoDataLoc(SoundSpotter*, float);  /* database start point (seconds) [0.0] */
  void ss_setHiDataLoc(SoundSpotter*, float);  /* database end point from end (seconds) [0.0] */
  void ss_setEnvFollow(SoundSpotter*, float);  /* envelope follow factor [0.0]-1.0 */
  void ss_setBetaParameter(SoundSpotter*, float);  /* distance probability factor 0.0-10.0 [1.0] */

  // Getters (accessors)
  SoundSpotterStatus ss_getStatus(SoundSpotter*); /* Return current state */

  // DSP loop
  void ss_run(SoundSpotter*,
	      int windowLength,      /* size of audio input buffer */
	      ss_sample* featuresIn, /* audio input features (optional) */
	      ss_sample* audioIn,    /* audio signal input */
	      ss_sample* featuresOut,/* audio output features */ 
	      ss_sample* audioOut);  /* audio output signal */	      

  // Auxillary float output
  int ss_reportResult(SoundSpotter* x);   /* get winning frame for last match */
  float ss_reportDistance(SoundSpotter* x);   /* get winning frame for last match */
};
