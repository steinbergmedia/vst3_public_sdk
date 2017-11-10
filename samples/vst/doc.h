//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : public.sdk/samples/vst/doc.h
// Created by  : Steinberg, 12/2005
// Description : Main Documentation of VST 3 Examples
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2017, Steinberg Media Technologies GmbH, All Rights Reserved
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

/** \mainpage VST 3 Examples
********************************************************************
The SDK includes some Plug-ins and Host implementation examples: the Legendaries \ref again "AGain" and \ref adelay ADelay,
thanks Paul Kellet the OpenSource \ref mda "mda Plug-ins", a basic \ref NoteExpressionSynth supporting "Note Expression Event" 
and an example of pitchnames support Plug-in (\ref PitchNames) and...:
\n
- \ref adelay
- \ref again
- \ref mdaplugs
- \ref NoteExpressionSynth
- \ref PitchNames
- \ref HostChecker
- \ref channelcontext
- \ref prefetchablesupport
- \ref programchange

Note: They use cmake as project generator: \ref cmakeUse

\n
\n
***************************
\section adelay ADelay Plug-in
***************************
Very simple delay Plug-in: 
- only one parameter (a delay)

Check the folder <a href="../../public.sdk/samples/vst/adelay" target=_blank>public.sdk/samples/vst/adelay</a> of the SDK!

Classes:
- \ref Steinberg::Vst::ADelayProcessor "ADelayProcessor"
- \ref Steinberg::Vst::ADelayController "ADelayController"
\n
\n
***************************
\section again AGain Plug-in
***************************
The SDK includes a AGain Plug-in which is a very simple VST 3 Plug-in. This Plug-in:
- is multichannel compatible,
- supports bypass processing,
- has an automated gain parameter,
- has an Event input bus (allowing to use noteOn velocity to control the gain factor),
- has a VU peak meter,
- uses the VSTGUI4 library
- a version of this Plug-in with side-chaining is available (showing a Plug-in using the same controller and different components)

\image html "again.jpg"

Check the folder <a href="../../public.sdk/samples/vst/again" target=_blank>public.sdk/samples/vst/again</a> of the SDK!

Classes:
- \ref Steinberg::Vst::AGain "AGain"
- \ref Steinberg::Vst::AGainWithSideChain "AGainWithSideChain" (used for side-chain version)
- \ref Steinberg::Vst::AGainController "AGainController"
\n
\n
***************************
\section mdaplugs mda Plug-ins
***************************
- Effects (stereo to stereo Plug-ins): 
	- Ambience    : Reverb
	- Bandisto    : Multi-band Distortion
	- BeatBox     : Drum Replacer
	- Combo       : Amp and Speaker Simulator
	- DeEsser     : High frequency Dynamics Processor
	- Degrade     : Sample quality reduction
	- Delay       : Simple stereo delay with feedback tone control
	- Detune      : Simple up/down pitch shifting thickener
	- Dither      : Range of dither types including noise shaping
	- DubDelay    : Delay with feedback saturation and time/pitch modulation
	- Dynamics    : Compressor / Limiter / Gate
	- Image	      : Stereo image adjustment and M-S matrix
	- Leslie      : Rotary speaker simulator
	- Limiter     : Opto-electronic style limiter
	- Loudness    : Equal loudness contours for bass EQ and mix correction
	- MultiBand   : Multi-band compressor with M-S processing modes
	- Overdrive   : Soft distortion
	- RePsycho!   : Drum loop pitch changer
	- RezFilter   : Resonant filter with LFO and envelope follower
	- RingMod     : Simple Ring Modulator
	- Round Panner: 3D panner
	- Shepard     : Continuously rising/falling tone generator
	- SpecMeter   : Stereo 13 Bands spectral Meter
	- Splitter    : Frequency / level crossover for setting up dynamic processing
	- Stereo Simulator: Haas delay and comb filtering
	- Sub-Bass Synthesizer: Several low frequency enhancement methods
	- TalkBox     : High resolution vocoder
	- TestTone    : Signal generator with pink and white noise, impulses and sweeps
	- Thru-Zero Flanger : Classic tape-flanging simulation
	- Tracker     : Pitch tracking oscillator, or pitch tracking EQ

- Instruments (1 Event input, 1 stereo Audio output): 
	- DX10        : Sounds similar to the later Yamaha DX synths including the heavy bass but with a warmer, cleaner tone.
	- EPiano      : Simple EPiano
	- JX10        : The Plug-in is designed for high quality (lower aliasing than most soft synths) and low processor usage
	- Piano       : Not designed to be the best sounding piano in the world, but boasts extremely low CPU and memory usage.

Based on the OpenSource mda Plug-ins (http://mda.smartelectronix.com/), this set of Plug-ins demonstrates how wrap DSP code in a VST 3 Plug-in.

Check the folder <a href="../../public.sdk/samples/vst/mda-vst3" target=_blank>public.sdk/samples/vst/mda-vst3</a> of the SDK!

Classes:
- \ref Steinberg::Vst::mda::BaseProcessor "BaseProcessor"
- \ref Steinberg::Vst::mda::BaseController "BaseController"
- \ref Steinberg::Vst::mda::BaseParameter "BaseParameter"
\n
\n
***************************
\section NoteExpressionSynth Note Expression Synth Plug-in
***************************
- Instrument Plug-in supporting note expression events
- It shows how easy it is to use vstgui4

\image html "note_expression_synth.jpg"

Check the folder <a href="../../public.sdk/samples/vst/note_expression_synth" target=_blank>public.sdk/samples/vst/note_expression_synth</a> of the SDK!

Classes:
- \ref Steinberg::Vst::NoteExpressionSynth::Processor "NoteExpressionSynth::Processor"
- \ref Steinberg::Vst::NoteExpressionSynth::Controller "NoteExpressionSynth::Controller"
- \ref Steinberg::Vst::NoteExpressionSynth::Voice "NoteExpressionSynth::Voice"
\n
\n
***************************
\section PitchNames PitchNames Plug-in
***************************
- Instrument Plug-in showing PitchNames support
- It shows how easy it is to use vstgui4

Check the folder <a href="../../public.sdk/samples/vst/pitchnames" target=_blank>public.sdk/samples/vst/pitchnames</a> of the SDK!

Classes:
- \ref Steinberg::Vst::PitchNamesController "PitchNamesController"
- \ref Steinberg::Vst::PitchNamesProcessor "PitchNamesProcessor"
- \ref VSTGUI::PitchNamesDataBrowserSource "PitchNamesDataBrowserSource"
\n
\n
***************************
\section HostChecker HostChecker Plug-in
***************************
- Instrument and Fx Plug-in checking the VST 3 support of a host.

\image html "hostchecker.jpg"

Check the folder <a href="../../public.sdk/samples/vst/hostchecker" target=_blank>public.sdk/samples/vst/hostchecker</a> of the SDK!

\n
***************************
\section channelcontext TestChannelContext Plug-in
***************************
Very simple Plug-in:
- showing how to use the \ref Steinberg::Vst::ChannelContext::IInfoListener interface
- using a generic UI

\image html "channelcontext.jpg"

Check the folder <a href="../../public.sdk/samples/vst/channelcontext" target=_blank>public.sdk/samples/vst/channelcontext</a> of the SDK!
\n
\n
\n
***************************
\section prefetchablesupport TestPrefetchableSupport Plug-in
***************************
Very simple Plug-in:
- showing how to use the \ref Steinberg::Vst::IPrefetchableSupport interface
- using a generic UI

\image html "prefetchablesupport.jpg"

Check the folder <a href="../../public.sdk/samples/vst/prefetchablesupport" target=_blank>public.sdk/samples/vst/prefetchablesupport</a> of the SDK!
\n
\n
\n
***************************
\section programchange TestProgramChange Plug-in
***************************
Very simple Plug-in:
- showing how to support Program List
- using a generic UI

\image html "programChange.jpg"

Check the folder <a href="../../public.sdk/samples/vst/programchange" target=_blank>public.sdk/samples/vst/programchange</a> of the SDK!
\n
*/
