/*
 * Compiler.h - interface to the iMatsh compiler
 *
 * Author: Michael A. Casey
 * Copyright (C) 2010 Michael Casey, Dartmouth College, All Rights Reserved
 *
 */

#include "iMatsh.h"
#include "sndfile.h"
#define _USE_MATH_DEFINES

#define IMATSH_DEFAULT_FRAME_SIZE 1024
#define IMATSH_DEFAULT_FRAME_HOP 1024
#define MAX_CHANNELS 2

class Compiler {
protected:

  uint32_t hop_size; // number of hops per query (h)
  uint32_t one_hop_in_samples; // length of one hop in samples (H)
  uint32_t shingle_size; // number of frames to sequence (s)
  uint32_t frame_size; // length of a frame in samples (l x H)
  uint32_t samples_per_hop; // length of hops in samples (h x H)
  long samples_per_shingle; // length of a 'sample' covered by feature shingle (s + l - 1) x H
  uint32_t qpos;           // currently compiling time instant (query position)
  float beta;
  float mix;
  SNDFILE *targetSound;
  SNDFILE *matshupSound;
  int target_channels;
  short* outbuf;  
  MatshupSet* const matshup;  
  float* HammingWindow;

  long compile_audio_shingle(MitPair& ip);
  int open_audio_file(SNDFILE **snd, const char* fname, int mode, SF_INFO* sfinfo);
  long read_audio_shingle(short* tbuf, const char* file, long samp_start, long num_samps, int num_out_channels=1);
  long read_audio_shingle(short* buf, SNDFILE* snd, long samp_start, long num_samps, int channels, int num_out_channels=1);
  void to_mono(short* buf, long num_samps, int channels);
  void up_mix(short* buf, long num_samps, int channels);
  long audio_shingle_to_outbuf(short *buf, float amp, long num_samps, int channel_offset=0, int num_channels=1);
  float sum_exp_distances(MitPair& mp, float beta);
  float compute_RMS(short* a, long n);
  long write_outfile(int last_frame=0);
  int update_outbuf();
  int init_sfinfo(SF_INFO* sfinfo);

  void window_audio_shingle(short* buf, uint32_t num_samps, int num_channels);
  void _apply_window(short* p, float* w, uint32_t n);
  float* _make_hamming_window(int num_samples, int num_channels=1);

public:  
  Compiler(MatshupSet* const matshup_set, const char* targetFileName, uint32_t shingle_size, uint32_t hop_size,
	   float beta, float mix, uint32_t frame_size=IMATSH_DEFAULT_FRAME_SIZE, 
	   uint32_t frame_hop=IMATSH_DEFAULT_FRAME_HOP);
  ~Compiler();
  int initialize(const char* targetFileName);
  int empty();
  long compile_next();
  int complete();
};

