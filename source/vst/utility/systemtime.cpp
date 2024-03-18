//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/systemtime.cpp
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

#include "systemtime.h"
#include "pluginterfaces/base/funknownimpl.h"
#include <limits>

#if SMTG_OS_OSX
#include <CoreAudio/CoreAudio.h>

//------------------------------------------------------------------------
static Steinberg::Vst::SystemTime::GetImplFunc makeNativeGetSystemTimeFunc ()
{
	return [] () {
		return static_cast<Steinberg::int64> (
		    AudioConvertHostTimeToNanos (AudioGetCurrentHostTime ()));
	};
}
#elif SMTG_OS_IOS
#include <mach/mach_time.h>

//------------------------------------------------------------------------
static Steinberg::Vst::SystemTime::GetImplFunc makeNativeGetSystemTimeFunc ()
{
	static struct mach_timebase_info timebaseInfo;
	mach_timebase_info (&timebaseInfo);

	return [&] () {
		double absTime = static_cast<double> (mach_absolute_time ());
		// nano seconds
		double d = (absTime / timebaseInfo.denom) * timebaseInfo.numer;
		return static_cast<Steinberg::int64> (d);
	};
}

#elif SMTG_OS_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

//------------------------------------------------------------------------
struct WinmmDll
{
	static WinmmDll& instance ()
	{
		static WinmmDll gInstance;
		return gInstance;
	}
	bool valid () const { return func != nullptr; }
	DWORD timeGetTime () const { return func (); }

private:
	using TimeGetTimeFunc = DWORD (WINAPI*) ();
	WinmmDll ()
	{
		if (dll = LoadLibraryA ("winmm.dll"))
		{
			func = reinterpret_cast<TimeGetTimeFunc> (GetProcAddress (dll, "timeGetTime"));
		}
	}
	TimeGetTimeFunc func {nullptr};
	HMODULE dll;
};

//------------------------------------------------------------------------
static Steinberg::Vst::SystemTime::GetImplFunc makeNativeGetSystemTimeFunc ()
{
	if (WinmmDll::instance ().valid ())
	{
		return [dll = WinmmDll::instance ()] ()
		{
			return static_cast<Steinberg::int64> (dll.timeGetTime ()) * 1000000;
		};
	}
	return [] () { return std::numeric_limits<Steinberg::int64>::max (); };
}
#elif SMTG_OS_LINUX
//------------------------------------------------------------------------
#include <time.h>
static uint64_t getUptimeByClockGettime ()
{
	struct timespec time_spec;

	if (clock_gettime (CLOCK_BOOTTIME, &time_spec) != 0)
		return 0;

	const uint64_t uptime = time_spec.tv_sec * 1000 + time_spec.tv_nsec / 1000000;
	return uptime;
}

static Steinberg::Vst::SystemTime::GetImplFunc makeNativeGetSystemTimeFunc ()
{
	return [] () { return static_cast<Steinberg::int64> (getUptimeByClockGettime ()); };
}
#else
//------------------------------------------------------------------------
static Steinberg::Vst::SystemTime::GetImplFunc makeNativeGetSystemTimeFunc ()
{
	return [] () { return std::numeric_limits<Steinberg::int64>::max (); };
}
#endif

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
SystemTime::SystemTime (IComponentHandler* componentHandler)
{
	if (auto chst = U::cast<IComponentHandlerSystemTime> (componentHandler))
	{
		getImpl = [host = std::move (chst)] ()->int64
		{
			int64 value = 0;
			if (host->getSystemTime (value) == kResultTrue)
				return value;
			return std::numeric_limits<int64>::max ();
		};
	}
	else
	{
		getImpl = makeNativeGetSystemTimeFunc ();
	}
}

//------------------------------------------------------------------------
SystemTime::SystemTime (const SystemTime& st)
{
	*this = st;
}

//------------------------------------------------------------------------
SystemTime::SystemTime (SystemTime&& st) noexcept
{
	*this = std::move (st);
}

//------------------------------------------------------------------------
SystemTime& SystemTime::operator= (const SystemTime& st)
{
	getImpl = st.getImpl;
	return *this;
}

//------------------------------------------------------------------------
SystemTime& SystemTime::operator= (SystemTime&& st) noexcept
{
	std::swap (getImpl, st.getImpl);
	return *this;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
