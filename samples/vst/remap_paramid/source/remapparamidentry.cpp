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

#include "remapparamidcids.h"
#include "remapparamidcontroller.h"
#include "remapparamidprocessor.h"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.h"

#define stringPluginName "Test Remap ParamID"

using namespace Steinberg;

//------------------------------------------------------------------------
//  VST Plug-in Entry
//------------------------------------------------------------------------

BEGIN_FACTORY_DEF ("Steinberg", "https://www.mycompanyname.com", "mailto:test@test.fr")

//---First Plug-in included in this factory-------
// its kVstAudioEffectClass component
DEF_CLASS2 (
    INLINE_UID_FROM_FUID (kTestRemapParamIDProcessorUID),
    PClassInfo::kManyInstances, // cardinality
    kVstAudioEffectClass, // the component category (do not changed this)
    stringPluginName, // here the Plug-in name (to be changed)
    Vst::kDistributable, // means that component and controller could be distributed on different
                         // computers
    TestRemapParamIDVST3Category, // Subcategory for this Plug-in (to be changed)
    FULL_VERSION_STR, // Plug-in version (to be changed)
    kVstVersionString, // the VST 3 SDK version (do not changed this, use always this define)
    Vst::TestRemapParamIDProcessor::createInstance) // function pointer called when this component
                                                    // should be instantiated

// its kVstComponentControllerClass component
DEF_CLASS2 (
    INLINE_UID_FROM_FUID (kTestRemapParamIDControllerUID),
    PClassInfo::kManyInstances, // cardinality
    kVstComponentControllerClass, // the Controller category (do not changed this)
    stringPluginName "Controller", // controller name (could be the same than component name)
    0, // not used here
    "", // not used here
    FULL_VERSION_STR, // Plug-in version (to be changed)
    kVstVersionString, // the VST 3 SDK version (do not changed this, use always this define)
    Vst::TestRemapParamIDController::createInstance) // function pointer called when this component
                                                     // should be instantiated

//----for others Plug-ins contained in this factory, put like for the first Plug-in different
//DEF_CLASS2---

END_FACTORY
