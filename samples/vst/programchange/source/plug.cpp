//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/programchange/source/plug.cpp
// Created by  : Steinberg, 02/2016
// Description : Plug-in Example for VST SDK 3.x using ProgramChange parameter
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

#include "plug.h"
#include "plugparamids.h"
#include "plugcids.h" // for class ids

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/base/futils.h"

#include "base/source/fstreamer.h"

#include <cstdio>

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// Plug Implementation
//------------------------------------------------------------------------
Plug::Plug () : bBypass (false), currentProgram (0), currentGainValue (0.f)
{
	// register its editor class (the same than used in plugentry.cpp)
	setControllerClass (PlugControllerUID);
}

//------------------------------------------------------------------------
tresult PLUGIN_API Plug::initialize (FUnknown* context)
{
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	// if everything Ok, continue
	if (result != kResultOk)
	{
		return result;
	}

	//---create Audio In/Out busses------
	// we want a stereo Input and a Stereo Output
	addAudioInput (STR16 ("Stereo In"), SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), SpeakerArr::kStereo);

	addEventInput (STR16 ("Event In"), 1);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Plug::process (ProcessData& data)
{
	//---1) Read inputs parameter changes-----------
	if (IParameterChanges* paramChanges = data.inputParameterChanges)
	{
		int32 numParamsChanged = paramChanges->getParameterCount ();
		// for each parameter which are some changes in this audio block:
		for (int32 i = 0; i < numParamsChanged; i++)
		{
			if (IParamValueQueue* paramQueue = paramChanges->getParameterData (i))
			{
				int32 offsetSamples;
				double value;
				int32 numPoints = paramQueue->getPointCount ();
				switch (paramQueue->getParameterId ())
				{
					case kBypassId:
						if (paramQueue->getPoint (numPoints - 1, offsetSamples, value) ==
						    kResultTrue)
						{
							bBypass = (value > 0.5f);
						}
						break;

					case kProgramId:
						if (paramQueue->getPoint (numPoints - 1, offsetSamples, value) ==
						    kResultTrue)
						{
							// here we get the last set program
							currentProgram = FromNormalized<ParamValue> (value, kNumProgs - 1);
						}
						break;

					case kGainId:
						if (paramQueue->getPoint (numPoints - 1, offsetSamples, value) ==
						    kResultTrue)
						{
							currentGainValue = static_cast<float> (value);
						}
						break;
				}
			}
		}
	}

	//--- ----------------------------------
	//---3) Process Audio---------------------
	//--- ----------------------------------
	if (data.numInputs == 0 || data.numOutputs == 0)
	{
		// nothing to do
		return kResultOk;
	}

	// (simplification) we suppose in this example that we have the same input channel count than
	// the output
	int32 numChannels = data.inputs[0].numChannels;

	//---get audio buffers----------------
	float** in = data.inputs[0].channelBuffers32;
	float** out = data.outputs[0].channelBuffers32;

	// check if all channel are silent then process silent
	if (data.inputs[0].silenceFlags == getChannelMask (data.inputs[0].numChannels))
	{
		// mark output silence too
		data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

		int32 sampleFrames = data.numSamples;

		// the plug-in has to be sure that if it sets the flags silence that the output buffer are
		// clear
		for (int32 i = 0; i < numChannels; i++)
		{
			// do not need to be cleared if the buffers are the same (in this case input buffer are
			// already cleared by the host)
			if (in[i] != out[i])
			{
				memset (out[i], 0, sampleFrames * sizeof (float));
			}
		}

		// nothing to do at this point
		return kResultOk;
	}

	// mark our outputs has not silent
	data.outputs[0].silenceFlags = 0;

	//---in bypass mode outputs should be like inputs-----
	if (bBypass)
	{
		int32 sampleFrames = data.numSamples;
		for (int32 i = 0; i < numChannels; i++)
		{
			// do not need to be copied if the buffers are the same
			if (in[i] != out[i])
			{
				memcpy (out[i], in[i], sampleFrames * sizeof (float));
			}
		}
	}
	else
	{
		// here we use the current program as gain value...
		float gain = currentGainValue;

		// in real plug-in it would be better to do dezippering to avoid jump (click) in gain value
		for (int32 i = 0; i < numChannels; i++)
		{
			int32 sampleFrames = data.numSamples;
			float* ptrIn = in[i];
			float* ptrOut = out[i];
			float tmp;
			while (--sampleFrames >= 0)
			{
				// apply gain
				tmp = (*ptrIn++) * gain;
				(*ptrOut++) = tmp;
			}
		}
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Plug::setState (IBStream* state)
{
	// called when we load a preset, the model has to be reloaded

	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	// read the bypass
	int32 savedBypass = 0;
	if (streamer.readInt32 (savedBypass) == false)
		return kResultFalse;

	// read the program
	int32 savedProgram = 0;
	if (streamer.readInt32 (savedProgram) == false)
		return kResultFalse;

	// read the Gain param
	float savedGain = 0.f;
	if (streamer.readFloat (savedGain) == false)
		return kResultFalse;

	bBypass = savedBypass > 0;
	currentProgram = savedProgram;
	currentGainValue = savedGain;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Plug::getState (IBStream* state)
{
	// here we need to save the model

	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	streamer.writeInt32 (bBypass ? 1 : 0);
	streamer.writeInt32 (currentProgram);
	streamer.writeFloat (currentGainValue);

	return kResultOk;
}

}} // namespaces
