//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/hostchecker/source/parameterchangescheck.cpp
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

#include "parameterchangescheck.h"
#include "eventlogger.h"
#include "logevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

//------------------------------------------------------------------------
//    ParameterChangesCheck
//------------------------------------------------------------------------
ParameterChangesCheck::ParameterChangesCheck () : mEventLogger (nullptr), mParameterIds (nullptr) {}

//------------------------------------------------------------------------
void ParameterChangesCheck::checkParameterChanges (Steinberg::Vst::IParameterChanges* paramChanges)
{
	if (!paramChanges)
	{
		mEventLogger->addLogEvent (kLogIdParameterChangesPointerIsNull);
		return;
	}

	checkParameterCount (paramChanges->getParameterCount ());
	checkAllChanges (paramChanges);
}

//------------------------------------------------------------------------
void ParameterChangesCheck::checkAllChanges (Steinberg::Vst::IParameterChanges* paramChanges)
{
	for (Steinberg::int32 paramIdx = 0; paramIdx < paramChanges->getParameterCount (); ++paramIdx)
	{
		Steinberg::Vst::IParamValueQueue* paramQueue = paramChanges->getParameterData (paramIdx);
		if (checkParameterQueue (paramQueue))
		{
			bool found = false;
			auto id = paramQueue->getParameterId ();
			for (auto item : mTempUsedId)
			{
				if (item == id)
				{
					mEventLogger->addLogEvent (kLogIdParameterIDMoreThanOneTimeinList);
					found = true;
					break;
				}
			}
			if (!found)
				mTempUsedId.emplace_back (id);

			checkParameterId (id);
			checkPoints (paramQueue);
		}
	}
	mTempUsedId.clear ();
}

//------------------------------------------------------------------------
void ParameterChangesCheck::checkPoints (Steinberg::Vst::IParamValueQueue* paramQueue)
{
	Steinberg::int32 lastLastSampleOffset = -1;
	Steinberg::int32 lastSampleOffset = -1;
	for (Steinberg::int32 pointIdx = 0; pointIdx < paramQueue->getPointCount (); ++pointIdx)
	{
		Steinberg::int32 sampleOffset = 0;
		Steinberg::Vst::ParamValue paramValue = 0;
		if (paramQueue->getPoint (pointIdx, sampleOffset, paramValue) == Steinberg::kResultOk)
		{
			checkNormalized (paramValue);
			checkSampleOffset (sampleOffset, lastSampleOffset);
			lastLastSampleOffset = lastSampleOffset;
			lastSampleOffset = sampleOffset;
			// here we have more than 3 points at the same sample position
			if (lastLastSampleOffset == sampleOffset)
				mEventLogger->addLogEvent (kLogIdParametersHaveSameSampleOffset);
		}
	}
}

//------------------------------------------------------------------------
void ParameterChangesCheck::setEventLogger (EventLogger* eventLogger)
{
	mEventLogger = eventLogger;
}

//------------------------------------------------------------------------
void ParameterChangesCheck::setParamIDs (ParamIDs* parameterID)
{
	mParameterIds = parameterID;
	updateParameterIDs ();
}

//------------------------------------------------------------------------
void ParameterChangesCheck::updateParameterIDs ()
{
	if (mParameterIds)
		mTempUsedId.resize (mParameterIds->size ());
}

//------------------------------------------------------------------------
void ParameterChangesCheck::checkParameterCount (Steinberg::int32 paramCount)
{
	if (!isValidParamCount (paramCount))
	{
		mEventLogger->addLogEvent (kLogIdInvalidParameterCount);
	}
}

//------------------------------------------------------------------------
bool ParameterChangesCheck::isValidParamCount (Steinberg::int32 paramCount) const
{
	return paramCount <= (Steinberg::int32)mParameterIds->size ();
}

//------------------------------------------------------------------------
void ParameterChangesCheck::checkParameterId (Steinberg::Vst::ParamID paramId)
{
	if (!isValidParamID (paramId))
	{
		mEventLogger->addLogEvent (kLogIdInvalidParameterID);
	}
}

//------------------------------------------------------------------------
bool ParameterChangesCheck::isValidParamID (Steinberg::Vst::ParamID paramId) const
{
	return mParameterIds->find (paramId) != mParameterIds->end ();
}

//------------------------------------------------------------------------
void ParameterChangesCheck::checkNormalized (double normVal)
{
	if (!isNormalized (normVal))
	{
		mEventLogger->addLogEvent (kLogIdInvalidParamValue);
	}
}

//------------------------------------------------------------------------
void ParameterChangesCheck::checkSampleOffset (Steinberg::int32 sampleOffset,
                                               Steinberg::int32 lastSampleOffset)
{
	if (!isValidSampleOffset (sampleOffset, lastSampleOffset))
	{
		mEventLogger->addLogEvent (kLogIdParametersAreNotSortedBySampleOffset);
	}
}

//------------------------------------------------------------------------
bool ParameterChangesCheck::isNormalized (double normVal) const
{
	return normVal >= 0. && normVal <= 1.;
}

//------------------------------------------------------------------------
bool ParameterChangesCheck::isValidSampleOffset (Steinberg::int32 sampleOffset, Steinberg::int32 lastSampleOffset) const
{
	return sampleOffset >= lastSampleOffset;
}

//------------------------------------------------------------------------
bool ParameterChangesCheck::checkParameterQueue (Steinberg::Vst::IParamValueQueue* paramQueue)
{
	if (!paramQueue)
	{
		mEventLogger->addLogEvent (kLogIdParameterQueueIsNullForValidIndex);
	}

	return paramQueue != nullptr;
}
