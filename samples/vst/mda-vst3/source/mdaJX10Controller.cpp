/*
 *  mdaJX10Controller.cpp
 *  mda-vst3
 *
 *  Created by Arne Scheffler on 6/14/08.
 *
 *  mda VST Plug-ins
 *
 *  Copyright (c) 2008 Paul Kellett
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "mdaJX10Controller.h"

namespace Steinberg {
namespace Vst {
namespace mda {

//-----------------------------------------------------------------------------
JX10Controller::JX10Controller ()
{
	addBypassParameter = false;
}

//-----------------------------------------------------------------------------
JX10Controller::~JX10Controller ()
{
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API JX10Controller::initialize (FUnknown* context)
{
	tresult res = BaseController::initialize (context);
	if (res == kResultTrue)
	{
		auto presetList = { "5th Sweep Pad",
						   "Echo Pad [SA]",
						   "Space Chimes [SA]",
						   "Solid Backing",
						   "Velocity Backing [SA]",
						   "Rubber Backing [ZF]",
						   "808 State Lead",
						   "Mono Glide",
						   "Detuned Techno Lead",
						   "Hard Lead [SA]",
						   "Bubble",
						   "Monosynth",
						   "Moogcury Lite",
						   "Gangsta Whine",
						   "Higher Synth [ZF]",
						   "303 Saw Bass",
						   "303 Square Bass",
						   "Analog Bass",
						   "Analog Bass 2",
						   "Low Pulses",
						   "Sine Infra-Bass",
						   "Wobble Bass [SA]",
						   "Squelch Bass",
						   "Rubber Bass [ZF]",
						   "Soft Pick Bass",
						   "Fretless Bass",
						   "Whistler",
						   "Very Soft Pad",
						   "Pizzicato",
						   "Synth Strings",
						   "Synth Strings 2",
						   "Leslie Organ",
						   "Click Organ",
						   "Hard Organ",
						   "Bass Clarinet",
						   "Trumpet",
						   "Soft Horn",
						   "Brass Section",
						   "Synth Brass",
						   "Detuned Syn Brass [ZF]",
						   "Power PWM",
						   "Water Velocity [SA]",
						   "Ghost [SA]",
						   "Soft E.Piano",
						   "Thumb Piano",
						   "Steel Drums [ZF]",
						   "Car Horn",
						   "Helicopter",
						   "Arctic Wind",
						   "Thip",
						   "Synth Tom",
						   "Squelchy Frog" };
		auto* presetParam = new IndexedParameter (
			USTRING ("Factory Presets"), USTRING ("%"), static_cast<int32> (presetList.size () - 1),
			0.0, ParameterInfo::kIsProgramChange | ParameterInfo::kIsList, kPresetParam);
		parameters.addParameter (presetParam);

		int32 i = 0;
		for (auto item : presetList)
			presetParam->setIndexString (i++, UString128 (item));

		ParamID pid = 0;
		parameters.addParameter (new ScaledParameter (USTRING ("OSC Mix"), USTRING ("%"), 0, 0.15, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("OSC Tune"), USTRING ("%"), 0, 0.6, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("OSC Fine"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, -100, 100));

		auto glideModeList = {"Poly", "Poly-Legato", "Poly-Glide",
		                      "Mono", "Mono-Legato", "Mono-Glide"};

		auto* glideModeParam = new IndexedParameter (
		    USTRING ("Glide"), nullptr, static_cast<int32> (glideModeList.size () - 1), 0,
		    ParameterInfo::kCanAutomate | ParameterInfo::kIsList, pid++);

		i = 0;
		for (auto item : glideModeList)
			glideModeParam->setIndexString (i++, UString128 (item));
		parameters.addParameter (glideModeParam);

		parameters.addParameter (new ScaledParameter (USTRING ("Gld Rate"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("Gld Bend"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("VCF Freq"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("VCF Reso"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("VCF Env"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("VCF LFO"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("VCF Vel"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("VCF Att"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("VCF Dec"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("VCF Sus"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("VCF Rel"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("ENV Att"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("ENV Dec"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("ENV Sus"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("ENV Rel"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("LFO Rate"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("Vibrato"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("Noise"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("Octave"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, -100, 100));
		parameters.addParameter (new ScaledParameter (USTRING ("Tuning"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, -100, 100));

		midiCCParamID[kCtrlModWheel] = kModWheelParam;
		parameters.addParameter (new ScaledParameter (USTRING("Mod Wheel"), USTRING("%"), 0, 0, 0, kModWheelParam, 0, 100));
		midiCCParamID[kPitchBend] = kPitchBendParam;
		parameters.addParameter (new ScaledParameter (USTRING("Pitch Bend"), USTRING("%"), 0, 0.5, 0, kPitchBendParam, -100, 100));
		midiCCParamID[kCtrlBreath] = kBreathParam;
		midiCCParamID[kCtrlFilterResonance] = kBreathParam;
		parameters.addParameter (new ScaledParameter (USTRING("Filter Mod+"), USTRING("%"), 0, 0., 0, kBreathParam, 0, 100));
		midiCCParamID[3] = kCtrler3Param;
		parameters.addParameter (new ScaledParameter (USTRING("Filter Mod-"), USTRING("%"), 0, 0., 0, kCtrler3Param, 0, 100));
		midiCCParamID[kCtrlExpression] = kCtrler3Param;
		parameters.addParameter (new ScaledParameter (USTRING("Filter Resonance"), USTRING("%"), 0, 0.5, 0, kExpressionParam, 0, 100));
		midiCCParamID[kAfterTouch] = kAftertouchParam;
		parameters.addParameter (new ScaledParameter (USTRING("Aftertouch"), USTRING("%"), 0, 0.5, 0, kAftertouchParam, 0, 100));
	}
	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API JX10Controller::terminate ()
{
	return BaseController::terminate ();
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API JX10Controller::setParamNormalized (ParamID tag, ParamValue value)
{
	tresult res = BaseController::setParamNormalized (tag, value);
	if (res == kResultOk && tag == kPresetParam) // preset change
	{
		int32 program = static_cast<int32> (parameters.getParameter (tag)->toPlain (value));
		for (int32 i = 0; i < 24; i++)
		{
			BaseController::setParamNormalized (i, JX10Processor::programParams[program][i]);
		}
		if (componentHandler)
			componentHandler->restartComponent (kParamValuesChanged);
	}
	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API JX10Controller::getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string)
{
	return BaseController::getParamStringByValue (tag, valueNormalized, string);
	/*
	UString128 result;
		switch (tag)
		{
			default:
				return BaseController::getParamStringByValue (tag, valueNormalized, string);
		}
		result.copyTo (string, 128);
		return kResultTrue;*/
	
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API JX10Controller::getParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized)
{
	return BaseController::getParamValueByString (tag, string, valueNormalized);
	/*
	switch (tag)
		{
			default:
				return BaseController::getParamValueByString (tag, string, valueNormalized);
		}
		return kResultFalse;*/
	
}

}}} // namespaces
