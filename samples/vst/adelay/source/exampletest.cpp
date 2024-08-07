//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/adelay/source/exampletest.cpp
// Created by  : Steinberg, 10/2010
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
#include "adelayprocessor.h"
#include "base/source/fstring.h"
#include "public.sdk/source/main/moduleinit.h"
#include "public.sdk/source/vst/testsuite/vsttestsuite.h"
#include "public.sdk/source/vst/utility/testing.h"
#include "pluginterfaces/base/funknownimpl.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
static ModuleInitializer InitTests ([] () {
	registerTest ("ExampleTest", nullptr, [] (FUnknown* context, ITestResult* testResult) {
		auto plugProvider = U::cast<ITestPlugProvider> (context);
		if (plugProvider)
		{
			auto controller = plugProvider->getController ();
			auto testController = U::cast<IDelayTestController> (controller);
			if (!controller)
			{
				testResult->addErrorMessage (String ("Unknown IEditController"));
				return false;
			}
			bool result = testController->doTest ();
			plugProvider->releasePlugIn (nullptr, controller);

			return (result);
		}
		return false;
	});
});

//------------------------------------------------------------------------
}} // namespaces
