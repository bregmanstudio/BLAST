#ifndef	FFMPEGINTERFACE_H
#define	FFMPEGINTERFACE_H

extern "C"{
#ifdef FFMPEG_51
#include <avcodec/avcodec.h>
#include <avformat/avformat.h>
#else
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#endif
};
#include <stdlib.h>

#define	SFM_READ	0		/* Dummy format request */

typedef struct {       
	struct FFAudioContext	*context;
	long			frames ;                
        int                     samplerate ;
	int			samplesize;
        int                     channels ;
        int                     format ;
        int                     sections ;
        int                     seekable ;
} SF_INFO;

struct FFAudioContext {
        int audioStream;
	SF_INFO soundInfo;
        AVFormatContext *pFormatCtx;
        AVCodecContext *pCodecCtx;
};

#ifdef __cplusplus
extern "C" {
#endif
  
  typedef struct FFAudioContext SNDFILE;
  SNDFILE *sf_open(const char *, int, SF_INFO *);
  
  void sf_close(SNDFILE *);  
  const char* sf_strerror (SNDFILE *sndfile) ;
  
  int sf_readf_double(SNDFILE *, double *, unsigned int);
  int sf_readf_float(SNDFILE *, float *, unsigned int);

#ifdef __cplusplus
}
#endif

#endif	/* FFMPEGINTERFACE_H */
