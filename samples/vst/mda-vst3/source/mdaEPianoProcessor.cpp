/*
 *  mdaEPianoProcessor.cpp
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

#include "mdaEPianoProcessor.h"
#include "mdaEPianoController.h"
#include "mdaEPianoData.h"

#include <cmath>

namespace Steinberg {
namespace Vst {
namespace mda {

#define NPARAMS 12       //number of parameters
#define SILENCE 0.0001f  //voice choking
#define WAVELEN 422414   //wave data bytes

float EPianoProcessor::programParams[][NPARAMS] = { 
	{ 0.500f, 0.500f, 0.500f, 0.500f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.146f, 0.000f },
	{ 0.500f, 0.500f, 1.000f, 0.800f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.146f, 0.500f },
	{ 0.500f, 0.500f, 0.000f, 0.000f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.246f, 0.000f },
	{ 0.500f, 0.500f, 0.500f, 0.500f, 0.250f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.246f, 0.000f },
	{ 0.500f, 0.500f, 0.500f, 0.500f, 0.750f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.246f, 0.000f }
};

//-----------------------------------------------------------------------------
EPianoProcessor::EPianoProcessor ()
: currentProgram (0)
{
	setControllerClass (EPianoController::uid);
	allocParameters (NPARAMS);
}

//-----------------------------------------------------------------------------
EPianoProcessor::~EPianoProcessor ()
{
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API EPianoProcessor::initialize (FUnknown* context)
{
	tresult res = Base::initialize (context);
	if (res == kResultTrue)
	{
		addEventInput (USTRING("MIDI in"), 1);
		addAudioOutput (USTRING("Stereo Out"), SpeakerArr::kStereo);

		ParamValue defValues[] = {0.500f, 0.500f, 0.500f, 0.500f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.146f, 0.000f};
		for (int32 i = 0; i < NPARAMS; i++)
			params[i] = defValues[i];

		muffvel = tune = sizevel = 0;
		waves = epianoData;

		//Waveform data and keymapping
		kgrp[ 0].root = 36;  kgrp[ 0].high = 39; //C1
		kgrp[ 3].root = 43;  kgrp[ 3].high = 45; //G1
		kgrp[ 6].root = 48;  kgrp[ 6].high = 51; //C2
		kgrp[ 9].root = 55;  kgrp[ 9].high = 57; //G2
		kgrp[12].root = 60;  kgrp[12].high = 63; //C3
		kgrp[15].root = 67;  kgrp[15].high = 69; //G3
		kgrp[18].root = 72;  kgrp[18].high = 75; //C4
		kgrp[21].root = 79;  kgrp[21].high = 81; //G4
		kgrp[24].root = 84;  kgrp[24].high = 87; //C5
		kgrp[27].root = 91;  kgrp[27].high = 93; //G5
		kgrp[30].root = 96;  kgrp[30].high =999; //C6

		kgrp[0].pos = 0;        kgrp[0].end = 8476;     kgrp[0].loop = 4400;  
		kgrp[1].pos = 8477;     kgrp[1].end = 16248;    kgrp[1].loop = 4903;  
		kgrp[2].pos = 16249;    kgrp[2].end = 34565;    kgrp[2].loop = 6398;  
		kgrp[3].pos = 34566;    kgrp[3].end = 41384;    kgrp[3].loop = 3938;  
		kgrp[4].pos = 41385;    kgrp[4].end = 45760;    kgrp[4].loop = 1633; //was 1636;  
		kgrp[5].pos = 45761;    kgrp[5].end = 65211;    kgrp[5].loop = 5245;  
		kgrp[6].pos = 65212;    kgrp[6].end = 72897;    kgrp[6].loop = 2937;  
		kgrp[7].pos = 72898;    kgrp[7].end = 78626;    kgrp[7].loop = 2203; //was 2204;  
		kgrp[8].pos = 78627;    kgrp[8].end = 100387;   kgrp[8].loop = 6368;  
		kgrp[9].pos = 100388;   kgrp[9].end = 116297;   kgrp[9].loop = 10452;  
		kgrp[10].pos = 116298;  kgrp[10].end = 127661;  kgrp[10].loop = 5217; //was 5220; 
		kgrp[11].pos = 127662;  kgrp[11].end = 144113;  kgrp[11].loop = 3099;  
		kgrp[12].pos = 144114;  kgrp[12].end = 152863;  kgrp[12].loop = 4284;  
		kgrp[13].pos = 152864;  kgrp[13].end = 173107;  kgrp[13].loop = 3916;  
		kgrp[14].pos = 173108;  kgrp[14].end = 192734;  kgrp[14].loop = 2937;  
		kgrp[15].pos = 192735;  kgrp[15].end = 204598;  kgrp[15].loop = 4732;  
		kgrp[16].pos = 204599;  kgrp[16].end = 218995;  kgrp[16].loop = 4733;  
		kgrp[17].pos = 218996;  kgrp[17].end = 233801;  kgrp[17].loop = 2285;  
		kgrp[18].pos = 233802;  kgrp[18].end = 248011;  kgrp[18].loop = 4098;  
		kgrp[19].pos = 248012;  kgrp[19].end = 265287;  kgrp[19].loop = 4099;  
		kgrp[20].pos = 265288;  kgrp[20].end = 282255;  kgrp[20].loop = 3609;  
		kgrp[21].pos = 282256;  kgrp[21].end = 293776;  kgrp[21].loop = 2446;  
		kgrp[22].pos = 293777;  kgrp[22].end = 312566;  kgrp[22].loop = 6278;  
		kgrp[23].pos = 312567;  kgrp[23].end = 330200;  kgrp[23].loop = 2283;  
		kgrp[24].pos = 330201;  kgrp[24].end = 348889;  kgrp[24].loop = 2689;  
		kgrp[25].pos = 348890;  kgrp[25].end = 365675;  kgrp[25].loop = 4370;  
		kgrp[26].pos = 365676;  kgrp[26].end = 383661;  kgrp[26].loop = 5225;  
		kgrp[27].pos = 383662;  kgrp[27].end = 393372;  kgrp[27].loop = 2811;  
		kgrp[28].pos = 383662;  kgrp[28].end = 393372;  kgrp[28].loop = 2811; //ghost
		kgrp[29].pos = 393373;  kgrp[29].end = 406045;  kgrp[29].loop = 4522;  
		kgrp[30].pos = 406046;  kgrp[30].end = 414486;  kgrp[30].loop = 2306;  
		kgrp[31].pos = 406046;  kgrp[31].end = 414486;  kgrp[31].loop = 2306; //ghost
		kgrp[32].pos = 414487;  kgrp[32].end = 422408;  kgrp[32].loop = 2169;  

		//extra xfade looping...
		for(int32 k=0; k<28; k++)
		{
			int32 p0 = kgrp[k].end;
			int32 p1 = kgrp[k].end - kgrp[k].loop;

			float xf = 1.0f;
			float dxf = -0.02f;

			while (xf > 0.0f)
			{
				waves[p0] = (short)((1.0f - xf) * (float)waves[p0] + xf * (float)waves[p1]);
				p0--;
				p1 --;
				xf += dxf;
			}
		}

		//initialise...
		for(int32 v=0; v<kNumVoices; v++) 
		{
			memset (&synthData.voice[v], 0, sizeof (VOICE));
			synthData.voice[v].env = 0.0f;
			synthData.voice[v].dec = 0.99f; //all notes off
		}
		volume = 0.2f;
		muff = 160.0f;
		synthData.sustain = synthData.activevoices = 0;
		tl = tr = lfo0 = dlfo = 0.0f;
		lfo1 = 1.0f;
		modwhl = 0.;
	}
	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API EPianoProcessor::terminate ()
{
	return Base::terminate ();
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API EPianoProcessor::setActive (TBool state)
{
	if (state)
	{
		Fs = static_cast<float> (getSampleRate ());
		iFs = 1.0f / Fs;
		dlfo = 6.283f * iFs * (float)exp ((float)(6.22f * params[5] - 2.61f)); //lfo rate
		recalculate ();
	}
	return Base::setActive (state);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API EPianoProcessor::setProcessing (TBool state)
{
	if (state)
	{
		synthData.init (); 
		synthData.clearEvents ();
	}
	
	Base::setProcessing (state);
	return kResultTrue;
}

//-----------------------------------------------------------------------------
void EPianoProcessor::setParameter (ParamID index, ParamValue newValue, int32 sampleOffset)
{
	if (index < NPARAMS)
		Base::setParameter (index, newValue, sampleOffset);
	else if (index == BaseController::kPresetParam) // program change
	{
		int32 program = (int32)(newValue * kNumPrograms);
		const float* newParams = programParams[program];
		if (newParams)
		{
			for (int32 i = 0; i < NPARAMS; i++)
				params[i] = newParams[i];
			recalculate ();
		}
	}
	else if (index == BaseController::kModWheelParam) // mod wheel
	{
		newValue *= 127.;
		modwhl = static_cast<float> (0.0078f * newValue);
	}
	else if (index == BaseController::kSustainParam)
	{
		synthData.sustain = newValue > 0.5;
		if (synthData.sustain==0)
		{
			for (auto& v : synthData.voice)
			{
				if (v.noteID = SustainNoteID)
				{
					v.dec = (float)exp (-iFs * exp (6.0 + 0.01 * (double)v.note - 5.0 * params[1]));
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
void EPianoProcessor::setCurrentProgram (Steinberg::uint32 val)
{
	if (val < kNumPrograms)
		currentProgram = val;
}

//-----------------------------------------------------------------------------
void EPianoProcessor::setCurrentProgramNormalized (ParamValue val)
{
	setCurrentProgram (std::min<int32> (kNumPrograms - 1, (int32)(val * kNumPrograms)));
}

//-----------------------------------------------------------------------------
void EPianoProcessor::doProcessing (ProcessData& data)
{
	int32 sampleFrames = data.numSamples;
	
	float* out0 = data.outputs[0].channelBuffers32[0];
	float* out1 = data.outputs[0].channelBuffers32[1];

	int32 frame=0, frames, v;
	float x, l, r, od=overdrive;
	int32 i;

	synthData.eventPos = 0;
	if (synthData.activevoices > 0 || synthData.hasEvents ())
	{    
		while (frame<sampleFrames)
		{
			frames = synthData.events[synthData.eventPos].sampleOffset;
			if (frames>sampleFrames) frames = sampleFrames;
			frames -= frame;
			frame += frames;

			while (--frames>=0)
			{
				VOICE *V = synthData.voice.data ();
				l = r = 0.0f;

				for(v=0; v<synthData.activevoices; v++)
				{
					V->frac += V->delta;  //integer-based linear interpolation
					V->pos += V->frac >> 16;
					V->frac &= 0xFFFF;
					if (V->pos > V->end) V->pos -= V->loop;
					//i = waves[V->pos];
					//i = (i << 7) + (V->frac >> 9) * (waves[V->pos + 1] - i) + 0x40400000;  //not working on intel mac !?!
					//x = V->env * (*(float *)&i - 3.0f);  //fast int->float
					//x = V->env * (float)i / 32768.0f;      
					i = waves[V->pos] + ((V->frac * (waves[V->pos + 1] - waves[V->pos])) >> 16);
					x = V->env * (float)i / 32768.0f;

					V->env = V->env * V->dec;  //envelope

					if (x>0.0f) { x -= od * x * x;  if (x < -V->env) x = -V->env; } //+= 0.5f * x * x; } //overdrive

					l += V->outl * x;
					r += V->outr * x;

					V++;
				}
				tl += tfrq * (l - tl);  //treble boost
				tr += tfrq * (r - tr);
				r  += treb * (r - tr);
				l  += treb * (l - tl);

				lfo0 += dlfo * lfo1;  //LFO for tremolo and autopan
				lfo1 -= dlfo * lfo0;
				l += l * lmod * lfo1;
				r += r * rmod * lfo1;  //worth making all these local variables?

				*out0++ = l;
				*out1++ = r;
			}

			if (frame<sampleFrames)
			{
				if (synthData.activevoices == 0 && params[4] > 0.5f) 
					{ lfo0 = -0.7071f;  lfo1 = 0.7071f; } //reset LFO phase - good idea?
				noteEvent (synthData.events[synthData.eventPos]);
				++synthData.eventPos;
			}
		}
	}
	if (fabs (tl) < 1.0e-10) tl = 0.0f; //anti-denormal
	if (fabs (tr) < 1.0e-10) tr = 0.0f;

	for(v=0; v<synthData.activevoices; v++) if (synthData.voice[v].env < SILENCE) synthData.voice[v] = synthData.voice[--synthData.activevoices];
}

//-----------------------------------------------------------------------------
void EPianoProcessor::noteEvent (const Event& event)
{
	float l=99.0f;
	int32  v, vl=0, k, s;

	if (event.type == Event::kNoteOnEvent)
	{
		auto& noteOn = event.noteOn;
		auto note = noteOn.pitch;
		float velocity = noteOn.velocity * 127;
		if (synthData.activevoices < poly) //add a note
		{
			vl = synthData.activevoices;
			synthData.activevoices++;
			synthData.voice[vl].f0 = synthData.voice[vl].f1 = 0.0f;
		}
		else //steal a note
		{
			for(v=0; v<poly; v++)  //find quietest voice
			{
				if (synthData.voice[v].env < l) { l = synthData.voice[v].env;  vl = v; }
			}
		}

		k = (note - 60) * (note - 60);
		l = fine + random * ((float)(k % 13) - 6.5f);  //random & fine tune
		if (note > 60) l += stretch * (float)k; //stretch

		s = size;
		if (velocity > 40) s += (int32)(sizevel * (float)(velocity - 40));  

		k = 0;
		while (note > (kgrp[k].high + s)) k += 3;  //find keygroup
		l += (float)(note - kgrp[k].root); //pitch
		l = 32000.0f * iFs * (float)exp (0.05776226505 * l);
		synthData.voice[vl].delta = (int32)(65536.0f * l);
		synthData.voice[vl].frac = 0;

		if (velocity > 48) k++; //mid velocity sample
		if (velocity > 80) k++; //high velocity sample
		synthData.voice[vl].pos = kgrp[k].pos;
		synthData.voice[vl].end = kgrp[k].end - 1;
		synthData.voice[vl].loop = kgrp[k].loop;

		synthData.voice[vl].env = (3.0f + 2.0f * velsens) * (float)pow (0.0078f * velocity, velsens); //velocity

		if (note > 60) synthData.voice[vl].env *= (float)exp (0.01f * (float)(60 - note)); //new! high notes quieter

		l = static_cast<float> (50.0f + params[4] * params[4] * muff + muffvel * (float)(velocity - 64)); //muffle
		if (l < (55.0f + 0.4f * (float)note)) l = 55.0f + 0.4f * (float)note;
		if (l > 210.0f) l = 210.0f;
		synthData.voice[vl].ff = l * l * iFs;

		synthData.voice[vl].note = note; //note->pan
		if (note <  12) note = 12;
		if (note > 108) note = 108;
		l = volume;
		synthData.voice[vl].outr = l + l * width * (float)(note - 60);
		synthData.voice[vl].outl = l + l - synthData.voice[vl].outr;

		if (note < 44) note = 44; //limit max decay length
		synthData.voice[vl].dec = (float)exp (-iFs * exp (-1.0 + 0.03 * (double)note - 2.0f * params[0]));
		synthData.voice[vl].noteID = noteOn.noteId;
	}
	else //note off
	{
		auto& noteOff = event.noteOff;
		auto note = noteOff.pitch;
		for(v=0; v<synthData.numVoices; v++) if (synthData.voice[v].noteID==noteOff.noteId) //any voices playing that note?
		{
			if (synthData.sustain==0)
			{
				synthData.voice[v].dec = (float)exp (-iFs * exp (6.0 + 0.01 * (double)note - 5.0 * params[1]));
			}
			else synthData.voice[v].noteID = SustainNoteID;
		}
	}
}

//-----------------------------------------------------------------------------
void EPianoProcessor::preProcess ()
{
	synthData.clearEvents ();
}

//-----------------------------------------------------------------------------
void EPianoProcessor::processEvent (const Event& e)
{
	synthData.processEvent (e);
}

//-----------------------------------------------------------------------------
void EPianoProcessor::recalculate ()
{
	size = (int32)(12.0f * params[2] - 6.0f);

	treb = static_cast<float> (4.0f * params[3] * params[3] - 1.0f); //treble gain
	if (params[3] > 0.5f) tfrq = 14000.0f; else tfrq = 5000.0f; //treble freq
	tfrq = 1.0f - (float)exp (-iFs * tfrq);

	rmod = lmod = static_cast<float> (params[4] + params[4] - 1.0f); //lfo depth
	if (params[4] < 0.5f) rmod = -rmod;

	dlfo = 6.283f * iFs * (float)exp (6.22f * params[5] - 2.61f); //lfo rate

	velsens = static_cast<float> (1.0f + params[6] + params[6]);
	if (params[6] < 0.25f) velsens -= static_cast<float> (0.75f - 3.0f * params[6]);

	width = static_cast<float> (0.03f * params[7]);
	poly = 1 + (int32)(31.9f * params[8]);
	fine = static_cast<float> (params[9] - 0.5f);
	random = static_cast<float> (0.077f * params[10] * params[10]);
	stretch = 0.0f; //0.000434f * (params[11] - 0.5f); parameter re-used for overdrive!
	overdrive = static_cast<float> (1.8f * params[11]);

	if (modwhl > 0.05f) //over-ride pan/trem depth
	{
		rmod = lmod = modwhl; //lfo depth
		if (params[4] < 0.5f) rmod = -rmod;
	}
}

}}} // namespaces

