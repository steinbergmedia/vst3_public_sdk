//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/vststructsizecheck.h
// Created by  : Steinberg, 09/2010
// Description : struct size test. Checks that struct sizes do not change after publicly released
//

#include "pluginterfaces/vst/ivstattributes.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstcontextmenu.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstplugview.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstrepresentation.h"
#include "pluginterfaces/vst/ivstunits.h"
#include "pluginterfaces/vst/vstpresetkeys.h"
#include "pluginterfaces/vst/vsttypes.h"

#include "pluginterfaces/base/typesizecheck.h"

#include <cstdio>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

// ivstaudioprocessor.h
SMTG_TYPE_SIZE_CHECK (ProcessSetup, 24, 20, 24)
SMTG_TYPE_SIZE_CHECK (AudioBusBuffers, 24, 16, 24)
SMTG_TYPE_SIZE_CHECK (ProcessData, 80, 48, 48)
// ivstcomponent.h
SMTG_TYPE_SIZE_CHECK (BusInfo, 276, 276, 276)
SMTG_TYPE_SIZE_CHECK (RoutingInfo, 12, 12, 12)
// ivstcontextmenu.h
SMTG_TYPE_SIZE_CHECK (IContextMenuItem, 264, 264, 264)
// ivsteditcontroller.h
SMTG_TYPE_SIZE_CHECK (ParameterInfo, 792, 792, 792)
// ivstevents.h
SMTG_TYPE_SIZE_CHECK (NoteOnEvent, 20, 20, 20)
SMTG_TYPE_SIZE_CHECK (NoteOffEvent, 16, 16, 16)
SMTG_TYPE_SIZE_CHECK (DataEvent, 16, 12, 12)
SMTG_TYPE_SIZE_CHECK (PolyPressureEvent, 12, 12, 12)
SMTG_TYPE_SIZE_CHECK (ScaleEvent, 16, 10, 12)
SMTG_TYPE_SIZE_CHECK (ChordEvent, 16, 12, 12)
SMTG_TYPE_SIZE_CHECK (Event, 48, 40, 40)
// ivstnoteexpression.h
SMTG_TYPE_SIZE_CHECK (NoteExpressionValueDescription, 32, 28, 32)
SMTG_TYPE_SIZE_CHECK (NoteExpressionValueEvent, 16, 16, 16)
SMTG_TYPE_SIZE_CHECK (NoteExpressionTextEvent, 24, 16, 16)
SMTG_TYPE_SIZE_CHECK (NoteExpressionTypeInfo, 816, 812, 816)
SMTG_TYPE_SIZE_CHECK (KeyswitchInfo, 536, 536, 536)
// ivstprocesscontext.h
SMTG_TYPE_SIZE_CHECK (FrameRate, 8, 8, 8)
SMTG_TYPE_SIZE_CHECK (Chord, 4, 4, 4)
SMTG_TYPE_SIZE_CHECK (ProcessContext, 112, 104, 112)
// ivstrepresentation.h
SMTG_TYPE_SIZE_CHECK (RepresentationInfo, 256, 256, 256)
// ivstunits.h
SMTG_TYPE_SIZE_CHECK (UnitInfo, 268, 268, 268)
SMTG_TYPE_SIZE_CHECK (ProgramListInfo, 264, 264, 264)

//------------------------------------------------------------------------
inline void printStructSizes ()
{
	// clang-format off

	// ivstaudioprocessor.h
	std::printf ("ProcessSetup                   = %zu\n", sizeof (ProcessSetup));
	std::printf ("AudioBusBuffers                = %zu\n", sizeof (AudioBusBuffers));
	std::printf ("ProcessData                    = %zu\n", sizeof (ProcessData));

	// ivstcomponent.h
	std::printf ("BusInfo                        = %zu\n", sizeof (BusInfo));
	std::printf ("RoutingInfo                    = %zu\n", sizeof (RoutingInfo));

	// ivstcontextmenu.h
	std::printf ("IContextMenuItem               = %zu\n", sizeof (IContextMenuItem));

	// ivsteditcontroller.h
	std::printf ("ParameterInfo                  = %zu\n", sizeof (ParameterInfo));

	// ivstevents.h
	std::printf ("NoteOnEvent                    = %zu\n", sizeof (NoteOnEvent));
	std::printf ("NoteOffEvent                   = %zu\n", sizeof (NoteOffEvent));
	std::printf ("DataEvent                      = %zu\n", sizeof (DataEvent));
	std::printf ("PolyPressureEvent              = %zu\n", sizeof (PolyPressureEvent));
	std::printf ("ChordEvent                     = %zu\n", sizeof (ChordEvent));
	std::printf ("ScaleEvent                     = %zu\n", sizeof (ScaleEvent));
	std::printf ("Event                          = %zu\n", sizeof (Event));

	// ivstnoteexpression.h
	std::printf ("NoteExpressionValueDescription = %zu\n", sizeof (NoteExpressionValueDescription));
	std::printf ("NoteExpressionValueEvent       = %zu\n", sizeof (NoteExpressionValueEvent));
	std::printf ("NoteExpressionTextEvent        = %zu\n", sizeof (NoteExpressionTextEvent));
	std::printf ("NoteExpressionTypeInfo         = %zu\n", sizeof (NoteExpressionTypeInfo));
	std::printf ("KeyswitchInfo                  = %zu\n", sizeof (KeyswitchInfo));

	// ivstprocesscontext.h
	std::printf ("FrameRate                      = %zu\n", sizeof (FrameRate));
	std::printf ("Chord                          = %zu\n", sizeof (Chord));
	std::printf ("ProcessContext                 = %zu\n", sizeof (ProcessContext));

	// ivstrepresentation.h
	std::printf ("RepresentationInfo             = %zu\n", sizeof (RepresentationInfo));

	// ivstunits.h
	std::printf ("UnitInfo                       = %zu\n", sizeof (UnitInfo));
	std::printf ("ProgramListInfo                = %zu\n", sizeof (ProgramListInfo));

	// clang-format on
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
