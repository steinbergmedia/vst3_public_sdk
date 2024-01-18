//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Examples
// Filename    : public.sdk/samples/vst/dataexchange/shared.h
// Created by  : Steinberg, 06/2023
// Description : VST Data Exchange API Example
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
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstdataexchange.h"
#include "pluginterfaces/vst/vsttypes.h"

#include <cassert>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
static DECLARE_UID (DataExchangeProcessorUID, 0x2AF3DF1C, 0x93D243B3, 0xBA13E61C, 0xDCFDAC5D);
static DECLARE_UID (DataExchangeControllerUID, 0xB49E781B, 0xED8F486F, 0x85D4306E, 0x2C6207A0);

//------------------------------------------------------------------------
FUnknown* createDataExchangeProcessor (void*);
FUnknown* createDataExchangeController (void*);

//------------------------------------------------------------------------
static SMTG_CONSTEXPR ParamID ParamIDEnableDataExchange = 1;

//------------------------------------------------------------------------
static SMTG_CONSTEXPR auto MessageIDForceMessageHandling = "ForceMessageHandling";
static SMTG_CONSTEXPR auto MessageKeyValue = "Value";

//------------------------------------------------------------------------
static SMTG_CONSTEXPR DataExchangeUserContextID SampleBufferQueueID = 2;

//------------------------------------------------------------------------
struct SampleBufferExchangeData
{
	int64 systemTime;
	SampleRate sampleRate;
	uint32 numChannels;
	uint32 numSamples;
	float sampleData[1]; // variable size
};

//------------------------------------------------------------------------
inline SampleBufferExchangeData& getSampleBufferExchangeData (const DataExchangeBlock& block)
{
	return *reinterpret_cast<SampleBufferExchangeData*> (block.data);
}

//------------------------------------------------------------------------
inline uint32 calculateExampleDataSizeForSamples (uint32 numSamples, uint32 numChannels)
{
	return sizeof (SampleBufferExchangeData) + ((sizeof (float) * numSamples) * numChannels);
}

//------------------------------------------------------------------------
inline uint32 sampleDataOffsetForChannel (uint32 channel, uint32 numSamplesInBuffer)
{
	return numSamplesInBuffer * channel;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
