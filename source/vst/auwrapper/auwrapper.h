//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/auwrapper/auwrapper.h
// Created by  : Steinberg, 12/2007
// Description : VST 3 -> AU Wrapper
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

/// \cond ignore

#pragma once

#ifdef SMTG_AUWRAPPER_USES_AUSDK
#if CA_USE_AUDIO_PLUGIN_ONLY
#include "AudioUnitSDK/AUBase.h"
#define AUWRAPPER_BASE_CLASS ausdk::AUBase
#else
#include "AudioUnitSDK/MusicDeviceBase.h"
#define AUWRAPPER_BASE_CLASS ausdk::MusicDeviceBase
#endif // CA_USE_AUDIO_PLUGIN_ONLY
#else
#if CA_USE_AUDIO_PLUGIN_ONLY
#include "AudioUnits/AUPublic/AUBase/AUBase.h"
#define AUWRAPPER_BASE_CLASS AUBase
#else
#include "AudioUnits/AUPublic/OtherBases/MusicDeviceBase.h"
#define AUWRAPPER_BASE_CLASS MusicDeviceBase
#endif // CA_USE_AUDIO_PLUGIN_ONLY
#endif // SMTG_AUWRAPPER_USES_AUSDK

#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/processdata.h"
#include "public.sdk/source/vst/utility/ringbuffer.h"
#include "public.sdk/source/vst/utility/rttransfer.h"
#include "base/source/fstring.h"
#include "base/source/timer.h"
#include "base/thread/include/flock.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmidilearn.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstunits.h"

#include <AudioToolbox/AudioToolbox.h>
#include <Cocoa/Cocoa.h>
#include <array>
#include <map>
#include <unordered_map>
#include <vector>

namespace Steinberg {
class VST3DynLibrary;

namespace Vst {

//------------------------------------------------------------------------
//------------------------------------------------------------------------
class AUWrapper : public AUWRAPPER_BASE_CLASS, public IComponentHandler, public ITimerCallback
{
public:
#ifdef SMTG_AUWRAPPER_USES_AUSDK
	using AUElement = ausdk::AUElement;
#else
	using AudioStreamBasicDescription = CAStreamBasicDescription;
#endif

	AUWrapper (ComponentInstanceRecord* ci);
	~AUWrapper ();

	//---ComponentBase---------------------
#if !CA_USE_AUDIO_PLUGIN_ONLY && !defined(SMTG_AUWRAPPER_USES_AUSDK)
	ComponentResult	Version () SMTG_OVERRIDE;
#endif
	void PostConstructor () SMTG_OVERRIDE;

	//---AUBase-----------------------------
	void Cleanup () SMTG_OVERRIDE;
	ComponentResult Initialize () SMTG_OVERRIDE;
#ifdef SMTG_AUWRAPPER_USES_AUSDK
	std::unique_ptr<AUElement> CreateElement (AudioUnitScope scope, AudioUnitElement element) SMTG_OVERRIDE;
#else
	AUElement* CreateElement (AudioUnitScope scope, AudioUnitElement element) SMTG_OVERRIDE;
#endif
	UInt32 SupportedNumChannels (const AUChannelInfo** outInfo) SMTG_OVERRIDE;
	bool StreamFormatWritable (AudioUnitScope scope, AudioUnitElement element) SMTG_OVERRIDE;
	ComponentResult ChangeStreamFormat (AudioUnitScope inScope, AudioUnitElement inElement, const AudioStreamBasicDescription& inPrevFormat, const AudioStreamBasicDescription& inNewFormat) SMTG_OVERRIDE;
	ComponentResult SetConnection (const AudioUnitConnection& inConnection) SMTG_OVERRIDE;
	ComponentResult GetParameterInfo (AudioUnitScope inScope, AudioUnitParameterID inParameterID, AudioUnitParameterInfo& outParameterInfo) SMTG_OVERRIDE;
	ComponentResult SetParameter (AudioUnitParameterID inID, AudioUnitScope inScope, AudioUnitElement inElement, AudioUnitParameterValue inValue, UInt32 inBufferOffsetInFrames) SMTG_OVERRIDE;

	ComponentResult SaveState (CFPropertyListRef* outData) SMTG_OVERRIDE;
	ComponentResult RestoreState (CFPropertyListRef inData) SMTG_OVERRIDE;

	ComponentResult Render (AudioUnitRenderActionFlags &ioActionFlags, const AudioTimeStamp &inTimeStamp, UInt32 inNumberFrames) SMTG_OVERRIDE;
	void processOutputEvents (const AudioTimeStamp &inTimeStamp);

#if !CA_USE_AUDIO_PLUGIN_ONLY && !defined(SMTG_AUWRAPPER_USES_AUSDK)
	int GetNumCustomUIComponents () SMTG_OVERRIDE;
	void GetUIComponentDescs (ComponentDescription* inDescArray) SMTG_OVERRIDE;
#endif

#ifdef SMTG_AUWRAPPER_USES_AUSDK
	OSStatus GetPropertyInfo (AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, UInt32 &outDataSize, bool &outWritable) SMTG_OVERRIDE;
#else
	OSStatus GetPropertyInfo (AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, UInt32 &outDataSize, Boolean &outWritable) SMTG_OVERRIDE;
#endif
	ComponentResult GetProperty (AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void* outData) SMTG_OVERRIDE;
	ComponentResult SetProperty (AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void* inData, UInt32 inDataSize) SMTG_OVERRIDE;
	bool CanScheduleParameters() const SMTG_OVERRIDE;

	Float64 GetLatency () SMTG_OVERRIDE;
	Float64 GetTailTime () SMTG_OVERRIDE;

	//---Factory presets
	OSStatus GetPresets (CFArrayRef* outData) const SMTG_OVERRIDE;
	OSStatus NewFactoryPresetSet (const AUPreset& inNewFactoryPreset) SMTG_OVERRIDE;

#if !CA_USE_AUDIO_PLUGIN_ONLY
	//---MusicDeviceBase-------------------------
	OSStatus HandleNoteOn (UInt8 inChannel, UInt8 inNoteNumber, UInt8 inVelocity, UInt32 inStartFrame) SMTG_OVERRIDE;
	OSStatus HandleNoteOff (UInt8 inChannel, UInt8 inNoteNumber, UInt8 inVelocity, UInt32 inStartFrame) SMTG_OVERRIDE;
	ComponentResult StartNote (MusicDeviceInstrumentID inInstrument, MusicDeviceGroupID inGroupID, NoteInstanceID* outNoteInstanceID, UInt32 inOffsetSampleFrame, const MusicDeviceNoteParams &inParams) SMTG_OVERRIDE;
	ComponentResult StopNote (MusicDeviceGroupID inGroupID, NoteInstanceID inNoteInstanceID, UInt32 inOffsetSampleFrame) SMTG_OVERRIDE;
	OSStatus GetInstrumentCount (UInt32 &outInstCount) const SMTG_OVERRIDE;

	//---AUMIDIBase------------------------------
	OSStatus HandleNonNoteEvent (UInt8 status, UInt8 channel, UInt8	data1, UInt8 data2, UInt32 inStartFrame) SMTG_OVERRIDE;
#endif

#if AUSDK_MIDI2_AVAILABLE
	OSStatus MIDIEventList (UInt32 inOffsetSampleFrame,
	                        const struct MIDIEventList* eventList) override;
	bool handleMIDIEventPacket (UInt32 inOffsetSampleFrame, const MIDIEventPacket* packet);
#endif

	//---custom----------------------------------
	void setControllerParameter (ParamID pid, ParamValue value);

	// return for a given midiChannel the unitID and the ProgramListID
	bool getProgramListAndUnit (int32 midiChannel, UnitID& unitId, ProgramListID& programListId);

	// restore preset state, add StateType "Project" to stream if loading from project
	ComponentResult restoreState (CFPropertyListRef inData, bool fromProject);

	//------------------------------------------------------------------------
#if !CA_USE_AUDIO_PLUGIN_ONLY && !defined(SMTG_AUWRAPPER_USES_AUSDK)
	static ComponentResult ComponentEntryDispatch (ComponentParameters* params, AUWrapper* This);
#endif
	//------------------------------------------------------------------------
	static CFBundleRef gBundleRef;
	//------------------------------------------------------------------------
	DECLARE_FUNKNOWN_METHODS

protected:
	//---from IComponentHandler-------------------
	tresult PLUGIN_API beginEdit (ParamID tag) SMTG_OVERRIDE;
	tresult PLUGIN_API performEdit (ParamID tag, ParamValue valueNormalized) SMTG_OVERRIDE;
	tresult PLUGIN_API endEdit (ParamID tag) SMTG_OVERRIDE;
	tresult PLUGIN_API restartComponent (int32 flags) SMTG_OVERRIDE;

	//---from ITimerCallback----------------------
	void onTimer (Timer* timer) SMTG_OVERRIDE;

	// internal helpers
	double getSampleRate () const { return sampleRate; }
	void updateProcessContext ();
	void syncParameterValues ();
	void cacheParameterValues ();
	void clearParameterValueCache ();
	void updateProgramChangesCache ();

	virtual IPluginFactory* getFactory ();
	void loadVST3Module ();
	void unloadVST3Module ();
	bool validateChannelPair (int inChannelsIn, int inChannelsOut, const AUChannelInfo* info,
	                          UInt32 numChanInfo) const;

	IAudioProcessor* audioProcessor;
	IEditController* editController;

	Timer* timer;

	HostProcessData processData;
	ParameterChanges processParamChanges;
	ParameterChanges outputParamChanges;
	ParameterChangeTransfer transferParamChanges;
	ParameterChangeTransfer outputParamTransfer;
	ProcessContext processContext;
	EventList eventList;

	typedef std::map<ParamID, AudioUnitParameterInfo> CachedParameterInfoMap;
	typedef std::map<UnitID, UnitInfo> UnitInfoMap;
	typedef std::vector<String> ClumpGroupVector;

	UnitInfoMap unitInfos;
	ClumpGroupVector clumpGroups;
	CachedParameterInfoMap cachedParameterInfos;
	Steinberg::Base::Thread::FLock parameterCacheChanging;

	NoteInstanceID noteCounter;
	double sampleRate;
	ParamID bypassParamID;

	AUPreset* presets;
	int32 numPresets;
	ParamID factoryProgramChangedID;

	AUParameterListenerRef paramListenerRef;
	std::vector<ParameterInfo> programParameters;

	static constexpr int32 kMaxProgramChangeParameters = 16;
	struct ProgramChangeInfo
	{
		ParamID pid {kNoParamId};
		int32 numPrograms {0};
	};
	using ProgramChangeInfoList = std::array<ProgramChangeInfo, kMaxProgramChangeParameters>; 
	using ProgramChangeInfoTransfer = RTTransferT<ProgramChangeInfoList>;
	ProgramChangeInfoList programChangeInfos;
	ProgramChangeInfoTransfer programChangeInfoTransfer;

	// midi mapping
	struct MidiMapping
	{
		using CC2ParamMap = std::unordered_map<CtrlNumber, ParamID>;
		using ChannelList = std::vector<CC2ParamMap>;
		using BusList = std::vector<ChannelList>;
		
		BusList busList;
		bool empty () const { return busList.empty () || busList[0].empty (); }
	};
	using MidiMappingTransfer = RTTransferT<MidiMapping>;

	MidiMappingTransfer midiMappingTransfer;
	MidiMapping midiMappingCache;

	struct MidiLearnEvent
	{
		int32 busIndex;
		int16 channel;
		CtrlNumber midiCC;
	};
	using MidiLearnRingBuffer = OneReaderOneWriter::RingBuffer<MidiLearnEvent>;
	MidiLearnRingBuffer midiLearnRingBuffer;
	IPtr<IMidiLearn> midiLearn;

	struct MIDIOutputCallbackHelper;

	int32 midiOutCount; // currently only 0 or 1 supported
	std::unique_ptr<MIDIOutputCallbackHelper> mCallbackHelper;
	EventList outputEvents;

	bool isInstrument;
	bool isBypassed;
	bool isOfflineRender;

private:
	void buildUnitInfos (IUnitInfo* unitInfoController, UnitInfoMap& units) const;
	void updateMidiMappingCache ();

	IPtr<VST3DynLibrary> dynLib;
};

//------------------------------------------------------------------------
class AutoreleasePool
{
public:
	AutoreleasePool () { ap = [[NSAutoreleasePool alloc] init]; }
	~AutoreleasePool () { [ap drain]; }
//------------------------------------------------------------------------
protected:
	NSAutoreleasePool* ap;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

/// \endcond
