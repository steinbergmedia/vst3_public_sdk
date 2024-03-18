//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/alignedalloc.h
// Created by  : Steinberg, 05/2023
// Description : aligned memory allocations
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

#pragma once

#include "pluginterfaces/base/ftypes.h"

#include <cstdlib>

#ifdef _MSC_VER
#include <malloc.h>
#endif

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** aligned allocation
 *
 *	note that you need to use aligned_free to free the block of memory
 *
 *	@param numBytes		number of bytes to allocate
 *	@param alignment	alignment of memory base address.
 *		must be a power of 2 and at least as large as sizeof (void*) or zero in which it uses malloc
 *		for allocation
 *
 *	@return 			allocated memory
 */
void* aligned_alloc (size_t numBytes, uint32_t alignment)
{
	if (alignment == 0)
		return malloc (numBytes);
	void* data {nullptr};
#if SMTG_OS_MACOS && MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_15
	posix_memalign (&data, alignment, numBytes);
#elif defined(_MSC_VER)
	data = _aligned_malloc (numBytes, alignment);
#else
	data = std::aligned_alloc (alignment, numBytes);
#endif
	return data;
}

//------------------------------------------------------------------------
void aligned_free (void* addr, uint32_t alignment)
{
	if (alignment == 0)
		std::free (addr);
	else
	{
#if defined(_MSC_VER)
		_aligned_free (addr);
#else
		std::free (addr);
#endif
	}
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
