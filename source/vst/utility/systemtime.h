//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/systemtime.h
// Created by  : Steinberg, 06/2023
// Description : VST Component System Time API Helper
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
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivsteditcontroller.h"

#include <functional>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** SystemTime Helper class
 *
 *	Get the system time on the vst controller side.
 *
 *	If supported by the host this uses the same clock as used in the
 *	realtime audio process block. Otherwise an approximation via platform APIs is used.
 *
 *	This can be used to synchronize audio and visuals. As known, the audio process block is always
 *	called ealier as the audio which was generated passes the audio monitors or headphones.
 *	Depending on the audio graph this can be so long that your eyes will see the visualization (if
 *	not synchronized) earlier then your ears will hear the sound.
 *	To synchronize you need to queue your visualization data on the controller side timestamped with
 *	the time from the process block and dequed when it's time for the data to be visualized.
 */
class SystemTime
{
public:
	SystemTime (IComponentHandler* componentHandler);
	SystemTime (const SystemTime& st);
	SystemTime (SystemTime&& st) noexcept;
	SystemTime& operator= (const SystemTime& st);
	SystemTime& operator= (SystemTime&& st) noexcept;

	/** get the current system time
	 */
	int64 get () const { return getImpl (); }

//------------------------------------------------------------------------
	using GetImplFunc = std::function<int64 ()>;

private:
	GetImplFunc getImpl;
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg
