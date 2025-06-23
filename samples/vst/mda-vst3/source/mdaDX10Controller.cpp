/*
 *  mdaDX10Controller.cpp
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

#include "mdaDX10Controller.h"

namespace Steinberg {
namespace Vst {
namespace mda {

//-----------------------------------------------------------------------------
DX10Controller::DX10Controller ()
{
	addBypassParameter = false;
}

//-----------------------------------------------------------------------------
DX10Controller::~DX10Controller ()
{
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Controller::initialize (FUnknown* context)
{
	tresult res = BaseController::initialize (context);
	if (res == kResultTrue)
	{
		auto presetList = {"Bright E.Piano", "Jazz E.Piano", "E.Piano Pad", "Fuzzy E.Piano",
		                   "Soft Chimes",    "Harpsichord",  "Funk Clav",   "Sitar",
		                   "Chiff Organ",    "Tinkle",       "Space Pad",   "Koto",
		                   "Harp",           "Jazz Guitar",  "Steel Drum",  "Log Drum",
		                   "Trumpet",        "Horn",         "Reed 1",      "Reed 2",
		                   "Violin",         "Chunky Bass",  "E.Bass",      "Clunk Bass",
		                   "Thick Bass",     "Sine Bass",    "Square Bass", "Upright Bass 1",
		                   "Upright Bass 2", "Harmonics",    "Scratch",     "Syn Tom"};

		auto* presetParam = new IndexedParameter (
		    USTRING ("Factory Presets"), nullptr, static_cast<int32> (presetList.size () - 1), 0.0,
		    ParameterInfo::kIsProgramChange | ParameterInfo::kIsList, kPresetParam);
		parameters.addParameter (presetParam);

		int32 i = 0;
		for (auto item : presetList)
			presetParam->setIndexString (i++, UString128 (item));

		ParamID pid = 0;

		parameters.addParameter (new ScaledParameter (USTRING("Attack"), USTRING("%"), 0, 0.15, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING("Decay"), USTRING("%"), 0, 0.6, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING("Release"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (USTRING("Coarse"), USTRING("ratio"), 0, 0.5, ParameterInfo::kCanAutomate, pid++);
		parameters.addParameter (USTRING("Fine"), USTRING("ratio"), 0, 0.5, ParameterInfo::kCanAutomate, pid++);
		parameters.addParameter (new ScaledParameter (USTRING("Mod Init"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING("Mod Dec"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING("Mod Sus"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING("Mod Rel"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING("Mod Vel"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING("Vibrato"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (USTRING("Octave"), USTRING(""), 0, 0.5, ParameterInfo::kCanAutomate, pid++);
		parameters.addParameter (USTRING("FineTune"), USTRING("cents"), 0, 0.5, ParameterInfo::kCanAutomate, pid++);
		parameters.addParameter (new ScaledParameter (USTRING("WaveForm"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (new ScaledParameter (USTRING("Mod Thru"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100));
		parameters.addParameter (USTRING("LFO Rate"), USTRING("Hz"), 0, 0.5, ParameterInfo::kCanAutomate, pid++);

		midiCCParamID[kCtrlModWheel] = kModWheelParam;
		parameters.addParameter (USTRING("Mod Wheel"), USTRING(""), 0, 0, 0, kModWheelParam);
		midiCCParamID[kPitchBend] = kPitchBendParam;
		parameters.addParameter (USTRING("Pitch Bend"), USTRING(""), 0, 0.5, 0, kPitchBendParam);
	}
	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Controller::terminate ()
{
	return BaseController::terminate ();
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Controller::setParamNormalized (ParamID tag, ParamValue value)
{
	tresult res = BaseController::setParamNormalized (tag, value);
	if (res == kResultOk && tag == kPresetParam) // preset change
	{
		int32 program = static_cast<int32> (parameters.getParameter (tag)->toPlain (value));
		for (int32 i = 0; i < 16; i++)
		{
			BaseController::setParamNormalized (i, DX10Processor::programParams[program][i]);
		}
		if (componentHandler)
			componentHandler->restartComponent (kParamValuesChanged);
	}
	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Controller::getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string)
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

}}} // namespaces
