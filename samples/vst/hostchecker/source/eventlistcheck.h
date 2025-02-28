//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/hostchecker/source/eventlistcheck.h
// Created by  : Steinberg, 12/2012
// Description : Event List check
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

class EventLogger;

namespace Steinberg {
namespace Vst {
class IEventList;
class IComponent;
struct ProcessSetup;
struct Event;
}
}

//------------------------------------------------------------------------
//	EventListCheck
//------------------------------------------------------------------------
class EventListCheck
{
public:
//------------------------------------------------------------------------
	EventListCheck ();

	static const Steinberg::int32 kMaxEvents = 2048;
	using Notes = std::set<Steinberg::int32>;

	void check (Steinberg::Vst::IEventList* events);
	void setComponent (Steinberg::Vst::IComponent* component) { mComponent = component; }
	void setProcessSetup (Steinberg::Vst::ProcessSetup setup);
	void setEventLogger (EventLogger* eventLogger);

//------------------------------------------------------------------------
protected:
	bool checkEventCount (Steinberg::Vst::IEventList* events);
	void checkEventProperties (const Steinberg::Vst::Event& event);
	bool checkEventBusIndex (Steinberg::int32 busIndex);
	bool checkEventSampleOffset (Steinberg::int32 sampleOffset);
	bool checkEventChannelIndex (Steinberg::int32 busIndex, Steinberg::int32 channelIndex);
	bool checkValidPitch (Steinberg::int16 pitch);
	bool isNormalized (double normVal) const;
	void checkNoteExpressionValueEvent (Steinberg::Vst::NoteExpressionTypeID type,
	                                    Steinberg::int32 id,
	                                    Steinberg::Vst::NoteExpressionValue exprVal) const;

	EventLogger* mEventLogger;
	Steinberg::Vst::IComponent* mComponent;
	Steinberg::Vst::ProcessSetup mSetup;
	Notes mNotePitches;
	Notes mNoteIDs;
};
