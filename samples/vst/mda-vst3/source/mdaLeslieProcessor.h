/*
 *  mdaLeslieProcessor.h
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

#pragma once

#include "mdaBaseProcessor.h"

namespace Steinberg {
namespace Vst {
namespace mda {

//-----------------------------------------------------------------------------
class LeslieProcessor : public BaseProcessor
{
public:
	LeslieProcessor ();
	~LeslieProcessor () override;
	
	int32 getVst2UniqueId () const SMTG_OVERRIDE { return 'mdaH'; }

	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;
	tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;
	tresult PLUGIN_API setProcessing (TBool state) SMTG_OVERRIDE;

	void doProcessing (ProcessData& data) SMTG_OVERRIDE;

//-----------------------------------------------------------------------------
	static FUnknown* createInstance (void*) { return (IAudioProcessor*)new LeslieProcessor; }
#ifdef SMTG_MDA_VST2_COMPATIBILITY
	inline static DECLARE_UID (uid, 0x5653546D, 0x6461486D, 0x6461206C, 0x65736C69);
#else
	inline static DECLARE_UID (uid, 0xFBD3AD80, 0x9E2847E0, 0xB87CDEC3, 0x5C0469B1);
#endif

//-----------------------------------------------------------------------------
protected:
	void recalculate () SMTG_OVERRIDE;

	float filo; //crossover filter coeff
	float fbuf1, fbuf2; //filter buffers
	float twopi; //speed, target, momentum, phase, width, ampmod, freqmod...
	float hspd, hset, hmom, hphi, hwid, hlev, hdep; 
	float lspd, lset, lmom, lphi, lwid, llev, gain;
	float *hbuf;  //HF delay buffer
	int32 size, hpos; //buffer length & pointer

	float chp, dchp, clp, dclp, shp, dshp, slp, dslp;
};

}}} // namespaces
