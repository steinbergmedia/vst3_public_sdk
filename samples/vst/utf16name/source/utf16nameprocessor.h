//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/utf16name/source/utf16nameprocessor.h
// Created by  : Steinberg, 11/2023
// Description : UTF16Name Example for VST 3
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

//------------------------------------------------------------------------
//  UTF16NameProcessor
//------------------------------------------------------------------------
class UTF16NameProcessor : public Vst::AudioEffect
{
public:
	UTF16NameProcessor ();
	~UTF16NameProcessor () SMTG_OVERRIDE;

	// Create function
	static FUnknown* createInstance (void* /*context*/)
	{
		return (Vst::IAudioProcessor*)new UTF16NameProcessor;
	}

	//--- ---------------------------------------------------------------------
	// AudioEffect overrides:
	//--- ---------------------------------------------------------------------
	/** Called at first after constructor */
	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;

	/** Called at the end before destructor */
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;

	/** Switch the Plug-in on/off */
	tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;

	/** Will be called before any process call */
	tresult PLUGIN_API setupProcessing (Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;

	/** Asks if a given sample size is supported see SymbolicSampleSizes. */
	tresult PLUGIN_API canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE;

	/** Here we go...the process call */
	tresult PLUGIN_API process (Vst::ProcessData& data) SMTG_OVERRIDE;

	/** For persistence */
	tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE;

//------------------------------------------------------------------------
protected:
};

//------------------------------------------------------------------------
} // namespace Steinberg
