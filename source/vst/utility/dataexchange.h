//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/dataexchange.h
// Created by  : Steinberg, 06/2023
// Description : VST Data Exchange API Helper
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

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstdataexchange.h"

#include <functional>
#include <memory>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Helper class to provide a single API for plug-ins to transfer data from the realtime audio
 *	process to the edit controller either via the backwards compatible message handling protocol
 *	(see IMessage) or the new IDataExchangeHandler/IDataExchangeReceiver API.
 *
 *	To use this, make an instance of DataExchangeHandler a member of your IAudioProcessor class and
 *	call onConnect, onDisconnect, onActivate and onDeactivate when the processor is (dis-)connected
 *	and (de)activated. In your IAudioProcessor::process method you call getCurrentOrNewBlock () to
 *	get a block fill it with the data you want to send and then call sendCurrentBlock.
 *	See DataExchangeReceiverHandler on how to receive that data.
 */
class DataExchangeHandler
{
public:
	struct Config
	{
		/** the size of one block in bytes */
		uint32 blockSize;
		/** the number of blocks to request */
		uint32 numBlocks;
		/** the alignment of the buffer */
		uint32 alignment {32};
		/** a user defined context ID */
		DataExchangeUserContextID userContextID {0};
	};
	/** the callback will be called on setup processing to get the required configuration for the
	 * data exchange */
	using ConfigCallback = std::function<bool (Config& config, const ProcessSetup& setup)>;

	DataExchangeHandler (IAudioProcessor* processor, ConfigCallback&& callback);
	DataExchangeHandler (IAudioProcessor* processor, const ConfigCallback& callback);
	~DataExchangeHandler () noexcept;

	/** call this in AudioEffect::connect
	 *
	 *	provide the hostContext you get via AudioEffect::initiailze to this method
	 */
	void onConnect (IConnectionPoint* other, FUnknown* hostContext);

	/** call this in AudioEffect::disconnect
	 */
	void onDisconnect (IConnectionPoint* other);

	/** call this in AudioEffect::setActive(true)
	 */
	void onActivate (const Vst::ProcessSetup& setup, bool forceUseMessageHandling = false);

	/** call this in AudioEffect::setActive(false)
	 */
	void onDeactivate ();

	//--- ---------------------------------------------------------------------
	/** Get the current or a new block
	 *
	 * 	On the first call this will always return a new block, only after sendCurrentBlock or
	 *	discardCurrentBlock is called a new block will be acquired.
	 *	This may return an invalid DataExchangeBlock (check the blockID for
	 *	InvalidDataExchangeBlockID) when the queue is full.
	 *
	 *	[call only in process call]
	 */
	DataExchangeBlock getCurrentOrNewBlock ();

	/** Send the current block to the receiver
	 *
	 *	[call only in process call]
	 */
	bool sendCurrentBlock ();

	/** Discard the current block
	 *
	 *	[call only in process call]
	 */
	bool discardCurrentBlock ();

	/** Enable or disable the acquiring of new blocks (per default it is enabled)
	 *
	 *	If you disable this then the getCurrentOrNewBlock will always return an invalid block.
	 *
	 *	[call only in process call]
	 */
	void enable (bool state);

	/** Ask if enabled
	 *
	 *	[call only in process call]
	 */
	bool isEnabled () const;
//------------------------------------------------------------------------

private:
	DataExchangeHandler (IAudioProcessor* processor);
	struct Impl;
	std::unique_ptr<Impl> impl;
};

//------------------------------------------------------------------------
/** Helper class to provide a single API for plug-ins to transfer data from the realtime audio
 *	process to the edit controller either via the message handling protocol (see IMessage) or the
 *	new IDataExchangeHandler/IDataExchangeReceiver API.
 *
 *	This is the other side of the DataExchangeHandler on the edit controller side. Make this a
 *	member of your edit controller and call onMessage for every IMessage you get via
 *	IConnectionPoint::notify. Your edit controller must implement the IDataExchangeReceiver
 *	interface.
 */
class DataExchangeReceiverHandler
{
public:
	DataExchangeReceiverHandler (IDataExchangeReceiver* receiver);
	~DataExchangeReceiverHandler () noexcept;

	/** call this for every message you receive via IConnectionPoint::notify
	 *
	 *	@return	true if the message was handled
	 */
	bool onMessage (IMessage* msg);

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};

//------------------------------------------------------------------------
inline bool operator!= (const DataExchangeBlock& lhs, const DataExchangeBlock& rhs)
{
	return lhs.data != rhs.data || lhs.size != rhs.size || lhs.blockID != rhs.blockID;
}

//------------------------------------------------------------------------
inline bool operator== (const DataExchangeBlock& lhs, const DataExchangeBlock& rhs)
{
	return lhs.data == rhs.data && lhs.size == rhs.size && lhs.blockID == rhs.blockID;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
