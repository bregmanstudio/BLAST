//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/11/13 09:08:27 $
//
// Category     : VST 2.x SDK Samples
// Filename     : soundspotterVST.h
// Created by   : Steinberg Media Technologies
// Description  : soundspotter plugin (Mono->Stereo)
//
// © 2006, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#ifndef __soundspotterVST__
#define __soundspotterVST__

#include "public.sdk/source/vst2.x/audioeffectx.h"
#include "SoundSpotter.h"
#include "SoundFile.h"

enum{
	SSBUFLEN = 1024,
	SSAUDIODBLEN = 44100*60,
	SSMAXSHINGLESZ = 32,
	SSMAXQUEUESZ = 100,
	SSNUMBASIS = SS_NUM_BASIS,
	SSMAXRADIUS = SS_MAX_RADIUS,
	SSNUMCHANNELS = 2
};

enum
{
	// Global
	kNumPrograms = 16,

	// Parameters Tags
	kOut=0,
	kFeedBack,
	kShingleSize,
	kQueueSize,
	kLoBasis,
	kNumBasis,
	kEnvFollow,
	kMatchRadius,
	kLoSec,
	kHiSec,
	kMix,
	kNumParams
};

class SoundspotterVST;

//------------------------------------------------------------------------
class SoundspotterVSTProgram
{
friend class SoundspotterVST;
public:
	SoundspotterVSTProgram ();
	~SoundspotterVSTProgram () {}

private:	
	float fFeedBack;
	float fOut;
	float fShingleSize;
	float fQueueSize;
	float fLoBasis;
	float fNumBasis;
	float fEnvFollow;
	float fMatchRadius;
	float fLoSec;
	float fHiSec;
	float fMix;
	char name[24];
};

//------------------------------------------------------------------------
class SoundspotterVST : public AudioEffectX
{
public:
	SoundspotterVST (audioMasterCallback audioMaster);
	~SoundspotterVST ();

	//---from AudioEffect-----------------------
	virtual void processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames);
	virtual char* openSourceFile(void);
	virtual void setProgram (VstInt32 program);
	virtual void setProgramName (char* name);
	virtual void getProgramName (char* name);
	virtual bool getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text);
	
	virtual void setParameter (VstInt32 index, float value);
	virtual float getParameter (VstInt32 index);
	virtual void getParameterLabel (VstInt32 index, char* label);
	virtual void getParameterDisplay (VstInt32 index, char* text);
	virtual void getParameterName (VstInt32 index, char* text);

	virtual void resume ();

	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion () { return 1000; }
	
	virtual VstPlugCategory getPlugCategory () { return kPlugCategEffect; }
	virtual void extract(char* fileName);
	virtual VstInt32 canDo (char* text);

protected:
	char sourceAudioPath[1024];

	void initializeSoundSpotter();
	void setShingleSize (float fshinglesize);
	void setQueueSize (float fqueuesize);
	void setLoBasis (float flobasis);
	void setNumBasis (float fnumbasis);
	void setEnvFollow (float fenvfollow);
	void setMatchRadius (float fmatchradius);
	void setLoSec(float flosec);
	void setHiSec(float fhisec);
	SoundspotterVSTProgram* programs;
	SoundSpotter* soundSpotter;

	float* bufferIn;
	float* bufferOut;
	float* bufferFeedback;
	float* audioDatabase;
	float* featuresIn;
	float* featuresOut;
	SoundFile* soundFile;

	float fFeedBack;
	float fOut;
	float fShingleSize;
	float fQueueSize;
	float fLoBasis;
	float fNumBasis;
	float fEnvFollow;
	float fMatchRadius;
	float fLoSec;
	float fHiSec;
	float fMix;

	long size;
	long cursor;
};
#endif

#ifndef __linux
#include "cfileselector.h"

class SoundspotterVSTEditor : public AEffGUIEditor
{
public:
	SoundspotterVSTEditor (AudioEffect* effect);
	virtual ~SoundspotterVSTEditor ();

protected:
	SoundspotterVST* soundspotterVSTEffect;
	char sourceAudioPath[1024];
	virtual bool open (void *ptr);
	virtual void close ();
	virtual void idle ();
};
#endif
