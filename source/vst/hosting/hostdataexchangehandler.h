//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/hostdataexchangehandler.h
// Created by  : Steinberg, 06/2023
// Description : VST Data Exchange API Host Helper
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

#include "pluginterfaces/vst/ivstdataexchange.h"

#include <memory>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
struct IDataExchangeHandlerHost
{
	virtual ~IDataExchangeHandlerHost () noexcept = default;

	/** return if the audioprocessor is in an inactive state
	 *	[main thread]
	 */
	virtual bool isProcessorInactive (IAudioProcessor* processor) = 0;
	/** return the data exchange receiver (most likely the edit controller) for the processor
	 *	[main thread]
	 */
	virtual IPtr<IDataExchangeReceiver> findDataExchangeReceiver (IAudioProcessor* processor) = 0;
	/** check if the requested queue size should be allowed
	 *	[main thread]
	 */
	virtual bool allowAllocateSize (uint32 blockSize, uint32 numBlocks, uint32 alignment) = 0;
	/** check if this call is made on the main thread
	 *	[any thread]
	 */
	virtual bool isMainThread () = 0;

	/** check if the number of queues can be changed in this moment.
	 *
	 *	this is only allowed if no other thread can access the IDataExchangeManagerHost in this
	 *	moment
	 *	[main thread]
	 */
	virtual bool allowQueueListResize (uint32 newNumQueues) = 0;

	/** notification that the number of open queues changed
	 *	[main thread]
	 */
	virtual void numberOfQueuesChanged (uint32 openMainThreadQueues,
	                                    uint32 openBackgroundThreadQueues) = 0;

	/** notification that a new queue was opened */
	virtual void onQueueOpened (IAudioProcessor* processor, DataExchangeQueueID queueID,
	                            bool dispatchOnMainThread) = 0;
	/** notification that a queue was closed */
	virtual void onQueueClosed (IAudioProcessor* processor, DataExchangeQueueID queueID,
	                            bool dispatchOnMainThread) = 0;

	/** notification that a new block is ready to be send
	 *	[process thread]
	 */
	virtual void newBlockReadyToBeSend (DataExchangeQueueID queueID) = 0;
};

//------------------------------------------------------------------------
struct HostDataExchangeHandler
{
	/** Constructor
	 *
	 *	allocate and deallocate this object on the main thread
	 *
	 *	the number of queues is constant
	 *
	 *	@param host the managing host
	 *	@param maxQueues number of maximal allowed open queues
	 */
	HostDataExchangeHandler (IDataExchangeHandlerHost& host, uint32 maxQueues = 64);
	~HostDataExchangeHandler () noexcept;

	/** get the IHostDataExchangeManager interface
	 *
	 *	the interface you must provide to the IAudioProcessor
	 */
	IDataExchangeHandler* getInterface () const;

	/** send blocks
	 *
	 *	the host should periodically call this method on the main thread to send all queued blocks
	 *	which should be send on the main thread
	 */
	uint32 sendMainThreadBlocks ();
	/** send blocks
	 *
	 *	the host should call this on a dedicated background thread
	 *	inside a mutex is used, so don't delete this object while calling this
	 *
	 *	@param queueId	only send blocks from the specified queue. If queueId is equal to
	 *					InvalidDataExchangeQueueID all blocks from all queues are send.
	 */
	uint32 sendBackgroundBlocks (DataExchangeQueueID queueId = InvalidDataExchangeQueueID);

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg
