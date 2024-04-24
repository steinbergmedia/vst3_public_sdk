//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/remap_paramid/source/remapparamidcontroller.h
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

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstremapparamid.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
//  TestRemapParamIDController
//------------------------------------------------------------------------
class TestRemapParamIDController : public EditController, public IRemapParamID
{
public:
//------------------------------------------------------------------------
	TestRemapParamIDController () = default;
	~TestRemapParamIDController () SMTG_OVERRIDE = default;

	// Create function
	static FUnknown* createInstance (void* /*context*/)
	{
		return (Vst::IEditController*)new TestRemapParamIDController;
	}

	//--- from IPluginBase -----------------------------------------------
	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;

	//--- from EditController --------------------------------------------
	tresult PLUGIN_API setComponentState (IBStream* state) SMTG_OVERRIDE;

	//--- from IRemapParamID ---------------------------------------------
	tresult PLUGIN_API getCompatibleParamID (const TUID pluginToReplaceUID /*in*/,
	                                         Vst::ParamID oldParamID /*in*/,
	                                         Vst::ParamID& newParamID /*out*/) override;

	//---Interface---------
	DEFINE_INTERFACES
		DEF_INTERFACE (IRemapParamID)
	END_DEFINE_INTERFACES (EditController)
	DELEGATE_REFCOUNT (EditController)

//------------------------------------------------------------------------
protected:
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
