//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/adelay/source/adelaycontroller.cpp
// Created by  : Steinberg, 06/2009
// Description : 
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

#include "adelaycontroller.h"
#include "adelayids.h"
#include "pluginterfaces/base/ibstream.h"

#if TARGET_OS_IPHONE
#include "interappaudio/iosEditor.h"
#endif
#include "base/source/fstreamer.h"

namespace Steinberg {
namespace Vst {

DEF_CLASS_IID (IDelayTestController)

//-----------------------------------------------------------------------------
tresult PLUGIN_API ADelayController::initialize (FUnknown* context)
{
	tresult result = EditController::initialize (context);
	if (result == kResultTrue)
	{
		parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0, ParameterInfo::kCanAutomate|ParameterInfo::kIsBypass, kBypassId);

		parameters.addParameter (STR16 ("Delay"), STR16 ("sec"), 0, 1, ParameterInfo::kCanAutomate, kDelayId);
	}
	return kResultTrue;
}

#if TARGET_OS_IPHONE
//-----------------------------------------------------------------------------
IPlugView* PLUGIN_API ADelayController::createView (FIDString name)
{
	if (FIDStringsEqual (name, ViewType::kEditor))
	{
		return new ADelayEditorForIOS (this);
	}
	return 0;
}
#endif

//------------------------------------------------------------------------
tresult PLUGIN_API ADelayController::setComponentState (IBStream* state)
{
	// we receive the current state of the component (processor part)
	// we read only the gain and bypass value...
	if (!state)
		return kResultFalse;
	
	IBStreamer streamer (state, kLittleEndian);
	float savedDelay = 0.f;
	if (streamer.readFloat (savedDelay) == false)
		return kResultFalse;
	setParamNormalized (kDelayId, static_cast<ParamValue> (savedDelay));

	int32 bypassState = 0;
	if (streamer.readInt32 (bypassState) == false)
	{
		// could be an old version, continue 
	}
	setParamNormalized (kBypassId, bypassState ? 1 : 0);

	return kResultOk;
}

//------------------------------------------------------------------------
bool PLUGIN_API ADelayController::doTest ()
{
	// this is called when running thru the validator
	// we can now run our own test cases
	return true;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
