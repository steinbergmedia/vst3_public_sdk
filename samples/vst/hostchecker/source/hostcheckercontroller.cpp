//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/hostchecker/source/hostcheckercontroller.cpp
// Created by  : Steinberg, 04/2012
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

#include "hostcheckercontroller.h"

#include "cids.h"
#include "editorsizecontroller.h"
#include "eventlogdatabrowsersource.h"
#include "hostcheckerprocessor.h"
#include "logevents.h"
#include "base/source/fstreamer.h"

#include "public.sdk/source/common/systemclipboard.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "public.sdk/source/vst/vstcomponentbase.h"
#include "public.sdk/source/vst/vstrepresentation.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstcontextmenu.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstpluginterfacesupport.h"

#include "pluginterfaces/vst/ivstdataexchange.h"
#include <sstream>

#define THREAD_CHECK_MSG(msg) "The host called '" msg "' in the wrong thread context.\n"

using namespace VSTGUI;

#if DEBUG
bool THREAD_CHECK_EXIT = false;
#else
bool THREAD_CHECK_EXIT = true;
#endif

namespace Steinberg {
namespace Vst {

class StringInt64Parameter : public Parameter
{
public:
//------------------------------------------------------------------------
	StringInt64Parameter (const TChar* title, ParamID tag, const TChar* units = nullptr,
	                      UnitID unitID = kRootUnitId, const TChar* shortTitle = nullptr,
	                      int32 flags = ParameterInfo::kNoFlags)
	{
		UString (info.title, str16BufferSize (String128)).assign (title);
		if (units)
			UString (info.units, str16BufferSize (String128)).assign (units);
		if (shortTitle)
			UString (info.shortTitle, str16BufferSize (String128)).assign (shortTitle);

		info.stepCount = 0;
		info.defaultNormalizedValue = valueNormalized = 0;
		info.flags = flags;
		info.id = tag;
		info.unitId = unitID;
	}
	void setValue (int64 value)
	{
		if (mValue != value)
		{
			changed ();
			mValue = value;
		}
	}

	void toString (ParamValue /*_valueNormalized*/, String128 string) const SMTG_OVERRIDE
	{
		UString wrapper (string, str16BufferSize (String128));
		if (!wrapper.printInt (mValue))
			string[0] = 0;
	}

	OBJ_METHODS (StringInt64Parameter, Parameter)
//------------------------------------------------------------------------
protected:
	StringInt64Parameter () {}

	int64 mValue {0};
};

//-----------------------------------------------------------------------------
class MyVST3Editor : public VST3Editor
{
public:
	MyVST3Editor (HostCheckerController* controller, UTF8StringPtr templateName,
	              UTF8StringPtr xmlFile);

	void canResize (bool val) { mCanResize = val; }

protected:
	~MyVST3Editor () override;

	bool PLUGIN_API open (void* parent, const PlatformType& type) override;
	void PLUGIN_API close () override;

	bool beforeSizeChange (const CRect& newSize, const CRect& oldSize) override;

	tresult PLUGIN_API onSize (ViewRect* newSize) override;
	tresult PLUGIN_API canResize () override;
	tresult PLUGIN_API checkSizeConstraint (ViewRect* rect) override;
	tresult PLUGIN_API onKeyDown (char16 key, int16 keyMsg, int16 modifiers) override;
	tresult PLUGIN_API onKeyUp (char16 key, int16 keyMsg, int16 modifiers) override;
	tresult PLUGIN_API onWheel (float distance) override;
	tresult PLUGIN_API onFocus (TBool /*state*/) override;
	tresult PLUGIN_API setFrame (IPlugFrame* frame) override;
	tresult PLUGIN_API attached (void* parent, FIDString type) override;
	tresult PLUGIN_API removed () override;

	tresult PLUGIN_API setContentScaleFactor (ScaleFactor factor) override;

	// IParameterFinder
	tresult PLUGIN_API findParameter (int32 xPos, int32 yPos, ParamID& resultTag) override;

	void valueChanged (CControl* pControl) override;

	//---from CBaseObject---------------
	CMessageResult notify (CBaseObject* sender, const char* message) SMTG_OVERRIDE;

private:
	CVSTGUITimer* checkTimer {nullptr};

	HostCheckerController* hostController {nullptr};

	uint32 openCount = 0;
	bool wasAlreadyClosed = false;
	bool onSizeWanted = false;
	bool inOpen = false;
	bool inOnsize = false;
	bool mCanResize = true;
	bool mAttached = false;
};

//-----------------------------------------------------------------------------
MyVST3Editor::MyVST3Editor (HostCheckerController* controller, UTF8StringPtr templateName,
                            UTF8StringPtr xmlFile)
: VST3Editor (controller, templateName, xmlFile), hostController (controller)
{
}

//-----------------------------------------------------------------------------
MyVST3Editor::~MyVST3Editor ()
{
	if (checkTimer)
		checkTimer->forget ();
}

//-----------------------------------------------------------------------------
bool PLUGIN_API MyVST3Editor::open (void* parent, const PlatformType& type)
{
	inOpen = true;

	openCount++;

	if (wasAlreadyClosed)
		hostController->addFeatureLog (kLogIdIPlugViewmultipleAttachSupported);

	bool res = VST3Editor::open (parent, type);
	if (hostController)
	{
		ViewRect rect2;
		if (hostController->getSavedSize (rect2))
			onSize (&rect2);
	}
	inOpen = false;

	return res;
}

//-----------------------------------------------------------------------------
void PLUGIN_API MyVST3Editor::close ()
{
	wasAlreadyClosed = true;

	openCount--;
	return VST3Editor::close ();
}

//-----------------------------------------------------------------------------
bool MyVST3Editor::beforeSizeChange (const CRect& newSize, const CRect& oldSize)
{
	if (!inOpen && !inOnsize)
	{
		if (!sizeRequest && newSize != oldSize)
		{
			onSizeWanted = true;
		}
	}

	bool res = VST3Editor::beforeSizeChange (newSize, oldSize);

	if (!inOpen && !inOnsize && !sizeRequest)
	{
		if (!res)
			onSizeWanted = false;
		else
			hostController->addFeatureLog (kLogIdIPlugFrameonResizeViewSupported);

		if (onSizeWanted)
		{
			if (checkTimer == nullptr)
				checkTimer = new CVSTGUITimer (this, 500);
			checkTimer->stop ();
			checkTimer->start ();
		}
	}
	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::onSize (Steinberg::ViewRect* newSize)
{
	inOnsize = true;
	if (!inOpen)
	{
		if (sizeRequest)
			hostController->addFeatureLog (kLogIdIPlugViewCalledSync);
		else if (onSizeWanted)
			hostController->addFeatureLog (kLogIdIPlugViewCalledAsync);

		onSizeWanted = false;

		hostController->addFeatureLog (kLogIdIPlugViewonSizeSupported);
	}

	if (openCount == 0)
		hostController->addFeatureLog (kLogIdIPlugViewCalledBeforeOpen);

	auto res = VST3Editor::onSize (newSize);
	inOnsize = false;

	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::canResize ()
{
	hostController->addFeatureLog (kLogIdIPlugViewcanResizeSupported);
	return mCanResize ? kResultTrue : kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::checkSizeConstraint (Steinberg::ViewRect* _rect)
{
	hostController->addFeatureLog (kLogIdIPlugViewcheckSizeConstraintSupported);
	return VST3Editor::checkSizeConstraint (_rect);
}

//------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::onKeyDown (char16 key, int16 keyMsg, int16 modifiers)
{
	if (!mAttached)
		hostController->addFeatureLog (kLogIdIPlugViewKeyCalledBeforeAttach);

	hostController->addFeatureLog (kLogIdIPlugViewOnKeyDownSupported);

	return VSTGUIEditor::onKeyDown (key, keyMsg, modifiers);
}

//------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::onKeyUp (char16 key, int16 keyMsg, int16 modifiers)
{
	if (!mAttached)
		hostController->addFeatureLog (kLogIdIPlugViewKeyCalledBeforeAttach);

	hostController->addFeatureLog (kLogIdIPlugViewOnKeyUpSupported);

	return VSTGUIEditor::onKeyUp (key, keyMsg, modifiers);
}

//------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::onWheel (float distance)
{
	if (!mAttached)
		hostController->addFeatureLog (kLogIdIPlugViewKeyCalledBeforeAttach);

	hostController->addFeatureLog (kLogIdIPlugViewOnWheelCalled);

	return VSTGUIEditor::onWheel (distance);
}

//------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::onFocus (TBool state)
{
	hostController->addFeatureLog (kLogIdIPlugViewOnFocusCalled);

	return VSTGUIEditor::onFocus (state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::setFrame (IPlugFrame* _frame)
{
	hostController->addFeatureLog (kLogIdIPlugViewsetFrameSupported);
	return VSTGUIEditor::setFrame (_frame);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::attached (void* parent, FIDString type)
{
	if (mAttached)
		hostController->addFeatureLog (kLogIdIPlugViewattachedWithoutRemoved);

	mAttached = true;
	return VSTGUIEditor::attached (parent, type);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::removed ()
{
	if (!mAttached)
		hostController->addFeatureLog (kLogIdIPlugViewremovedWithoutAttached);

	mAttached = false;
	return VSTGUIEditor::removed ();
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::setContentScaleFactor (ScaleFactor factor)
{
	hostController->addFeatureLog (kLogIdIPlugViewsetContentScaleFactorSupported);
	return VST3Editor::setContentScaleFactor (factor);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API MyVST3Editor::findParameter (int32 xPos, int32 yPos, ParamID& resultTag)
{
	hostController->addFeatureLog (kLogIdIParameterFinderSupported);
	return VST3Editor::findParameter (xPos, yPos, resultTag);
}

//-----------------------------------------------------------------------------
VSTGUI::CMessageResult MyVST3Editor::notify (CBaseObject* sender, const char* message)
{
	if (sender == checkTimer)
	{
		if (onSizeWanted)
			hostController->addFeatureLog (kLogIdIPlugViewNotCalled);

		checkTimer->forget ();
		checkTimer = nullptr;
		return kMessageNotified;
	}

	return VST3Editor::notify (sender, message);
}

//-----------------------------------------------------------------------------
void MyVST3Editor::valueChanged (CControl* pControl)
{
	if (pControl->getTag () == kBypassTag)
	{
	}
	VST3Editor::valueChanged (pControl);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
HostCheckerController::HostCheckerController ()
{
	mScoreMap.emplace (kLogIdRestartParamValuesChangedSupported, 2.f);
	mScoreMap.emplace (kLogIdRestartParamTitlesChangedSupported, 2.f);
	mScoreMap.emplace (kLogIdRestartNoteExpressionChangedSupported, 1.f);
	mScoreMap.emplace (kLogIdRestartKeyswitchChangedSupported, 1.f);

	mScoreMap.emplace (kLogIdIComponentHandler2Supported, 2.f);
	mScoreMap.emplace (kLogIdIComponentHandler2SetDirtySupported, 2.f);
	mScoreMap.emplace (kLogIdIComponentHandler2RequestOpenEditorSupported, 2.f);
	mScoreMap.emplace (kLogIdIComponentHandler3Supported, 2.f);
	mScoreMap.emplace (kLogIdIComponentHandlerBusActivationSupported, 1.f);
	mScoreMap.emplace (kLogIdIProgressSupported, 1.f);
	mScoreMap.emplace (kLogIdIPlugInterfaceSupportSupported, 2.f);
	mScoreMap.emplace (kLogIdIPlugFrameonResizeViewSupported, 2.f);
	mScoreMap.emplace (kLogIdIPrefetchableSupportSupported, 1.f);
	mScoreMap.emplace (kLogIdAudioPresentationLatencySamplesSupported, 1.f);
	mScoreMap.emplace (kLogIdIProcessContextRequirementsSupported, 1.f);

	mScoreMap.emplace (kLogIdProcessContextPlayingSupported, 2.f);
	mScoreMap.emplace (kLogIdProcessContextRecordingSupported, 1.f);
	mScoreMap.emplace (kLogIdProcessContextCycleActiveSupported, 1.f);
	mScoreMap.emplace (kLogIdProcessContextSystemTimeSupported, 1.f);
	mScoreMap.emplace (kLogIdProcessContextContTimeSupported, 1.f);
	mScoreMap.emplace (kLogIdProcessContextTimeMusicSupported, 2.f);
	mScoreMap.emplace (kLogIdProcessContextBarPositionSupported, 1.f);
	mScoreMap.emplace (kLogIdProcessContextCycleSupported, 1.f);
	mScoreMap.emplace (kLogIdProcessContextTempoSupported, 2.f);
	mScoreMap.emplace (kLogIdProcessContextTimeSigSupported, 1.f);
	mScoreMap.emplace (kLogIdProcessContextChordSupported, 1.f);
	mScoreMap.emplace (kLogIdProcessContextSmpteSupported, 1.f);
	mScoreMap.emplace (kLogIdProcessContextClockSupported, 1.f);
	mScoreMap.emplace (kLogIdCanProcessSampleSize32, 1.f);
	mScoreMap.emplace (kLogIdCanProcessSampleSize64, 1.f);
	mScoreMap.emplace (kLogIdGetTailSamples, 1.f);
	mScoreMap.emplace (kLogIdGetLatencySamples, 2.f);
	mScoreMap.emplace (kLogIdGetBusArrangements, 1.f);
	mScoreMap.emplace (kLogIdSetBusArrangements, 1.f);
	mScoreMap.emplace (kLogIdGetRoutingInfo, 1.f);
	mScoreMap.emplace (kLogIdActivateAuxBus, 1.f);
	mScoreMap.emplace (kLogIdParametersFlushSupported, 1.f);
	mScoreMap.emplace (kLogIdSilentFlagsSupported, 2.f);
	mScoreMap.emplace (kLogIdSilentFlagsSCSupported, 2.f);

	mScoreMap.emplace (kLogIdIEditController2Supported, 1.f);
	mScoreMap.emplace (kLogIdSetKnobModeSupported, 1.f);
	mScoreMap.emplace (kLogIdOpenHelpSupported, 1.f);
	mScoreMap.emplace (kLogIdOpenAboutBoxSupported, 1.f);
	mScoreMap.emplace (kLogIdIMidiMappingSupported, 1.f);
	mScoreMap.emplace (kLogIdUnitSupported, 1.f);
	mScoreMap.emplace (kLogIdGetUnitByBusSupported, 1.f);
	mScoreMap.emplace (kLogIdChannelContextSupported, 1.f);
	mScoreMap.emplace (kLogIdINoteExpressionControllerSupported, 1.f);
	mScoreMap.emplace (kLogIdINoteExpressionPhysicalUIMappingSupported, 1.f);
	mScoreMap.emplace (kLogIdIKeyswitchControllerSupported, 1.f);
	mScoreMap.emplace (kLogIdIMidiLearnSupported, 1.f);
	mScoreMap.emplace (kLogIdIMidiLearn_onLiveMIDIControllerInputSupported, 1.f);

	mScoreMap.emplace (kLogIdIAttributeListInSetStateSupported, 1.f);
	mScoreMap.emplace (kLogIdIAttributeListInGetStateSupported, 1.f);

	mScoreMap.emplace (kLogIdIXmlRepresentationControllerSupported, 1.f);
	mScoreMap.emplace (kLogIdIAutomationStateSupported, 1.f);

	mScoreMap.emplace (kLogIdIEditControllerHostEditingSupported, 1.f);

	mScoreMap.emplace (kLogIdIPlugViewonSizeSupported, 1.f);
	mScoreMap.emplace (kLogIdIPlugViewcanResizeSupported, 1.f);
	mScoreMap.emplace (kLogIdIPlugViewcheckSizeConstraintSupported, 1.f);
	mScoreMap.emplace (kLogIdIPlugViewsetFrameSupported, 1.f);
	mScoreMap.emplace (kLogIdIPlugViewOnWheelCalled, 1.f);
	mScoreMap.emplace (kLogIdIPlugViewOnKeyDownSupported, 1.f);
	mScoreMap.emplace (kLogIdIPlugViewOnKeyUpSupported, 1.f);
	mScoreMap.emplace (kLogIdIPlugViewOnFocusCalled, 1.f);
	mScoreMap.emplace (kLogIdIPlugViewsetContentScaleFactorSupported, 1.f);

	mScoreMap.emplace (kLogIdIParameterFinderSupported, 1.f);
	mScoreMap.emplace (kLogIdIParameterFunctionNameSupported, 1.f);

	mScoreMap.emplace (kLogIdIParameterFunctionNameDryWetSupported, 1.f);
	mScoreMap.emplace (kLogIdIParameterFunctionNameLowLatencySupported, 1.f);
	mScoreMap.emplace (kLogIdIParameterFunctionNameRandomizeSupported, 1.f);

	mScoreMap.emplace (kLogIdIComponentHandlerSystemTimeSupported, 1.f);
	mScoreMap.emplace (kLogIdIDataExchangeHandlerSupported, 1.f);
	mScoreMap.emplace (kLogIdIDataExchangeReceiverSupported, 1.f);

	mScoreMap.emplace (kLogIdIRemapParamIDSupported, 1.f);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::initialize (FUnknown* context)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::initialize"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdinitializeCalledinWrongThread);
	}

	tresult result = EditControllerEx1::initialize (context);
	if (result == kResultOk)
	{
		// create a unit Latency parameter
		UnitInfo unitInfo {};
		unitInfo.id = kUnitId;
		unitInfo.parentUnitId = kRootUnitId; // attached to the root unit
		Steinberg::UString (unitInfo.name, USTRINGSIZE (unitInfo.name)).assign (USTRING ("Setup"));
		unitInfo.programListId = kNoProgramListId;

		auto* unit = new Unit (unitInfo);
		addUnit (unit);

		// add second unit
		unitInfo.id = kUnit2Id;
		unitInfo.parentUnitId = kRootUnitId; // attached to the root unit
		Steinberg::UString (unitInfo.name, USTRINGSIZE (unitInfo.name))
		    .assign (USTRING ("Second Unit"));
		unitInfo.programListId = kNoProgramListId;
		unit = new Unit (unitInfo);
		addUnit (unit);

		parameters.addParameter (STR16 ("Processing Load"), STR16 (""), 0, 0,
		                         ParameterInfo::kCanAutomate, kProcessingLoadTag);
		parameters.addParameter (STR16 ("Generate Peaks"), STR16 (""), 0, 0,
		                         ParameterInfo::kNoFlags, kGeneratePeaksTag);
		parameters.addParameter (new RangeParameter (
		    STR16 ("Latency"), kLatencyTag, nullptr, 0, HostChecker::kMaxLatency, 0,
		    HostChecker::kMaxLatency, ParameterInfo::kNoFlags, kUnitId, nullptr));
		parameters.addParameter (STR16 ("CanResize"), STR16 (""), 1, 1, ParameterInfo::kNoFlags,
		                         kCanResizeTag);

		parameters.addParameter (new RangeParameter (STR16 ("Scoring"), kScoreTag, nullptr, 0, 100,
		                                             0, 100, ParameterInfo::kIsReadOnly));

		parameters.addParameter (STR16 ("Bypass"), STR16 (""), 1, 0,
		                         ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass,
		                         kBypassTag);

		parameters.addParameter (new RangeParameter (STR16 ("ProgressValue"), kProgressValueTag,
		                                             nullptr, 0, 100, 0, 100,
		                                             ParameterInfo::kIsReadOnly));
		parameters.addParameter (STR16 ("TriggerProgress"), STR16 (""), 1, 0,
		                         ParameterInfo::kNoFlags, kTriggerProgressTag);

		parameters.addParameter (STR16 ("KeyswitchChanged"), STR16 (""), 1, 0,
		                         ParameterInfo::kIsHidden, kRestartKeyswitchChangedTag);
		parameters.addParameter (STR16 ("NoteExpressionChanged"), STR16 (""), 1, 0,
		                         ParameterInfo::kIsHidden, kRestartNoteExpressionChangedTag);
		parameters.addParameter (STR16 ("ParamValuesChanged"), STR16 (""), 1, 0,
		                         ParameterInfo::kIsHidden, kRestartParamValuesChangedTag);
		parameters.addParameter (STR16 ("ParamTitlesChanged"), STR16 (""), 1, 0,
		                         ParameterInfo::kIsHidden, kRestartParamTitlesChangedTag);

		parameters.addParameter (STR16 ("ParamWhichCouldBeHidden"), STR16 (""), 0, 0,
		                         ParameterInfo::kCanAutomate, kParamWhichCouldBeHiddenTag,
		                         kUnit2Id);
		parameters.addParameter (STR16 ("TriggerHidden"), STR16 (""), 1, 0, ParameterInfo::kNoFlags,
		                         kTriggerHiddenTag);

		parameters.addParameter (STR16 ("Copy2Clipboard"), STR16 (""), 1, 0,
		                         ParameterInfo::kIsHidden, kCopy2ClipboardTag);

		parameters.addParameter (STR16 ("ParamRandomize"), STR16 (""), 1, 0,
		                         ParameterInfo::kNoFlags, kParamRandomizeTag);

		parameters.addParameter (STR16 ("ParamLowLatency"), STR16 (""), 1, 0,
		                         ParameterInfo::kNoFlags, kParamLowLatencyTag);

		parameters.addParameter (STR16 ("ParamProcessMode"), STR16 (""), 2, 0,
		                         ParameterInfo::kIsHidden, kParamProcessModeTag);

		//---ProcessContext parameters------------------------
		parameters.addParameter (new StringInt64Parameter (
		    STR16 ("ProjectTimeSamples"), kProcessContextProjectTimeSamplesTag, STR16 ("Samples"),
		    kUnitId, nullptr, ParameterInfo::kIsReadOnly));

		parameters
		    .addParameter (new RangeParameter (
		        STR16 ("ProjectTimeMusic"), kProcessContextProjectTimeMusicTag, STR16 ("PPQ"), -10,
		        kMaxInt32u, 0, 0, ParameterInfo::kIsReadOnly, kUnitId))
		    ->setPrecision (2);

		parameters
		    .addParameter (new RangeParameter (
		        STR16 ("BarPositionMusic"), kProcessContextBarPositionMusicTag, STR16 ("PPQ"), -64,
		        kMaxInt32u, 0, 0, ParameterInfo::kIsReadOnly, kUnitId))
		    ->setPrecision (0);

		parameters
		    .addParameter (new RangeParameter (STR16 ("Tempo"), kProcessContextTempoTag,
		                                       STR16 ("BPM"), 0, 400, 120, 0,
		                                       ParameterInfo::kIsReadOnly, kUnitId))
		    ->setPrecision (3);
		parameters
		    .addParameter (new RangeParameter (STR16 ("SigNumerator"),
		                                       kProcessContextTimeSigNumeratorTag, STR16 (""), 1,
		                                       128, 4, 0, ParameterInfo::kIsReadOnly, kUnitId))
		    ->setPrecision (0);
		parameters
		    .addParameter (new RangeParameter (STR16 ("SigDenominator"),
		                                       kProcessContextTimeSigDenominatorTag, STR16 (""), 1,
		                                       128, 4, 0, ParameterInfo::kIsReadOnly, kUnitId))
		    ->setPrecision (0);

		parameters.addParameter (new StringInt64Parameter (STR16 ("State"), kProcessContextStateTag,
		                                                   STR16 (""), kUnitId, nullptr,
		                                                   ParameterInfo::kIsReadOnly));
		parameters.addParameter (
		    new StringInt64Parameter (STR16 ("ProjectSystemTime"), kProcessContextSystemTimeTag,
		                              STR16 ("ns"), kUnitId, nullptr, ParameterInfo::kIsReadOnly));
		parameters.addParameter (new StringInt64Parameter (
		    STR16 ("ProjectSystemTime"), kProcessContextContinousTimeSamplesTag, STR16 ("Samples"),
		    kUnitId, nullptr, ParameterInfo::kIsReadOnly));

		//--- ------------------------------
		for (uint32 i = 0; i < HostChecker::kParamWarnCount; i++)
		{
			parameters.addParameter (
			    STR16 ("ProcessWarn"), STR16 (""), HostChecker::kParamWarnStepCount, 0,
			    ParameterInfo::kIsReadOnly | ParameterInfo::kIsHidden, kProcessWarnTag + i);
		}

		auto addUnitFunc = [=] (const int32 unitId, const int32 parentUnitId, const int32 idx,
		                        const Steinberg::tchar* name) -> bool {
			UnitInfo unitInfo {};
			unitInfo.id = unitId;
			unitInfo.parentUnitId = parentUnitId; // attached to the root unit

			String128 index;
			Steinberg::UString (index, 128).printInt (idx);
			Steinberg::UString (unitInfo.name, USTRINGSIZE (unitInfo.name))
			    .assign (name)
			    .append (index);
			unitInfo.programListId = kNoProgramListId;
			auto unit = new Unit (unitInfo);
			return addUnit (unit);
		};

		auto addParamFunc = [=] (const int32 paramId, const int32 parentUnitId, const int32 idx,
		                         const Steinberg::tchar* name) -> void {

			String128 index;
			Steinberg::UString (index, 128).printInt (idx);
			String128 _name;
			Steinberg::UString (_name, 128).assign (name).append (index);

			parameters.addParameter (_name, STR16 (""), 0, 0, ParameterInfo::kNoFlags, paramId,
			                         parentUnitId);
		};

		int32 paramTagStart = kParamUnitStructStart;
		int32 unitIDStart = kUnitParamIdStart;
		for (uint32 i = 0; i < HostChecker::kParamUnitStruct1Count; i++)
		{
			int32 parentUnitId = unitIDStart;
			addUnitFunc (unitIDStart, kRootUnitId, i + 1, STR16 ("L1-Unit "));
			for (uint32 k = 0; k < 2; k++)
				addParamFunc (paramTagStart++, parentUnitId, k + 1, STR16 ("L1-Param "));

			unitIDStart++;

			for (uint32 j = 0; j < HostChecker::kParamUnitStruct2Count; j++)
			{
				int32 parentUnit2Id = unitIDStart;
				addUnitFunc (unitIDStart, parentUnitId, j + 1, STR16 ("L2-Unit "));

				for (uint32 k = 0; k < 2; k++)
					addParamFunc (paramTagStart++, parentUnit2Id, k + 1, STR16 ("L2-Param "));
				unitIDStart++;

				for (uint32 l = 0; l < HostChecker::kParamUnitStruct3Count; l++)
				{
					addUnitFunc (unitIDStart, parentUnit2Id, l + 1, STR16 ("L3-Unit "));

					for (uint32 k = 0; k < 2; k++)
						addParamFunc (paramTagStart++, unitIDStart, k + 1, STR16 ("L3-Param "));
					unitIDStart++;
				}
			}
		}
		mDataSource = VSTGUI::owned (new EventLogDataBrowserSource (this));
	}

	if (auto plugInterfaceSupport = U::cast<IPlugInterfaceSupport> (context))
	{
		addFeatureLog (kLogIdIPlugInterfaceSupportSupported);

		if (plugInterfaceSupport->isPlugInterfaceSupported (IAutomationState::iid) == kResultTrue)
			addFeatureLog (kLogIdIAutomationStateSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (IEditControllerHostEditing::iid) ==
		    kResultTrue)
			addFeatureLog (kLogIdIEditControllerHostEditingSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (IMidiMapping::iid) == kResultTrue)
			addFeatureLog (kLogIdIMidiMappingSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (IMidiLearn::iid) == kResultTrue)
			addFeatureLog (kLogIdIMidiLearnSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (ChannelContext::IInfoListener::iid) ==
		    kResultTrue)
			addFeatureLog (kLogIdChannelContextSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (INoteExpressionController::iid) ==
		    kResultTrue)
			addFeatureLog (kLogIdINoteExpressionControllerSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (
		        INoteExpressionPhysicalUIMapping::iid) == kResultTrue)
			addFeatureLog (kLogIdINoteExpressionPhysicalUIMappingSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (IXmlRepresentationController::iid) ==
		    kResultTrue)
			addFeatureLog (kLogIdIXmlRepresentationControllerSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (IParameterFunctionName::iid) ==
		    kResultTrue)
			addFeatureLog (kLogIdIParameterFunctionNameSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (IComponentHandlerSystemTime::iid) ==
		    kResultTrue)
			addFeatureLog (kLogIdIComponentHandlerSystemTimeSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (IDataExchangeHandler::iid) ==
		    kResultTrue)
			addFeatureLog (kLogIdIDataExchangeHandlerSupported);

		if (plugInterfaceSupport->isPlugInterfaceSupported (IRemapParamID::iid) == kResultTrue)
			addFeatureLog (kLogIdIRemapParamIDSupported);
	}
	else
	{
		addFeatureLog (kLogIdIPlugInterfaceSupportNotSupported);
	}

	// check COM behavior (limited to IHostApplication for now)
	if (hostContext)
	{
		if (auto hostApp = U::cast<IHostApplication> (hostContext))
		{
			if (auto iUnknown = U::cast<FUnknown> (hostApp))
			{
				if (auto hostApp2 = U::cast<IHostApplication> (iUnknown))
				{
					SMTG_ASSERT (hostApp == hostApp2)
				}
				else
				{ // should deliver the right pointer normally!
					addFeatureLog (kLogWrongCOMBehaviorFUnknown1);
				}
			}
			else
			{ // should deliver the right pointer normally!
				addFeatureLog (kLogWrongCOMBehaviorFUnknown2);
			}
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::terminate ()
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::terminate"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdterminateCalledinWrongThread);
	}

	tresult result = EditControllerEx1::terminate ();
	if (result == kResultOk)
	{
		mDataSource = nullptr;
		mDataBrowserMap.clear ();
	}

	if (mProgressTimer)
	{
		mProgressTimer->forget ();
		mProgressTimer = nullptr;
	}
	return result;
}

//-----------------------------------------------------------------------------
float HostCheckerController::updateScoring (int64 iD)
{
	float score = 0;
	float total = 0;

	if (iD >= 0)
		mScoreMap[iD].use = true;

	for (auto& item : mScoreMap)
	{
		auto scoreEntry = item.second;
		total += scoreEntry.factor;
		if (scoreEntry.use)
			score += scoreEntry.factor;
	}
	if (total)
		score = score / total;
	else
		score = 0;

	if (auto val = parameters.getParameter (kScoreTag))
		val->setNormalized (score);

	return score;
}

//-----------------------------------------------------------------------------
void HostCheckerController::onProgressTimer (VSTGUI::CVSTGUITimer* /*timer*/)
{
	if (!mInProgress)
	{
		if (auto progress = U::cast<IProgress> (componentHandler))
			progress->start (IProgress::ProgressType::UIBackgroundTask, STR ("Test Progress"),
			                 mProgressID);
		mInProgress = true;
	}
	else
	{
		const float stepInc = 1.0 / 60.0 / 5.0; // ~5sec
		auto newVal = parameters.getParameter (kProgressValueTag)->getNormalized () + stepInc;
		// we have finished
		if (newVal > 1)
			setParamNormalized (kTriggerProgressTag, 0);
		else
		{
			setParamNormalized (kProgressValueTag, newVal);

			if (auto progress = U::cast<IProgress> (componentHandler))
				progress->update (mProgressID, newVal);
		}
	}
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setComponentState (IBStream* state)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::setComponentState"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdSetComponentStateCalledinWrongThread);
	}

	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	// detect state from HostChecker or AGain
	auto pos = streamer.tell ();
	auto end = streamer.seek (0, FSeekMode::kSeekEnd);
	bool stateFromAGain = (end - pos == 3 * 4);
	streamer.seek (pos, FSeekMode::kSeekSet);

	// We inform the host that we need a remapping
	if (stateFromAGain)
	{
		componentHandler->restartComponent (kParamIDMappingChanged);

		// we should read the AGain state here
		// TODO
		return kResultOk;
	}

	// version
	uint32 version;
	streamer.readInt32u (version);
	if (version < 1 || version > 1000)
	{
		version = 1;
		streamer.seek (-4, kSeekCurrent);
	}

	float saved = 0.f;
	if (streamer.readFloat (saved) == false)
		return kResultFalse;
	if (saved != 12345.67f)
	{
		SMTG_ASSERT (false)
	}

	uint32 latency;
	if (streamer.readInt32u (latency) == false)
		return kResultFalse;

	uint32 bypass;
	if (streamer.readInt32u (bypass) == false)
		return kResultFalse;

	float processingLoad = 0.f;
	if (version > 1)
	{
		if (streamer.readFloat (processingLoad) == false)
			return kResultFalse;
		setParamNormalized (kProcessingLoadTag, processingLoad);
	}

	setParamNormalized (kBypassTag, bypass > 0 ? 1 : 0);

	return kResultOk;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getUnitByBus (MediaType type, BusDirection dir,
                                                        int32 busIndex, int32 channel,
                                                        UnitID& unitId /*out*/)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::getUnitByBus"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdGetUnitByBusCalledinWrongThread);
	}

	if (type == kEvent && dir == kInput)
	{
		if (busIndex == 0 && channel == 0)
		{
			unitId = kRootUnitId;
			return kResultTrue;
		}
	}
	addFeatureLog (kLogIdGetUnitByBusSupported);
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setComponentHandler (IComponentHandler* handler)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::setComponentHandler"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdSetComponentHandlerCalledinWrongThread);
	}

	tresult res = EditControllerEx1::setComponentHandler (handler);
	if (componentHandler2)
	{
		addFeatureLog (kLogIdIComponentHandler2Supported);

		if (componentHandler2->requestOpenEditor () == kResultTrue)
			addFeatureLog (kLogIdIComponentHandler2RequestOpenEditorSupported);
	}

	if (auto handler3 = U::cast<IComponentHandler3> (componentHandler))
		addFeatureLog (kLogIdIComponentHandler3Supported);

	if (auto componentHandlerBusActivationSupport =
	        U::cast<IComponentHandlerBusActivation> (componentHandler))
		addFeatureLog (kLogIdIComponentHandlerBusActivationSupported);

	if (auto progress = U::cast<IProgress> (componentHandler))
		addFeatureLog (kLogIdIProgressSupported);

	return res;
}

//-----------------------------------------------------------------------------
int32 PLUGIN_API HostCheckerController::getUnitCount ()
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::getUnitCount"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdGetUnitCountCalledinWrongThread);
	}

	addFeatureLog (kLogIdUnitSupported);
	return EditControllerEx1::getUnitCount ();
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setParamNormalized (ParamID tag, ParamValue value)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::setParamNormalized"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdSetParamNormalizedCalledinWrongThread);
	}

	//--- ----------------------------------------
	if (tag == kLatencyTag && mLatencyInEdit)
	{
		mWantedLatency = value;
		// return kResultTrue;
	}
	//--- ----------------------------------------
	else if (tag == kProcessingLoadTag)
	{
	}
	//--- ----------------------------------------
	else if (tag == kParamProcessModeTag)
	{
	}
	//--- ----------------------------------------
	else if (tag == kParamRandomizeTag)
	{
		addFeatureLog (kLogIdIParameterFunctionNameRandomizeSupported);
	}
	//--- ----------------------------------------
	else if (tag == kParamLowLatencyTag)
	{
		addFeatureLog (kLogIdIParameterFunctionNameLowLatencySupported);
	}
	//--- ----------------------------------------
	else if (tag == kTriggerHiddenTag)
	{
		auto param = parameters.getParameter (kParamWhichCouldBeHiddenTag);
		auto& info = param->getInfo ();
		if (value > 0.5)
		{
			info.flags |= (ParameterInfo::kIsHidden | ParameterInfo::kIsReadOnly);
			info.flags &= ~ParameterInfo::kCanAutomate;
		}
		else
		{
			info.flags &= (~(ParameterInfo::kIsHidden | ParameterInfo::kIsReadOnly) |
			               ParameterInfo::kCanAutomate);
		}
		auto res = EditControllerEx1::setParamNormalized (tag, value);
		componentHandler->restartComponent (kParamTitlesChanged);
		return res;
	}
	else if (tag == kTriggerProgressTag)
	{
		if (value > 0.5)
		{
			if (mProgressTimer == nullptr)
				mProgressTimer =
				    new CVSTGUITimer ([this] (CVSTGUITimer* timer) { onProgressTimer (timer); },
				                      1000 / 60); // 60 Hz
			mProgressTimer->stop ();
			mProgressTimer->start ();
		}
		else
		{
			if (mProgressTimer)
				mProgressTimer->stop ();
			setParamNormalized (kProgressValueTag, 0);
			mInProgress = false;

			if (auto progress = U::cast<IProgress> (componentHandler))
				progress->finish (mProgressID);
		}
	}
	//--- ----------------------------------------
	else if (tag >= kProcessWarnTag && tag <= kProcessWarnTag + HostChecker::kParamWarnCount)
	{
		bool latencyRestartWanted = false;
		int32 tagOffset = (tag - kProcessWarnTag) * HostChecker::kParamWarnBitCount;
		uint32 idValue = static_cast<uint32> (value * HostChecker::kParamWarnStepCount);
		for (uint32 i = 0; i < HostChecker::kParamWarnBitCount; i++)
		{
			if (idValue & (1L << i))
			{
				addFeatureLog (tagOffset + i);
				if (tagOffset + i == kLogIdInformLatencyChanged && componentHandler)
					latencyRestartWanted = true;
			}
		}
		if (latencyRestartWanted)
			componentHandler->restartComponent (kLatencyChanged);
	}
	//--- ----------------------------------------
	else if (tag == kRestartKeyswitchChangedTag)
	{
		if (value > 0.)
		{
			if (componentHandler->restartComponent (kKeyswitchChanged) == kResultTrue)
			{
				addFeatureLog (kLogIdRestartKeyswitchChangedSupported);
			}
			mNumKeyswitch++;
			if (mNumKeyswitch > 10)
				mNumKeyswitch = 0;
			EditControllerEx1::setParamNormalized (tag, value);
			value = 0;
		}
	}
	//--- ----------------------------------------
	else if (tag == kRestartNoteExpressionChangedTag)
	{
		if (value > 0.)
		{
			if (componentHandler->restartComponent (kNoteExpressionChanged) == kResultTrue)
			{
				addFeatureLog (kLogIdRestartNoteExpressionChangedSupported);
			}
			EditControllerEx1::setParamNormalized (tag, value);
			value = 0;
		}
	}
	//--- ----------------------------------------
	else if (tag == kRestartParamValuesChangedTag)
	{
		if (value > 0.)
		{
			if (componentHandler->restartComponent (kParamValuesChanged) == kResultTrue)
			{
				addFeatureLog (kLogIdRestartParamValuesChangedSupported);
			}
			EditControllerEx1::setParamNormalized (tag, value);
			value = 0;
		}
	}
	//--- ----------------------------------------
	else if (tag == kRestartParamTitlesChangedTag)
	{
		if (value > 0.)
		{
			if (componentHandler->restartComponent (kParamTitlesChanged) == kResultTrue)
			{
				addFeatureLog (kLogIdRestartParamTitlesChangedSupported);
			}
			EditControllerEx1::setParamNormalized (tag, value);
			value = 0;
		}
	}

	//--- ----------------------------------------
	else if (tag == kCopy2ClipboardTag)
	{
		if (mDataSource && value > 0.)
		{
			std::ostringstream s;
			auto val = parameters.getParameter (kScoreTag);
			if (val)
			{
				s << "/* VST3 Hostname: ";
				if (auto hostApp = U::cast<IHostApplication> (hostContext))
				{
					Vst::String128 name;
					if (hostApp->getName (name) == kResultTrue)
						s << Vst::StringConvert::convert (name);
				}
				s << ", Scoring=";
				s << int32 (val->getNormalized () * 100 + 0.5);
				s << " (checking ";
				s << kVstVersionString;
				s << ")*/\n";
			}

			s << "ID,Severity,Description,Count\n";
			auto list = mDataSource->getLogEvents ();
			int32 i = 0;
			for (auto& item : list)
			{
				// if (item.count > 0)
				{
					s << item.id;
					s << ", ";
					s << logEventSeverity[item.id];
					s << ", ";
					s << logEventDescriptions[i];
					s << ", ";
					s << item.count;
					s << "\n";
				}
				i++;
			}
			SystemClipboard::copyTextToClipboard (s.str ());
			EditControllerEx1::setParamNormalized (tag, value);
			value = 0;
		}
	}

	return EditControllerEx1::setParamNormalized (tag, value);
}

//------------------------------------------------------------------------
tresult HostCheckerController::beginEdit (ParamID tag)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::beginEdit"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdBeginEditCalledinWrongThread);
	}

	if (tag == kLatencyTag)
		mLatencyInEdit = true;

	return EditControllerEx1::beginEdit (tag);
}

//-----------------------------------------------------------------------------
tresult HostCheckerController::endEdit (ParamID tag)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::endEdit"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdEndEditCalledinWrongThread);
	}

	if (tag == kLatencyTag && mLatencyInEdit)
	{
		mLatencyInEdit = false;
		setParamNormalized (tag, mWantedLatency);
	}
	return EditControllerEx1::endEdit (tag);
}

//-----------------------------------------------------------------------------
IPlugView* PLUGIN_API HostCheckerController::createView (FIDString name)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::createView"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdCreateViewCalledinWrongThread);
	}

	if (auto componentHandlerBusActivationSupport =
	        U::cast<IComponentHandlerBusActivation> (componentHandler))
		addFeatureLog (kLogIdIComponentHandlerBusActivationSupported);

	if (ConstString (name) == ViewType::kEditor)
	{
		if (componentHandler2)
		{
			if (componentHandler2->setDirty (true) == kResultTrue)
				addFeatureLog (kLogIdIComponentHandler2SetDirtySupported);
		}

		auto view = new MyVST3Editor (this, "HostCheckerEditor", "hostchecker.uidesc");
		if (sizeFactor != 0)
		{
			ViewRect rect (0, 0, width, height);
			view->setRect (rect);
			view->setZoomFactor (sizeFactor);
		}
		view->canResize (parameters.getParameter (kCanResizeTag)->getNormalized () > 0);

		return view;
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
CView* HostCheckerController::createCustomView (UTF8StringPtr name,
                                                const UIAttributes& /*attributes*/,
                                                const IUIDescription* /*description*/,
                                                VST3Editor* editor)
{
	if (ConstString (name) == "HostCheckerDataBrowser")
	{
		auto item = mDataBrowserMap.find (editor);
		if (item != mDataBrowserMap.end ())
		{
			item->second->remember ();
			return item->second;
		}

		auto dataBrowser = VSTGUI::owned (
		    new CDataBrowser (CRect (0, 0, 100, 100), mDataSource,
		                      CDataBrowser::kDrawRowLines | CDataBrowser::kDrawColumnLines |
		                          CDataBrowser::kDrawHeader | CDataBrowser::kVerticalScrollbar));

		mDataBrowserMap.emplace (editor, dataBrowser);
		dataBrowser->remember ();
		return dataBrowser;
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
void HostCheckerController::willClose (VSTGUI::VST3Editor* editor)
{
	mDataBrowserMap.erase (editor);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::connect (IConnectionPoint* other)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::connect"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdConnectCalledinWrongThread);
	}

	tresult tResult = ComponentBase::connect (other);
	if (peerConnection)
	{
		for (int32 paramIdx = 0; paramIdx < getParameterCount (); ++paramIdx)
		{
			ParameterInfo paramInfo = {};
			if (getParameterInfo (paramIdx, paramInfo) == kResultOk)
			{
				if (auto newMsg = owned (allocateMessage ()))
				{
					newMsg->setMessageID ("Parameter");
					if (IAttributeList* attr = newMsg->getAttributes ())
					{
						attr->setInt ("ID", paramInfo.id);
					}
					sendMessage (newMsg);
				}
			}
		}

		if (auto proc = U::cast<IAudioProcessor> (other))
		{
			if (auto newMsg = owned (allocateMessage ()))
			{
				newMsg->setMessageID ("LogEvent");
				if (IAttributeList* attr = newMsg->getAttributes ())
				{
					attr->setInt ("ID", kLogIdProcessorControllerConnection);
					attr->setInt ("Count", 1);
				}

				notify (newMsg);
			}
		}
	}

	return tResult;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::notify (IMessage* message)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::notify"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdnotifyCalledinWrongThread);
	}

	if (!message)
		return kInvalidArgument;

	if (FIDStringsEqual (message->getMessageID (), "LogEvent"))
	{
		int64 id;
		if (message->getAttributes ()->getInt ("ID", id) != kResultOk)
			return kResultFalse;
		int64 count;
		if (message->getAttributes ()->getInt ("Count", count) != kResultOk)
			return kResultFalse;
		addFeatureLog (id, static_cast<int32> (count), false);
	}

	if (FIDStringsEqual (message->getMessageID (), "Latency"))
	{
		ParamValue value;
		if (message->getAttributes ()->getFloat ("Value", value) == kResultOk)
		{
			componentHandler->restartComponent (kLatencyChanged);
		}
	}

	if (dataExchange.onMessage (message))
		return kResultOk;

	return ComponentBase::notify (message);
}

//-----------------------------------------------------------------------------
void HostCheckerController::addFeatureLog (int64 iD, int32 count, bool addToLastCount)
{
	updateScoring (iD);

	if (!mDataSource)
		return;

	LogEvent logEvt;
	logEvt.id = iD;
	logEvt.count = count;

	if (mDataSource->updateLog (logEvt, addToLastCount))
	{
		for (auto& item : mDataBrowserMap)
			item.second->invalidateRow (static_cast<int32_t> (logEvt.id));
	}
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setKnobMode (KnobMode mode)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::setKnobMode"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdsetKnobModeCalledinWrongThread);
	}

	addFeatureLog (kLogIdSetKnobModeSupported);
	return EditControllerEx1::setKnobMode (mode);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::openHelp (TBool onlyCheck)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::openHelp"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdopenHelpCalledinWrongThread);
	}

	addFeatureLog (kLogIdOpenHelpSupported);
	return EditControllerEx1::openHelp (onlyCheck);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::openAboutBox (TBool onlyCheck)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::openAboutBox"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdopenAboutBoxCalledinWrongThread);
	}

	addFeatureLog (kLogIdOpenAboutBoxSupported);
	return EditControllerEx1::openAboutBox (onlyCheck);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setChannelContextInfos (IAttributeList* list)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::setChannelContextInfos"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdsetChannelContextInfosCalledinWrongThread);
	}

	if (!list)
		return kResultFalse;

	// optional we can ask for the Channel Name Length
	int64 length;
	if (list->getInt (ChannelContext::kChannelNameLengthKey, length) == kResultTrue)
	{
	}

	// get the Channel Name where we, as plug-in, are instantiated
	String128 name;
	if (list->getString (ChannelContext::kChannelNameKey, name, sizeof (name)) == kResultTrue)
	{
	}

	// get the Channel UID
	if (list->getString (ChannelContext::kChannelUIDKey, name, sizeof (name)) == kResultTrue)
	{
	}

	// get Channel Index
	int64 index;
	if (list->getInt (ChannelContext::kChannelIndexKey, index) == kResultTrue)
	{
	}

	// get the Channel Color
	int64 color;
	if (list->getInt (ChannelContext::kChannelColorKey, color) == kResultTrue)
	{
		//	ColorSpec channelColor = (ColorSpec)color;
	}

	addFeatureLog (kLogIdChannelContextSupported);

	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getXmlRepresentationStream (
    RepresentationInfo& info /*in*/, IBStream* stream /*out*/)
{
	if (!threadChecker->test (
	        THREAD_CHECK_MSG ("HostCheckerController::getXmlRepresentationStream"),
	        THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdgetXmlRepresentationStreamCalledinWrongThread);
	}

	addFeatureLog (kLogIdIXmlRepresentationControllerSupported);

	String name (info.name);
	if (name == GENERIC_8_CELLS)
	{
		XmlRepresentationHelper helper (info, "Steinberg Media Technologies", "VST3 Host Checker",
		                                HostCheckerProcessorUID.toTUID (), stream);

		helper.startPage ("Main Page");
		helper.startEndCellOneLayer (LayerType::kKnob, 0);
		helper.startEndCellOneLayer (LayerType::kKnob, 1);
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.endPage ();

		helper.startPage ("Page 2");
		helper.startEndCellOneLayer (LayerType::kSwitch, 2);
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty
		helper.endPage ();

		return kResultTrue;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getMidiControllerAssignment (
    int32 busIndex, int16 /*channel*/, CtrlNumber midiControllerNumber, ParamID& id)
{
	if (!threadChecker->test (
	        THREAD_CHECK_MSG ("HostCheckerController::getMidiControllerAssignment"),
	        THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdgetMidiControllerAssignmentCalledinWrongThread);
	}

	addFeatureLog (kLogIdIMidiMappingSupported);

	if (busIndex != 0)
		return kResultFalse;

	switch (midiControllerNumber)
	{
		case ControllerNumbers::kCtrlPan: id = kProcessingLoadTag; return kResultOk;
		case ControllerNumbers::kCtrlExpression: id = kGeneratePeaksTag; return kResultOk;
		case ControllerNumbers::kCtrlEffect1: id = kBypassTag; return kResultOk;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::onLiveMIDIControllerInput (int32 /*busIndex*/,
                                                                     int16 /*channel*/,
                                                                     CtrlNumber /*midiCC*/)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::onLiveMIDIControllerInput"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdOnLiveMIDIControllerInputCalledinWrongThread);
	}

	addFeatureLog (kLogIdIMidiLearn_onLiveMIDIControllerInputSupported);
	return kResultTrue;
}

//-----------------------------------------------------------------------------
int32 PLUGIN_API HostCheckerController::getNoteExpressionCount (int32 /*busIndex*/,
                                                                int16 /*channel*/)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::getNoteExpressionCount"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdgetNoteExpressionCountCalledinWrongThread);
	}

	addFeatureLog (kLogIdINoteExpressionControllerSupported);
	return 1;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getNoteExpressionInfo (int32 /*busIndex*/,
                                                                 int16 /*channel*/,
                                                                 int32 noteExpressionIndex,
                                                                 NoteExpressionTypeInfo& info)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::getNoteExpressionInfo"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdgetNoteExpressionInfoCalledinWrongThread);
	}
	if (noteExpressionIndex == 0)
	{
		UString (info.title, USTRINGSIZE (info.title)).assign (USTRING ("Volume"));
		UString (info.shortTitle, USTRINGSIZE (info.shortTitle)).assign (USTRING ("Vol"));
		UString (info.units, USTRINGSIZE (info.units)).assign (USTRING ("dB"));
		info.typeId = kVolumeTypeID;
		info.unitId = -1;
		info.valueDesc;
		info.associatedParameterId = kNoParamId;
		info.flags = 0;

		return kResultTrue;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getNoteExpressionStringByValue (
    int32 /*busIndex*/, int16 /*channel*/, NoteExpressionTypeID id,
    NoteExpressionValue valueNormalized, String128 string)
{
	if (!threadChecker->test (
	        THREAD_CHECK_MSG ("HostCheckerController::getNoteExpressionStringByValue"),
	        THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdgetNoteExpressionStringByValueCalledinWrongThread);
	}
	addFeatureLog (kLogIdGetNoteExpressionStringByValueSupported);

	if (id == kVolumeTypeID)
	{
		char text[32];
		snprintf (text, 32, "%d", (int32) (100 * valueNormalized + 0.5));
		Steinberg::UString (string, 128).fromAscii (text);

		return kResultTrue;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getNoteExpressionValueByString (
    int32 /*busIndex*/, int16 /*channel*/, NoteExpressionTypeID id, const TChar* string,
    NoteExpressionValue& valueNormalized)
{
	if (!threadChecker->test (
	        THREAD_CHECK_MSG ("HostCheckerController::getNoteExpressionValueByString"),
	        THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdgetNoteExpressionValueByStringCalledinWrongThread);
	}
	addFeatureLog (kLogIdGetNoteExpressionValueByStringSupported);

	if (id == kVolumeTypeID)
	{
		String wrapper ((TChar*)string);
		double tmp = 0.0;
		if (wrapper.scanFloat (tmp))
		{
			valueNormalized = tmp / 100.;
			return kResultTrue;
		}
	}

	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getPhysicalUIMapping (int32 busIndex, int16 channel,
                                                                PhysicalUIMapList& list)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::getPhysicalUIMapping"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdgetPhysicalUIMappingCalledinWrongThread);
	}

	addFeatureLog (kLogIdINoteExpressionPhysicalUIMappingSupported);

	if (busIndex == 0 && channel == 0)
	{
		for (uint32 i = 0; i < list.count; ++i)
		{
			if (kPUIXMovement == list.map[i].physicalUITypeID)
				list.map[i].noteExpressionTypeID = kVolumeTypeID;
		}
		return kResultTrue;
	}
	return kResultFalse;
}

//--- IKeyswitchController ---------------------------
//------------------------------------------------------------------------
int32 PLUGIN_API HostCheckerController::getKeyswitchCount (int32 /*busIndex*/, int16 /*channel*/)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::getKeyswitchCount"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdgetKeyswitchCountCalledinWrongThread);
	}

	addFeatureLog (kLogIdIKeyswitchControllerSupported);
	return mNumKeyswitch;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getKeyswitchInfo (int32 /*busIndex*/, int16 /*channel*/,
                                                            int32 keySwitchIndex,
                                                            KeyswitchInfo& info)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::getKeyswitchInfo"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdgetKeyswitchInfoCalledinWrongThread);
	}
	addFeatureLog (kLogIdIKeyswitchControllerSupported);
	if (keySwitchIndex < mNumKeyswitch)
	{
		String indexStr;
		indexStr.printInt64 (keySwitchIndex + int64 (1));

		info.typeId = kNoteOnKeyswitchTypeID;
		UString title (info.title, USTRINGSIZE (info.title));
		title.assign (USTRING ("Accentuation "));
		title.append (indexStr);
		UString shortTitle (info.shortTitle, USTRINGSIZE (info.shortTitle));
		shortTitle.assign (USTRING ("Acc"));
		shortTitle.append (indexStr);

		info.keyswitchMin = 2 * keySwitchIndex;
		info.keyswitchMax = info.keyswitchMin + 1;
		info.keyRemapped = -1;
		info.unitId = -1;
		info.flags = 0;
		return kResultTrue;
	}
	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setAutomationState (int32 /*state*/)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::setAutomationState"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdsetAutomationStateCalledinWrongThread);
	}

	addFeatureLog (kLogIdIAutomationStateSupported);
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::beginEditFromHost (ParamID paramID)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::beginEditFromHost"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdBeginEditFromHostCalledinWrongThread);
	}

	addFeatureLog (kLogIdIEditControllerHostEditingSupported);
	mEditFromHost[paramID]++;
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::endEditFromHost (ParamID paramID)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::endEditFromHost"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdEndEditFromHostCalledinWrongThread);
	}

	addFeatureLog (kLogIdIEditControllerHostEditingSupported);
	mEditFromHost[paramID]--;
	if (mEditFromHost[paramID] < 0)
	{
		addFeatureLog (kLogIdIEditControllerHostEditingMisused);
		mEditFromHost[paramID] = 0;
	}
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getParameterIDFromFunctionName (UnitID /*unitID*/,
                                                                          FIDString functionName,
                                                                          ParamID& paramID)
{
	addFeatureLog (kLogIdIParameterFunctionNameSupported);

	if (FIDStringsEqual (functionName, FunctionNameType::kDryWetMix))
	{
		addFeatureLog (kLogIdIParameterFunctionNameDryWetSupported);

		paramID = kProcessingLoadTag;
	}
	else if (FIDStringsEqual (functionName, FunctionNameType::kRandomize))
	{
		addFeatureLog (kLogIdIParameterFunctionNameRandomizeSupported);

		paramID = kParamRandomizeTag;
	}
	else if (FIDStringsEqual (functionName, FunctionNameType::kLowLatencyMode))
	{
		addFeatureLog (kLogIdIParameterFunctionNameLowLatencySupported);

		paramID = kParamLowLatencyTag;
	}
	else
		paramID = kNoParamId;

	return (paramID != kNoParamId) ? kResultTrue : kResultFalse;
}

//------------------------------------------------------------------------
void PLUGIN_API HostCheckerController::queueOpened (DataExchangeUserContextID /*userContextID*/,
                                                    uint32 /*blockSize*/,
                                                    TBool& /*dispatchOnBackgroundThread*/)
{
}

//------------------------------------------------------------------------
void PLUGIN_API HostCheckerController::queueClosed (DataExchangeUserContextID /*userContextID*/)
{
}

//------------------------------------------------------------------------
void PLUGIN_API HostCheckerController::onDataExchangeBlocksReceived (
    DataExchangeUserContextID /*userContextID*/, uint32 /*numBlocks*/, DataExchangeBlock* block,
    TBool /*onBackgroundThread*/)
{
	// note that we should compensate the timing using a queue and the current systemTime before
	// updating the values!
	if (auto pc = reinterpret_cast<ProcessContext*> (block->data))
	{
		if (auto systemTime = U::cast<IComponentHandlerSystemTime> (componentHandler))
		{
			// when the queue is implemented use this currentTime to find the correct ProcessContext
			// to use
			int64 currentSystemTime;
			systemTime->getSystemTime (currentSystemTime);
		}

		if (auto val = reinterpret_cast<StringInt64Parameter*> (
		        parameters.getParameter (kProcessContextProjectTimeSamplesTag)))
			val->setValue (pc->projectTimeSamples);
		if (auto val = reinterpret_cast<StringInt64Parameter*> (
		        parameters.getParameter (kProcessContextContinousTimeSamplesTag)))
			val->setValue (pc->continousTimeSamples);
		if (auto val = parameters.getParameter (kProcessContextProjectTimeMusicTag))
			val->setNormalized (val->toNormalized (pc->projectTimeMusic));
		if (auto val = parameters.getParameter (kProcessContextBarPositionMusicTag))
			val->setNormalized (val->toNormalized (pc->barPositionMusic));

		if (auto val = parameters.getParameter (kProcessContextTempoTag))
			val->setNormalized (val->toNormalized (pc->tempo));
		if (auto val = parameters.getParameter (kProcessContextTimeSigNumeratorTag))
			val->setNormalized (val->toNormalized (pc->timeSigNumerator));
		if (auto val = parameters.getParameter (kProcessContextTimeSigDenominatorTag))
			val->setNormalized (val->toNormalized (pc->timeSigDenominator));

		if (auto val = reinterpret_cast<StringInt64Parameter*> (
		        parameters.getParameter (kProcessContextStateTag)))
			val->setValue (pc->state);
		if (auto val = reinterpret_cast<StringInt64Parameter*> (
		        parameters.getParameter (kProcessContextSystemTimeTag)))
			val->setValue (pc->systemTime);
	}
}

//------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getCompatibleParamID (const TUID pluginToReplaceUID,
                                                                Vst::ParamID oldParamID,
                                                                Vst::ParamID& newParamID)
{
	addFeatureLog (kLogIdIRemapParamIDSupported);

	//--- We want to replace the AGain plug-in-------
	//--- check if the host is asking for remapping parameter of the AGain plug-in
	static const FUID AGainProcessorUID (0x84E8DE5F, 0x92554F53, 0x96FAE413, 0x3C935A18);
	FUID uidToCheck (FUID::fromTUID (pluginToReplaceUID));
	if (AGainProcessorUID != uidToCheck)
		return kResultFalse;

	//--- host wants to remap from AGain------------
	newParamID = kNoParamId;
	switch (oldParamID)
	{
		//--- map the kGainId (0) to our param kGeneratePeaksTag
		case 0:
		{
			newParamID = kGeneratePeaksTag;
			break;
		}
	}
	//--- return kResultTrue if the mapping happens------------
	return (newParamID == kNoParamId) ? kResultFalse : kResultTrue;
}

//------------------------------------------------------------------------
void HostCheckerController::extractCurrentInfo (EditorView* editor)
{
	auto rect = editor->getRect ();
	height = rect.getHeight ();
	width = rect.getWidth ();

	auto* vst3editor = dynamic_cast<VST3Editor*> (editor);
	if (vst3editor)
		sizeFactor = vst3editor->getZoomFactor ();
}

//------------------------------------------------------------------------
void HostCheckerController::editorRemoved (EditorView* editor)
{
	extractCurrentInfo (editor);
	editors.erase (std::find (editors.begin (), editors.end (), editor));

	editorsSubCtlerMap.erase (editor);
}

//------------------------------------------------------------------------
void HostCheckerController::editorDestroyed (EditorView* /*editor*/)
{
}

//------------------------------------------------------------------------
void HostCheckerController::editorAttached (EditorView* editor)
{
	editors.push_back (editor);
	extractCurrentInfo (editor);
}

//------------------------------------------------------------------------
VSTGUI::IController* HostCheckerController::createSubController (
    UTF8StringPtr name, const IUIDescription* /*description*/, VST3Editor* editor)
{
	if (UTF8StringView (name) == "EditorSizeController")
	{
		auto sizeFunc = [&] (float _sizeFactor) {
			sizeFactor = _sizeFactor;
			for (auto& editor : editors)
			{
				auto* vst3editor = dynamic_cast<VST3Editor*> (editor);
				if (vst3editor)
					vst3editor->setZoomFactor (sizeFactor);
			}
		};
		auto subController = new EditorSizeController (this, sizeFunc, sizeFactor);
		editorsSubCtlerMap.insert ({editor, subController});
		return subController;
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::queryInterface (const TUID iid, void** obj)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::queryInterface"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdQueryInterfaceCalledinWrongThread);
	}

	if (FUnknownPrivate::iidEqual (iid, IMidiMapping::iid))
	{
		addRef ();
		*obj = static_cast<IMidiMapping*> (this);
		addFeatureLog (kLogIdIMidiMappingSupported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, IEditController2::iid))
	{
		addRef ();
		*obj = static_cast<IEditController2*> (this);
		addFeatureLog (kLogIdIEditController2Supported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, IXmlRepresentationController::iid))
	{
		addRef ();
		*obj = static_cast<IXmlRepresentationController*> (this);
		addFeatureLog (kLogIdIXmlRepresentationControllerSupported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, ChannelContext::IInfoListener::iid))
	{
		addRef ();
		*obj = static_cast<ChannelContext::IInfoListener*> (this);
		addFeatureLog (kLogIdChannelContextSupported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, INoteExpressionController::iid))
	{
		addRef ();
		*obj = static_cast<INoteExpressionController*> (this);
		addFeatureLog (kLogIdINoteExpressionControllerSupported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, INoteExpressionPhysicalUIMapping::iid))
	{
		addRef ();
		*obj = static_cast<INoteExpressionPhysicalUIMapping*> (this);
		addFeatureLog (kLogIdINoteExpressionPhysicalUIMappingSupported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, IKeyswitchController::iid))
	{
		addRef ();
		*obj = static_cast<IKeyswitchController*> (this);
		addFeatureLog (kLogIdIKeyswitchControllerSupported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, IMidiLearn::iid))
	{
		addRef ();
		*obj = static_cast<IMidiLearn*> (this);
		addFeatureLog (kLogIdIMidiLearnSupported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, IAutomationState::iid))
	{
		addRef ();
		*obj = static_cast<IAutomationState*> (this);
		addFeatureLog (kLogIdIAutomationStateSupported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, IEditControllerHostEditing::iid))
	{
		addRef ();
		*obj = static_cast<IEditControllerHostEditing*> (this);
		addFeatureLog (kLogIdIEditControllerHostEditingSupported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, IParameterFunctionName::iid))
	{
		addRef ();
		*obj = static_cast<IParameterFunctionName*> (this);
		addFeatureLog (kLogIdIParameterFunctionNameSupported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, IDataExchangeReceiver::iid))
	{
		addRef ();
		*obj = static_cast<IDataExchangeReceiver*> (this);
		addFeatureLog (kLogIdIDataExchangeReceiverSupported);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual (iid, IRemapParamID::iid))
	{
		addRef ();
		*obj = static_cast<IRemapParamID*> (this);
		addFeatureLog (kLogIdIRemapParamIDSupported);
		return kResultOk;
	}

	return EditControllerEx1::queryInterface (iid, obj);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setState (IBStream* state)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::setState"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdsetStateCalledinWrongThread);
	}

	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	uint32 version = 1;
	if (streamer.readInt32u (version) == false)
		return kResultFalse;

	if (streamer.readInt32u (height) == false)
		return kResultFalse;
	if (streamer.readInt32u (width) == false)
		return kResultFalse;
	if (streamer.readDouble (sizeFactor) == false)
		return kResultFalse;

	for (auto& item : editorsSubCtlerMap)
	{
		item.second->setSizeFactor (sizeFactor);
	}

	// since version 2
	if (version > 1)
	{
		bool canResize = true;
		streamer.readBool (canResize);
		parameters.getParameter (kCanResizeTag)->setNormalized (canResize ? 1 : 0);
	}

	return kResultOk;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getState (IBStream* state)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerController::getState"),
	                          THREAD_CHECK_EXIT))
	{
		addFeatureLog (kLogIdgetStateCalledinWrongThread);
	}

	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	uint32 version = 2;
	streamer.writeInt32u (version);
	streamer.writeInt32u (height);
	streamer.writeInt32u (width);
	streamer.writeDouble (sizeFactor);

	// since version 2
	bool canResize = parameters.getParameter (kCanResizeTag)->getNormalized () > 0;
	streamer.writeBool (canResize);

	return kResultOk;
}

//-----------------------------------------------------------------------------
}
} // namespaces
