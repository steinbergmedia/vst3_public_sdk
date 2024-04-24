//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/remap_paramid/source/remapparamidcontroller.cpp
// Created by  : Steinberg, 02/2024
// Description : Remap ParamID Example for VST 3 (by replacing AGain)
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2024, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this
//     software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "remapparamidcontroller.h"
#include "remapparamidcids.h"
#include "base/source/fstreamer.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// TestRemapParamIDController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API TestRemapParamIDController::initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated

	//---do not forget to call parent ------
	tresult result = EditController::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	//---this parameter is the one which is compatible with the kGainId parameter of AGain
	parameters.addParameter (STR16 ("compatible Gain"), nullptr, 0, 0.5,
	                         ParameterInfo::kCanAutomate, kMyGainParamTag);

	//---Bypass parameter---
	parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
	                         ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, kBypassId);

	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API TestRemapParamIDController::setComponentState (IBStream* state)
{
	// Here you get the state of the component (Processor part)
	if (!state)
		return kResultFalse;

	//---Compatible with AGain state -----------
	IBStreamer streamer (state, kLittleEndian);
	float savedGain = 0.f;
	if (streamer.readFloat (savedGain) == false)
		return kResultFalse;
	setParamNormalized (kMyGainParamTag, savedGain);

	// jump the GainReduction
	streamer.seek (sizeof (float), kSeekCurrent);

	int32 bypassState = 0;
	if (streamer.readInt32 (bypassState) == false)
		return kResultFalse;
	setParamNormalized (kBypassId, bypassState ? 1 : 0);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API TestRemapParamIDController::getCompatibleParamID (const TUID pluginToReplaceUID,
                                                                     Vst::ParamID oldParamID,
                                                                     Vst::ParamID& newParamID)
{
	//--- We want to replace the AGain plug-in-------
	//--- check if the host is asking for remapping parameter of the AGain plug-in
	static const FUID AGainProcessorUID (0x84E8DE5F, 0x92554F53, 0x96FAE413, 0x3C935A18);
	FUID uidToCheck (FUID::fromTUID (pluginToReplaceUID));
	if (AGainProcessorUID != uidToCheck)
		return kResultFalse;

	//--- host wants to remap from AGain------------
	newParamID = -1;
	switch (oldParamID)
	{
		//--- map the kGainId (0) to our param kMyGainParamTag
		case 0:
		{
			newParamID = kMyGainParamTag;
			break;
		}
	}
	//--- return kResultTrue if the mapping happens------------
	return (newParamID == -1) ? kResultFalse : kResultTrue;
}
//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
