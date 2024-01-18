//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/dataexchange.cpp
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

#include "dataexchange.h"
#include "public.sdk/source/vst/utility/alignedalloc.h"
#include "public.sdk/source/vst/utility/ringbuffer.h"
#include "base/source/timer.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/vst/ivsthostapplication.h"

#include <string_view>

#ifdef _MSC_VER
#include <malloc.h>
#endif

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

static constexpr DataExchangeBlock InvalidDataExchangeBlock = {nullptr, 0,
                                                               InvalidDataExchangeBlockID};

//------------------------------------------------------------------------
bool operator== (const DataExchangeHandler::Config& c1, const DataExchangeHandler::Config& c2)
{
	return c1.userContextID == c2.userContextID && c1.alignment == c2.alignment &&
	       c1.numBlocks == c2.numBlocks && c1.blockSize == c2.blockSize;
}

static constexpr auto MessageIDDataExchange = "DataExchange";
static constexpr auto MessageIDQueueOpened = "DataExchangeQueueOpened";
static constexpr auto MessageIDQueueClosed = "DataExchangeQueueClosed";
static constexpr auto MessageKeyData = "Data";
static constexpr auto MessageKeyBlockSize = "BlockSize";
static constexpr auto MessageKeyUserContextID = "UserContextID";

//------------------------------------------------------------------------
struct MessageHandler : ITimerCallback
{
	using RingBuffer = OneReaderOneWriter::RingBuffer<void*>;

	IPtr<Timer> timer;
	IPtr<IHostApplication> hostApp;
	IConnectionPoint* connection {nullptr};
	RingBuffer realtimeBuffer;
	RingBuffer messageBuffer;
	RingBuffer rtOnlyBuffer;
	void* lockedRealtimeBlock {nullptr};
	DataExchangeHandler::Config config {};

	MessageHandler (FUnknown* hostContext, IConnectionPoint* connection) : connection (connection)
	{
		hostApp = U::cast<IHostApplication> (hostContext);
	}

	~MessageHandler () noexcept override
	{
		if (timer)
			timer->stop ();
		timer = nullptr;
	}

	bool openQueue (const DataExchangeHandler::Config& c)
	{
		if (!hostApp || !connection)
			return false;
		config = c;

		timer = Timer::create (this, 1);
		if (!timer)
			return false;

		realtimeBuffer.resize (config.numBlocks);
		messageBuffer.resize (config.numBlocks);
		rtOnlyBuffer.resize (config.numBlocks);
		for (auto i = 0u; i < config.numBlocks; ++i)
		{
			auto data = aligned_alloc (config.blockSize, config.alignment);
			realtimeBuffer.push (data);
		}
		if (auto msg = owned (allocateMessage (hostApp)))
		{
			msg->setMessageID (MessageIDQueueOpened);
			if (auto attr = msg->getAttributes ())
			{
				attr->setInt (MessageKeyUserContextID, config.userContextID);
				attr->setInt (MessageKeyBlockSize, config.blockSize);
			}
			connection->notify (msg);
		}
		return true;
	}
	bool closeQueue ()
	{
		if (timer)
			timer->stop ();
		timer = nullptr;
		void* data;
		while (realtimeBuffer.pop (data))
		{
			aligned_free (data, config.alignment);
		}
		while (messageBuffer.pop (data))
		{
			aligned_free (data, config.alignment);
		}
		while (rtOnlyBuffer.pop (data))
		{
			aligned_free (data, config.alignment);
		}
		if (auto msg = owned (allocateMessage (hostApp)))
		{
			msg->setMessageID (MessageIDQueueClosed);
			if (auto attr = msg->getAttributes ())
			{
				attr->setInt (MessageKeyUserContextID, config.userContextID);
			}
			connection->notify (msg);
		}
		return true;
	}

	void* lockBlock ()
	{
		if (lockedRealtimeBlock != nullptr)
			return nullptr;
		void* data;
		if (rtOnlyBuffer.pop (data))
		{
			lockedRealtimeBlock = data;
			return lockedRealtimeBlock;
		}
		if (realtimeBuffer.pop (data))
		{
			lockedRealtimeBlock = data;
			return lockedRealtimeBlock;
		}
		return nullptr;
	}

	bool freeBlock (bool send)
	{
		if (send)
		{
			if (messageBuffer.push (lockedRealtimeBlock))
			{
				lockedRealtimeBlock = nullptr;
				return true;
			}
			return false;
		}
		if (rtOnlyBuffer.push (lockedRealtimeBlock))
		{
			lockedRealtimeBlock = nullptr;
			return true;
		}
		return false;
	}

	void onTimer (Timer* timer) override
	{
		void* data;
		while (messageBuffer.pop (data))
		{
			if (auto msg = owned (allocateMessage (hostApp)))
			{
				msg->setMessageID (MessageIDDataExchange);
				if (auto attributes = msg->getAttributes ())
				{
					attributes->setInt (MessageKeyUserContextID, config.userContextID);
					attributes->setBinary (MessageKeyData, data, config.blockSize);
					connection->notify (msg);
				}
			}
			realtimeBuffer.push (data);
		}
	}
};

//------------------------------------------------------------------------
struct DataExchangeHandler::Impl
{
	Config config {};
	ConfigCallback configCallback;
	IDataExchangeHandler* exchangeHandler {nullptr};
	IConnectionPoint* connectionPoint {nullptr};
	FUnknown* hostContext {nullptr};
	IAudioProcessor* processor {nullptr};
	std::unique_ptr<MessageHandler> fallbackMessageHandler;

	DataExchangeQueueID queueID {InvalidDataExchangeQueueID};
	DataExchangeBlock currentBlock {InvalidDataExchangeBlock};
	bool enabled {true};
	bool internalUseExchangeManager {true};

	bool isOpen () const { return queueID != InvalidDataExchangeQueueID; }

	bool openQueue (bool forceUseMessageHandling)
	{
		if (exchangeHandler && !forceUseMessageHandling)
		{
			internalUseExchangeManager = true;
			return exchangeHandler->openQueue (processor, config.blockSize, config.numBlocks,
			                                   config.alignment, config.userContextID,
			                                   &queueID) == kResultTrue;
		}
		internalUseExchangeManager = false;
		fallbackMessageHandler = std::make_unique<MessageHandler> (hostContext, connectionPoint);
		if (fallbackMessageHandler->openQueue (config))
		{
			queueID = 0;
			return true;
		}
		fallbackMessageHandler.reset ();
		return false;
	}
	void closeQueue ()
	{
		if (queueID == InvalidDataExchangeQueueID)
			return;
		if (internalUseExchangeManager)
			exchangeHandler->closeQueue (queueID);
		else if (fallbackMessageHandler)
		{
			fallbackMessageHandler->closeQueue ();
			fallbackMessageHandler.reset ();
		}
		currentBlock = InvalidDataExchangeBlock;
		queueID = InvalidDataExchangeQueueID;
	}
	DataExchangeBlock lockBlock ()
	{
		if (!isOpen ())
			return InvalidDataExchangeBlock;
		else if (currentBlock.blockID != InvalidDataExchangeBlockID)
			return currentBlock;
		else if (internalUseExchangeManager)
		{
			auto res = exchangeHandler->lockBlock (queueID, &currentBlock);
			if (res != kResultTrue)
				currentBlock = InvalidDataExchangeBlock;
			return currentBlock;
		}
		else if (fallbackMessageHandler)
		{
			if (auto data = fallbackMessageHandler->lockBlock ())
			{
				currentBlock.data = data;
				currentBlock.size = config.blockSize;
				currentBlock.blockID = 0;
				return currentBlock;
			}
		}
		return InvalidDataExchangeBlock;
	}
	bool freeBlock (bool send)
	{
		if (!isOpen () || currentBlock.blockID == InvalidDataExchangeBlockID)
			return true;
		if (internalUseExchangeManager)
		{
			auto res = exchangeHandler->freeBlock (queueID, currentBlock.blockID, send);
			currentBlock = InvalidDataExchangeBlock;
			return res == kResultTrue;
		}
		else if (fallbackMessageHandler)
		{
			if (fallbackMessageHandler->freeBlock (send))
			{
				currentBlock = InvalidDataExchangeBlock;
				return true;
			}
		}
		return false;
	}
};

//------------------------------------------------------------------------
DataExchangeHandler::DataExchangeHandler (IAudioProcessor* processor)
{
	impl = std::make_unique<Impl> ();
	impl->processor = processor;
}

//------------------------------------------------------------------------
DataExchangeHandler::DataExchangeHandler (IAudioProcessor* processor, ConfigCallback&& callback)
: DataExchangeHandler (processor)
{
	impl->configCallback = std::move (callback);
}

//------------------------------------------------------------------------
DataExchangeHandler::DataExchangeHandler (IAudioProcessor* processor,
                                          const ConfigCallback& callback)
: DataExchangeHandler (processor)
{
	impl->configCallback = callback;
}

//------------------------------------------------------------------------
DataExchangeHandler::~DataExchangeHandler () noexcept
{
}

//------------------------------------------------------------------------
void DataExchangeHandler::onConnect (IConnectionPoint* other, FUnknown* hostContext)
{
	impl->connectionPoint = other;
	impl->hostContext = hostContext;
	impl->exchangeHandler = U::cast<IDataExchangeHandler> (hostContext);
}

//------------------------------------------------------------------------
void DataExchangeHandler::onDisconnect (IConnectionPoint* other)
{
	impl->closeQueue ();
	impl->connectionPoint = nullptr;
	impl->hostContext = nullptr;
	impl->exchangeHandler = nullptr;
}

//------------------------------------------------------------------------
void DataExchangeHandler::onActivate (const Vst::ProcessSetup& setup, bool forceUseMessageHandling)
{
	Config conf {};
	if (impl->configCallback (conf, setup))
	{
		if (impl->isOpen ())
		{
			if (impl->config == conf)
				return;
			impl->closeQueue ();
		}
		impl->config = conf;
		impl->openQueue (forceUseMessageHandling);
	}
}

//------------------------------------------------------------------------
void DataExchangeHandler::onDeactivate ()
{
	if (impl->isOpen ())
		impl->closeQueue ();
}

//------------------------------------------------------------------------
void DataExchangeHandler::enable (bool state)
{
	impl->enabled = state;
}

//------------------------------------------------------------------------
bool DataExchangeHandler::isEnabled () const
{
	return impl->enabled;
}

//------------------------------------------------------------------------
DataExchangeBlock DataExchangeHandler::getCurrentOrNewBlock ()
{
	if (!isEnabled ())
		return InvalidDataExchangeBlock;
	return impl->lockBlock ();
}

//------------------------------------------------------------------------
bool DataExchangeHandler::sendCurrentBlock ()
{
	return impl->freeBlock (true);
}

//------------------------------------------------------------------------
bool DataExchangeHandler::discardCurrentBlock ()
{
	return impl->freeBlock (false);
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
struct DataExchangeReceiverHandler::Impl
{
	IDataExchangeReceiver* receiver {nullptr};
};

//------------------------------------------------------------------------
DataExchangeReceiverHandler::DataExchangeReceiverHandler (IDataExchangeReceiver* receiver)
{
	impl = std::make_unique<Impl> ();
	impl->receiver = receiver;
}

//------------------------------------------------------------------------
DataExchangeReceiverHandler::~DataExchangeReceiverHandler () noexcept = default;

//------------------------------------------------------------------------
bool DataExchangeReceiverHandler::onMessage (IMessage* msg)
{
	std::string_view msgID = msg->getMessageID ();
	if (msgID == MessageIDDataExchange)
	{
		if (auto attributes = msg->getAttributes ())
		{
			const void* data;
			uint32 sizeInBytes;
			if (attributes->getBinary (MessageKeyData, data, sizeInBytes) != kResultTrue)
				return false;
			int64 userContext;
			if (attributes->getInt (MessageKeyUserContextID, userContext) != kResultTrue)
				return false;
			DataExchangeBlock block;
			block.size = sizeInBytes;
			block.data = const_cast<void*> (data);
			block.blockID = 0;
			impl->receiver->onDataExchangeBlocksReceived (static_cast<uint32> (userContext), 1,
			                                              &block, false);
			return true;
		}
	}
	else if (msgID == MessageIDQueueOpened)
	{
		if (auto attributes = msg->getAttributes ())
		{
			int64 userContext;
			if (attributes->getInt (MessageKeyUserContextID, userContext) != kResultTrue)
				return false;
			int64 blockSize;
			if (attributes->getInt (MessageKeyBlockSize, blockSize) != kResultTrue)
				return false;
			TBool backgroundThread = false;
			impl->receiver->queueOpened (static_cast<uint32> (userContext),
			                             static_cast<uint32> (blockSize), backgroundThread);
			return true;
		}
	}
	else if (msgID == MessageIDQueueClosed)
	{
		if (auto attributes = msg->getAttributes ())
		{
			int64 userContext;
			if (attributes->getInt (MessageKeyUserContextID, userContext) != kResultTrue)
				return false;
			impl->receiver->queueClosed (static_cast<uint32> (userContext));
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
