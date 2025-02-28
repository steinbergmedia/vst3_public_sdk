/*
 *  mdaAmbienceProcessor.h
 *  mda-vst3
 *
 *  Created by Arne Scheffler on 6/13/08.
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
class AmbienceProcessor : public BaseProcessor
{
public:
	AmbienceProcessor ();
	~AmbienceProcessor () override;

	int32 getVst2UniqueId () const SMTG_OVERRIDE { return 'mdaA'; }

	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;
	tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;
	tresult PLUGIN_API setProcessing (TBool state) SMTG_OVERRIDE;

	void doProcessing (ProcessData& data) SMTG_OVERRIDE;

//-----------------------------------------------------------------------------
	static FUnknown* createInstance (void*) { return (IAudioProcessor*)new AmbienceProcessor; }
	inline static DECLARE_UID (uid, 0xAF799E3A, 0xC94B444A, 0xB5B24FD5, 0xD3E3F9E4);
//-----------------------------------------------------------------------------
protected:
	void recalculate () SMTG_OVERRIDE;
	void clearBuffers ();

	float *buf1, *buf2, *buf3, *buf4;
	float fil, fbak, damp, wet, dry, size;
	int32  pos, den, rdy;
};

}}} // namespaces
