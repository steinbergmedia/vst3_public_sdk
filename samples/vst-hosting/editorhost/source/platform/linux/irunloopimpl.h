//-----------------------------------------------------------------------------
// Flags       : clang-format auto
// Project     : VST SDK
//
// Category    : EditorHost
// Filename    : public.sdk/samples/vst-hosting/editorhost/source/platform/linux/irunloopimpl.h
// Created by  : Steinberg 09.2023
// Description : Example of opening a plug-in editor
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2023, Steinberg Media Technologies GmbH, All Rights Reserved
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

#include "public.sdk/samples/vst-hosting/editorhost/source/platform/linux/runloop.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/gui/iplugview.h"

#include <algorithm>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Linux {

//------------------------------------------------------------------------
class RunLoopImpl : U::ImplementsNonDestroyable<U::Directly<IRunLoop>>
{
public:
	static IRunLoop& instance ()
	{
		static RunLoopImpl impl;
		return impl;
	}

	tresult PLUGIN_API registerEventHandler (IEventHandler* handler, FileDescriptor fd) override;
	tresult PLUGIN_API unregisterEventHandler (IEventHandler* handler) override;
	tresult PLUGIN_API registerTimer (ITimerHandler* handler, TimerInterval milliseconds) override;
	tresult PLUGIN_API unregisterTimer (ITimerHandler* handler) override;

private:
	using TimerID = uint64_t;
	using EventHandler = IPtr<Linux::IEventHandler>;
	using TimerHandler = IPtr<Linux::ITimerHandler>;
	using EventHandlers = std::unordered_map<Linux::FileDescriptor, EventHandler>;
	using TimerHandlers = std::unordered_map<TimerID, TimerHandler>;

	EventHandlers eventHandlers;
	TimerHandlers timerHandlers;
};

//------------------------------------------------------------------------
inline tresult PLUGIN_API RunLoopImpl::registerEventHandler (IEventHandler* handler,
                                                             FileDescriptor fd)
{
	if (!handler || eventHandlers.find (fd) != eventHandlers.end ())
		return kInvalidArgument;

	Vst::EditorHost::RunLoop::instance ().registerFileDescriptor (
	    fd, [handler] (int fd) { handler->onFDIsSet (fd); });
	eventHandlers.emplace (fd, handler);
	return kResultTrue;
}

//------------------------------------------------------------------------
inline tresult PLUGIN_API RunLoopImpl::unregisterEventHandler (IEventHandler* handler)
{
	if (!handler)
		return kInvalidArgument;

	auto it = std::find_if (eventHandlers.begin (), eventHandlers.end (),
	                        [&] (const auto& elem) { return elem.second == handler; });
	if (it == eventHandlers.end ())
		return kResultFalse;

	Vst::EditorHost::RunLoop::instance ().unregisterFileDescriptor (it->first);
	eventHandlers.erase (it);
	return kResultTrue;
}

//------------------------------------------------------------------------
inline tresult PLUGIN_API RunLoopImpl::registerTimer (ITimerHandler* handler,
                                                      TimerInterval milliseconds)
{
	if (!handler || milliseconds == 0)
		return kInvalidArgument;

	auto id = Vst::EditorHost::RunLoop::instance ().registerTimer (
	    milliseconds, [handler] (auto) { handler->onTimer (); });
	timerHandlers.emplace (id, handler);
	return kResultTrue;
}

//------------------------------------------------------------------------
inline tresult PLUGIN_API RunLoopImpl::unregisterTimer (ITimerHandler* handler)
{
	if (!handler)
		return kInvalidArgument;

	auto it = std::find_if (timerHandlers.begin (), timerHandlers.end (),
	                        [&] (const auto& elem) { return elem.second == handler; });
	if (it == timerHandlers.end ())
		return kResultFalse;

	Vst::EditorHost::RunLoop::instance ().unregisterTimer (it->first);
	timerHandlers.erase (it);
	return kResultTrue;
}

//------------------------------------------------------------------------
} // Linux
} // Steinberg
