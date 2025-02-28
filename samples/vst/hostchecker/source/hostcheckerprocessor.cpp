//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/hostchecker/source/hostcheckerprocessor.cpp
// Created by  : Steinberg, 04/2012
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
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "hostcheckerprocessor.h"
#include "cids.h"
#include "hostcheckercontroller.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "public.sdk/source/vst/vsteventshelper.h"

#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstpluginterfacesupport.h"

#include "base/source/fstreamer.h"

#include <cmath>

namespace Steinberg {
namespace Vst {

#define THREAD_CHECK_MSG(msg) "The host called '" msg "' in the wrong thread context.\n"

bool THREAD_CHECK_EXIT = false;

static constexpr int64 kRefreshRateForExchangePC = 40000000; // 25Hz

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
HostCheckerProcessor::HostCheckerProcessor ()
{
	mCurrentState = State::kUninitialized;

	mLatency = 256;

	setControllerClass (HostCheckerControllerUID);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::initialize (FUnknown* context)
{
	tresult result = AudioEffect::initialize (context);
	if (result == kResultOk)
	{
		dataExchangeHandler =
		    new DataExchangeHandler (this, [this] (auto& config, const auto& /*setup*/) {
			    config.numBlocks = 5;
			    config.blockSize = sizeof (ProcessContext);
			    return true;
		    });

		if (mCurrentState != State::kUninitialized)
			addLogEvent (kLogIdInvalidStateInitializedMissing);

		mCurrentState = State::kInitialized;

		addAudioInput (USTRING ("Audio Input"), SpeakerArr::kStereo);
		addAudioInput (USTRING ("Aux Input 1"), SpeakerArr::kStereo, kAux, 0);
		auto AAXContext = U::cast<IVst3ToAAXWrapper> (context);
		if (AAXContext == nullptr)
		{
			addAudioInput (USTRING ("Aux Input 2"), SpeakerArr::kMono, kAux, 0);
			addAudioInput (USTRING ("Aux Input 3"), SpeakerArr::kMono, kAux, 0);
			addAudioInput (USTRING ("Aux Input 4"), SpeakerArr::kMono, kAux, 0);
			addAudioInput (USTRING ("Aux Input 5"), SpeakerArr::kMono, kAux, 0);
			addAudioInput (USTRING ("Aux Input 6"), SpeakerArr::kMono, kAux, 0);
			addAudioInput (USTRING ("Aux Input 7"), SpeakerArr::kMono, kAux, 0);
			addAudioInput (USTRING ("Aux Input 8"), SpeakerArr::kMono, kAux, 0);
			addAudioInput (USTRING ("Aux Input 9"), SpeakerArr::kMono, kAux, 0);
			addAudioInput (USTRING ("Aux Input 10"), SpeakerArr::kMono, kAux, 0);
		}

		addAudioOutput (USTRING ("Audio Output"), SpeakerArr::kStereo);

		addEventInput (USTRING ("Event Input 1"), 1);
		addEventInput (USTRING ("Event Input 2"), 1);

		addEventOutput (USTRING ("Event Output 1"), 1);
		addEventOutput (USTRING ("Event Output 2"), 1);

		mHostCheck.setComponent (this);
	}

	if (auto plugInterfaceSupport = U::cast<IPlugInterfaceSupport> (context))
	{
		addLogEvent (kLogIdIPlugInterfaceSupportSupported);

		if (plugInterfaceSupport->isPlugInterfaceSupported (IAudioPresentationLatency::iid) ==
		    kResultTrue)
			addLogEvent (kLogIdAudioPresentationLatencySamplesSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (IPrefetchableSupport::iid) ==
		    kResultTrue)
			addLogEvent (kLogIdIPrefetchableSupportSupported);
		if (plugInterfaceSupport->isPlugInterfaceSupported (IProcessContextRequirements::iid) ==
		    kResultTrue)
			addLogEvent (kLogIdIProcessContextRequirementsSupported);
	}
	else
	{
		addLogEvent (kLogIdIPlugInterfaceSupportNotSupported);
	}
	return result;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::terminate ()
{
	delete dataExchangeHandler;
	dataExchangeHandler = nullptr;

	if (mCurrentState == State::kUninitialized)
	{
		// redundance
	}
	else if (mCurrentState != State::kSetupDone)
	{
		// wrong state
	}
	mCurrentState = State::kUninitialized;
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
void HostCheckerProcessor::addLogEvent (Steinberg::int32 logId)
{
	mHostCheck.getEventLogger ().addLogEvent (logId);
}

//-----------------------------------------------------------------------------
void HostCheckerProcessor::addLogEventMessage (const LogEvent& logEvent)
{
	auto* evt = NEW LogEvent (logEvent);
	{
		Base::Thread::FGuard guard (msgQueueLock);
		msgQueue.push_back (evt);
	}
}

//-----------------------------------------------------------------------------
void HostCheckerProcessor::sendNowAllLogEvents ()
{
	const EventLogger::Codes& errors = mHostCheck.getEventLogs ();
	auto iter = errors.begin ();
	while (iter != errors.end ())
	{
		if ((*iter).fromProcessor && (*iter).count > 0)
		{
			sendLogEventMessage ((*iter));
		}
		++iter;
	}
	mHostCheck.getEventLogger ().resetLogEvents ();
}

//-----------------------------------------------------------------------------
void HostCheckerProcessor::sendLogEventMessage (const LogEvent& logEvent)
{
	if (auto message = owned (allocateMessage ()))
	{
		message->setMessageID ("LogEvent");
		IAttributeList* attributes = message->getAttributes ();
		if (attributes)
		{
			SMTG_ASSERT (logEvent.id >= 0);
			attributes->setInt ("ID", logEvent.id);
			attributes->setInt ("Count", logEvent.count);
			sendMessage (message);
		}
	}
}

//-----------------------------------------------------------------------------
ProcessContext* HostCheckerProcessor::getCurrentExchangeData ()
{
	if (!dataExchangeHandler)
		return nullptr;

	auto block = dataExchangeHandler->getCurrentOrNewBlock ();
	if (block.blockID == InvalidDataExchangeBlockID)
		return nullptr;
	if (mCurrentExchangeBlock != block)
	{
		mCurrentExchangeBlock = block;
	}
	return reinterpret_cast<ProcessContext*> (mCurrentExchangeBlock.data);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::setAudioPresentationLatencySamples (
    BusDirection /*dir*/, int32 /*busIndex*/, uint32 /*latencyInSamples*/)
{
	addLogEvent (kLogIdAudioPresentationLatencySamplesSupported);
	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::getPrefetchableSupport (PrefetchableSupport& prefetchable)
{
	addLogEvent (kLogIdIPrefetchableSupportSupported);
	prefetchable = kIsYetPrefetchable;
	return kResultTrue;
}

//-----------------------------------------------------------------------------
uint32 PLUGIN_API HostCheckerProcessor::getProcessContextRequirements ()
{
	addLogEvent (kLogIdIProcessContextRequirementsSupported);

	processContextRequirements.needSystemTime ();
	processContextRequirements.needContinousTimeSamples ();
	processContextRequirements.needProjectTimeMusic ();
	processContextRequirements.needBarPositionMusic ();
	processContextRequirements.needCycleMusic ();
	processContextRequirements.needSamplesToNextClock ();
	processContextRequirements.needTempo ();
	processContextRequirements.needTimeSignature ();
	processContextRequirements.needChord ();
	processContextRequirements.needFrameRate ();
	processContextRequirements.needTransportState ();

	return AudioEffect::getProcessContextRequirements ();
}

//-----------------------------------------------------------------------------
void HostCheckerProcessor::informLatencyChanged ()
{
	auto* evt = NEW LogEvent;
	evt->id = kLogIdInformLatencyChanged;
	{
		Base::Thread::FGuard guard (msgQueueLock);
		msgQueue.push_back (evt);
	}
}

//-----------------------------------------------------------------------------
void HostCheckerProcessor::sendLatencyChanged ()
{
	if (IMessage* newMsg = allocateMessage ())
	{
		newMsg->setMessageID ("Latency");
		if (auto* attr = newMsg->getAttributes ())
			attr->setFloat ("Value", mWantedLatency);
		sendMessage (newMsg);
	}
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::process (ProcessData& data)
{
	mHostCheck.validate (data, mMinimumOfInputBufferCount, mMinimumOfOutputBufferCount);

	if (mCurrentState != State::kProcessing)
	{
		addLogEvent (kLogIdInvalidStateProcessingMissing);
	}
	if (mSetActiveCalled)
	{
		mSetActiveCalled = false;
		addLogEvent (kLogIdSetActiveCalledSupported);
	}
	if (mCheckGetLatencyCall)
	{
		mCheckGetLatencyCall = false;
		if (mGetLatencyCalled)
		{
			if (!mGetLatencyCalledAfterSetActive)
			{
				addLogEvent (kLogIdGetLatencyCalledbeforeSetActive);
			}
		}
		else
			addLogEvent (kLogIdgetLatencyNotCalled);
	}

	// flush parameters case
	if (data.numInputs == 0 && data.numOutputs == 0)
	{
		addLogEvent (kLogIdParametersFlushSupported);
	}
	if (data.processMode == Vst::kOffline)
		addLogEvent (kLogIdProcessModeOfflineSupported);
	else if (data.processMode == kRealtime)
		addLogEvent (kLogIdProcessModeRealtimeSupported);
	else if (data.processMode == kPrefetch)
		addLogEvent (kLogIdProcessModePrefetchSupported);

	if (data.processContext)
	{
		if (dataExchangeHandler)
		{
			if (!dataExchangeHandler->isEnabled ())
				dataExchangeHandler->enable (true);

			if (data.processContext->systemTime - mLastExchangeBlockSendSystemTime >
			    kRefreshRateForExchangePC)
			{
				mLastExchangeBlockSendSystemTime = data.processContext->systemTime;
				if (auto pc = getCurrentExchangeData ())
				{
					memcpy (pc, data.processContext, sizeof (ProcessContext));
					dataExchangeHandler->sendCurrentBlock ();
				}
				else
					dataExchangeHandler->discardCurrentBlock ();
			}
		}

		if (data.processContext->state & ProcessContext::kPlaying)
			addLogEvent (kLogIdProcessContextPlayingSupported);
		if (data.processContext->state & ProcessContext::kRecording)
			addLogEvent (kLogIdProcessContextRecordingSupported);
		if (data.processContext->state & ProcessContext::kCycleActive)
			addLogEvent (kLogIdProcessContextCycleActiveSupported);

		if (data.processContext->state & ProcessContext::kSystemTimeValid)
			addLogEvent (kLogIdProcessContextSystemTimeSupported);
		if (data.processContext->state & ProcessContext::kContTimeValid)
			addLogEvent (kLogIdProcessContextContTimeSupported);
		if (data.processContext->state & ProcessContext::kProjectTimeMusicValid)
			addLogEvent (kLogIdProcessContextTimeMusicSupported);
		if (data.processContext->state & ProcessContext::kBarPositionValid)
			addLogEvent (kLogIdProcessContextBarPositionSupported);
		if (data.processContext->state & ProcessContext::kCycleValid)
			addLogEvent (kLogIdProcessContextCycleSupported);
		if (data.processContext->state & ProcessContext::kTempoValid)
			addLogEvent (kLogIdProcessContextTempoSupported);
		if (data.processContext->state & ProcessContext::kTimeSigValid)
			addLogEvent (kLogIdProcessContextTimeSigSupported);
		if (data.processContext->state & ProcessContext::kChordValid)
			addLogEvent (kLogIdProcessContextChordSupported);

		if (data.processContext->state & ProcessContext::kSmpteValid)
			addLogEvent (kLogIdProcessContextSmpteSupported);
		if (data.processContext->state & ProcessContext::kClockValid)
			addLogEvent (kLogIdProcessContextClockSupported);

		if (mLastProjectTimeSamples != kMinInt64)
		{
			bool playbackChanged = (data.processContext->state & ProcessContext::kPlaying) !=
			                       (mLastState & ProcessContext::kPlaying);
			if ((mLastState & ProcessContext::kPlaying) == 0)
			{
				if (mLastProjectTimeSamples != data.processContext->projectTimeSamples)
				{
					if (playbackChanged)
						addLogEvent (kLogIdProcessPlaybackChangedDiscontinuityDetected);
					else
						addLogEvent (kLogIdProcessDiscontinuityDetected);
				}
			}
			else if (data.processContext->state & ProcessContext::kPlaying)
			{
				if (mLastProjectTimeSamples + mLastNumSamples !=
				    data.processContext->projectTimeSamples)
				{
					if (playbackChanged)
						addLogEvent (kLogIdProcessPlaybackChangedDiscontinuityDetected);
					else
						addLogEvent (kLogIdProcessDiscontinuityDetected);
				}
			}
			if ((data.processContext->state & ProcessContext::kContTimeValid) &&
			    (mLastContinuousProjectTimeSamples != kMinInt64))
			{
				if (mLastContinuousProjectTimeSamples + mLastNumSamples !=
				    data.processContext->continousTimeSamples)
				{
					if (playbackChanged)
						addLogEvent (kLogIdProcessPlaybackChangedContinuousDiscontinuityDetected);
					else
						addLogEvent (kLogIdProcessContinuousDiscontinuityDetected);
				}
			}
		}
		mLastProjectTimeSamples = data.processContext->projectTimeSamples;
		mLastContinuousProjectTimeSamples = data.processContext->continousTimeSamples;
		mLastState = data.processContext->state;
		mLastNumSamples = data.numSamples;
	} // (data.processContext)

	Algo::foreach (data.inputParameterChanges, [&] (IParamValueQueue& paramQueue) {
		Algo::foreachLast (paramQueue, [&] (ParamID id, int32 /*sampleOffset*/, ParamValue value) {
			if (id == kBypassTag)
			{
				mBypass = value > 0;
				mBypassProcessorFloat.setActive (mBypass);
				mBypassProcessorDouble.setActive (mBypass);
			}
			else if (id == kLatencyTag)
			{
				mWantedLatency = static_cast<uint32> (value * HostChecker::kMaxLatency);
				addLogEvent (kLogIdInformLatencyChanged);
			}
			else if (id == kProcessingLoadTag)
			{
				mProcessingLoad = static_cast<float> (value);
			}
			else if (id == kGeneratePeaksTag)
			{
				mGeneratePeaks = static_cast<float> (value);
			}
		});
	});

	if (mBypassProcessorFloat.isActive ())
	{
		if (data.symbolicSampleSize == kSample32)
			mBypassProcessorFloat.process (data);
		else // kSample64
			mBypassProcessorDouble.process (data);
	}
	else if (data.numSamples && data.numOutputs)
	{
		if (data.numInputs > 0 && data.inputs[0].silenceFlags != 0)
		{
			addLogEvent (kLogIdSilentFlagsSupported);
		}
		if (data.numInputs > 1 && data.inputs[1].silenceFlags != 0)
		{
			addLogEvent (kLogIdSilentFlagsSCSupported);
		}
		// Generate Processing load
		if (mProcessingLoad > 0)
		{
			int32 countLoop = static_cast<int32> (mProcessingLoad * 400);
			if (data.symbolicSampleSize == kSample32)
			{
				auto tmp1 = data.outputs[0].channelBuffers32[0][0];
				for (int32 i = 0; i < data.inputs[0].numChannels; i++)
				{
					for (int32 s = 0; s < data.numSamples; s++)
					{
						auto tmp2 = data.inputs[0].channelBuffers32[i][s];
						for (int32 loop = 0; loop < countLoop; loop++)
						{
							tmp2 = sinf (tmp2) * cosf (tmp2);
						}
						data.outputs[0].channelBuffers32[0][0] = tmp2;
					}
				}
				data.outputs[0].channelBuffers32[0][0] = tmp1;
			}
			else // kSample64
			{
				auto tmp1 = data.outputs[0].channelBuffers64[0][0];
				for (int32 i = 0; i < data.inputs[0].numChannels; i++)
				{
					for (int32 s = 0; s < data.numSamples; s++)
					{
						auto tmp2 = data.inputs[0].channelBuffers64[i][s];
						for (int32 loop = 0; loop < countLoop; loop++)
						{
							tmp2 = sin (tmp2) * cos (tmp2);
						}
						data.outputs[0].channelBuffers64[0][0] = tmp2;
					}
				}
				data.outputs[0].channelBuffers64[0][0] = tmp1;
			}
		}

		// Generate output (peak at a given tempo) (overwrite the input)
		if (mGeneratePeaks > 0 && data.processContext)
		{
			if (data.symbolicSampleSize == kSample32)
				Algo::clear32 (data.outputs, data.numSamples, data.numOutputs);
			else // kSample64
				Algo::clear64 (data.outputs, data.numSamples, data.numOutputs);

			float coef = mGeneratePeaks * mLastBlockMarkerValue;

			double distance2BarPosition =
			    (data.processContext->projectTimeMusic - data.processContext->barPositionMusic) /
			    (4. * data.processContext->timeSigNumerator) *
			    data.processContext->timeSigDenominator / 2.;

			// Normalized Tempo [0, 360] => [0, 1]
			double tempo = data.processContext->tempo / 360.;

			if (data.symbolicSampleSize == kSample32)
			{
				for (int32 i = 0; i < data.outputs[0].numChannels && i < 1; i++)
				{
					data.outputs[0].channelBuffers32[i][0] = coef;
					if (data.processContext->state & ProcessContext::kTempoValid &&
					    data.numSamples > 3)
						data.outputs[0].channelBuffers32[i][3] = static_cast<float> (tempo);
				}
				if (data.processContext->state & ProcessContext::kBarPositionValid)
				{
					for (int32 i = 1; i < data.outputs[0].numChannels; i++)
					{
						data.outputs[0].channelBuffers32[i][0] =
						    static_cast<float> (distance2BarPosition);
					}
				}
			}
			else // kSample64
			{
				for (int32 i = 0; i < data.outputs[0].numChannels && i < 1; i++)
				{
					data.outputs[0].channelBuffers64[i][0] = coef;
					if (data.processContext->state & ProcessContext::kTempoValid &&
					    data.numSamples > 3)
						data.outputs[0].channelBuffers64[i][3] = tempo;
				}
				if (data.processContext->state & ProcessContext::kBarPositionValid)
				{
					for (int32 i = 1; i < data.outputs[0].numChannels; i++)
					{
						data.outputs[0].channelBuffers64[i][0] = distance2BarPosition;
					}
				}
			}
			// mLastBlockMarkerValue = -mLastBlockMarkerValue;

			data.outputs[0].silenceFlags = 0x0;

			const float kMaxNotesToDisplay = 5.f;

			// check event from all input events and send them to the output event
			Algo::foreach (data.inputEvents, [&] (Event& event) {
				switch (event.type)
				{
					//--- -------------------
					case Event::kNoteOnEvent:
						mNumNoteOns++;
						if (data.symbolicSampleSize == kSample32)
							data.outputs[0].channelBuffers32[0][event.sampleOffset] =
							    mNumNoteOns / kMaxNotesToDisplay;
						else // kSample64
							data.outputs[0].channelBuffers64[0][event.sampleOffset] =
							    mNumNoteOns / kMaxNotesToDisplay;
						if (data.outputEvents)
						{
							data.outputEvents->addEvent (event);

							Event evtMIDICC {};
							Helpers::initLegacyMIDICCOutEvent (
							    evtMIDICC, kCtrlModWheel, static_cast<uint8> (event.noteOn.channel),
							    static_cast<uint8> (event.noteOn.velocity * 127));
							data.outputEvents->addEvent (evtMIDICC);
						}
						break;
					//--- -------------------
					case Event::kNoteOffEvent:
						if (data.symbolicSampleSize == kSample32)
							data.outputs[0].channelBuffers32[1][event.sampleOffset] =
							    -mNumNoteOns / kMaxNotesToDisplay;
						else // kSample64
							data.outputs[0].channelBuffers64[1][event.sampleOffset] =
							    -mNumNoteOns / kMaxNotesToDisplay;
						if (data.outputEvents)
							data.outputEvents->addEvent (event);
						mNumNoteOns--;
						break;
					//--- -------------------
					default: break;
				}
			});
		}
		else
		{
			//---get audio buffers----------------
			uint32 sampleFramesSize = getSampleFramesSizeInBytes (processSetup, data.numSamples);
			void** in = getChannelBuffersPointer (processSetup, data.inputs[0]);
			void** out = getChannelBuffersPointer (processSetup, data.outputs[0]);

			int32 minNum = Min (data.outputs[0].numChannels, data.inputs[0].numChannels);

			for (int32 i = 0; i < minNum; i++)
			{
				// do not need to be copied if the buffers are the same
				if (in[i] != out[i])
				{
					memcpy (out[i], in[i], sampleFramesSize);
				}
			}
			data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

			for (int32 i = minNum; i < data.outputs[0].numChannels; i++)
			{
				memset (out[i], 0, sampleFramesSize);
				data.outputs[0].silenceFlags |= ((uint64)1 << i);
			}
		}
	}

	if (data.outputParameterChanges)
	{
		int32 idx;
		if (mLastProcessMode != data.processMode)
		{
			if (auto* queue =
			        data.outputParameterChanges->addParameterData (kParamProcessModeTag, idx))
				queue->addPoint (0, data.processMode * 0.5, idx);
			mLastProcessMode = data.processMode;
		}

		const EventLogger::Codes& errors = mHostCheck.getEventLogs ();
		auto iter = errors.begin ();

		uint32 warnIdValue[HostChecker::kParamWarnCount] = {0};
		while (iter != errors.end ())
		{
			if ((*iter).fromProcessor && (*iter).count > 0)
			{
				int64 id = (*iter).id;
				int64 offset = id / HostChecker::kParamWarnBitCount;
				id = id % HostChecker::kParamWarnBitCount;
				if (offset >= HostChecker::kParamWarnCount)
				{
					break;
				}
				warnIdValue[offset] |= 1 << id;
				// addLogEventMessage ((*iter));
			}
			++iter;
		}
		for (uint32 i = 0; i < HostChecker::kParamWarnCount; i++)
		{
			if (warnIdValue[i] != 0)
			{
				if (auto* queue =
				        data.outputParameterChanges->addParameterData (kProcessWarnTag + i, idx))
					queue->addPoint (
					    0, (double)warnIdValue[i] / double (HostChecker::kParamWarnStepCount), idx);
			}
		}
		mHostCheck.getEventLogger ().resetLogEvents ();
	}
	return kResultOk;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::setupProcessing (ProcessSetup& setup)
{
	if (mCurrentState != State::kInitialized && mCurrentState != State::kSetupDone)
	{
		addLogEvent (kLogIdInvalidStateInitializedMissing);
	}

	mCurrentState = State::kSetupDone;

	mHostCheck.setProcessSetup (setup);
	return AudioEffect::setupProcessing (setup);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::setActive (TBool state)
{
	if (dataExchangeHandler)
	{
		if (state)
			dataExchangeHandler->onActivate (processSetup);
		else
			dataExchangeHandler->onDeactivate ();
	}

	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerProcessor::setActive"),
	                          THREAD_CHECK_EXIT))
	{
		addLogEvent (kLogIdSetActiveCalledinWrongThread);
	}

	// we should not be in kActivated State!
	if (mCurrentState == State::kProcessing)
	{
		addLogEvent (kLogIdInvalidStateSetActiveWrong);
	}
	mCheckGetLatencyCall = true;

	if (!state)
	{
		// only possible previous State: kActivated
		if (mCurrentState == State::kSetupDone)
		{
			addLogEvent (kLogIdsetActiveFalseRedundant);
		}

		mCurrentState = State::kSetupDone;
		mBypassProcessorFloat.reset ();
		mBypassProcessorDouble.reset ();

		mGetLatencyCalledAfterSetActive = false;
	}
	else
	{
		mSetActiveCalled = true;

		// only possible previous State: kSetupDone
		if (mCurrentState == State::kActivated)
		{
			addLogEvent (kLogIdsetActiveTrueRedundant);
		}
		else if (mCurrentState != State::kSetupDone)
		{
			addLogEvent (kLogIdInvalidStateSetupMissing);
		}

		mCurrentState = State::kActivated;
		mLatency = mWantedLatency;

		// prepare bypass
		mBypassProcessorFloat.setup (*this, processSetup, mLatency);
		mBypassProcessorDouble.setup (*this, processSetup, mLatency);
	}
	mLastBlockMarkerValue = -0.5f;
	mNumNoteOns = 0;

	sendNowAllLogEvents ();

	return AudioEffect::setActive (state);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::notify (IMessage* message)
{
	if (!message)
		return kInvalidArgument;

	if (FIDStringsEqual (message->getMessageID (), "Parameter"))
	{
		int64 paramId = -1;
		if (message->getAttributes ()->getInt ("ID", paramId) == kResultOk)
		{
			mHostCheck.addParameter (static_cast<ParamID> (paramId));
		}
	}

	return kResultOk;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::canProcessSampleSize (int32 symbolicSampleSize)
{
	if (symbolicSampleSize == kSample32)
	{
		addLogEvent (kLogIdCanProcessSampleSize32);
		return kResultTrue;
	}
	if (symbolicSampleSize == kSample64)
	{
		addLogEvent (kLogIdCanProcessSampleSize64);
		return kResultTrue;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
uint32 PLUGIN_API HostCheckerProcessor::getLatencySamples ()
{
	mCheckGetLatencyCall = true;
	mGetLatencyCalled = true;

	if (mSetActiveCalled)
		mGetLatencyCalledAfterSetActive = true;
	addLogEvent (kLogIdGetLatencySamples);
	return mLatency;
}

//-----------------------------------------------------------------------------
uint32 PLUGIN_API HostCheckerProcessor::getTailSamples ()
{
	addLogEvent (kLogIdGetTailSamples);
	return mLatency;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::getRoutingInfo (RoutingInfo& inInfo, RoutingInfo& outInfo)
{
	addLogEvent (kLogIdGetRoutingInfo);
	return AudioEffect::getRoutingInfo (inInfo, outInfo);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::activateBus (MediaType type, BusDirection dir, int32 index,
                                                      TBool state)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerProcessor::activateBus"),
	                          THREAD_CHECK_EXIT))
	{
		addLogEvent (kLogIdactivateBusCalledinWrongThread);
	}

	if (type == kAudio && dir == kInput)
	{
		if (index < 0 || index >= static_cast<int32> (getBusList (kAudio, kInput)->size ()))
			addLogEvent (kLogIdInvalidActivateAuxBus);
		else if (index > 0)
			addLogEvent (kLogIdActivateAuxBus);
	}

	auto result = AudioEffect::activateBus (type, dir, index, state);

	if (result == kResultTrue && type == kAudio)
	{
		if (auto list = getBusList (type, dir))
		{
			int32 lastActive = -1;
			for (int32 idx = static_cast<int32> (list->size ()) - 1; idx >= 0; --idx)
			{
				if (list->at (idx)->isActive ())
				{
					lastActive = idx;
					break;
				}
			}
			if (dir == kInput)
				mMinimumOfInputBufferCount = lastActive + 1;
			else
				mMinimumOfOutputBufferCount = lastActive + 1;
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::setBusArrangements (SpeakerArrangement* inputs,
                                                             int32 numIns,
                                                             SpeakerArrangement* outputs,
                                                             int32 numOuts)
{
	addLogEvent (kLogIdSetBusArrangements);
	return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::getBusArrangement (BusDirection dir, int32 busIndex,
                                                            SpeakerArrangement& arr)
{
	addLogEvent (kLogIdGetBusArrangements);
	return AudioEffect::getBusArrangement (dir, busIndex, arr);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::connect (IConnectionPoint* other)
{
	auto res = AudioEffect::connect (other);
	if (dataExchangeHandler)
		dataExchangeHandler->onConnect (other, getHostContext ());
	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::disconnect (IConnectionPoint* other)
{
	if (dataExchangeHandler)
		dataExchangeHandler->onDisconnect (other);

	return AudioEffect::disconnect (other);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::setProcessing (TBool state)
{
	if (state)
	{
		// only possible previous State: kActivated
		if (mCurrentState != State::kActivated)
			addLogEvent (kLogIdInvalidStateSetProcessingWrong);

		if (mCurrentState == State::kProcessing)
			addLogEvent (kLogIdsetProcessingTrueRedundant);
		mCurrentState = State::kProcessing;
	}
	else
	{
		if (mCurrentState != State::kProcessing)
			addLogEvent (kLogIdsetProcessingFalseRedundant);
		mCurrentState = State::kActivated;
	}

	AudioEffect::setProcessing (state);
	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::setState (IBStream* state)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerProcessor::setState"),
	                          THREAD_CHECK_EXIT))
	{
		addLogEvent (kLogIdProcessorSetStateCalledinWrongThread);
	}

	if (auto stream = U::cast<IStreamAttributes> (state))
	{
		if (IAttributeList* list = stream->getAttributes ())
		{
			addLogEvent (kLogIdIAttributeListInSetStateSupported);
		}
	}

	IBStreamer streamer (state, kLittleEndian);

	// version
	uint32 version;
	streamer.readInt32u (version);
	if (version < 1 || version > 1000)
	{
		version = 1;
		streamer.seek (-4, kSeekCurrent);
	}

	float saved = 0.f;
	if (streamer.readFloat (saved) == false)
		return kResultFalse;
	if (saved != 12345.67f)
	{
		SMTG_ASSERT (false)
	}

	uint32 latency = mLatency;
	if (streamer.readInt32u (latency) == false)
		return kResultFalse;

	uint32 bypass;
	if (streamer.readInt32u (bypass) == false)
		return kResultFalse;

	float processingLoad = 0.f;
	if (version > 1)
	{
		if (streamer.readFloat (processingLoad) == false)
			return kResultFalse;
	}

	mBypass = bypass > 0;
	mBypassProcessorFloat.setActive (mBypass);
	mBypassProcessorDouble.setActive (mBypass);
	mProcessingLoad = processingLoad;

	if (latency != mLatency)
	{
		mLatency = latency;
		sendLatencyChanged ();
	}

	return kResultOk;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerProcessor::getState (IBStream* state)
{
	if (!threadChecker->test (THREAD_CHECK_MSG ("HostCheckerProcessor::getState"),
	                          THREAD_CHECK_EXIT))
	{
		addLogEvent (kLogIdProcessorGetStateCalledinWrongThread);
	}

	if (!state)
		return kResultFalse;

	if (auto stream = U::cast<IStreamAttributes> (state))
	{
		if (IAttributeList* list = stream->getAttributes ())
		{
			addLogEvent (kLogIdIAttributeListInGetStateSupported);
		}
	}

	IBStreamer streamer (state, kLittleEndian);

	// version
	streamer.writeInt32u (2);

	float toSave = 12345.67f;
	streamer.writeFloat (toSave);
	streamer.writeInt32u (mLatency);
	streamer.writeInt32u (mBypass ? 1 : 0);
	streamer.writeFloat (mProcessingLoad);
	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
