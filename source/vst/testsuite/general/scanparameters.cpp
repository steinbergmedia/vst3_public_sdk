//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/general/scanparameters.cpp
// Created by  : Steinberg, 04/2005
// Description : VST Test Suite
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2019, Steinberg Media Technologies GmbH, All Rights Reserved
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

#include "public.sdk/source/vst/testsuite/general/scanparameters.h"
#include "public.sdk/source/vst/hosting/stringconvert.h"
#include "base/source/fstring.h"
#include "pluginterfaces/vst/ivstunits.h"
#include <unordered_map>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// ScanParametersTest
//------------------------------------------------------------------------
ScanParametersTest::ScanParametersTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API ScanParametersTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	if (!controller)
	{
		addMessage (testResult, STR ("No Edit Controller supplied!"));
		return true;
	}

	int32 numParameters = controller->getParameterCount ();
	if (numParameters <= 0)
	{
		addMessage (testResult, STR ("This component does not export any parameters!"));
		return true;
	}

	addMessage (testResult, printf ("This component exports %d parameter(s)", numParameters));

	FUnknownPtr<IUnitInfo> iUnitInfo2 (controller);
	if (!iUnitInfo2 && numParameters > 20)
	{
		addMessage (
		    testResult,
		    STR ("Note: it could be better to use UnitInfo in order to sort Parameters (>20)."));
	}

	// used for ID check
	std::unordered_map<int32, int32> paramIds;

	bool foundBypass = false;
	for (int32 i = 0; i < numParameters; ++i)
	{
		ParameterInfo paramInfo = {0};

		tresult result = controller->getParameterInfo (i, paramInfo);
		if (result != kResultOk)
		{
			addErrorMessage (testResult, printf ("Param %03d: is missing!!!", i));
			return false;
		}

		int32 paramId = paramInfo.id;
		if (paramId < 0)
		{
			addErrorMessage (testResult, printf ("Param %03d: Invalid Id!!!", i));
			return false;
		}

		// check if ID is already used by another parameter
		auto search = paramIds.find (paramId);
		if (search != paramIds.end ())
		{
			addErrorMessage (
			    testResult, printf ("Param %03d: ID already used (by %03d)!!!", i, search->second));
			return false;
		}
		else
			paramIds[paramId] = i;

		const tchar* paramType = kEmptyString;
		if (paramInfo.stepCount < 0)
		{
			addErrorMessage (testResult, printf ("Param %03d: invalid stepcount!!!", i));
			return false;
		}
		if (paramInfo.stepCount == 0)
			paramType = STR ("Float");
		else if (paramInfo.stepCount == 1)
			paramType = STR ("Toggle");
		else
			paramType = STR ("Discrete");

		auto paramTitle = VST3::StringConvert::convert (paramInfo.title);
		auto paramUnits = VST3::StringConvert::convert (paramInfo.units);

		addMessage (
		    testResult,
		    printf (
		        "   Param %03d (ID = %d): [title=\"%s\"] [unit=\"%s\"] [type = %s, default = %lf, unit = %d]",
		        i, paramId, paramTitle.data (), paramUnits.data (), paramType,
		        paramInfo.defaultNormalizedValue, paramInfo.unitId));

		if (paramTitle.empty ())
		{
			addErrorMessage (testResult, printf ("Param %03d: has no title!!!", i));
			return false;
		}

		if (paramInfo.defaultNormalizedValue != -1.f &&
		    (paramInfo.defaultNormalizedValue < 0. || paramInfo.defaultNormalizedValue > 1.))
		{
			addErrorMessage (testResult,
			                 printf ("Param %03d: defaultValue is not normalized!!!", i));
			return false;
		}

		int32 unitId = paramInfo.unitId;
		if (unitId < -1)
		{
			addErrorMessage (testResult, printf ("Param %03d: No appropriate unit ID!!!", i));
			return false;
		}
		if (unitId >= -1)
		{
			FUnknownPtr<IUnitInfo> iUnitInfo (controller);
			if (!iUnitInfo && unitId != 0)
			{
				addErrorMessage (
				    testResult,
				    printf (
				        "IUnitInfo interface is missing, but ParameterInfo::unitID is not %03d (kRootUnitId).",
				        kRootUnitId));
				return false;
			}
			else if (iUnitInfo)
			{
				bool found = false;
				int32 uc = iUnitInfo->getUnitCount ();
				for (int32 ui = 0; ui < uc; ++ui)
				{
					UnitInfo uinfo = {0};
					if (iUnitInfo->getUnitInfo (ui, uinfo) != kResultTrue)
					{
						addErrorMessage (testResult, STR ("IUnitInfo::getUnitInfo (..) failed."));
						return false;
					}
					if (uinfo.id == unitId)
						found = true;
				}
				if (!found && unitId != kRootUnitId)
				{
					addErrorMessage (
					    testResult,
					    STR ("Parameter has a UnitID, which isn't defined in IUnitInfo."));
					return false;
				}
			}
		}
		if (((paramInfo.flags & ParameterInfo::kCanAutomate) != 0) &&
		    ((paramInfo.flags & ParameterInfo::kIsReadOnly) != 0))
		{
			addErrorMessage (
			    testResult,
			    STR ("Parameter must not be kCanAutomate and kReadOnly at the same time."));
			return false;
		}

		if ((paramInfo.flags & ParameterInfo::kIsBypass) != 0)
		{
			if (!foundBypass)
				foundBypass = true;
			else
			{
				addErrorMessage (testResult, STR ("There can only be one bypass (kIsBypass)."));
				return false;
			}
		}
	}

	if (foundBypass == false)
	{
		StringObject subCat;
		plugProvider->getSubCategories (subCat);
		if (subCat.findFirst ("Instrument") >= 0)
			addMessage (testResult, STR ("No bypass parameter found. This is an instrument."));
		else
			addMessage (testResult, STR ("Warning: No bypass parameter found. Is this intended ?"));
	}

	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
