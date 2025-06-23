//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/utf16name/source/utf16namecontroller.h
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

#include "public.sdk/source/vst/vsteditcontroller.h"

namespace Steinberg {

//------------------------------------------------------------------------
//  UTF16NameController
//------------------------------------------------------------------------
class UTF16NameController : public Vst::EditControllerEx1
{
public:
//------------------------------------------------------------------------
	UTF16NameController () = default;
	~UTF16NameController () SMTG_OVERRIDE = default;

	// Create function
	static FUnknown* createInstance (void* /*context*/)
	{
		return (Vst::IEditController*)new UTF16NameController;
	}

	// IPluginBase
	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;

	// EditController
	tresult PLUGIN_API setComponentState (IBStream* state) SMTG_OVERRIDE;
	IPlugView* PLUGIN_API createView (FIDString name) SMTG_OVERRIDE;
	tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API setParamNormalized (Vst::ParamID tag, Vst::ParamValue value) SMTG_OVERRIDE;
	tresult PLUGIN_API getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized,
	                                          Vst::String128 string) SMTG_OVERRIDE;
	tresult PLUGIN_API getParamValueByString (Vst::ParamID tag, Vst::TChar* string,
	                                          Vst::ParamValue& valueNormalized) SMTG_OVERRIDE;

	//---Interface---------
	DEFINE_INTERFACES
	// Here you can add more supported VST3 interfaces
	// DEF_INTERFACE (Vst::IXXX)
	END_DEFINE_INTERFACES (EditControllerEx1)
	DELEGATE_REFCOUNT (EditControllerEx1)

//------------------------------------------------------------------------
protected:
};

//------------------------------------------------------------------------
} // namespace Steinberg
