//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/11/13 09:08:27 $
//
// Category     : VST 2.x SDK Samples
// Filename     : soundspotterVSTmain.cpp
// Created by   : Michael A. Casey, Bregman Media Labs
// Description  : Simple Delay plugin (Mono->Stereo)
//
//-------------------------------------------------------------------------------------------------------

#ifndef __soundspotterVST__
#include "soundspotterVST.h"
#endif

//-------------------------------------------------------------------------------------------------------
AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return new SoundspotterVST (audioMaster);
}

