/*
 *  mdaComboProcessor.h
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
class ComboProcessor : public BaseProcessor
{
public:
	ComboProcessor ();
	~ComboProcessor () override;
	
	int32 getVst2UniqueId () const SMTG_OVERRIDE { return 'mdaX'; }

	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;
	tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;
	tresult PLUGIN_API setProcessing (TBool state) SMTG_OVERRIDE;

	void doProcessing (ProcessData& data) SMTG_OVERRIDE;

//-----------------------------------------------------------------------------
	static FUnknown* createInstance (void*) { return (IAudioProcessor*)new ComboProcessor; }
#ifdef SMTG_MDA_VST2_COMPATIBILITY
	inline static DECLARE_UID (uid, 0x5653546D, 0x6461586D, 0x64612063, 0x6F6D626F);
#else
	inline static DECLARE_UID (uid, 0x11C1BD22, 0x888F4F17, 0xB2E2A77B, 0x51CEDCD6);
#endif

//-----------------------------------------------------------------------------
protected:
	void recalculate () SMTG_OVERRIDE;
	void clearBuffers ();
	float filterFreq(float hz);

	float clip, drive, trim, lpf, hpf, mix1, mix2;
	float ff1, ff2, ff3, ff4, ff5, bias;
	float ff6, ff7, ff8, ff9, ff10;
	float hhf, hhq, hh0, hh1; //hpf

	float *buffer, *buffe2;
	int32 size, bufpos, del1, del2;
	int mode, ster;
};

}}} // namespaces
