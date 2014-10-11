//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/11/13 09:08:27 $
//
// Category     : VST 2.x SDK Samples
// Filename     : jumblatron.cpp
// Created by   : Steinberg Media Technologies
// Description  : soundspotter plugin (Mono->Stereo)
//
// © 2006, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#ifndef __jumblatron__
#include "jumblatron.h"
#endif

//----------------------------------------------------------------------------- 
JumblatronProgram::JumblatronProgram ()
{
	// default Program Values
	fFeedBack = 0.5f;
	fOut = 1.0f;
	fShingleSize = 4.0f/SSMAXSHINGLESZ;
	fQueueSize = 4.0f/SSMAXQUEUESZ;
	fLoBasis = 7.0f/SSNUMBASIS;
	fNumBasis = 35.0f/SSNUMBASIS;
	fEnvFollow = 0.5f;
	fMatchRadius = 0.0f;
	fLoSec = 0.0f;
	fHiSec = 0.0f;
	fMix = 0.5f;
	strcpy (name, "Init");
}


//-----------------------------------------------------------------------------
Jumblatron::Jumblatron (audioMasterCallback audioMaster)
: AudioEffectX (audioMaster, kNumPrograms, kNumParams),
bufferIn(0),
bufferOut(0),
bufferFeedback(0),
soundFile(0),
soundSpotter(0),
featuresIn(0),
featuresOut(0),
audioDatabase(0),
programs(0),
size(SSBUFLEN),
cursor(0)
{  
	printf("Soundspotter...");fflush(stdout);
	initializeSoundSpotter(); // allocate buffers
	programs = new JumblatronProgram[numPrograms];
	fFeedBack = fOut = 0;
	if (programs)
		setProgram (0);
	setNumInputs (1);	// mono input (although database can be stereo)
	setNumOutputs (2);	// stereo output
	setUniqueID (*(int*)"Sspt");	// this should be unique, use the Steinberg web page for plugin Id registration
	soundFile = new SoundFile();
#ifdef __linux
	if (canHostDo ((char*)"openFileSelector")){
	  if(openSourceFile()){
	    printf("Opening %s\n", sourceAudioPath);
	    extract(sourceAudioPath);
	  }
	}
#else
	editor = new JumblatronEditor (this);
	canDo ("offline"); // Enable VST offline process
#endif
	resume ();		// flush buffers
}

//------------------------------------------------------------------------
Jumblatron::~Jumblatron ()
{
	if (bufferIn)
		delete[] bufferIn;
	if (bufferOut)
		delete[] bufferOut;
	if (bufferFeedback)
		delete[] bufferFeedback;
	if (audioDatabase)
		delete[] audioDatabase;
	if (featuresIn)
		delete[] featuresIn;
	if (featuresOut)
		delete[] featuresOut;
	if (soundFile)
		delete soundFile;
	if (soundSpotter)
		delete soundSpotter;
	if (programs)
		delete[] programs;
}

char* Jumblatron::openSourceFile()
{
	VstFileType aiffType ("AIFF File", "AIFF", "aif", "aiff", "audio/aiff", "audio/x-aiff");
	VstFileType aifcType ("AIFC File", "AIFC", "aif", "aifc", "audio/x-aifc");
	VstFileType waveType ("Wave File", "WAVE", "wav", "wav",  "audio/wav", "audio/x-wav");
	VstFileType sdIIType ("SoundDesigner II File", "Sd2f", "sd2", "sd2");
	VstFileType mpg3Type ("MP3 File", "MP3", "mp3", "MPEG3", "mpeg3");
	VstFileType types[] = {waveType, aiffType, aifcType, sdIIType, mpg3Type};

	VstFileSelect vstFileSelect;
	memset (&vstFileSelect, 0, sizeof (VstFileSelect));

	vstFileSelect.command     = kVstFileLoad;
	vstFileSelect.type        = kVstFileType;
	strcpy (vstFileSelect.title, "Open source audio file");
	vstFileSelect.nbFileTypes = 5;
	vstFileSelect.fileTypes   = (VstFileType*)&types;
	vstFileSelect.returnPath  = sourceAudioPath; 
	vstFileSelect.initialPath = 0;
	vstFileSelect.future[0] = 1;	// utf-8 path on macosx
#ifndef __linux
	int retVal = openFileSelector (&vstFileSelect);
	if (retVal)
	{
	  strncpy(sourceAudioPath,vstFileSelect.returnPath,1024);
	  closeFileSelector(&vstFileSelect);
	  printf("SUCCESS::open #%0Xd,(#%0Xd),", (unsigned int)&vstFileSelect.returnPath,(unsigned int)&sourceAudioPath);fflush(stdout);
	  return vstFileSelect.returnPath;
	}
	printf("FAILED::open #%0Xd[%s],(#%0Xd)[%s],", (unsigned int)&vstFileSelect.returnPath,vstFileSelect.returnPath,(unsigned int)&sourceAudioPath,sourceAudioPath);fflush(stdout);
	return NULL;
#else
	strncpy(sourceAudioPath, "/home/mkc/.renoise/soundspotter_db.wav",1024);
	return sourceAudioPath;
#endif
}


//------------------------------------------------------------------------
// Probably don't need this, but just in case it's here
VstInt32 Jumblatron::canDo (char* text)
{
	if (!strcmp (text, "wantsUTF8Paths"))
		return 1;
	return 0;
}

//------------------------------------------------------------------------
void Jumblatron::initializeSoundSpotter(){
	soundSpotter = new SoundSpotter(44100, size, SSNUMCHANNELS); //soundFile->getNumChannels()
	bufferIn = new float[size];  // MONO INPUT
	bufferOut = new float[size*SSNUMCHANNELS]; //soundFile->getNumChannels()]; // MULTI-CHANNEL OUTPUT
	bufferFeedback = new float[size];  // MONO FEEDBACK
	featuresIn = new float[size]; // Auxillary vectors for FeautreExtractor
	featuresOut = new float[size];
	audioDatabase = 0; // new float[SSAUDIODBLEN];
	soundSpotter->setStatus(THRU);
	soundSpotter->setMinASB(5);
	soundSpotter->setBasisWidth(40);
	soundSpotter->setQueueSize(10);
	soundSpotter->setEnvFollow(0.5);
	soundSpotter->setShingleSize(4);
	soundSpotter->setLoDataLoc(0);
	soundSpotter->setHiDataLoc(0);
	printf("soundspotter initialized\n");
	fflush(stdout);
}

//------------------------------------------------------------------------
void Jumblatron::extract(char* fileName){
	printf("Jumblatron::extract %s,", fileName);fflush(stdout);
	int retVal = soundFile->sfOpen(fileName);
	if(retVal){
		soundSpotter->setAudioDatabaseBuf( soundFile->getSoundBuf(), soundFile->getBufLen(), soundFile->getNumChannels() );
		soundSpotter->setStatus(EXTRACT);
		resume ();		// flush buffers
	}
	else{
		printf("EXRACT failed, sfOpen cannot open soundFile: %s\n", fileName); 
	}
	printf("done.\n");fflush(stdout);
}

//------------------------------------------------------------------------
void Jumblatron::setProgram (VstInt32 program)
{
	JumblatronProgram* ap = &programs[program];
	curProgram = program;
	setParameter (kOut, ap->fOut);
	setParameter (kFeedBack, ap->fFeedBack);
	setParameter (kShingleSize, ap->fShingleSize);
	setParameter (kQueueSize, ap->fQueueSize);
	setParameter (kLoBasis, ap->fLoBasis);
	setParameter (kNumBasis, ap->fNumBasis);
	setParameter (kEnvFollow, ap->fEnvFollow);
	setParameter (kMatchRadius, ap->fMatchRadius);
	setParameter (kLoSec, ap->fLoSec);
	setParameter (kHiSec, ap->fHiSec);
	setParameter (kMix, ap->fMix);
}


//------------------------------------------------------------------------
void Jumblatron::setShingleSize (float fshinglesize)
{
	fShingleSize = fshinglesize;
	programs[curProgram].fShingleSize = fShingleSize;
	soundSpotter->setShingleSize((int)(fShingleSize * SSMAXSHINGLESZ));
}

//------------------------------------------------------------------------
void Jumblatron::setQueueSize (float fqueuesize)
{
	fQueueSize = fqueuesize;
	programs[curProgram].fQueueSize = fQueueSize;
	soundSpotter->setQueueSize((int)(fQueueSize * SSMAXQUEUESZ));
}

//------------------------------------------------------------------------
void Jumblatron::setLoBasis (float flobasis)
{
	fLoBasis = flobasis;
	programs[curProgram].fLoBasis = fLoBasis;
	soundSpotter->setMinASB((int)(fLoBasis * soundSpotter->getNASB()));
}

//------------------------------------------------------------------------
void Jumblatron::setNumBasis (float fnumbasis)
{
	fNumBasis = fnumbasis;
	programs[curProgram].fNumBasis = fNumBasis;
	soundSpotter->setBasisWidth((int)(fNumBasis * soundSpotter->getNASB()));
}

//------------------------------------------------------------------------
void Jumblatron::setEnvFollow (float fenvfollow)
{
	fEnvFollow = fenvfollow;
	programs[curProgram].fEnvFollow = fEnvFollow;
	soundSpotter->setEnvFollow(fEnvFollow);
}

//------------------------------------------------------------------------
void Jumblatron::setMatchRadius (float fmatchradius)
{
	fMatchRadius = fmatchradius;
	programs[curProgram].fMatchRadius = fMatchRadius;
	soundSpotter->setMatchRadius(fMatchRadius * SSMAXRADIUS);
}


void Jumblatron::setLoSec(float flosec)
{
	fLoSec = flosec;
	programs[curProgram].fLoSec = fLoSec;
	soundSpotter->setLoDataLoc(fLoSec * soundSpotter->getAudioDatabaseFrames()/44100.0f);
}

void Jumblatron::setHiSec(float fhisec)
{
	fHiSec = fhisec;
	programs[curProgram].fHiSec = fHiSec;
	soundSpotter->setHiDataLoc(fHiSec * soundSpotter->getAudioDatabaseFrames()/44100.0f);
}

//------------------------------------------------------------------------
void Jumblatron::setProgramName (char *name)
{
	strcpy (programs[curProgram].name, name);
}

//------------------------------------------------------------------------
void Jumblatron::getProgramName (char *name)
{
	if (!strcmp (programs[curProgram].name, "Init"))
		sprintf (name, "%s %d", programs[curProgram].name, curProgram + 1);
	else
		strcpy (name, programs[curProgram].name);
}

//-----------------------------------------------------------------------------------------
bool Jumblatron::getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text)
{
	if (index < kNumPrograms)
	{
		strcpy (text, programs[index].name);
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
void Jumblatron::resume ()
{
	memset (bufferIn, 0, size * sizeof (float));
	memset (bufferOut, 0, size * sizeof (float) * SSNUMCHANNELS);
	memset (featuresIn, 0, size * sizeof (float));
	memset (featuresOut, 0, size * sizeof (float));
	memset (bufferFeedback, 0, size * sizeof (float));	
	cursor = 0;
	AudioEffectX::resume ();
}

//------------------------------------------------------------------------
void Jumblatron::setParameter (VstInt32 index, float value)
{
	JumblatronProgram* ap = &programs[curProgram];
	switch (index)
	{
		case kOut :      fOut = ap->fOut = value;			break;
		case kFeedBack : fFeedBack = ap->fFeedBack = value; break;
		case kShingleSize : setShingleSize(value);          break;
		case kQueueSize : setQueueSize(value);				break;
		case kLoBasis : setLoBasis(value);					break;
		case kNumBasis : setNumBasis(value);				break;
		case kEnvFollow : setEnvFollow(value);				break;
		case kMatchRadius : setMatchRadius(value);			break;
		case kLoSec : setLoSec(value);						break;
		case kHiSec : setHiSec(value);						break;
		case kMix : fMix = ap->fMix = value;				break;
	}
}

//------------------------------------------------------------------------
float Jumblatron::getParameter (VstInt32 index)
{
	float v = 0;
	switch (index)
	{
	case kOut :      v = fOut;		break;
	case kFeedBack : v = fFeedBack; break;
	case kShingleSize : v = fShingleSize; break;
	case kQueueSize : v = fQueueSize; break;
	case kLoBasis : v = fLoBasis; break;
	case kNumBasis : v = fNumBasis; break;
	case kEnvFollow : v = fEnvFollow; break;
	case kMatchRadius : v = fMatchRadius; break;
	case kLoSec : v = fLoSec; break;
	case kHiSec : v = fHiSec; break;
	case kMix : v = fMix; break;
	}
	return v;
}

//------------------------------------------------------------------------
void Jumblatron::getParameterName (VstInt32 index, char *label)
{
	switch (index)
	{
	case kOut :      strcpy (label, "Volume");		break;
	case kFeedBack : strcpy (label, "FeedBack");	break;
	case kShingleSize : strcpy (label, "Shingle"); break;
	case kQueueSize : strcpy (label, "Queue"); break; 
	case kLoBasis : strcpy (label, "LoFeat"); break;
	case kNumBasis : strcpy (label, "NumFeat"); break;
	case kEnvFollow : strcpy (label, "EnvFollw"); break;
	case kMatchRadius : strcpy (label, "Radius"); break;
	case kLoSec : strcpy (label, "LoSec"); break;
	case kHiSec : strcpy (label, "HiSec"); break;
	case kMix : strcpy (label, "Wet/Dry"); break;
	}
}

//------------------------------------------------------------------------
void Jumblatron::getParameterDisplay (VstInt32 index, char *text)
{
	switch (index)
	{
	case kOut : dB2string (fOut, text, kVstMaxParamStrLen);			break;
	case kFeedBack : float2string (fFeedBack, text, kVstMaxParamStrLen);	break;
	case kShingleSize : int2string ((int)(fShingleSize*SSMAXSHINGLESZ), text, kVstMaxParamStrLen);	break;
	case kQueueSize : int2string ((int)(fQueueSize*SSMAXQUEUESZ), text, kVstMaxParamStrLen);	break;
	case kLoBasis : int2string ((int)(fLoBasis*SSNUMBASIS), text, kVstMaxParamStrLen);	break;
	case kNumBasis : int2string ((int)(fNumBasis*SSNUMBASIS), text, kVstMaxParamStrLen);	break;
	case kEnvFollow : float2string (fEnvFollow, text, kVstMaxParamStrLen);	break;
	case kMatchRadius : float2string (fMatchRadius, text, kVstMaxParamStrLen);	break;
	case kLoSec : float2string (fLoSec * soundSpotter->getAudioDatabaseFrames()/44100.0f , text, kVstMaxParamStrLen);	break;
	case kHiSec : float2string (fHiSec * soundSpotter->getAudioDatabaseFrames()/44100.0f, text, kVstMaxParamStrLen);	break;
	case kMix : float2string (fMix, text, kVstMaxParamStrLen);	break;
	}
}

//------------------------------------------------------------------------
void Jumblatron::getParameterLabel (VstInt32 index, char *label)
{
	switch (index)
	{
	case kOut :      strcpy (label, "dB");		break;
	case kFeedBack : strcpy (label, "amount");	break;
	case kShingleSize : strcpy (label, "shingle"); break;
	case kQueueSize : strcpy (label, "queue"); break;
	case kLoBasis : strcpy (label, "loFeat"); break;
	case kNumBasis : strcpy (label, "numFeat"); break;
	case kEnvFollow : strcpy (label, "envFollw"); break;
	case kMatchRadius : strcpy (label, "radius"); break;
	case kLoSec : strcpy (label, "loSec"); break;
	case kHiSec : strcpy (label, "hiSec"); break;
	case kMix : strcpy (label, "wet/dry"); break;
	}
}

//------------------------------------------------------------------------
bool Jumblatron::getEffectName (char* name)
{
	strcpy (name, "SoundSpotter");
	return true;
}

//------------------------------------------------------------------------
bool Jumblatron::getProductString (char* text)
{
	strcpy (text, "SoundSpotter");
	return true;
}

//------------------------------------------------------------------------
bool Jumblatron::getVendorString (char* text)
{
	strcpy (text, "SoundSpotter Media Technologies");
	return true;
}

void Jumblatron::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* in = inputs[0];
	//float* in2 = inputs[1];
	float* out1 = outputs[0];
	float* out2 = outputs[1];
	float* fb = bufferFeedback;
	int numFrames = sampleFrames;
	int numChannels = (soundSpotter->getStatus()==THRU)?1:soundSpotter->getAudioDatabaseNumChannels();
	while( numFrames-- ){
		bufferIn[cursor] = *in + fFeedBack * *fb++;
		*out1 = fOut * ( (1.0f-fMix) * bufferOut[cursor*numChannels] + fMix * *in ); // volume controlled output
		if(out2){
			if(numChannels==1){
				*out2++ = *out1;
			}
			else{
				*out2++ =  fOut * ( (1.0f-fMix) * bufferOut[cursor*numChannels+1] + fMix * *in );
			}
		}
		in++;
		out1++;
		cursor++;
	}

	//  run soundspotter when application buffer is full
	if(cursor >= size){
		// soundspotter sample replacement method
		// numChannels is for bufferOut which is multi-channel interleaved
		// beware, bufferOut must have sz bufLen*WindowLength*numChannels
		soundSpotter->run(size, featuresIn, bufferIn, featuresOut, bufferOut);
		cursor = 0;
	}	

	// Mono feedback loop with 1 cycle delay
	out1 = outputs[0];
	fb = bufferFeedback;
	numFrames = sampleFrames;
	while(numFrames--)
		*fb++ = *out1++;

}

#ifndef __linux


JumblatronEditor::JumblatronEditor (AudioEffect* effect)
: AEffGUIEditor (effect)
{
	jumblatronEffect = (Jumblatron*)effect;
}

JumblatronEditor::~JumblatronEditor ()
{
}

bool JumblatronEditor::open (void *ptr)
{
	AEffGUIEditor::open (ptr);

	VstFileType aiffType ("AIFF File", "AIFF", "aif", "aiff", "audio/aiff", "audio/x-aiff");
	VstFileType aifcType ("AIFC File", "AIFC", "aif", "aifc", "audio/x-aifc");
	VstFileType waveType ("Wave File", "WAVE", "wav", "wav",  "audio/wav", "audio/x-wav");
	VstFileType sdIIType ("SoundDesigner II File", "Sd2f", "sd2", "sd2");
	VstFileType mpg3Type ("MP3 File", "MP3", "mp3", "MPEG3", "mpeg3");
	VstFileType types[] = {waveType, aiffType, aifcType, sdIIType, mpg3Type};

	VstFileSelect vstFileSelect;
	memset (&vstFileSelect, 0, sizeof (VstFileSelect));

	vstFileSelect.command     = kVstFileLoad;
	vstFileSelect.type        = kVstFileType;
	strcpy (vstFileSelect.title, "Open source audio file");
	vstFileSelect.nbFileTypes = 5;
	vstFileSelect.fileTypes   = (VstFileType*)&types;
	vstFileSelect.returnPath  = sourceAudioPath;
	vstFileSelect.initialPath = 0;
	vstFileSelect.future[0] = 1;	// utf-8 path on macosx
	CFileSelector selector (NULL);
	if (selector.run (&vstFileSelect))
	{
		jumblatronEffect->extract(vstFileSelect.returnPath);
	}
	else
	{
	}
	close();
	return true;
}

void JumblatronEditor::close ()
{
	AEffGUIEditor::close ();
}

void JumblatronEditor::idle ()
{
	AEffGUIEditor::idle ();
}


#endif

