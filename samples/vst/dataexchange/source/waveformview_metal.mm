//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Examples
// Filename    : public.sdk/samples/vst/dataexchange/waveformview_metal.mm
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

#import "shared.h"

#import "nanovg.h"
#import "nanovg_mtl.h"
#import "vstgui/contrib/externalview_metal.h"
#import "waveformview.h"
#import <CoreAudio/CoreAudio.h>
#import <mutex>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

using namespace VSTGUI;

//------------------------------------------------------------------------
struct MetalLayerView : ExternalView::ExternalNSViewBase<NSView>
{
	CAMetalLayer* metalLayer {nullptr};

	MetalLayerView () : Base ([NSView new])
	{
		metalLayer = [CAMetalLayer new];
		view.layer = metalLayer;
		metalLayer.geometryFlipped = YES;
		metalLayer.opaque = NO;
		metalLayer.contentsGravity = kCAGravityBottomLeft;
		metalLayer.device = MTLCreateSystemDefaultDevice ();
	}

	void setContentScaleFactor (double scaleFactor) override
	{
		metalLayer.contentsScale = scaleFactor;
		auto size = view.bounds.size;
		size.width *= scaleFactor;
		size.height *= scaleFactor;
		metalLayer.drawableSize = size;
	}

	void setViewSize (IntRect frame, IntRect visible) override
	{
		Base::setViewSize (frame, visible);
		auto size = view.bounds.size;
		size.width *= metalLayer.contentsScale;
		size.height *= metalLayer.contentsScale;
		metalLayer.drawableSize = size;
	}
};

//------------------------------------------------------------------------
std::pair<VSTGUI::CExternalView*, NVGcontext*> WaveformViewManager::createNanoVGViewAndContext ()
{
	auto metalView = std::make_shared<MetalLayerView> ();
	auto externalView = new CExternalView ({}, metalView);
	auto context = nvgCreateMTL (metalView->metalLayer, NVG_ANTIALIAS);
	mnvgClearWithColor (context, nvgRGBA (0, 0, 0, 255));
	return {externalView, context};
}

//------------------------------------------------------------------------
void WaveformViewManager::releaseNanoVGContext (NVGcontext* context)
{
	nvgDeleteMTL (context);
}

//------------------------------------------------------------------------
bool WaveformViewManager::preRender (VSTGUI::CExternalView* view, NVGcontext* context)
{
	mnvgClearWithColor (context, nvgRGBA (0, 0, 0, 255));
	return true;
}

//------------------------------------------------------------------------
bool WaveformViewManager::postRender (VSTGUI::CExternalView* view, NVGcontext* context)
{
	return true;
}

//------------------------------------------------------------------------
struct DisplayLinkRenderThread : WaveformViewManager::IRenderThread
{
	static DisplayLinkRenderThread& instance ()
	{
		static DisplayLinkRenderThread thread;
		return thread;
	}

	void start (ThreadFunc&& f) final
	{
		assert (_displayLink == nullptr);
		func = std::move (f);
		auto result = CVDisplayLinkCreateWithActiveCGDisplays (&_displayLink);
		assert (result == kCVReturnSuccess);
		result = CVDisplayLinkSetOutputCallback (_displayLink, displayLinkRender, this);
		assert (result == kCVReturnSuccess);
		result = CVDisplayLinkSetCurrentCGDisplay (_displayLink, CGMainDisplayID ());
		assert (result == kCVReturnSuccess);
		CVDisplayLinkStart (_displayLink);
	}

	void stop () final
	{
		assert (_displayLink != nullptr);
		CVDisplayLinkStop (_displayLink);
		CVDisplayLinkRelease (_displayLink);
		_displayLink = nullptr;
	}

private:
	CVDisplayLinkRef _displayLink {nullptr};
	ThreadFunc func;

	static CVReturn displayLinkRender (CVDisplayLinkRef displayLink, const CVTimeStamp* now,
	                                   const CVTimeStamp* outputTime, CVOptionFlags flagsIn,
	                                   CVOptionFlags* flagsOut, void* displayLinkContext)
	{
		auto This = reinterpret_cast<DisplayLinkRenderThread*> (displayLinkContext);
		This->func ();
		return kCVReturnSuccess;
	}
};

//------------------------------------------------------------------------
auto WaveformViewManager::createRenderThread () -> IRenderThread&
{
	return DisplayLinkRenderThread::instance ();
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
