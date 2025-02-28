//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/hostchecker/source/processcontextcheck.cpp
// Created by  : Steinberg, 12/2012
// Description : Process Context check
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

#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "processcontextcheck.h"
#include "eventlogger.h"
#include "logevents.h"

using namespace Steinberg::Vst;
//------------------------------------------------------------------------
//	ProcessContextCheck
//------------------------------------------------------------------------
ProcessContextCheck::ProcessContextCheck () : mEventLogger (nullptr), mSampleRate (0) {}

//------------------------------------------------------------------------
void ProcessContextCheck::setEventLogger (EventLogger* eventLogger) { mEventLogger = eventLogger; }

//------------------------------------------------------------------------
void ProcessContextCheck::check (ProcessContext* context)
{
	if (!context)
	{
		mEventLogger->addLogEvent (kLogIdProcessContextPointerNull);
		return;
	}

	if (context->sampleRate != mSampleRate)
	{
		mEventLogger->addLogEvent (kLogIdInvalidProcessContextSampleRate);
	}
	if (context->state & ProcessContext::StatesAndFlags::kSystemTimeValid)
	{
		if (mLastSystemTime >= context->systemTime)
		{
			mEventLogger->addLogEvent (kLogIdInvalidProcessContextSystemTime);
		}
		mLastSystemTime = context->systemTime;
	}
}

//------------------------------------------------------------------------
void ProcessContextCheck::setSampleRate (double sampleRate) { mSampleRate = sampleRate; }
