#include "Compiler.h"

/*
 * Window logic:
 *    h = one_hop_in_samples (fhop)
 *    H = hop multiple (hop_size) fft_hop_size
 *    N = l x H (frame_size), l = fft_size / H
 *    s = shingle_size (ssz)
 *    W = (s + l - 1) x H // samples_per_shingle
 *   Hh = H*h // samples_per_hop
 *
 *   uint32_t hop_size; // number of hops per query (h)
 *   uint32_t one_hop_in_samples; // length of one hop in samples (H)
 *   uint32_t shingle_size; // number of frames to sequence (s)
 *   uint32_t frame_size; // length of a frame in samples (l x H)
 *   uint32_t samples_per_hop; // length of hops in samples (h x H)
 *   long samples_per_shingle; // length of a 'sample' covered by feature shingle (s + l - 1) x H
 *
 */
Compiler::Compiler(MatshupSet* const ms, const char* tfn, uint32_t ssz, uint32_t hsz, float beta, float mix, 
		   uint32_t fsz, uint32_t fhop, const char* tfeatn) :
  hop_size(hsz),
  one_hop_in_samples(fhop),
  shingle_size(ssz),
  frame_size(fsz),
  samples_per_hop(hop_size*fhop),
  samples_per_shingle((shingle_size-1) * fhop + frame_size), // num samples 'covered' by a feature shingle
  qpos(0),
  beta(beta),
  mix(mix),
  targetSound(0),
  matshupSound(0),
  target_channels(0),
  outbuf(0),
  matshup(ms)
{
  initialize(tfn, tfeatn);  
}
  
Compiler::~Compiler(){
  delete[] outbuf;
  delete[] HammingWindow;
}

int Compiler::initialize(const char* targetFileName, const char* targetFeatureName){  
  std::string str(targetFeatureName);
  if ( str.find(".") == std::string::npos) 
    aCollage::error("Cannot get file extension from targetFileName %s.", targetFileName);  
  std::string matshupName(str.substr(0,str.find(".")));
  matshupName.append(".imatsh.wav");
  SF_INFO sfinfo;
  init_sfinfo(&sfinfo);
  open_audio_file(&targetSound, targetFileName, SFM_READ, &sfinfo);
  target_channels = sfinfo.channels;
  sfinfo.channels = MAX_CHANNELS;
  open_audio_file(&matshupSound, matshupName.c_str(), SFM_WRITE, &sfinfo);
  outbuf = new short[samples_per_shingle * sfinfo.channels];
  memset(outbuf, 0, samples_per_shingle*sizeof(short)*sfinfo.channels);
  HammingWindow = _make_hamming_window(samples_per_shingle, sfinfo.channels);
  return EXIT_GOOD;
}

int Compiler::init_sfinfo(SF_INFO* sfinfo){
  sfinfo->frames = 0;
  sfinfo->samplerate = 0;
  sfinfo->channels = 0;
  sfinfo->format = 0;
  sfinfo->sections = 0;
  sfinfo->seekable = 0;
  return EXIT_GOOD;
}

int Compiler::empty(){
  return matshup->empty();
}

long Compiler::compile_next(){  
  NNresult r;
  r.query_pos = qpos;
  MitPair current_subset = matshup->equal_range(r);
  long numSamples = compile_audio_shingle( current_subset );
  write_outfile();
  update_outbuf();
  matshup->erase(current_subset.first, current_subset.second);
  qpos+=hop_size;
  return numSamples;
}

// Assume non-overlapping query_pos shingles with (shingle_size-1)*frame_hop + frame_size samples
// track_pos shingles can overlap
// overlap-add frame_size-frame_hop samples into next outputbuf
long Compiler::compile_audio_shingle( MitPair& mp ){
  Mit it = mp.first;
  long nSamples = 0;
  if( it != mp.second){
    short tbuf[samples_per_shingle * MAX_CHANNELS];
    short sbuf[samples_per_shingle * MAX_CHANNELS];
    memset(tbuf, 0, samples_per_shingle*MAX_CHANNELS*sizeof(short));
    memset(sbuf, 0, samples_per_shingle*MAX_CHANNELS*sizeof(short));
    long nTarget, nSource;
    float prob;
    float expNormDists = sum_exp_distances(mp, beta);
    float rmsTarget, rmsSource, smix = mix, amp=0;
    float eps = std::numeric_limits<float>::epsilon();
    NNresult r = *it;
    nTarget = read_audio_shingle(tbuf, targetSound, r.query_pos*one_hop_in_samples, samples_per_shingle, target_channels, MAX_CHANNELS);
    if(shingle_size!=hop_size)
      window_audio_shingle(tbuf, samples_per_shingle, MAX_CHANNELS);
    //audio_shingle_to_outbuf(tbuf, 0.1*mix,  nTarget, 0, MAX_CHANNELS); // Mix to stereo channels
    rmsTarget = compute_RMS(tbuf, nTarget, MAX_CHANNELS);
    while( it != mp.second ){
      r = *it++;
      nSource = read_audio_shingle(sbuf, r.media.c_str(), r.track_pos*one_hop_in_samples, samples_per_shingle, MAX_CHANNELS);
      if(shingle_size!=hop_size)      
	window_audio_shingle(sbuf, samples_per_shingle, MAX_CHANNELS);
      nSamples = min( nTarget, nSource );
      if(nSamples){
	rmsSource = compute_RMS(sbuf, nSamples, MAX_CHANNELS);
	if (BALANCE_TARGET_SOURCE_AUDIO){
	  if ( rmsTarget > eps && rmsSource > eps){
	    amp = 0.9 * rmsTarget / rmsSource;
	  }
	  else{
	    amp = 0.0f;
	  }
	} // ELSE DO NOT BALANCE_TARGET_SOURCE_AUDIO
	else{
	  amp = 1.0f;
	}
	prob = expf( -beta * r.dist) / expNormDists;
	fprintf(stdout, "dist: %f prob: %f amp: %f smix: %f\n", fabs(r.dist),  prob, amp, smix);	
	fprintf(stdout, "audio: %f %f %f %d %d %d\n", fabs(r.dist), prob, amp*prob*smix, r.query_pos, r.track_pos, r.media_idx);
	audio_shingle_to_outbuf(sbuf, amp*prob*smix,  nSamples, 0, MAX_CHANNELS); // Mix to stereo output
      }
    }
  }
  return nSamples;
}

int Compiler::open_audio_file(SNDFILE **snd, const char* fname, int mode, SF_INFO* sfinfo){
  *snd = sf_open(fname, mode, sfinfo);
  if( ! *snd ){
    complete();
    char ebuf[MAXSTR];
    sf_error_str(*snd, ebuf, MAXSTR);
    aCollage::error("SoundFile %s failed to open. %s", fname, ebuf);
  }
  if( sfinfo->channels > MAX_CHANNELS ){
    sf_close(*snd);
    *snd=0;
    complete();
    aCollage::error("Unsupported channels: %d", sfinfo->channels);
  }
  return sfinfo->channels;
}

long Compiler::read_audio_shingle(short* buf, const char* file, long samp_start, long num_samps, int num_out_channels){
  SNDFILE* snd;
  SF_INFO sfinfo;
  init_sfinfo(&sfinfo);
  open_audio_file(&snd, file, SFM_READ, &sfinfo);
  long num_read = read_audio_shingle(buf, snd, samp_start, num_samps, sfinfo.channels, num_out_channels);
  sf_close(snd);
  return num_read;
}

long Compiler::read_audio_shingle(short* buf, SNDFILE* snd, long samp_start, long num_samps, int channels, int num_out_channels){
  sf_seek(snd, samp_start, SEEK_SET);
  num_samps = sf_readf_short(snd, buf, num_samps);
  if(num_out_channels==1 && channels>1){
    to_mono(buf, num_samps, channels); // take channel 1 only
  }
  else if(channels==1 && num_out_channels>1){
    up_mix(buf, num_samps, num_out_channels);
  }
  return num_samps;
}

void Compiler::window_audio_shingle(short* buf, uint32_t num_samps, int num_channels){
  uint32_t n = num_samps * num_channels;
  short* p = buf; 
  float* w = HammingWindow; 
  _apply_window(p, w, n);
}

inline void Compiler::_apply_window(short* p, float* w, uint32_t n){
  while(n--){
    *p = (short)(*p * *w) ;
    p++;
    w++;
  }
}

float* Compiler::_make_hamming_window(int num_samples, int num_channels){
  int M = num_samples;
  float* win = new float[num_samples*num_channels];
  float* w = win;
  for(int n= 0; n<M; n++)
    for(int c=0; c<num_channels; c++)
      *w++ = 0.5 - 0.5 * cos((2*M_PI*n)/(M-1));
  return win;
}

void Compiler::to_mono(short* buf, long num_samps, int channels){
  short *op = buf, *bp = buf;    
  while(num_samps--){
    *op = *bp;
    op++;
    bp+=channels;
  }
}

void Compiler::up_mix(short* buf, long num_samps, int channels){
  short *op = buf+num_samps*channels-1, *bp = buf+num_samps-1;    
  int c;
  while(num_samps--){
    c = channels;
    while(c--){
      *op-- = *bp;
    }
    bp--;
  }
}

long Compiler::audio_shingle_to_outbuf(short *buf, float amp, long num_samps, int channel_offset, int num_channels){
  short *op = outbuf + channel_offset;
  long n = num_samps * num_channels;
  if( amp > std::numeric_limits<float>::epsilon() ){
    if(num_channels==1){
      while(n--){
	*op += (short)(amp * *buf++);
	op+=MAX_CHANNELS;
      }
    }
    else{
      while(n--){
	*op++ += (short)(amp * *buf++);
      }
    }
  }
  return num_samps;
}

long Compiler::write_outfile(int last_frame){
  uint32_t num_written = 0;
  if(last_frame)
    num_written = sf_writef_short(matshupSound, outbuf, samples_per_shingle - samples_per_hop); 
  else
     num_written = sf_writef_short(matshupSound, outbuf, samples_per_hop); 
  if(!last_frame && num_written != samples_per_hop){
    complete();
    aCollage::error("Short soundfile write in update_outbuf");
  }
  return num_written;
}

int Compiler::update_outbuf(){
  long n = samples_per_shingle;
  memmove( outbuf, outbuf + samples_per_hop * MAX_CHANNELS, (n - samples_per_hop) * sizeof(short) * MAX_CHANNELS );
  memset( outbuf + (n - samples_per_hop) * MAX_CHANNELS, 0, samples_per_hop * sizeof(short) * MAX_CHANNELS);
  return EXIT_GOOD;
}

float Compiler::sum_exp_distances(MitPair& mp, float beta){
  Mit it = mp.first;
  NNresult r;
  float d = 0.0f;
  while( it != mp.second ){
    r = *it++;
    d+= expf( -beta * r.dist );
  }
  return d;
}

float Compiler::compute_RMS(short* a, long n, int n_channels){
  float y = 0.f, x=0.f;
  long nn = n;
  while(nn--){
    x = (float)*a;
    a += n_channels;
    y += x*x;
  }
  float rms = sqrtf ( y / n );
  return rms;
}


int Compiler::complete(){
  // write remainder of last frame
  write_outfile(1);
  // close open file descriptors
  if(matshupSound)
    sf_close(matshupSound);
  if(targetSound)
    sf_close(targetSound);
  return EXIT_GOOD;
}
