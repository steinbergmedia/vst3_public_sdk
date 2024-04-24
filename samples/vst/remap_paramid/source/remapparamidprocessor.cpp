//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/remap_paramid/source/remapparamidprocessor.cpp
// Created by  : Steinberg, 02/2024
// Description : Remap ParamID Example for VST 3
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

#include "remapparamidprocessor.h"
#include "remapparamidcids.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// TestRemapParamIDProcessor
//------------------------------------------------------------------------
TestRemapParamIDProcessor::TestRemapParamIDProcessor ()
{
	//--- set the wanted controller for our processor
	setControllerClass (kTestRemapParamIDControllerUID);
}

//------------------------------------------------------------------------
TestRemapParamIDProcessor::~TestRemapParamIDProcessor ()
{
}

//------------------------------------------------------------------------
tresult PLUGIN_API TestRemapParamIDProcessor::initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated

	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	// if everything Ok, continue
	if (result != kResultOk)
	{
		return result;
	}

	//--- create Audio IO ------
	addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API TestRemapParamIDProcessor::process (Vst::ProcessData& data)
{
	//--- First : Read inputs parameter changes-----------
	if (data.inputParameterChanges)
	{
		int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
		for (int32 index = 0; index < numParamsChanged; index++)
		{
			if (auto* paramQueue = data.inputParameterChanges->getParameterData (index))
			{
				Vst::ParamValue value;
				int32 sampleOffset;
				int32 numPoints = paramQueue->getPointCount ();
				switch (paramQueue->getParameterId ())
				{
					case kMyGainParamTag:
						if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
						    kResultTrue)
						{
							fGain = (float)value;
						}
						break;

					case kBypassId:
						if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
						    kResultTrue)
						{
							bBypass = (value > 0.5f);
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
	uint32 sampleFramesSize = getSampleFramesSizeInBytes (processSetup, data.numSamples);
	void** in = getChannelBuffersPointer (processSetup, data.inputs[0]);
	void** out = getChannelBuffersPointer (processSetup, data.outputs[0]);

	//---check if silence---------------
	// check if all channel are silent then process silent
	if (data.inputs[0].silenceFlags == getChannelMask (data.inputs[0].numChannels))
	{
		// mark output silence too (it will help the host to propagate the silence)
		data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

		// the plug-in has to be sure that if it sets the flags silence that the output buffer are
		// clear
		for (int32 i = 0; i < numChannels; i++)
		{
			// do not need to be cleared if the buffers are the same (in this case input buffer are
			// already cleared by the host)
			if (in[i] != out[i])
			{
				memset (out[i], 0, sampleFramesSize);
			}
		}
	}
	else // we have to process (no silence)
	{
		// mark our outputs has not silent
		data.outputs[0].silenceFlags = 0;

		//---in bypass mode outputs should be like inputs-----
		if (bBypass)
		{
			for (int32 i = 0; i < numChannels; i++)
			{
				// do not need to be copied if the buffers are the same
				if (in[i] != out[i])
				{
					memcpy (out[i], in[i], sampleFramesSize);
				}
			}
		}
		else
		{
			//---apply gain factor----------
			// if the applied gain is nearly zero, we could say that the outputs are zeroed and we
			// set the silence flags.
			if (fGain < 0.0000001)
			{
				for (int32 i = 0; i < numChannels; i++)
				{
					memset (out[i], 0, sampleFramesSize);
				}
				// this will set to 1 all channels
				data.outputs[0].silenceFlags = getChannelMask (data.outputs[0].numChannels);
			}
			else
			{
				if (data.symbolicSampleSize == Vst::kSample32)
					processAudio<Sample32> ((Sample32**)in, (Sample32**)out, numChannels,
					                        data.numSamples, fGain);
				else
					processAudio<Sample64> ((Sample64**)in, (Sample64**)out, numChannels,
					                        data.numSamples, fGain);
			}
		}
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API TestRemapParamIDProcessor::setBusArrangements (SpeakerArrangement* inputs,
                                                                  int32 numIns,
                                                                  SpeakerArrangement* outputs,
                                                                  int32 numOuts)
{
	//--- support only Stereo => Stereo -----------------
	if (numIns == 1 && numOuts == 1)
	{
		if (SpeakerArr::getChannelCount (inputs[0]) == 2 &&
		    SpeakerArr::getChannelCount (outputs[0]) == 2)
		{
			return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
		}
	}
	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API TestRemapParamIDProcessor::canProcessSampleSize (int32 symbolicSampleSize)
{
	if (symbolicSampleSize == Vst::kSample32 || symbolicSampleSize == Vst::kSample64)
		return kResultTrue;

	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API TestRemapParamIDProcessor::setState (IBStream* state)
{
	// called when we load a preset, the model has to be reloaded

	// Compatible with AGain state

	IBStreamer streamer (state, kLittleEndian);
	float savedGain = 0.f;
	if (streamer.readFloat (savedGain) == false)
		return kResultFalse;

	// ignore this one
	float tmp = 0.f;
	if (streamer.readFloat (tmp) == false)
		return kResultFalse;

	int32 savedBypass = 0;
	if (streamer.readInt32 (savedBypass) == false)
		return kResultFalse;

	fGain = savedGain;
	bBypass = savedBypass > 0;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API TestRemapParamIDProcessor::getState (IBStream* state)
{
	// here we need to save the model

	// Compatible with AGain state
	IBStreamer streamer (state, kLittleEndian);

	streamer.writeFloat (fGain);
	streamer.writeFloat (0.f); // unuse here
	streamer.writeInt32 (bBypass ? 1 : 0);

	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
