//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/utf16name/source/utf16nameentry.cpp
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


#include "utf16nameprocessor.h"
#include "utf16namecontroller.h"
#include "utf16namecids.h"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.h"

// name with UTF characters
#define stringPluginNameU	u"UTF16Name öüäéèê-やあ-مرحبًا"
#define stringCompanyNameU	u"Steinberg Media Technologies - öüäéèê-やあ-مرحبًا"

using namespace Steinberg::Vst;

#define CONCAT(prefix, text) prefix##text
#define U(text) CONCAT(u, text)

//------------------------------------------------------------------------
//  VST Plug-in Entry
//------------------------------------------------------------------------
// Windows: do not forget to include a .def file in your project to export
// GetPluginFactory function!
//------------------------------------------------------------------------

BEGIN_FACTORY_DEF (stringCompanyName, stringCompanyWeb, stringCompanyEmail)

	//---First Plug-in included in this factory-------
	// its kVstAudioEffectClass component
	DEF_CLASS_W2 (INLINE_UID_FROM_FUID(kUTF16NameProcessorUID),
				PClassInfo::kManyInstances,	// cardinality
				kVstAudioEffectClass,	// the component category (do not change this)
				stringPluginNameU,		// here the Plug-in name (to be changed)
				Vst::kDistributable,	// means that component and controller could be distributed on different computers
				UTF16NameVST3Category,	// Subcategory for this Plug-in (to be changed)
				stringCompanyNameU,
				U(FULL_VERSION_STR),	// Plug-in version (to be changed)
				U(kVstVersionString),	// the VST 3 SDK version (do not change this, use always this define)
				UTF16NameProcessor::createInstance)	// function pointer called when this component should be instantiated

	// its kVstComponentControllerClass component
	DEF_CLASS_W2 (INLINE_UID_FROM_FUID (kUTF16NameControllerUID),
				PClassInfo::kManyInstances, // cardinality
				kVstComponentControllerClass,// the Controller category (do not changed this)
				stringPluginNameU,		// controller name (could be the same than component name)
				0,						// not used here
				"",						// not used here
				U(""),					// not used here
				U(FULL_VERSION_STR),	// Plug-in version (to be changed)
				U(kVstVersionString),	// the VST 3 SDK version (do not changed this, use always this define)
				UTF16NameController::createInstance)// function pointer called when this component should be instantiated
END_FACTORY

#define TEXT "toto" 
#define UTEXT u##TEXT