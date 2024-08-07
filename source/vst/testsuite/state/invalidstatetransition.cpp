//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/state/invalidstatetransition.cpp
// Created by  : Steinberg, 04/2005
// Description : VST Test Suite
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

#include "public.sdk/source/vst/testsuite/state/invalidstatetransition.h"
#include "pluginterfaces/base/funknownimpl.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// InvalidStateTransitionTest
//------------------------------------------------------------------------
InvalidStateTransitionTest::InvalidStateTransitionTest (ITestPlugProvider* plugProvider)
: TestEnh (plugProvider, kSample32)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API InvalidStateTransitionTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	auto plugBase = U::cast<IPluginBase> (vstPlug);
	if (!plugBase)
		return false;

	// created
	tresult result = plugBase->initialize (TestingPluginContext::get ());
	if (result == kResultFalse)
		return false;

	// setupProcessing is missing !
	/*result = audioEffect->setupProcessing (processSetup);
	if (result != kResultTrue)
	    return false;*/

	// initialized
	result = vstPlug->setActive (false);
	if (result == kResultOk)
		return false;

	result = vstPlug->setActive (true);
	if (result == kResultFalse)
		return false;

	// allocated
	result = plugBase->initialize (TestingPluginContext::get ());
	if (result == kResultOk)
		return false;

	result = vstPlug->setActive (false);
	if (result == kResultFalse)
		return false;

	// deallocated (initialized)
	result = plugBase->initialize (TestingPluginContext::get ());
	if (result == kResultOk)
		return false;

	result = plugBase->terminate ();
	if (result == kResultFalse)
		return false;

	// terminated (created)
	result = vstPlug->setActive (false);
	if (result == kResultOk)
		return false;

	result = plugBase->terminate ();
	if (result == kResultOk)
		return false;

	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
