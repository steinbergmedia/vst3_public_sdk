//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : Common Base Classes
// Filename    : public.sdk/source/main/pluginfactory.cpp
// Created by  : Steinberg, 01/2004
// Description : Standard Plug-In Factory
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

#include "pluginfactory.h"

#if SMTG_OS_LINUX
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/gui/iplugview.h"
#include "base/source/timer.h"
#endif

#include <cstdlib>

namespace Steinberg {

CPluginFactory* gPluginFactory = nullptr;

//------------------------------------------------------------------------
//  CPluginFactory implementation
//------------------------------------------------------------------------
CPluginFactory::CPluginFactory (const PFactoryInfo& info)
: classes (nullptr), classCount (0), maxClassCount (0)
{
	FUNKNOWN_CTOR

	factoryInfo = info;
}

//------------------------------------------------------------------------
CPluginFactory::~CPluginFactory ()
{
	if (gPluginFactory == this)
		gPluginFactory = nullptr;

	if (classes)
		free (classes);

	FUNKNOWN_DTOR
}

//------------------------------------------------------------------------
IMPLEMENT_REFCOUNT (CPluginFactory)

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::queryInterface (FIDString _iid, void** obj)
{
	QUERY_INTERFACE (_iid, obj, IPluginFactory::iid, IPluginFactory)
	QUERY_INTERFACE (_iid, obj, IPluginFactory2::iid, IPluginFactory2)
	QUERY_INTERFACE (_iid, obj, IPluginFactory3::iid, IPluginFactory3)
	QUERY_INTERFACE (_iid, obj, FUnknown::iid, IPluginFactory)
	*obj = nullptr;
	return kNoInterface;
}

//------------------------------------------------------------------------
bool CPluginFactory::registerClass (const PClassInfo* info, FUnknown* (*createFunc) (void*),
                                    void* context)
{
	if (!info || !createFunc)
		return false;

	PClassInfo2 info2;
	memcpy (&info2, info, sizeof (PClassInfo));
	return registerClass (&info2, createFunc, context);
}

//------------------------------------------------------------------------
bool CPluginFactory::registerClass (const PClassInfo2* info, FUnknown* (*createFunc) (void*),
                                    void* context)
{
	if (!info || !createFunc)
		return false;

	if (classCount >= maxClassCount)
	{
		if (!growClasses ())
			return false;
	}

	PClassEntry& entry = classes[classCount];
	entry.info8 = *info;
	entry.info16.fromAscii (*info);
	entry.createFunc = createFunc;
	entry.context = context;
	entry.isUnicode = false;

	classCount++;
	return true;
}

//------------------------------------------------------------------------
bool CPluginFactory::registerClass (const PClassInfoW* info, FUnknown* (*createFunc) (void*),
                                    void* context)
{
	if (!info || !createFunc)
		return false;

	if (classCount >= maxClassCount)
	{
		if (!growClasses ())
			return false;
	}

	PClassEntry& entry = classes[classCount];
	entry.info16 = *info;
	entry.createFunc = createFunc;
	entry.context = context;
	entry.isUnicode = true;

	classCount++;
	return true;
}

//------------------------------------------------------------------------
bool CPluginFactory::growClasses ()
{
	static const int32 delta = 10;

	size_t size = (maxClassCount + delta) * sizeof (PClassEntry);
	void* memory = classes;

	if (!memory)
		memory = malloc (size);
	else
		memory = realloc (memory, size);

	if (!memory)
		return false;

	classes = static_cast<PClassEntry*> (memory);
	maxClassCount += delta;
	return true;
}

//------------------------------------------------------------------------
bool CPluginFactory::isClassRegistered (const FUID& cid)
{
	for (int32 i = 0; i < classCount; i++)
	{
		if (FUnknownPrivate::iidEqual (cid, classes[i].info16.cid))
			return true;
	}
	return false;
}

//------------------------------------------------------------------------
void CPluginFactory::removeAllClasses ()
{
	classCount = 0;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::getFactoryInfo (PFactoryInfo* info)
{
	if (info)
		memcpy (info, &factoryInfo, sizeof (PFactoryInfo));
	return kResultOk;
}

//------------------------------------------------------------------------
int32 PLUGIN_API CPluginFactory::countClasses ()
{
	return classCount;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::getClassInfo (int32 index, PClassInfo* info)
{
	if (info && (index >= 0 && index < classCount))
	{
		if (classes[index].isUnicode)
		{
			memset (info, 0, sizeof (PClassInfo));
			return kResultFalse;
		}

		memcpy (info, &classes[index].info8, sizeof (PClassInfo));
		return kResultOk;
	}
	return kInvalidArgument;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::getClassInfo2 (int32 index, PClassInfo2* info)
{
	if (info && (index >= 0 && index < classCount))
	{
		if (classes[index].isUnicode)
		{
			memset (info, 0, sizeof (PClassInfo2));
			return kResultFalse;
		}

		memcpy (info, &classes[index].info8, sizeof (PClassInfo2));
		return kResultOk;
	}
	return kInvalidArgument;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::getClassInfoUnicode (int32 index, PClassInfoW* info)
{
	if (info && (index >= 0 && index < classCount))
	{
		memcpy (info, &classes[index].info16, sizeof (PClassInfoW));
		return kResultOk;
	}
	return kInvalidArgument;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::createInstance (FIDString cid, FIDString _iid, void** obj)
{
	for (int32 i = 0; i < classCount; i++)
	{
		if (memcmp (classes[i].info16.cid, cid, sizeof (TUID)) == 0)
		{
			FUnknown* instance = classes[i].createFunc (classes[i].context);
			if (instance)
			{
				if (instance->queryInterface (_iid, obj) == kResultOk)
				{
					instance->release ();
					return kResultOk;
				}
				instance->release ();
			}
			break;
		}
	}

	*obj = nullptr;
	return kNoInterface;
}

#if SMTG_OS_LINUX
//------------------------------------------------------------------------
namespace /*anonymous*/ {

//------------------------------------------------------------------------
class LinuxPlatformTimer : public U::Extends<Timer, U::Directly<Linux::ITimerHandler>>
{
public:
	~LinuxPlatformTimer () noexcept override { stop (); }

	tresult init (ITimerCallback* cb, uint32 timeout)
	{
		if (!runLoop || cb == nullptr || timeout == 0)
			return kResultFalse;
		auto result = runLoop->registerTimer (this, timeout);
		if (result == kResultTrue)
		{
			callback = cb;
			timerRegistered = true;
		}
		return result;
	}

	void PLUGIN_API onTimer () override { callback->onTimer (this); }

	void stop () override
	{
		if (timerRegistered)
		{
			if (runLoop)
				runLoop->unregisterTimer (this);
			timerRegistered = false;
		}
	}

	bool timerRegistered {false};
	ITimerCallback* callback;

	static IPtr<Linux::IRunLoop> runLoop;
};
IPtr<Linux::IRunLoop> LinuxPlatformTimer::runLoop;

//------------------------------------------------------------------------
Timer* createLinuxTimer (ITimerCallback* cb, uint32 milliseconds)
{
	if (!LinuxPlatformTimer::runLoop)
		return nullptr;
	auto timer = NEW LinuxPlatformTimer;
	if (timer->init (cb, milliseconds) == kResultTrue)
		return timer;
	timer->release ();
	return nullptr;
}

} // anonymous
#endif // SMTG_OS_LINUX

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::setHostContext (FUnknown* context)
{
#if SMTG_OS_LINUX
	if (auto runLoop = U::cast<Linux::IRunLoop> (context))
	{
		LinuxPlatformTimer::runLoop = runLoop;
		InjectCreateTimerFunction (createLinuxTimer);
	}
	else
	{
		LinuxPlatformTimer::runLoop.reset ();
		InjectCreateTimerFunction (nullptr);
	}
	return kResultTrue;
#else
	(void) context;
	return kNotImplemented;
#endif
}

//------------------------------------------------------------------------
} // namespace Steinberg
