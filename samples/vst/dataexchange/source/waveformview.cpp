//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Examples
// Filename    : public.sdk/samples/vst/dataexchange/waveformview.cpp
// Created by  : Steinberg, 06/2023
// Description : VST Data Exchange API Example
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

#include "waveformview.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/iviewlistener.h"

#include "nanovg.h"

#include <deque>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

using namespace VSTGUI;

//------------------------------------------------------------------------
struct ViewAndContext
{
	CExternalView* view {nullptr};
	NVGcontext* context {nullptr};
	CColor leftColor {kRedCColor};
	CColor rightColor {kGreenCColor};
	WaveformViewManager::DrawCallbackToken callbackToken {};
};

//------------------------------------------------------------------------
struct WaveformViewManager::Impl : ViewListenerAdapter
{
	AudioBufferDataQueue& _audioBufferDataQueue;
	std::deque<AudioBufferData> _audioPeakValues {1000};
	std::deque<AudioBufferData> _audioPeakFutureValues;

	std::vector<ViewAndContext> views;

	using Mutex = std::recursive_mutex;
	using LockGuard = std::lock_guard<Mutex>;
	Mutex peakValueMutex;

	int64 _lastRenderTimer {-1};

	FramesPerSeconds fps;

	Impl (AudioBufferDataQueue& dataQueue) : _audioBufferDataQueue (dataQueue) {}

	void viewWillDelete (CView* view) override
	{
		auto it = std::find_if (views.begin (), views.end (),
		                        [&] (const auto& el) { return el.view == view; });
		if (it == views.end ())
			return;
		view->unregisterViewListener (this);
		releaseNanoVGContext (it->context);
		views.erase (it);
	}

	void viewAttached (CView* view) override
	{
		auto it = std::find_if (views.begin (), views.end (),
		                        [&] (const auto& el) { return el.view == view; });
		if (it == views.end ())
			return;
		it->callbackToken = RenderThreadManager::instance ().registerDrawCallback (
		    [this, context = *it] () { render (context); });
	}

	void viewRemoved (CView* view) override
	{
		auto it = std::find_if (views.begin (), views.end (),
		                        [&] (const auto& el) { return el.view == view; });
		if (it == views.end ())
			return;
		RenderThreadManager::instance ().unregisterDrawCallback (it->callbackToken);
	}

	void addView (ViewAndContext&& ctx)
	{
		ctx.view->registerViewListener (this);
		views.emplace_back (std::move (ctx));
	}

	void render (const ViewAndContext& view)
	{
		if (!preRender (view.view, view.context))
			return;

		LockGuard g (peakValueMutex);

		auto devicePixelRatio = 1.;

		devicePixelRatio = view.view->getFrame ()->getScaleFactor ();

		auto size = view.view->getViewSize ().getSize ();
		nvgBeginFrame (view.context, size.x, size.y, devicePixelRatio);

		float xAdd = size.x / _audioPeakValues.size ();
		nvgStrokeWidth (view.context, xAdd);

		// left channel
		float x = 0.f;
		float y = 0.f;
		nvgStrokeColor (view.context, nvgRGBA (view.leftColor.red, view.leftColor.green,
		                                       view.leftColor.blue, view.leftColor.alpha));
		nvgBeginPath (view.context);
		for (auto value : _audioPeakValues)
		{
			if (value.peak.empty ())
				continue;
			y = size.y * ((value.peak[0].min + 1.f) / 2.f);
			nvgMoveTo (view.context, x, y);
			y = size.y * ((value.peak[0].max + 1.f) / 2.f);
			nvgLineTo (view.context, x, y);
			x += xAdd;
		}
		nvgStroke (view.context);

		// right channel
		x = 0.f;
		nvgStrokeColor (view.context, nvgRGBA (view.rightColor.red, view.rightColor.green,
		                                       view.rightColor.blue, view.rightColor.alpha));
		nvgBeginPath (view.context);
		for (auto value : _audioPeakValues)
		{
			if (value.peak.size () < 2)
				continue;
			y = size.y * ((value.peak[1].min + 1.f) / 2.f);
			nvgMoveTo (view.context, x, y);
			y = size.y * ((value.peak[1].max + 1.f) / 2.f);
			nvgLineTo (view.context, x, y);
			x += xAdd;
		}
		nvgStroke (view.context);

		nvgEndFrame (view.context);

		postRender (view.view, view.context);

		fps.increaseDrawCount ();
	}
};

//------------------------------------------------------------------------
WaveformViewManager::WaveformViewManager (SystemTime&& systemTime)
: systemTime (std::move (systemTime))
{
	impl = std::make_unique<Impl> (audioBufferDataQueue);
}

//------------------------------------------------------------------------
WaveformViewManager::WaveformViewManager (const SystemTime& systemTime) : systemTime (systemTime)
{
	impl = std::make_unique<Impl> (audioBufferDataQueue);
}

//------------------------------------------------------------------------
WaveformViewManager::~WaveformViewManager () noexcept
{
}

//------------------------------------------------------------------------
auto WaveformViewManager::createNewView (CColor leftChannel, CColor rightChannel) -> CExternalView*
{
	auto p = createNanoVGViewAndContext ();
	if (p.first && p.second)
		impl->addView ({p.first, p.second, leftChannel, rightChannel});

	return p.first;
}

//------------------------------------------------------------------------
double WaveformViewManager::getFramesPerSeconds () const
{
	return impl->fps.get ();
}

//------------------------------------------------------------------------
void WaveformViewManager::renderIfNeeded ()
{
	if (!impl->peakValueMutex.try_lock ())
		return;

	auto currentTime = systemTime.get ();

	bool atLeastOneValueChanged {false};
	while (!impl->_audioPeakFutureValues.empty () &&
	       impl->_audioPeakFutureValues.front ().systemTime <= currentTime)
	{
		// if the queued buffer has reached the currentTime we push it into the buffer we draw
		impl->_audioPeakValues.pop_front ();
		impl->_audioPeakValues.push_back (std::move (impl->_audioPeakFutureValues.front ()));
		impl->_audioPeakFutureValues.pop_front ();
		atLeastOneValueChanged = true;
	}

	AudioBufferData bufferData;
	while (impl->_audioBufferDataQueue.pop (bufferData))
	{
		if (bufferData.systemTime > currentTime)
		{
			// if the systemTime of the buffer is in the future of the current time
			// we queue the buffer
			impl->_audioPeakFutureValues.push_back (std::move (bufferData));
		}
		else
		{
			// otherwise we push the buffer into the queue that is drawn
			impl->_audioPeakValues.pop_front ();
			impl->_audioPeakValues.push_back (std::move (bufferData));
			atLeastOneValueChanged = true;
		}
	}

	impl->peakValueMutex.unlock ();
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
