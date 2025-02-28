//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/hostchecker/source/parameterchangescheck.h
// Created by  : Steinberg, 12/2012
// Description : ParameterChanges check
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

#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstnoteexpression.h"
#include <set>
#include <vector>

class EventLogger;

namespace Steinberg {
namespace Vst {
class IParamValueQueue;
}
}

//------------------------------------------------------------------------
//	EventListCheck
//------------------------------------------------------------------------
class ParameterChangesCheck
{
public:
//------------------------------------------------------------------------
	ParameterChangesCheck ();

	using ParamIDs = std::set<Steinberg::Vst::ParamID>;

	void checkParameterChanges (Steinberg::Vst::IParameterChanges* paramChanges);
	void setEventLogger (EventLogger* eventLogger);
	void setParamIDs (ParamIDs* parameterID);
	void updateParameterIDs ();
//------------------------------------------------------------------------
protected:
	void checkAllChanges (Steinberg::Vst::IParameterChanges* paramChanges);
	void checkParameterCount (Steinberg::int32 paramCount);
	void checkParameterId (Steinberg::Vst::ParamID paramId);
	void checkNormalized (double normVal);
	void checkSampleOffset (Steinberg::int32 sampleOffset, Steinberg::int32 lastSampleOffset);
	bool checkParameterQueue (Steinberg::Vst::IParamValueQueue* paramQueue);
	void checkPoints (Steinberg::Vst::IParamValueQueue* paramQueue);

	bool isNormalized (double normVal) const;
	bool isValidSampleOffset (Steinberg::int32 sampleOffset, Steinberg::int32 lastSampleOffset) const;
	bool isValidParamID (Steinberg::Vst::ParamID paramId) const;
	bool isValidParamCount (Steinberg::int32 paramCount) const;

	EventLogger* mEventLogger;
	ParamIDs* mParameterIds;
	std::vector<Steinberg::Vst::ParamID> mTempUsedId;
};
