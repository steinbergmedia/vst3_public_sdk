//-----------------------------------------------------------------------------
// Project     : SDK Core
// Version     : 1.0
//
// Category    : Common Base Classes
// Filename    : public.sdk/source/main/macmain.cpp
// Created by  : Steinberg, 01/2004
// Description : Mac OS X Bundle Entry
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

#include "pluginterfaces/base/fplatform.h"

#ifndef __CF_USE_FRAMEWORK_INCLUDES__
#define __CF_USE_FRAMEWORK_INCLUDES__ 1
#endif

#include <CoreFoundation/CoreFoundation.h>

//------------------------------------------------------------------------
CFBundleRef ghInst = nullptr;
int bundleRefCounter = 0; // counting for bundleEntry/bundleExit pairs
void* moduleHandle = nullptr;
#define VST_MAX_PATH 2048
char gPath[VST_MAX_PATH] = {0};

//------------------------------------------------------------------------
bool InitModule (); ///< must be provided by plug-in: called when the library is loaded
bool DeinitModule (); ///< must be provided by plug-in: called when the library is unloaded

//------------------------------------------------------------------------
extern "C" {
/** bundleEntry and bundleExit must be provided by the plug-in! */
SMTG_EXPORT_SYMBOL bool bundleEntry (CFBundleRef);
SMTG_EXPORT_SYMBOL bool bundleExit (void);
}

#include <vector>

std::vector<CFBundleRef> gBundleRefs;

//------------------------------------------------------------------------
/** must be called from host right after loading bundle
Note: this could be called more than one time! */
bool bundleEntry (CFBundleRef ref)
{
	if (ref)
	{
		bundleRefCounter++;
		CFRetain (ref);

		// hold all bundle refs until plug-in is fully uninitialized
		gBundleRefs.push_back (ref);

		if (!moduleHandle)
		{
			ghInst = ref;
			moduleHandle = ref;

			// obtain the bundle path
			CFURLRef tempURL = CFBundleCopyBundleURL (ref);
			CFURLGetFileSystemRepresentation (tempURL, true, reinterpret_cast<UInt8*> (gPath), VST_MAX_PATH);
			CFRelease (tempURL);
		}

		if (bundleRefCounter == 1)
			return InitModule ();
	}
	return true;
}

//------------------------------------------------------------------------
/** must be called from host right before unloading bundle
Note: this could be called more than one time! */
bool bundleExit (void)
{
	if (--bundleRefCounter == 0)
	{
		DeinitModule ();

		// release the CFBundleRef's once all bundleExit clients called in
		// there is no way to identify the proper CFBundleRef of the bundleExit call
		for (size_t i = 0; i < gBundleRefs.size (); i++)
			CFRelease (gBundleRefs[i]);
		gBundleRefs.clear ();
	}
	else if (bundleRefCounter < 0)
		return false;
	return true;
}
