//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Validator
// Filename    : main.cpp
// Created by  : Steinberg, 04/2005
// Description : main entry point
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
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "validator.h"
#include "public.sdk/source/vst/utility/stringconvert.h"

void* moduleHandle = nullptr;
extern bool InitModule ();
extern bool DeinitModule ();

//------------------------------------------------------------------------
int run (int argc, char* argv[])
{
	InitModule ();

	auto result = Steinberg::Vst::Validator (argc, argv).run ();

	DeinitModule ();

	return result;
}

//------------------------------------------------------------------------
#if SMTG_OS_WINDOWS
//------------------------------------------------------------------------
using Utf8String = std::string;

//------------------------------------------------------------------------
using Utf8Args = std::vector<Utf8String>;
Utf8Args toUtf8Args (int argc, wchar_t* wargv[])
{
	Utf8Args utf8Args;
	for (int i = 0; i < argc; i++)
	{
		auto str = reinterpret_cast<const Steinberg::Vst::TChar*> (wargv[i]);
		utf8Args.push_back (VST3::StringConvert::convert (str));
	}

	return utf8Args;
}

//------------------------------------------------------------------------
using Utf8ArgPtrs = std::vector<char*>;
Utf8ArgPtrs toUtf8ArgPtrs (Utf8Args& utf8Args)
{
	Utf8ArgPtrs utf8ArgPtrs;
	for (auto& el : utf8Args)
	{
		utf8ArgPtrs.push_back (el.data ());
	}

	return utf8ArgPtrs;
}

//------------------------------------------------------------------------
int wmain (int argc, wchar_t* wargv[])
{
	Utf8Args utf8Args = toUtf8Args (argc, wargv);
	Utf8ArgPtrs utf8ArgPtrs = toUtf8ArgPtrs (utf8Args);

	char** argv = &(utf8ArgPtrs.at (0));
	return run (argc, argv);
}

#else

//------------------------------------------------------------------------
int main (int argc, char* argv[])
{
	return run (argc, argv);
}

#endif // SMTG_OS_WINDOWS
