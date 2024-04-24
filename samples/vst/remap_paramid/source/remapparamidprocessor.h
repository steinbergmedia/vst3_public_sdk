//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/remap_paramid/source/remapparamidprocessor.h
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

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
//  TestRemapParamIDProcessor
//------------------------------------------------------------------------
class TestRemapParamIDProcessor : public AudioEffect
{
public:
	TestRemapParamIDProcessor ();
	~TestRemapParamIDProcessor () SMTG_OVERRIDE;

	// Create function
	static FUnknown* createInstance (void* /*context*/)
	{
		return (Vst::IAudioProcessor*)new TestRemapParamIDProcessor;
	}

	//--- ---------------------------------------------------------------------
	// AudioEffect overrides:
	//--- ---------------------------------------------------------------------
	/** Called at first after constructor */
	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;

	/** Bus arrangement managing: in this example we support only Stereo/Stereo */
	tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs, int32 numIns,
	                                       SpeakerArrangement* outputs,
	                                       int32 numOuts) SMTG_OVERRIDE;

	/** Asks if a given sample size is supported see SymbolicSampleSizes. */
	tresult PLUGIN_API canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE;

	/** Here we go...the process call */
	tresult PLUGIN_API process (Vst::ProcessData& data) SMTG_OVERRIDE;

	/** For persistence */
	tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE;

//------------------------------------------------------------------------
protected:
	//--- ---------------------------------------------------------------------
	template <typename SampleType>
	SampleType processAudio (SampleType** in, SampleType** out, int32 numChannels,
	                         int32 sampleFrames, float gain)
	{
		SampleType vuPPM = 0;

		// in real Plug-in it would be better to do dezippering to avoid jump (click) in gain value
		for (int32 i = 0; i < numChannels; i++)
		{
			int32 samples = sampleFrames;
			auto* ptrIn = (SampleType*)in[i];
			auto* ptrOut = (SampleType*)out[i];
			SampleType tmp;
			while (--samples >= 0)
			{
				// apply gain
				tmp = (*ptrIn++) * gain;
				(*ptrOut++) = tmp;

				// check only positive values
				if (tmp > vuPPM)
				{
					vuPPM = tmp;
				}
			}
		}
		return vuPPM;
	}

	float fGain {1.f};
	bool bBypass {false};
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
