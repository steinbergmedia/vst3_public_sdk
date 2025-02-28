/*
 *  mdaTalkBoxProcessor.h
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
class TalkBoxProcessor : public BaseProcessor
{
public:
	TalkBoxProcessor ();
	~TalkBoxProcessor () override;
	
	int32 getVst2UniqueId () const SMTG_OVERRIDE { return 'mda&'; }

	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;
	tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;
	tresult PLUGIN_API setProcessing (TBool state) SMTG_OVERRIDE;

	void doProcessing (ProcessData& data) SMTG_OVERRIDE;

//-----------------------------------------------------------------------------
	static FUnknown* createInstance (void*) { return (IAudioProcessor*)new TalkBoxProcessor; }
#ifdef SMTG_MDA_VST2_COMPATIBILITY
	inline static DECLARE_UID (uid, 0x5653546D, 0x6461266D, 0x64612074, 0x616C6B62);
#else
	inline static DECLARE_UID (uid, 0xF235EEE4, 0x92084883, 0xAD5C8632, 0x34DFE073);
#endif

//-----------------------------------------------------------------------------
protected:
	void recalculate () SMTG_OVERRIDE;
	void lpc (float *buf, float *car, int32 n, int32 o);
	void lpc_durbin (float *r, int32 p, float *k, float *g);

	float *car0, *car1;
	float *window;
	float *buf0, *buf1;

	float emphasis;
	int32 K, N, O, pos, swap;
	float wet, dry, FX;

	float d0, d1, d2, d3, d4;
	float u0, u1, u2, u3, u4;
};

}}} // namespaces
