//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/memoryibstream.h
// Created by  : Steinberg, 12/2023
// Description : 
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

#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/ibstream.h"
#include <vector>

//------------------------------------------------------------------------
namespace Steinberg {

//------------------------------------------------------------------------
class ResizableMemoryIBStream : public U::Implements<U::Directly<IBStream>>
{
public:
	inline ResizableMemoryIBStream (size_t reserve = 0);

	inline tresult PLUGIN_API read (void* buffer, int32 numBytes, int32* numBytesRead) override;
	inline tresult PLUGIN_API write (void* buffer, int32 numBytes, int32* numBytesWritten) override;
	inline tresult PLUGIN_API seek (int64 pos, int32 mode, int64* result) override;
	inline tresult PLUGIN_API tell (int64* pos) override;

	inline size_t getCursor () const;
	inline const void* getData () const;
	inline void rewind ();

private:
	std::vector<uint8> data;
	size_t cursor {0};
};

//------------------------------------------------------------------------
inline ResizableMemoryIBStream::ResizableMemoryIBStream (size_t reserve)
{
	if (reserve)
		data.reserve (reserve);
}

//------------------------------------------------------------------------
inline tresult PLUGIN_API ResizableMemoryIBStream::read (void* buffer, int32 numBytes,
                                                         int32* numBytesRead)
{
	if (numBytes < 0 || buffer == nullptr)
		return kInvalidArgument;
	auto byteCount = std::min<int64> (numBytes, data.size () - cursor);
	if (byteCount > 0)
	{
		memcpy (buffer, data.data () + cursor, byteCount);
		cursor += byteCount;
	}
	if (numBytesRead)
		*numBytesRead = static_cast<int32> (byteCount);
	return kResultTrue;
}

//------------------------------------------------------------------------
inline tresult PLUGIN_API ResizableMemoryIBStream::write (void* buffer, int32 numBytes,
                                                          int32* numBytesWritten)
{
	if (numBytes < 0 || buffer == nullptr)
		return kInvalidArgument;
	auto requiredSize = cursor + numBytes;
	if (requiredSize >= data.capacity ())
	{
		auto mod = (requiredSize % 1024);
		if (mod)
		{
			auto reserve = requiredSize + (1024 - mod);
			data.reserve (reserve);
		}
	}
	data.resize (requiredSize);
	memcpy (data.data () + cursor, buffer, numBytes);
	cursor += numBytes;
	if (numBytesWritten)
		*numBytesWritten = numBytes;

	return kResultTrue;
}

//------------------------------------------------------------------------
inline tresult PLUGIN_API ResizableMemoryIBStream::seek (int64 pos, int32 mode, int64* result)
{
	int64 newCursor = static_cast<int64> (cursor);
	switch (mode)
	{
		case kIBSeekSet: newCursor = pos; break;
		case kIBSeekCur: newCursor += pos; break;
		case kIBSeekEnd: newCursor = data.size () + pos; break;
		default: return kInvalidArgument;
	}
	if (newCursor < 0)
		return kInvalidArgument;
	if (newCursor >= data.size ())
		return kInvalidArgument;
	if (result)
		*result = newCursor;
	cursor = static_cast<size_t> (newCursor);
	return kResultTrue;
}

//------------------------------------------------------------------------
inline tresult PLUGIN_API ResizableMemoryIBStream::tell (int64* pos)
{
	if (pos == nullptr)
		return kInvalidArgument;
	*pos = static_cast<int64> (cursor);
	return kResultTrue;
}

//------------------------------------------------------------------------
inline size_t ResizableMemoryIBStream::getCursor () const
{
	return cursor;
}

//------------------------------------------------------------------------
inline const void* ResizableMemoryIBStream::getData () const
{
	return data.data ();
}

//------------------------------------------------------------------------
inline void ResizableMemoryIBStream::rewind ()
{
	cursor = 0;
}

//------------------------------------------------------------------------
} // Steinberg
