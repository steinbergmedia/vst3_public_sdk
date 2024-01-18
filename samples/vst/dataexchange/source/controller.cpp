//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Examples
// Filename    : public.sdk/samples/vst/dataexchange/controller.cpp
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

#include "shared.h"
#include "vstgui/lib/cexternalview.h"
#include "vstgui/lib/cvstguitimer.h"
#include "vstgui/lib/iviewlistener.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "waveformview.h"
#include "public.sdk/source/vst/utility/dataexchange.h"
#include "public.sdk/source/vst/utility/ringbuffer.h"
#include "public.sdk/source/vst/utility/rttransfer.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/base/funknownimpl.h"

#include <deque>
#include <mutex>
#include <queue>
#include <string_view>
#include <thread>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

using namespace VSTGUI;

//------------------------------------------------------------------------
struct DataExchangeController : EditController,
                                IDataExchangeReceiver,
                                VST3EditorDelegate,
                                ViewListenerAdapter
{
	//---Interface---------
	OBJ_METHODS (DataExchangeController, EditController)
	DEFINE_INTERFACES
		DEF_INTERFACE (IDataExchangeReceiver)
	END_DEFINE_INTERFACES (EditController)
	REFCOUNT_METHODS (EditController)

	using Mutex = std::mutex;
	using LockGuard = std::lock_guard<Mutex>;
	using AudioBufferDataQueue = OneReaderOneWriter::RingBuffer<AudioBufferData>;

	DataExchangeReceiverHandler dataExchange {this};

	uint32 numOpenEditors {0};

	std::vector<CView*> peakImageViews;

	AudioBufferData currentAudioBufferData {};
	uint32 currentAudioBufferCounter {0u};
	std::atomic<float> pixelsPerMillisecond {1.f};

	std::unique_ptr<WaveformViewManager> viewManager;
	SharedPointer<CVSTGUITimer> fpsCheckTimer;

	bool autoReEnableDataExchange {true};

	enum
	{
		UITagStart = 10000,

		UITagPeakLeft = 0,
		UITagPeakRight,
		UITagDisplayFreq,
		UITagForceMessageHandling,
		UITagFPSDisplay,
		UITagAutoReenableDataExchange
	};
	std::vector<IPtr<Parameter>> uiOnlyParameters;

	WaveformViewManager& getViewManager ()
	{
		if (!viewManager)
		{
			viewManager =
			    std::make_unique<WaveformViewManager> (SystemTime (getComponentHandler ()));
		}
		return *viewManager.get ();
	}

	tresult PLUGIN_API initialize (FUnknown* context) override
	{
		auto res = EditController::initialize (context);
		if (res == kResultTrue)
		{
			parameters.addParameter (STR ("Enable Data Exchange"), STR ("On/Off"), 1, 0.,
			                         Vst::ParameterInfo::kIsHidden, ParamIDEnableDataExchange, 0,
			                         STR ("DataExchange"));

			// UI only parameters
			auto param = owned (new Parameter (STR ("PeakLeft"), UITagPeakLeft + UITagStart));
			param->setPrecision (2);
			uiOnlyParameters.push_back (param);
			param = owned (new Parameter (STR ("PeakRight"), UITagPeakRight + UITagStart));
			param->setPrecision (2);
			uiOnlyParameters.push_back (param);
			param = owned (new RangeParameter (STR ("Display Freq"), UITagDisplayFreq + UITagStart,
			                                   nullptr, 0.01, 10, 1.));
			param->setPrecision (2);
			param->setNormalized (param->toNormalized (pixelsPerMillisecond));
			uiOnlyParameters.push_back (param);

			auto strListParam = owned (new StringListParameter (
			    STR ("Force Message Handling"), UITagForceMessageHandling + UITagStart));
			strListParam->appendString (STR ("Off"));
			strListParam->appendString (STR ("On"));
			uiOnlyParameters.push_back (strListParam);

			param = owned (new RangeParameter (STR ("FPS"), UITagFPSDisplay + UITagStart, nullptr,
			                                   0, 10000, 0.));
			param->setPrecision (0);
			uiOnlyParameters.push_back (param);

			strListParam = owned (new StringListParameter (
			    STR ("Auto Reenable Data Exchange"), UITagAutoReenableDataExchange + UITagStart));
			strListParam->appendString (STR ("Off"));
			strListParam->appendString (STR ("On"));
			strListParam->setNormalized (autoReEnableDataExchange ? 1. : 0.);
			uiOnlyParameters.push_back (strListParam);
		}
		return res;
	}

	tresult PLUGIN_API terminate () override
	{
		viewManager.reset ();
		return EditController::terminate ();
	}

	tresult PLUGIN_API notify (IMessage* message) override
	{
		if (dataExchange.onMessage (message))
			return kResultTrue;
		return EditController::notify (message);
	}

	void PLUGIN_API queueOpened (DataExchangeUserContextID userContextID, uint32 blockSize,
	                             TBool& dispatchOnBackgroundThread) override
	{
		dispatchOnBackgroundThread = userContextID == SampleBufferQueueID;
	}
	void PLUGIN_API queueClosed (DataExchangeUserContextID userContextID) override {}

	void PLUGIN_API onDataExchangeBlocksReceived (DataExchangeUserContextID userContextID,
	                                              uint32 numBlocks, DataExchangeBlock* block,
	                                              TBool onBackgroundThread) override
	{
		if (userContextID == SampleBufferQueueID)
		{
			// assert (onBackgroundThread == true);
			for (auto idx = 0u; idx < numBlocks; ++idx)
			{
				const auto& data = getSampleBufferExchangeData (block[idx]);
				// n * pixel per millisecond
				auto numSamplesPerPixel =
				    static_cast<uint32> (std::ceil (data.sampleRate / 1000 * pixelsPerMillisecond));

				if (currentAudioBufferData.peak.size () < data.numChannels)
				{
					currentAudioBufferData.systemTime = data.systemTime;
					currentAudioBufferData.sampleRate = data.sampleRate;
					currentAudioBufferData.peak.resize (data.numChannels);
					std::fill (currentAudioBufferData.peak.begin (),
					           currentAudioBufferData.peak.end (), PeakValue ());
				}

				auto sampleCounter = 0;
				while (sampleCounter < data.numSamples)
				{
					auto numSamples =
					    std::min<uint32> (numSamplesPerPixel - currentAudioBufferCounter,
					                      data.numSamples - sampleCounter);

					for (auto channel = 0u; channel < data.numChannels; ++channel)
					{
						for (auto sampleIdx = 0u; sampleIdx < numSamples; ++sampleIdx)
						{
							auto sample = data.sampleData[sampleDataOffsetForChannel (
							                                  channel, data.numSamples) +
							                              sampleCounter + sampleIdx];
							if (currentAudioBufferData.peak[channel].min > sample)
								currentAudioBufferData.peak[channel].min = sample;
							if (currentAudioBufferData.peak[channel].max < sample)
								currentAudioBufferData.peak[channel].max = sample;
						}
					}
					sampleCounter += numSamples;
					currentAudioBufferCounter += numSamples;
					if (currentAudioBufferCounter >= numSamplesPerPixel)
					{
						getViewManager ().pushAudioBufferData (std::move (currentAudioBufferData));
						currentAudioBufferData.systemTime = data.systemTime;
						currentAudioBufferData.sampleRate = data.sampleRate;
						currentAudioBufferData.peak.resize (data.numChannels);
						std::fill (currentAudioBufferData.peak.begin (),
						           currentAudioBufferData.peak.end (), PeakValue ());
						currentAudioBufferCounter = 0;
					}
				}
			}
			getViewManager ().renderIfNeeded ();
		}
	}

	Parameter* getParameterObject (ParamID tag) override
	{
		if (tag >= UITagStart)
		{
			tag -= UITagStart;
			if (tag <= uiOnlyParameters.size ())
				return uiOnlyParameters[tag];
			return nullptr;
		}
		return EditController::getParameterObject (tag);
	}

	tresult beginEdit (ParamID tag) override
	{
		if (tag >= UITagStart)
			return kResultTrue;
		return EditController::beginEdit (tag);
	}

	tresult performEdit (ParamID tag, ParamValue valueNormalized) override
	{
		if (tag >= UITagStart)
		{
			tag -= UITagStart;
			if (tag == UITagDisplayFreq)
			{
				auto value = uiOnlyParameters[UITagDisplayFreq]->toPlain (valueNormalized);
				pixelsPerMillisecond = value;
			}
			else if (tag == UITagForceMessageHandling)
			{
				if (auto msg = owned (allocateMessage ()))
				{
					msg->setMessageID (MessageIDForceMessageHandling);
					if (auto attributes = msg->getAttributes ())
					{
						attributes->setInt (MessageKeyValue, valueNormalized > 0.5 ? 1 : 0);
						sendMessage (msg);
					}
				}
			}
			else if (tag == UITagAutoReenableDataExchange)
			{
				autoReEnableDataExchange = valueNormalized > 0.5;
				if (autoReEnableDataExchange)
					enableDataExchange (true);
			}
			return kResultTrue;
		}
		return EditController::performEdit (tag, valueNormalized);
	}

	tresult endEdit (ParamID tag) override
	{
		if (tag >= UITagStart)
			return kResultTrue;
		return EditController::endEdit (tag);
	}

	void enableDataExchange (bool state)
	{
		beginEdit (ParamIDEnableDataExchange);
		if (auto param = getParameterObject (ParamIDEnableDataExchange))
			param->setNormalized (state ? 1. : 0);
		performEdit (ParamIDEnableDataExchange, state ? 1. : 0.);
		endEdit (ParamIDEnableDataExchange);
	}

	tresult PLUGIN_API setParamNormalized (ParamID tag, ParamValue value) override
	{
		auto result = EditController::setParamNormalized (tag, value);
		if (numOpenEditors > 0 && tag == ParamIDEnableDataExchange)
		{
			if (autoReEnableDataExchange && value < 0.5)
				enableDataExchange (true);
		}
		return result;
	}

	IPlugView* PLUGIN_API createView (const char* _name) override
	{
		std::string_view name (_name);
		if (name == ViewType::kEditor)
		{
			auto* view = new VST3Editor (this, "view", "editor.uidesc");
			return view;
		}
		return nullptr;
	}

	void editorAttached (EditorView* /*editor*/) override
	{
		if (++numOpenEditors == 1)
		{
			// start streaming realtime audio when an editor is open
			enableDataExchange (true);
			fpsCheckTimer = VSTGUI::makeOwned<CVSTGUITimer> (
			    [this] (auto) {
				    auto fps = getViewManager ().getFramesPerSeconds ();
				    if (auto param = getParameterObject (UITagFPSDisplay + UITagStart))
				    {
					    param->setNormalized (param->toNormalized (fps));
				    }
			    },
			    1000);
		}
	}
	void editorRemoved (EditorView* /*editor*/) override
	{
		if (--numOpenEditors == 0)
		{
			// stop streaming realtime audio when all editors are closed
			enableDataExchange (false);
			fpsCheckTimer = nullptr;
		}
	}
	void viewWillDelete (CView* view) override
	{
		auto it = std::find (peakImageViews.begin (), peakImageViews.end (), view);
		if (it != peakImageViews.end ())
		{
			view->unregisterViewListener (this);
			peakImageViews.erase (it);
		}
	}

	CView* createCustomView (UTF8StringPtr name, const UIAttributes& attributes,
	                         const IUIDescription* description, VST3Editor* editor) override
	{
		if (std::string_view (name) == "PeakImageView")
		{
			CColor leftChannelColor = {255, 0, 0, 255};
			CColor rightChannelColor = {0, 255, 0, 128};
			description->getColor ("waveform.left", leftChannelColor);
			description->getColor ("waveform.right", rightChannelColor);
			if (auto view = getViewManager ().createNewView (leftChannelColor, rightChannelColor))
			{
				peakImageViews.push_back (view);
				view->registerViewListener (this);
				return view;
			}
		}
		return nullptr;
	}
};

//------------------------------------------------------------------------
FUnknown* createDataExchangeController (void*)
{
	return static_cast<IEditController*> (new DataExchangeController);
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
