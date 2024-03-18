//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Examples
// Filename    : public.sdk/samples/vst/dataexchange/factory.cpp
// Created by  : Steinberg, 06/2023
// Description : VST Data Exchange API Example
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
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "shared.h"
#include "version.h" // for versioning
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"

//------------------------------------------------------------------------
#include "public.sdk/source/main/pluginfactory_constexpr.h"

#define stringPluginName "DataExchange"

BEGIN_FACTORY_DEF (stringCompanyName, stringCompanyWeb, stringCompanyEmail, 2)

DEF_CLASS (Steinberg::Vst::DataExchangeProcessorUID, //
           Steinberg::PClassInfo::kManyInstances, //
           kVstAudioEffectClass, //
           stringPluginName, //
           Steinberg::Vst::kDistributable, //
           "Fx", //
           FULL_VERSION_STR, //
           kVstVersionString, //
           Steinberg::Vst::createDataExchangeProcessor, //
           nullptr)

DEF_CLASS (Steinberg::Vst::DataExchangeControllerUID, //
           Steinberg::PClassInfo::kManyInstances, //
           kVstComponentControllerClass, //
           stringPluginName "Controller", //
           0, //
           "", //
           FULL_VERSION_STR, //
           kVstVersionString, //
           Steinberg::Vst::createDataExchangeController, //
           nullptr)

END_FACTORY
