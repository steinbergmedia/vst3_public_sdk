//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Examples
// Filename    : public.sdk/samples/vst/dataexchange/waveformview_direct3d.cpp
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

#include "vstgui/contrib/externalview_hwnd.h"
#include "vstgui/lib/platform/win32/comptr.h"
#include "vstgui/lib/platform/win32/win32factory.h"

#include <d3d11.h>
#include <dxgi1_3.h>
#include <mutex>

#include "nanovg.h"
#define NANOVG_D3D11_IMPLEMENTATION
#include "nanovg_d3d11.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

using namespace VSTGUI;

//------------------------------------------------------------------------
struct D3D11View : ExternalView::ExternalHWNDBase
{
	ExternalView::HWNDWindow childWindow;

	COM::Ptr<ID3D11Device> device;
	COM::Ptr<ID3D11DeviceContext> context;
	COM::Ptr<IDXGISwapChain> swapchain;

	COM::Ptr<ID3D11RenderTargetView> render_view;
	COM::Ptr<ID3D11DepthStencilView> stencil_view;
	COM::Ptr<ID3D11Texture2D> stencil;
	COM::Ptr<ID3D11Texture2D> buffer;

	HANDLE nextFrameHandle {nullptr};
	bool firstFrameRendered {false};

	using Mutex = std::recursive_mutex;
	using LockGuard = std::lock_guard<Mutex>;
	Mutex mutex;

	D3D11_VIEWPORT viewport {};

	D3D11View (HINSTANCE inst) : ExternalHWNDBase (inst), childWindow (inst)
	{
		childWindow.create (nullptr, {0, 0, 20, 20}, container.getHWND (), 0,
		                    WS_CHILD | WS_VISIBLE);
		child = childWindow.getHWND ();
	}

	~D3D11View () noexcept { child = nullptr; }

	bool attach (void* parent, PlatformViewType parentViewType) override
	{
		LockGuard g (mutex);
		if (!Base::attach (parent, parentViewType))
			return false;
		RECT r;
		GetClientRect (child, &r);
		return onResize ({r.right - r.left, r.bottom - r.top});
	}

	bool remove () override
	{
		LockGuard g (mutex);
		unbind ();
		releaseResources ();
		return Base::remove ();
	}

	void setViewSize (IntRect frame, IntRect visible) override
	{
		Base::setViewSize (frame, visible);
		LockGuard g (mutex);
		onResize (frame.size);
	}

	void releaseResources ()
	{
		if (nextFrameHandle)
			CloseHandle (nextFrameHandle);
		swapchain.reset ();
		context.reset ();
		device.reset ();
	}

	bool createResources ()
	{
		RECT r;
		GetClientRect (child, &r);

		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.Width = r.right - r.left;
		desc.BufferDesc.Height = r.bottom - r.top;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.RefreshRate.Numerator = 60;
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = 2;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.OutputWindow = child;
		desc.Windowed = TRUE;
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		UINT dev_flag = 0;
#if 0 // !defined(NDEBUG)
		dev_flag = D3D11_CREATE_DEVICE_DEBUG;
#endif
		const D3D_FEATURE_LEVEL req_levels[] = {
		    D3D_FEATURE_LEVEL_11_0,
		};

		auto hr = D3D11CreateDeviceAndSwapChain (nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		                                         dev_flag, req_levels, std::size (req_levels),
		                                         D3D11_SDK_VERSION, &desc, swapchain.adoptPtr (),
		                                         device.adoptPtr (), nullptr, context.adoptPtr ());
		COM::Ptr<IDXGISwapChain2> sc2;
		if (swapchain->QueryInterface<IDXGISwapChain2> (sc2.adoptPtr ()) == S_OK)
		{
			nextFrameHandle = sc2->GetFrameLatencyWaitableObject ();
		}

		return SUCCEEDED (hr);
	}

	bool onResize (ExternalView::IntSize size)
	{
		unbind ();
		auto hr = swapchain->ResizeBuffers (2, size.width, size.height, DXGI_FORMAT_R8G8B8A8_UNORM,
		                                    DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
		if (!SUCCEEDED (hr))
			return false;
		hr = swapchain->GetBuffer (0, __uuidof (ID3D11Texture2D),
		                           reinterpret_cast<void**> (buffer.adoptPtr ()));
		if (!SUCCEEDED (hr))
			return false;
		render_view = createRenderTargetView (buffer.get ());
		stencil = createStencilBuffer (size.width, size.height);
		stencil_view = createStencilView (stencil.get ());

		bind ();

		viewport.Width = size.width;
		viewport.Height = size.height;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1;
		return true;
	}

	auto createRenderTargetView (ID3D11Texture2D* texture) const -> COM::Ptr<ID3D11RenderTargetView>
	{
		COM::Ptr<ID3D11RenderTargetView> render_view;
		D3D11_RENDER_TARGET_VIEW_DESC view_desc;
		D3D11_TEXTURE2D_DESC tex_desc;
		HRESULT hr;

		texture->GetDesc (&tex_desc);

		view_desc.Format = tex_desc.Format;
		view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		view_desc.Texture2D.MipSlice = 0;

		hr = device->CreateRenderTargetView (texture, &view_desc, render_view.adoptPtr ());
		if (!SUCCEEDED (hr))
			return {};

		return render_view;
	}

	auto createStencilBuffer (UINT w, UINT h) -> COM::Ptr<ID3D11Texture2D>
	{
		COM::Ptr<ID3D11Texture2D> stencil;
		D3D11_TEXTURE2D_DESC stencil_desc;
		HRESULT hr;

		stencil_desc.ArraySize = 1;
		stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		stencil_desc.CPUAccessFlags = 0;
		stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		stencil_desc.Height = h;
		stencil_desc.Width = w;
		stencil_desc.MipLevels = 1;
		stencil_desc.MiscFlags = 0;
		stencil_desc.SampleDesc.Count = 1;
		stencil_desc.SampleDesc.Quality = 0;
		stencil_desc.Usage = D3D11_USAGE_DEFAULT;

		hr = device->CreateTexture2D (&stencil_desc, nullptr, stencil.adoptPtr ());
		if (!SUCCEEDED (hr))
			return {};

		return stencil;
	}

	auto createStencilView (ID3D11Texture2D* texture) -> COM::Ptr<ID3D11DepthStencilView>
	{
		COM::Ptr<ID3D11DepthStencilView> stencil_view;
		D3D11_DEPTH_STENCIL_VIEW_DESC stencil_vdesc;
		D3D11_TEXTURE2D_DESC tex_desc;
		HRESULT hr;

		texture->GetDesc (&tex_desc);

		stencil_vdesc.Format = tex_desc.Format;
		stencil_vdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		stencil_vdesc.Flags = 0;
		stencil_vdesc.Texture2D.MipSlice = 0;

		hr = device->CreateDepthStencilView (texture, &stencil_vdesc, stencil_view.adoptPtr ());
		if (!SUCCEEDED (hr))
			return {};

		return stencil_view;
	}

	void unbind ()
	{
		ID3D11RenderTargetView* targets[1] = {nullptr};
		context->OMSetRenderTargets (1, targets, nullptr);
		render_view.reset ();
		stencil_view.reset ();
		stencil.reset ();
		buffer.reset ();
	}

	void bind ()
	{
		ID3D11RenderTargetView* targets[1] = {render_view.get ()};
		context->OMSetRenderTargets (1, targets, stencil_view.get ());
	}

	bool clearBuffer (CColor c)
	{
		const FLOAT color[] = {1.0f / 255 * c.red, 1.0f / 255 * c.green, 1.0f / 255 * c.blue,
		                       1.0f / 255 * c.alpha};
		auto rview = render_view.get ();
		auto sview = stencil_view.get ();
		if (!rview || !sview)
			return false;
		context->ClearRenderTargetView (rview, color);
		context->ClearDepthStencilView (sview, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

		context->OMSetRenderTargets (1, &rview, sview);
		context->RSSetViewports (1, &viewport);

		return true;
	}

	void waitForNextFrame ()
	{
		if (firstFrameRendered && nextFrameHandle)
		{
			auto hr = WaitForSingleObject (nextFrameHandle, INFINITE);
			if (hr != WAIT_OBJECT_0)
			{
				printf ("%d", hr);
			}
		}
	}

	bool presentAndSwapBuffer ()
	{
		auto hr = swapchain->Present (0, 0);
		firstFrameRendered = true;
		return SUCCEEDED (hr);
	}

	void lock () { mutex.lock (); }
	void unlock () { mutex.unlock (); }
};

//------------------------------------------------------------------------
struct D3D11RenderThread : WaveformViewManager::IRenderThread
{
	D3D11RenderThread () = default;

	static D3D11RenderThread& instance ()
	{
		static D3D11RenderThread thread;
		return thread;
	}

	void start (ThreadFunc&& f) override
	{
		running = true;
		thread = std::thread ([this, func = std::move (f)] () {
			while (running)
			{
				func ();
				std::this_thread::sleep_for (std::chrono::milliseconds (1));
			}
		});
	}

	void stop () override
	{
		if (!thread.joinable ())
			return;
		running = false;
		thread.join ();
	}

private:
	std::thread thread;
	std::atomic<bool> running {false};
};

//------------------------------------------------------------------------
std::pair<CExternalView*, NVGcontext*> WaveformViewManager::createNanoVGViewAndContext ()
{
	auto instance = getPlatformFactory ().asWin32Factory ()->getInstance ();
	auto d3d11View = std::make_shared<D3D11View> (instance);
	if (!d3d11View->createResources ())
		return {};
	auto externalView = new CExternalView ({}, d3d11View);
	auto context = nvgCreateD3D11 (d3d11View->device.get (), NVG_ANTIALIAS);
	return {externalView, context};
}

//------------------------------------------------------------------------
void WaveformViewManager::releaseNanoVGContext (NVGcontext* context)
{
	nvgDeleteD3D11 (context);
}

//------------------------------------------------------------------------
bool WaveformViewManager::preRender (VSTGUI::CExternalView* view, NVGcontext* context)
{
	auto d3d11View = static_cast<D3D11View*> (view->getExternalView ());
	assert (d3d11View);
	d3d11View->waitForNextFrame ();
	d3d11View->lock ();
	if (!d3d11View->clearBuffer (kBlackCColor))
	{
		d3d11View->unlock ();
		return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool WaveformViewManager::postRender (VSTGUI::CExternalView* view, NVGcontext* context)
{
	auto d3d11View = static_cast<D3D11View*> (view->getExternalView ());
	assert (d3d11View);
	d3d11View->presentAndSwapBuffer ();
	d3d11View->unlock ();

	return true;
}

//------------------------------------------------------------------------
auto WaveformViewManager::createRenderThread () -> IRenderThread&
{
	return D3D11RenderThread::instance ();
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
