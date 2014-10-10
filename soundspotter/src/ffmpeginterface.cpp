/* 
 * Interface to the FFMPEG libraries.  Make them look like the 
 * libsndfile interface.
 * 
 * Malcolm Slaney - Yahoo! Research - July 2008
 */

/* 
 * http://www.inb.uni-luebeck.de/~boehme/using_libavcodec.html
 * http://www.inb.uni-luebeck.de/~boehme/libavcodec_update.html
 */

/* Compile for testing with
  gcc -I/usr/local/ffmpeg/include -L/usr/local/ffmpeg/lib ffmpeginterface.c \
	-lavcodec -lavformat -lavutil -lm  -lavdevice -lavcodec -lavformat \
	-lavutil -lz -DMAIN
 */


#include	"ffmpeginterface.h"

// static int audioStream = -1;

struct FFAudioContext *OpenFFSoundFile(const char *filename){
        unsigned int            i;
	static int	doneInit = 0;
	struct FFAudioContext *res = (FFAudioContext*) calloc(1, sizeof(*res));

	if (res == 0) return 0;

	if (!doneInit){
		av_register_all();
		doneInit = 1;
	}

	// Open audio file
	if(av_open_input_file(&res->pFormatCtx, filename, NULL, 0, NULL)!=0){
		printf("Can't open %s.\n", filename);
		exit(1);
	}

	// Retrieve stream information
	if(av_find_stream_info(res->pFormatCtx)<0){
		// Couldn't find stream information
		printf("Can't find stream information in %s.\n", filename); 
		exit(1);
	}

	dump_format(res->pFormatCtx, 0, filename, 0);

	// Find the first audio stream
	res->audioStream=-1;
	// printf("Found %d streams.\n", res->pFormatCtx->nb_streams);
	for(i=0; i<res->pFormatCtx->nb_streams; i++){
		// printf("%d:\n", i);
		// printf("stream is %x\n", res->pFormatCtx->streams[i]);
		// printf("codec is %x\n", res->pFormatCtx->streams[i]->codec);
		// printf("type is %x\n", 
		// 	res->pFormatCtx->streams[i]->codec->codec_type);
		if(res->pFormatCtx->streams[i]->codec->codec_type==
				CODEC_TYPE_AUDIO) {
			res->audioStream=i;
			break;
		}
	}
	if (res->audioStream==-1){
		printf("Couldn't find an audio stream in %s.\n", filename);
		exit(1);
	}

	// AVCodecContext *pCodecCtx;
	res->pCodecCtx = res->pFormatCtx->streams[res->audioStream]->codec;

	AVCodec         *pCodec;

	pCodec=avcodec_find_decoder(res->pCodecCtx->codec_id);
	if(pCodec==NULL){
		// Codec not found
		printf("Can't find codec for %s.\n", filename);
		exit(1);
	}

	// Inform the codec that we can handle truncated bitstreams -- i.e.,
	// bitstreams where frame boundaries can fall in the middle of packets
	if(pCodec->capabilities & CODEC_CAP_TRUNCATED)
		res->pCodecCtx->flags|=CODEC_FLAG_TRUNCATED;

	// Open codec
	if(avcodec_open(res->pCodecCtx, pCodec)<0){
		// Could not open codec
		printf("Can't open the codec for %s.\n", filename); 
		exit(1);
	}

	// AVFrame *pFrame;

	// pFrame=avcodec_alloc_frame();
	return res;
}


/*
 * See 
 *	http://www.irisa.fr/texmex/people/dufouil/ffmpegdoxy/avcodec_8h.html#da524e6f5a5e11d4beb79dcbff0ee9b7
 */

/* Read some samples from the FFMPEG library.  This routine reads packets from 
 * the compressed stream.  When it finds one that is an audio packet, it 
 * passes it to the decode_audio routine.  This causes one packet's worth
 * of data to be returned to the client.
 *
 * Pass in a buffer of length big enough to hold the packet worth of data.
 * Return the number of *bytes* that are available 
 *
 * There should be an error if the packet is too small.
*/
int ReadFFSamples(struct FFAudioContext *ffContext, 
		int16_t *samples, int size){
	AVPacket packet;

	if (ffContext->pCodecCtx == 0)		// Return if already closed.
		return 0;

	while(av_read_frame(ffContext->pFormatCtx, &packet)>=0) {
		// Is this a packet from the audio stream?
		if(packet.stream_index==ffContext->audioStream){
			int err;
			err = avcodec_decode_audio2(ffContext->pCodecCtx, 
				samples, 
				&size, packet.data, packet.size);
			if (0){
			  printf("Decoded %d bytes to get %d sample bytes.\n",
				size, err);
			  fflush(stdout);
			}
			return size;
		}
	}
	// printf("All done decoding!\n");

	// Close all the stuff so we reclaim memory.
	if(packet.data!=NULL)
		av_free_packet(&packet);

	// Close the codec
	if (ffContext->pCodecCtx){
		avcodec_close(ffContext->pCodecCtx);
		ffContext->pCodecCtx = 0;
	}

	// Close the video file
	if (ffContext->pFormatCtx){
		av_close_input_file(ffContext->pFormatCtx);
		ffContext->pFormatCtx = 0;
	}

	return 0;
}

/*
 * Provide a sndlib API to the FFMPEG libraries.
 * Open a sound file.
 */

SNDFILE *sf_open(const char *filename, int mode, SF_INFO *sfinfo){
	SNDFILE *ffContext;

	ffContext = OpenFFSoundFile(filename);
	if (sfinfo){
		sfinfo->context = ffContext;
		sfinfo->samplerate = ffContext->pCodecCtx->sample_rate;
		sfinfo->frames = ffContext->pFormatCtx->duration *
			sfinfo->samplerate / AV_TIME_BASE;
		sfinfo->channels = ffContext->pCodecCtx->channels;
		sfinfo->format = 0;
		sfinfo->sections = 0;
		sfinfo->seekable = 0;
		/* From SampleFormat enum */
		sfinfo->samplesize = ffContext->pCodecCtx->sample_fmt + 1;
		if (0){
	          printf("Reading sound data from %s\n", filename);
		  printf("  sfinfo is at 0x%x.\n", (unsigned int)sfinfo);
	          printf("  Number of frames: %ld\n", sfinfo->frames);
	          printf("  Sampling rate: %d\n", sfinfo->samplerate);
	          printf("  Number of channels: %d\n", sfinfo->channels);
		}
  
	}
	ffContext->soundInfo = *sfinfo;		// Make it persistant
	return ffContext;
}


/* Read frameCountReq frames of sound data, return the number of *frames* 
 * available in the buffer.  See
 * 	http://svn.annodex.net/annodex-core/libsndfile-1.0.11/doc/api.html
 *
 * We can only request one buffer at a time.   Request the buffer (thus
 * decoding the packet) and fill in the floating point buffer.  Save the 
 * remaining samples for the next time.
 */
#define	kSampleBufferSize	AVCODEC_MAX_AUDIO_FRAME_SIZE
static int16_t sampleBuffer[kSampleBufferSize*2];
static int bufferCount = 0, bufferIndex = 0;
static int allReturnedCount = 0;

#define	SAMPLE_SCALE		(32768.0)		// Scale from ints to doubles

int sf_readf_double(SNDFILE *ffContext, double *doubleFrames, 
			unsigned int frameCountReq){
	int numBytes = ffContext->soundInfo.samplesize;
	int numChannels = ffContext->soundInfo.channels;
	int i;
	int sampleCountReq = frameCountReq * numChannels;

	for (i=0; i<sampleCountReq; i++){
		if (bufferCount <= 0 || bufferIndex >= bufferCount){
			int sizeReq = frameCountReq * numChannels;
			if (sizeReq > kSampleBufferSize) 
				sizeReq = kSampleBufferSize;
			bufferCount = ReadFFSamples(ffContext, sampleBuffer, 
						kSampleBufferSize);
			if (bufferCount <= 0)
				break;
			bufferCount /= numBytes;
			bufferIndex = 0;
		}
		doubleFrames[i] = sampleBuffer[bufferIndex++]/SAMPLE_SCALE;
	}
	allReturnedCount += i/numChannels;
	// printf("Returned %d frames so far.\n", allReturnedCount);
	fflush(stdout);
	return i/numChannels;
}

// single-precision version
int sf_readf_float(SNDFILE *ffContext, float *floatFrames, 
			unsigned int frameCountReq){
	int numBytes = ffContext->soundInfo.samplesize;
	int numChannels = ffContext->soundInfo.channels;
	int i;
	int sampleCountReq = frameCountReq * numChannels;

	for (i=0; i<sampleCountReq; i++){
		if (bufferCount <= 0 || bufferIndex >= bufferCount){
			int sizeReq = frameCountReq * numChannels;
			if (sizeReq > kSampleBufferSize) 
				sizeReq = kSampleBufferSize;
			bufferCount = ReadFFSamples(ffContext, sampleBuffer, 
						kSampleBufferSize);
			if (bufferCount <= 0)
				break;
			bufferCount /= numBytes;
			bufferIndex = 0;
		}
		floatFrames[i] = sampleBuffer[bufferIndex++]/SAMPLE_SCALE;
	}
	allReturnedCount += i/numChannels;
	// printf("Returned %d frames so far.\n", allReturnedCount);
	fflush(stdout);
	return i/numChannels;
}


const char* sf_strerror (SNDFILE *sndfile) {
	return "Got a sndfile(ffmpeg) error.";
}

	
void sf_close(SNDFILE *ffContext){
	// Close the codec
	if (ffContext->pCodecCtx){
		avcodec_close(ffContext->pCodecCtx);
		ffContext->pCodecCtx = 0;
	}

	// Close the video file
	if (ffContext->pFormatCtx){
		av_close_input_file(ffContext->pFormatCtx);
		ffContext->pFormatCtx = 0;
	}
	free(ffContext);
}

#ifdef	MAIN
#include	<math.h>

#define	kSamples	AVCODEC_MAX_AUDIO_FRAME_SIZE


main(){ 
	int	outBufSize;
	FILE	*fp = fopen("test.raw", "w");
	struct FFAudioContext *ffContext;
	SF_INFO	sfinfo;
	int16_t sampleBuffer[2*kSamples]; 
	double	dSampleBuffer[2*kSamples];
	const char      *filename="/homes/malcolm/Projects/CaseyAudioDatabase/Music/03 How Insensitive.mp3";
	// const char      *filename="/homes/malcolm/Projects/CaseyAudioDatabase/Music/14563120.wma";
	// const char      *filename="/homes/malcolm/Projects/CaseyAudioDatabase/Music/Insensitive.wav";
	// ffContext = OpenFFSoundFile(filename);
	ffContext = sf_open(filename, 0, &sfinfo);

	while (1){
		int numBytes = ffContext->soundInfo.samplesize;
		int numChannels = ffContext->soundInfo.channels;
		int frameSize = numBytes * numChannels;

		//outBufSize = ReadFFSamples(ffContext, sampleBuffer, kSamples);
		int 	i;
		int size = sf_readf_double(ffContext, dSampleBuffer, kSamples);
		// printf("Got back %d frames.\n", size);
		if (size<=0)
			break;
		for (i=0; i<size*numChannels; i++){
			sampleBuffer[i] = round(dSampleBuffer[i]*SAMPLE_SCALE);
		}
		if (fp){
			fwrite(sampleBuffer, frameSize, size, fp);
		}
	}
}


#endif	/* MAIN */
