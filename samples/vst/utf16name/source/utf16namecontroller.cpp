//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/utf16name/source/utf16namecontroller.cpp
// Created by  : Steinberg, 11/2023
// Description : UTF16Name Example for VST 3
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2025, Steinberg Media Technologies GmbH, All Rights Reserved
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

#include "utf16namecontroller.h"
#include "utf16namecids.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstunits.h"

namespace Steinberg {

//------------------------------------------------------------------------
// UTF16NameController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API UTF16NameController::initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated

	//---do not forget to call parent ------
	tresult result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	// Here you could register some parameters

	//--- Create Units-------------
	Vst::UnitInfo unitInfo {};
	unitInfo.id = 1;
	unitInfo.parentUnitId = Vst::kRootUnitId; // attached to the root unit
	unitInfo.programListId = Vst::kNoProgramListId;

	// English
	UString (unitInfo.name, str16BufferSize (Vst::String128)).assign (STR16 ("Folder 1"));
	auto unit = new Vst::Unit (unitInfo);
	addUnit (unit);

	// Japanese
	unitInfo.id++;
	UString (unitInfo.name, str16BufferSize (Vst::String128)).assign (STR16 ("フォルダー2"));
	unit = new Vst::Unit (unitInfo);
	addUnit (unit);

	// Korean
	unitInfo.id++;
	UString (unitInfo.name, str16BufferSize (Vst::String128)).assign (STR16 ("폴더 3"));
	unit = new Vst::Unit (unitInfo);
	addUnit (unit);

	// Arabic
	unitInfo.id++;
	UString (unitInfo.name, str16BufferSize (Vst::String128)).assign (STR16 ("المجلد 4"));
	unit = new Vst::Unit (unitInfo);
	addUnit (unit);

	// Persian
	unitInfo.id++;
	UString (unitInfo.name, str16BufferSize (Vst::String128)).assign (STR16 ("پوشه 5"));
	unit = new Vst::Unit (unitInfo);
	addUnit (unit);

	//---Create Parameters------------
	int32 stepCount = 0;
	Vst::ParamValue defaultVal = 0;
	int32 flags = Vst::ParameterInfo::kNoFlags;
	int32 tag = 100;
	Vst::UnitID unitId = 1;
	
	// English
	parameters.addParameter (STR16 ("Hello"), nullptr, stepCount, defaultVal, flags, tag, unitId);
	
	// Japanese
	tag++;
	unitId++;
	parameters.addParameter (STR16 ("こんにちは"), nullptr, stepCount, defaultVal, flags, tag, unitId);
	
	// Korean
	tag++;
	unitId++;
	parameters.addParameter (STR16 ("안녕하세요"), nullptr, stepCount, defaultVal, flags, tag, unitId);
	
	flags = Vst::ParameterInfo::kCanAutomate;

	// Arabic
	tag++;
	unitId++;
	parameters.addParameter (STR16 ("مرحبا"), nullptr, stepCount, defaultVal, flags, tag, unitId);
	
	// Persian
	tag++;
	unitId++;
	parameters.addParameter (STR16 ("سلام"), nullptr, stepCount, defaultVal, flags, tag, unitId);

	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API UTF16NameController::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API UTF16NameController::setComponentState (IBStream* state)
{
	// Here you get the state of the component (Processor part)
	if (!state)
		return kResultFalse;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API UTF16NameController::setState (IBStream* state)
{
	// Here you get the state of the controller

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API UTF16NameController::getState (IBStream* state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API UTF16NameController::createView (FIDString name)
{
	// Here the Host wants to open your editor (if you have one)
	if (FIDStringsEqual (name, Vst::ViewType::kEditor))
	{
		// create your editor here and return a IPlugView ptr of it
		return nullptr;
	}
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API UTF16NameController::setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized (tag, value);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API UTF16NameController::getParamStringByValue (Vst::ParamID tag,
                                                               Vst::ParamValue valueNormalized,
                                                               Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API UTF16NameController::getParamValueByString (Vst::ParamID tag, Vst::TChar* string,
                                                               Vst::ParamValue& valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

//------------------------------------------------------------------------
} // namespace Steinberg
