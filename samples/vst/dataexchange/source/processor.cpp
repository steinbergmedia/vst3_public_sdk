//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Examples
// Filename    : public.sdk/samples/vst/dataexchange/processor.cpp
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
#include "public.sdk/source/vst/utility/dataexchange.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#include <algorithm>
#include <cassert>
#include <string_view>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

static constexpr DataExchangeBlock InvalidDataExchangeBlock = {nullptr, 0,
                                                               InvalidDataExchangeBlockID};

//------------------------------------------------------------------------
template <typename ExchangeDataT, uint32 blockAlignment = 0>
struct ExchangeDataProcessor : DataExchangeHandler
{
	using Base = ExchangeDataProcessor<ExchangeDataT, blockAlignment>;

private:
	DataExchangeBlock currentBlock {InvalidDataExchangeBlock};

protected:
	int32 displayLatency {0};

	virtual DataExchangeUserContextID getUserContextID () const = 0;
	virtual uint32 getQueueBlockSize (SampleRate sampleRate) const = 0;
	virtual uint32 getNumQueueBlocks () const = 0;
	virtual void onNewLockedBlock () = 0;
	virtual void onOpen (SampleRate sr) = 0;

public:
	ExchangeDataProcessor (IAudioProcessor* processor)
	: DataExchangeHandler (processor, [this] (auto& config, const auto& setup) {
		onOpen (setup.sampleRate);
		config.blockSize = getQueueBlockSize (setup.sampleRate);
		config.numBlocks = getNumQueueBlocks ();
		config.alignment = blockAlignment;
		config.userContextID = getUserContextID ();
		return true;
	})
	{
	}

	void setDisplayLatency (int32 latency) { displayLatency = latency; }

protected:
	bool lockBlock ()
	{
		if (!isEnabled ())
			return false;
		return getCurrentData () != nullptr;
	}

	void freeBlock (bool send = true)
	{
		if (!isEnabled ())
			return;
		if (send)
			sendCurrentBlock ();
		else
			discardCurrentBlock ();
		currentBlock.blockID = InvalidDataExchangeBlockID;
	}

	ExchangeDataT* getCurrentData ()
	{
		auto block = getCurrentOrNewBlock ();
		if (block.blockID == InvalidDataExchangeBlockID)
			return nullptr;
		if (currentBlock != block)
		{
			currentBlock = block;
			onNewLockedBlock ();
		}
		return reinterpret_cast<ExchangeDataT*> (currentBlock.data);
	}
};

//------------------------------------------------------------------------
struct AudioBlockExchangeProcessor : ExchangeDataProcessor<SampleBufferExchangeData>
{
private:
	uint32 numFilled {0};
	uint32 numSamplesToSend {0};

	float* left {nullptr};
	float* right {nullptr};

public:
	using Base::Base;

	uint32 calcNumSamplesToSend (SampleRate sampleRate) const
	{
		return sampleRate / 120; // 120 Hz
	}
	DataExchangeUserContextID getUserContextID () const override { return SampleBufferQueueID; }

	void onOpen (SampleRate sampleRate) override
	{
		numSamplesToSend = calcNumSamplesToSend (sampleRate);
	}
	uint32 getQueueBlockSize (SampleRate sampleRate) const override
	{
		return calculateExampleDataSizeForSamples (calcNumSamplesToSend (sampleRate), 2);
	}
	uint32 getNumQueueBlocks () const override { return 32; }
	void onNewLockedBlock () override
	{
		auto data = getCurrentData ();
		assert (data);
		*data = {}; // initialize with default
		data->numSamples = numSamplesToSend;
		left = &data->sampleData[0];
		right = &data->sampleData[sampleDataOffsetForChannel (1, numSamplesToSend)];
		numFilled = 0;
	}
	bool onProcess (const ProcessData& processData)
	{
		if (!isEnabled ())
			return true;

		auto data = getCurrentData ();
		if (!data)
			return true;

		float* outputLeft = processData.outputs[0].channelBuffers32[0];
		float* outputRight = processData.outputs[0].channelBuffers32[1];

		auto numSamples = static_cast<uint32> (processData.numSamples);
		while (numSamples > 0)
		{
			auto numSamplesFreeInBlock = numSamplesToSend - numFilled;
			auto numSamplesToCopy = std::min<uint32> (numSamplesFreeInBlock, numSamples);
			memcpy (left, outputLeft, numSamplesToCopy * sizeof (float));
			memcpy (right, outputRight, numSamplesToCopy * sizeof (float));
			numFilled += numSamplesToCopy;
			numSamples -= numSamplesToCopy;
			left += numSamplesToCopy;
			right += numSamplesToCopy;
			outputLeft += numSamplesToCopy;
			outputRight += numSamplesToCopy;
			if (numFilled == numSamplesToSend)
			{
				data->numChannels = 2;
				data->sampleRate = processData.processContext->sampleRate;
				data->systemTime = processData.processContext->systemTime;
				freeBlock ();
				if (!lockBlock ())
					return false;
				data = getCurrentData ();
				if (data == nullptr)
					return false;
			}
		}
		return true;
	}
};

//------------------------------------------------------------------------
struct DataExchangeProcessor : AudioEffect, IAudioPresentationLatency
{
	OBJ_METHODS (DataExchangeProcessor, AudioEffect)
	DEFINE_INTERFACES
		DEF_INTERFACE (IAudioPresentationLatency)
	END_DEFINE_INTERFACES (AudioEffect)
	REFCOUNT_METHODS (AudioEffect)

	//--- ---------------------------------------------------------------------
	AudioBlockExchangeProcessor audioBlockProcessor {this};
	bool enableDataExchange {false};
	bool firstProcessCall {true};
	static bool forceUseMessageHandling;

	//--- ---------------------------------------------------------------------
	DataExchangeProcessor ()
	{
		setControllerClass (DataExchangeControllerUID);
		processContextRequirements.needSystemTime ();
	}

	//--- ---------------------------------------------------------------------
	tresult PLUGIN_API initialize (FUnknown* context) override
	{
		auto res = AudioEffect::initialize (context);
		if (res == kResultTrue)
		{
			addAudioInput (STR16 ("AudioInput"), Vst::SpeakerArr::kStereo);
			addAudioOutput (STR16 ("AudioOutput"), Vst::SpeakerArr::kStereo);
		}
		return res;
	}

	//--- ---------------------------------------------------------------------
	tresult PLUGIN_API terminate () override { return AudioEffect::terminate (); }

	//--- ---------------------------------------------------------------------
	tresult PLUGIN_API connect (IConnectionPoint* other) override
	{
		auto res = AudioEffect::connect (other);
		audioBlockProcessor.onConnect (other, getHostContext ());
		return res;
	}

	//--- ---------------------------------------------------------------------
	tresult PLUGIN_API disconnect (IConnectionPoint* other) override
	{
		audioBlockProcessor.onDisconnect (other);
		return AudioEffect::disconnect (other);
	}

	//--- ---------------------------------------------------------------------
	tresult PLUGIN_API notify (IMessage* message) override
	{
		if (std::string_view (MessageIDForceMessageHandling) == message->getMessageID ())
		{
			if (auto attributes = message->getAttributes ())
			{
				int64 value;
				if (attributes->getInt (MessageKeyValue, value) == kResultTrue)
				{
					forceUseMessageHandling = value;
				}
			}
		}
		return AudioEffect::notify (message);
	}

	//--- ---------------------------------------------------------------------
	tresult PLUGIN_API setBusArrangements (Vst::SpeakerArrangement* inputs, int32 numIns,
	                                       Vst::SpeakerArrangement* outputs, int32 numOuts) override
	{
		if (numIns == numOuts && numIns == 1 && inputs[0] == outputs[0] &&
		    inputs[0] == SpeakerArr::kStereo)
		{
			return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
		}
		return kResultFalse;
	}

	//--- ---------------------------------------------------------------------
	tresult PLUGIN_API setAudioPresentationLatencySamples (BusDirection dir, int32 busIndex,
	                                                       uint32 latencyInSamples) override
	{
		if (dir == BusDirections::kOutput && busIndex == 0)
		{
			audioBlockProcessor.setDisplayLatency (latencyInSamples);
		}
		return kResultTrue;
	}

	//--- ---------------------------------------------------------------------
	tresult PLUGIN_API setActive (TBool state) override
	{
		if (state)
		{
			audioBlockProcessor.onActivate (processSetup, forceUseMessageHandling);
			firstProcessCall = true;
		}
		else
		{
			audioBlockProcessor.onDeactivate ();
		}
		return AudioEffect::setActive (state);
	}

	//--- ---------------------------------------------------------------------
	void processParameterChange (IParamValueQueue* queue)
	{
		int32 pointCount = queue->getPointCount ();
		ParamID paramID = queue->getParameterId ();
		if (pointCount <= 0)
			return;
		ParamValue value;
		int32 sampleOffset;
		if (queue->getPoint (pointCount - 1, sampleOffset, value) != kResultTrue)
			return;
		switch (paramID)
		{
			case ParamIDEnableDataExchange:
			{
				enableDataExchange = value > 0.5;
				if (audioBlockProcessor.isEnabled () != enableDataExchange)
					audioBlockProcessor.enable (enableDataExchange);
				break;
			}
		}
	}

	//--- ---------------------------------------------------------------------
	void processInputParameterChanges (IParameterChanges* changes)
	{
		int32 numParamsChanged = changes->getParameterCount ();
		for (int32 index = 0; index < numParamsChanged; index++)
		{
			if (auto paramQueue = changes->getParameterData (index))
				processParameterChange (paramQueue);
		}
	}

	//--- ---------------------------------------------------------------------
	void stopDataExchange (ProcessData* data)
	{
		audioBlockProcessor.enable (false);
		if (data)
		{
			if (auto paramChanges = data->outputParameterChanges)
			{
				int32 index;
				if (auto queue = paramChanges->addParameterData (ParamIDEnableDataExchange, index))
				{
					queue->addPoint (0, 0., index);
				}
			}
		}
	}

	//--- ---------------------------------------------------------------------
	tresult PLUGIN_API process (ProcessData& data) override
	{
		if (data.inputParameterChanges)
			processInputParameterChanges (data.inputParameterChanges);
		if (firstProcessCall)
		{
			firstProcessCall = false;
			audioBlockProcessor.enable (enableDataExchange);
		}
		if (data.numSamples)
		{
			for (int32 channel = 0; channel < data.inputs[0].numChannels; channel++)
			{
				float* inputChannel = data.inputs[0].channelBuffers32[channel];
				float* outputChannel = data.outputs[0].channelBuffers32[channel];
				if (inputChannel == outputChannel)
					continue;
				for (auto sample = 0; sample < data.numSamples; ++sample)
				{
					auto inputSample = inputChannel[sample];
					outputChannel[sample] = inputSample;
				}
				data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;
			}
			if (enableDataExchange)
			{
				auto res = !audioBlockProcessor.onProcess (data);
				// check overflow and stop if necessary
				if (res)
					stopDataExchange (&data);
			}
		}
		return kResultTrue;
	}
};

bool DataExchangeProcessor::forceUseMessageHandling = false;

//------------------------------------------------------------------------
FUnknown* createDataExchangeProcessor (void*)
{
	return static_cast<IAudioProcessor*> (new DataExchangeProcessor);
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
