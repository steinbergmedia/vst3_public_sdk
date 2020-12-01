//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/state/validstatetransition.cpp
// Created by  : Steinberg, 04/2005
// Description : VST Test Suite
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2020, Steinberg Media Technologies GmbH, All Rights Reserved
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

#include "public.sdk/source/vst/testsuite/state/validstatetransition.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// ValidStateTransitionTest
//------------------------------------------------------------------------
ValidStateTransitionTest::ValidStateTransitionTest (ITestPlugProvider* plugProvider,
                                                    ProcessSampleSize sampleSize)
: ProcessTest (plugProvider, sampleSize)
{
	if (sampleSize == kSample32)
		strcpy (name, "Valid State Transition 32bits");
	else
		strcpy (name, "Valid State Transition 64bits");
}

//------------------------------------------------------------------------
bool PLUGIN_API ValidStateTransitionTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug || !audioEffect)
		return false;

	printTestHeader (testResult);
	if (!canProcessSampleSize (testResult))
		return true;

	vstPlug->setActive (false);

	for (int32 i = 0; i < 3; ++i)
	{
		tresult result = audioEffect->setupProcessing (processSetup);
		if (result != kResultTrue)
			return false;

		result = vstPlug->setActive (true);
		if (result != kResultTrue)
			return false;

		result = vstPlug->setActive (false);
		if (result != kResultTrue)
			return false;

		result = vstPlug->terminate ();
		if (result != kResultTrue)
			return false;

		result = vstPlug->initialize (TestingPluginContext::get ());
		if (result != kResultTrue)
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
