/*
 *  mdaTestToneProcessor.cpp
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

#include "mdaTestToneProcessor.h"
#include "mdaTestToneController.h"

#include <cmath>
#include <cstdlib>

namespace Steinberg {
namespace Vst {
namespace mda {

//-----------------------------------------------------------------------------
TestToneProcessor::TestToneProcessor ()
{
	setControllerClass (TestToneController::uid);
	allocParameters (8);
}

//-----------------------------------------------------------------------------
TestToneProcessor::~TestToneProcessor ()
{
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TestToneProcessor::initialize (FUnknown* context)
{
	tresult res = BaseProcessor::initialize (context);
	if (res == kResultTrue)
	{
		addAudioInput (USTRING("Stereo In"), SpeakerArr::kStereo);
		addAudioOutput (USTRING("Stereo Out"), SpeakerArr::kStereo);

		param (TestToneParam::Mode) = 0.47f; //mode
		param (TestToneParam::Level) = 0.71f; //level dB
		param (TestToneParam::Channel) = 0.50f; //pan dB
		param (TestToneParam::F1) = 0.57f; //freq1 B
		param (TestToneParam::F2) = 0.50f; //freq2 Hz
		param (TestToneParam::Thru) = 0.00f; //thru dB
		param (TestToneParam::Sweep) = 0.30f; //sweep ms
		param (TestToneParam::ZerodB) = 1.00f; //cal dBFS

		recalculate ();
	}
	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TestToneProcessor::terminate ()
{
	return BaseProcessor::terminate ();
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TestToneProcessor::setActive (TBool state)
{
	return BaseProcessor::setActive (state);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TestToneProcessor::setProcessing (TBool state)
{
	if (state)
	{
		zz0 = zz1 = zz2 = zz3 = zz4 = zz5 = phi = 0.0f;
	}

	BaseProcessor::setProcessing (state);
	return kResultOk;
}

//-----------------------------------------------------------------------------
void TestToneProcessor::doProcessing (ProcessData& data)
{
	int32 sampleFrames = data.numSamples;
	
	float* in1 = data.inputs[0].channelBuffers32[0];
	float* in2 = data.inputs[0].channelBuffers32[1];
	float* out1 = data.outputs[0].channelBuffers32[0];
	float* out2 = data.outputs[0].channelBuffers32[1];

	float a, b, x = 0.0f, twopi=6.2831853f;
	float z0=zz0, z1=zz1, z2=zz2, z3=zz3, z4=zz4, z5=zz5;
	float ph=phi, dph=dphi, l=left, r = right, t=thru;
	float s=sw, sx=swx, ds=swd, fsc=fscale;
	int32 st=swt;
	int32 m = mode;

	--in1;	
	--in2;
	--out1;
	--out2;
	while (--sampleFrames >= 0)
	{
		a = *++in1;	
		b = *++in2;

		switch (m)
		{
			case 1: if (st>0) { st--; x = 0.f; } else //impulse
					{ 
					x=1.f; 
					st= (int32)(len*getSampleRate ());
					}
					break;

			case 2: //noise
			#if WIN32  
			case 3: x = (float)(rand() - 16384); //for RAND_MAX = 32767
			#else //mac/gcc
			case 3: x = (float)((rand() & 0x7FFF) - 16384);
			#endif
					if (m==3)
					{
					z0 = 0.997f * z0 + 0.029591f * x; //pink filter
					z1 = 0.985f * z1 + 0.032534f * x;
					z2 = 0.950f * z2 + 0.048056f * x;
					z3 = 0.850f * z3 + 0.090579f * x;
					z4 = 0.620f * z4 + 0.108990f * x;
					z5 = 0.250f * z5 + 0.255784f * x;
					x = z0 + z1 + z2 + z3 + z4 + z5;
					} 
					break;

			case 4: x = 0.f; break; //mute

			case 0: //tones
			case 5:
			case 9: ph = (float)fmod(ph+dph,twopi); 
					x = (float)sin(ph); 
					break;

			case 6: //log sweep & step
			case 7: if (st>0) { st--; ph=0.f; } else
					{
					s += ds;
					if (m==7) dph = fsc * (float)pow (10.0f, 0.1f * (float)int (s));
					else dph = fsc * (float)pow (10.0f, 0.1f * s);
					x = (float)sin(ph);
					ph = (float)fmod(ph+dph,twopi);
					if (s>sx) { l=0.f; r=0.f; } 
					}
					break; 

			case 8: //lin sweep
					if (st>0) { st--; ph=0.f; } else
					{
					s += ds;
					x = (float)sin(ph);
					ph = (float)fmod(ph+s,twopi);
					if (s>sx) { l=0.f; r=0.f; } 
					} 
					break; 
		}

		*++out1 = t*a + l*x;
		*++out2 = t*b + r*x;
	}
	zz0=z0; zz1=z1; zz2=z2; zz3=z3, zz4=z4; zz5=z5; 
	phi=ph; sw=s; swt=st;
	if (s>sx) recalculate ();
}

//-----------------------------------------------------------------------------
void TestToneProcessor::recalculate ()
{
	float f, df, twopi=6.2831853f;

	//calcs here!	
	mode = std::min<int32> (8, static_cast<int32> (9 * param (TestToneParam::Mode)));
	left = 0.05f * (float)int (60.f*param (TestToneParam::Level));
	left = (float)pow (10.0f, left - 3.f);
	if (mode==2) left*=0.0000610f; //scale white for RAND_MAX = 32767
	if (mode==3) left*=0.0000243f; //scale pink for RAND_MAX = 32767
	int32 channel = std::min<int32> (2, static_cast<int32> (3 * param (TestToneParam::Channel)));
	switch (channel)
	{
		case 0: right = 0.f; break;
		case 1:	right = left; break;
		case 2: right = left; left = 0.f; break;
	}
	len = 1.f + 0.5f*(float)int (62*param (TestToneParam::Sweep));
	swt= (int32)(len*getSampleRate ());

	if (param (TestToneParam::ZerodB)>0.8) //output level trim
	{
		if (param (TestToneParam::ZerodB)>0.96) cal = 0.f;
		else if (param (TestToneParam::ZerodB)>0.92) cal = -0.01000001f;
		else if (param (TestToneParam::ZerodB)>0.88) cal = -0.02000001f;
		else if (param (TestToneParam::ZerodB)>0.84) cal = -0.1f;
		else cal = -0.2f;

		calx = (float)pow (10.0f, 0.05f*cal); 
		left*=calx; right*=calx; 
		calx = 0.f;
	}
	else //output level calibrate
	{
		cal = (float)int (25.f*param (TestToneParam::ZerodB) - 21.1f);
		calx = cal;
	}

	df=0.f;
	if (param (TestToneParam::F2)>0.6) df = static_cast<float> (1.25f*param (TestToneParam::F2) - 0.75f);
	if (param (TestToneParam::F2)<0.4) df = static_cast<float> (1.25f * param (TestToneParam::F2) - 0.50f);

	switch (mode)
	{
		case 0: //MIDI note
				f = static_cast<float> (std::min<int32> (128, static_cast<int32> (129 * param (TestToneParam::F1))));
				dphi = static_cast<float> (51.37006f*powf (1.0594631f,f+df)/getSampleRate ());
				break;

		case 5: //sine
				f = 13.f + std::min<int32> (30, static_cast<int32> (31 * param (TestToneParam::F1)));
				f= powf (10.0f, 0.1f*(f+df));
				dphi= static_cast<float> (twopi*f/getSampleRate ());
				break;

		case 6: //log sweep & step        
		case 7: sw = 13.f + std::min<int32> (30, static_cast<int32> (31 * param (TestToneParam::F1)));
				swx = 13.f + std::min<int32> (30, static_cast<int32> (31 * param (TestToneParam::F2)));
				if (sw>swx) { swd=swx; swx=sw; sw=swd; } //only sweep up
				if (mode==7) swx += 1.f;
				swd = static_cast<float> ((swx-sw) / (len*getSampleRate ()));
				swt= 2 * (int32)getSampleRate ();
				break; 

		case 8: //lin sweep
				sw = 200.f * (float)floor(100.f*param (TestToneParam::F1));
				swx = 200.f * (float)floor(100.f*param (TestToneParam::F2));
				if (sw>swx) { swd=swx; swx=sw; sw=swd; } //only sweep up
				sw = static_cast<float> (twopi*sw/getSampleRate ());
				swx = static_cast<float> (twopi*swx/getSampleRate ());
				swd = static_cast<float> ((swx-sw) / (len*getSampleRate ()));
				swt= 2 * (int32)getSampleRate ();
				break; 
		default:
				sw = swx = 0.f;
				break;
	}
	thru = (float)pow (10.0f, (0.05f * (float)int (40.f*param (TestToneParam::Thru))) - 2.f);
	if (param (TestToneParam::Thru)==0.0f) thru=0.0f;
	fscale = static_cast<float> (twopi/getSampleRate ());
}

}}} // namespaces

