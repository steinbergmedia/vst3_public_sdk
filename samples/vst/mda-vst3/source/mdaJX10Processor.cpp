/*
 *  mdaJX10Processor.cpp
 *  mda-vst3
 *
 *  Created by Arne Scheffler on 6/14/08.
 *
 *  mda VST Plug-ins
 *
 *  Copyright (c) 2008 Paul Kellett
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "mdaJX10Processor.h"
#include "mdaJX10Controller.h"

#include <cmath>

namespace Steinberg {
namespace Vst {
namespace mda {

#define NPARAMS  24      //number of parameters
#define SILENCE  0.001f  //voice choking
#define PI       3.1415926535897932f
#define TWOPI    6.2831853071795864f
#define ANALOG   0.002f  //oscillator drift

float JX10Processor::programParams[kNumPrograms][NPARAMS] = { 
	{1.0f, 0.37f, 0.25f, 0.3f, 0.32f, 0.5f, 0.9f, 0.6f, 0.12f, 0.0f, 0.5f, 0.9f, 0.89f, 0.9f, 0.73f, 0.0f, 0.5f, 1.0f, 0.71f, 0.81f, 0.65f, 0.0f, 0.5f, 0.5f},
	{0.88f, 0.51f, 0.5f, 0.0f, 0.49f, 0.5f, 0.46f, 0.76f, 0.69f, 0.1f, 0.69f, 1.0f, 0.86f, 0.76f, 0.57f, 0.3f, 0.8f, 0.68f, 0.66f, 0.79f, 0.13f, 0.25f, 0.45f, 0.5f},
	{0.88f, 0.51f, 0.5f, 0.16f, 0.49f, 0.5f, 0.49f, 0.82f, 0.66f, 0.08f, 0.89f, 0.85f, 0.69f, 0.76f, 0.47f, 0.12f, 0.22f, 0.55f, 0.66f, 0.89f, 0.34f, 0.0f, 1.0f, 0.5f},
	{1.0f, 0.26f, 0.14f, 0.0f, 0.35f, 0.5f, 0.3f, 0.25f, 0.7f, 0.0f, 0.63f, 0.0f, 0.35f, 0.0f, 0.25f, 0.0f, 0.5f, 1.0f, 0.3f, 0.81f, 0.5f, 0.5f, 0.5f, 0.5f},
	{0.41f, 0.5f, 0.79f, 0.0f, 0.08f, 0.32f, 0.49f, 0.01f, 0.34f, 0.0f, 0.93f, 0.61f, 0.87f, 1.0f, 0.93f, 0.11f, 0.48f, 0.98f, 0.32f, 0.81f, 0.5f, 0.0f, 0.5f, 0.5f},
	{0.29f, 0.76f, 0.26f, 0.0f, 0.18f, 0.76f, 0.35f, 0.15f, 0.77f, 0.14f, 0.54f, 0.0f, 0.42f, 0.13f, 0.21f, 0.0f, 0.56f, 0.0f, 0.32f, 0.2f, 0.58f, 0.22f, 0.53f, 0.5f},
	{1.0f, 0.65f, 0.24f, 0.4f, 0.34f, 0.85f, 0.65f, 0.63f, 0.75f, 0.16f, 0.5f, 0.0f, 0.3f, 0.0f, 0.25f, 0.17f, 0.5f, 1.0f, 0.03f, 0.81f, 0.5f, 0.0f, 0.68f, 0.5f},
	{0.0f, 0.25f, 0.5f, 1.0f, 0.46f, 0.5f, 0.51f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.25f, 0.37f, 0.5f, 1.0f, 0.38f, 0.81f, 0.62f, 0.0f, 0.5f, 0.5f},
	{0.84f, 0.51f, 0.15f, 0.45f, 0.41f, 0.42f, 0.54f, 0.01f, 0.58f, 0.21f, 0.67f, 0.0f, 0.09f, 1.0f, 0.25f, 0.2f, 0.85f, 1.0f, 0.3f, 0.83f, 0.09f, 0.4f, 0.49f, 0.5f},
	{0.71f, 0.75f, 0.53f, 0.18f, 0.24f, 1.0f, 0.56f, 0.52f, 0.69f, 0.19f, 0.7f, 1.0f, 0.14f, 0.65f, 0.95f, 0.07f, 0.91f, 1.0f, 0.15f, 0.84f, 0.33f, 0.0f, 0.49f, 0.5f},
	{0.0f, 0.25f, 0.43f, 0.0f, 0.71f, 0.48f, 0.23f, 0.77f, 0.8f, 0.32f, 0.63f, 0.4f, 0.18f, 0.66f, 0.14f, 0.0f, 0.38f, 0.65f, 0.16f, 0.48f, 0.5f, 0.0f, 0.67f, 0.5f},
	{0.62f, 0.26f, 0.51f, 0.79f, 0.35f, 0.54f, 0.64f, 0.39f, 0.51f, 0.65f, 0.0f, 0.07f, 0.52f, 0.24f, 0.84f, 0.13f, 0.3f, 0.76f, 0.21f, 0.58f, 0.3f, 0.0f, 0.36f, 0.5f},
	{0.81f, 1.0f, 0.21f, 0.78f, 0.15f, 0.35f, 0.39f, 0.17f, 0.69f, 0.4f, 0.62f, 0.0f, 0.47f, 0.19f, 0.37f, 0.0f, 0.5f, 0.2f, 0.33f, 0.38f, 0.53f, 0.0f, 0.12f, 0.5f},
	{0.0f, 0.51f, 0.52f, 0.96f, 0.44f, 0.5f, 0.41f, 0.46f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 0.15f, 0.5f, 1.0f, 0.32f, 0.81f, 0.49f, 0.0f, 0.83f, 0.5f},
	{0.48f, 0.51f, 0.22f, 0.0f, 0.0f, 0.5f, 0.5f, 0.47f, 0.73f, 0.3f, 0.8f, 0.0f, 0.1f, 0.0f, 0.07f, 0.0f, 0.42f, 0.0f, 0.22f, 0.21f, 0.59f, 0.16f, 0.98f, 0.5f},
	{0.0f, 0.51f, 0.5f, 0.83f, 0.49f, 0.5f, 0.55f, 0.75f, 0.69f, 0.35f, 0.5f, 0.0f, 0.56f, 0.0f, 0.56f, 0.0f, 0.8f, 1.0f, 0.24f, 0.26f, 0.49f, 0.0f, 0.07f, 0.5f},
	{0.75f, 0.51f, 0.5f, 0.83f, 0.49f, 0.5f, 0.55f, 0.75f, 0.69f, 0.35f, 0.5f, 0.14f, 0.49f, 0.0f, 0.39f, 0.0f, 0.8f, 1.0f, 0.24f, 0.26f, 0.49f, 0.0f, 0.07f, 0.5f},
	{1.0f, 0.25f, 0.2f, 0.81f, 0.19f, 0.5f, 0.3f, 0.51f, 0.85f, 0.09f, 0.0f, 0.0f, 0.88f, 0.0f, 0.21f, 0.0f, 0.5f, 1.0f, 0.46f, 0.81f, 0.5f, 0.0f, 0.27f, 0.5f},
	{1.0f, 0.25f, 0.2f, 0.72f, 0.19f, 0.86f, 0.48f, 0.43f, 0.94f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.61f, 1.0f, 0.32f, 0.81f, 0.5f, 0.0f, 0.27f, 0.5f},
	{0.97f, 0.26f, 0.3f, 0.0f, 0.35f, 0.5f, 0.8f, 0.4f, 0.52f, 0.0f, 0.5f, 0.0f, 0.77f, 0.0f, 0.25f, 0.0f, 0.5f, 1.0f, 0.3f, 0.81f, 0.16f, 0.0f, 0.0f, 0.5f},
	{0.0f, 0.25f, 0.5f, 0.65f, 0.35f, 0.5f, 0.33f, 0.76f, 0.53f, 0.0f, 0.5f, 0.0f, 0.3f, 0.0f, 0.25f, 0.0f, 0.55f, 0.25f, 0.3f, 0.81f, 0.52f, 0.0f, 0.14f, 0.5f},
	{1.0f, 0.26f, 0.22f, 0.64f, 0.82f, 0.59f, 0.72f, 0.47f, 0.34f, 0.34f, 0.82f, 0.2f, 0.69f, 1.0f, 0.15f, 0.09f, 0.5f, 1.0f, 0.07f, 0.81f, 0.46f, 0.0f, 0.24f, 0.5f},
	{1.0f, 0.26f, 0.22f, 0.71f, 0.35f, 0.5f, 0.67f, 0.7f, 0.26f, 0.0f, 0.5f, 0.48f, 0.69f, 1.0f, 0.15f, 0.0f, 0.5f, 1.0f, 0.07f, 0.81f, 0.46f, 0.0f, 0.24f, 0.5f},
	{0.49f, 0.25f, 0.66f, 0.81f, 0.35f, 0.5f, 0.36f, 0.15f, 0.75f, 0.2f, 0.5f, 0.0f, 0.38f, 0.0f, 0.25f, 0.0f, 0.6f, 1.0f, 0.22f, 0.19f, 0.5f, 0.0f, 0.17f, 0.5f},
	{0.37f, 0.51f, 0.77f, 0.71f, 0.22f, 0.5f, 0.33f, 0.47f, 0.71f, 0.16f, 0.59f, 0.0f, 0.0f, 0.0f, 0.25f, 0.04f, 0.58f, 0.0f, 0.22f, 0.15f, 0.44f, 0.33f, 0.15f, 0.5f},
	{0.5f, 0.51f, 0.17f, 0.8f, 0.34f, 0.5f, 0.51f, 0.0f, 0.58f, 0.0f, 0.67f, 0.0f, 0.09f, 0.0f, 0.25f, 0.2f, 0.85f, 0.0f, 0.3f, 0.81f, 0.7f, 0.0f, 0.0f, 0.5f},
	{0.23f, 0.51f, 0.38f, 0.0f, 0.35f, 0.5f, 0.33f, 1.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.29f, 0.0f, 0.25f, 0.68f, 0.39f, 0.58f, 0.36f, 0.81f, 0.64f, 0.38f, 0.92f, 0.5f},
	{0.39f, 0.51f, 0.27f, 0.38f, 0.12f, 0.5f, 0.35f, 0.78f, 0.5f, 0.0f, 0.5f, 0.0f, 0.3f, 0.0f, 0.25f, 0.35f, 0.5f, 0.8f, 0.7f, 0.81f, 0.5f, 0.0f, 0.5f, 0.5f},
	{0.0f, 0.25f, 0.5f, 0.0f, 0.35f, 0.5f, 0.23f, 0.2f, 0.75f, 0.0f, 0.5f, 0.0f, 0.22f, 0.0f, 0.25f, 0.0f, 0.47f, 0.0f, 0.3f, 0.81f, 0.5f, 0.8f, 0.5f, 0.5f},
	{1.0f, 0.51f, 0.24f, 0.0f, 0.0f, 0.35f, 0.42f, 0.26f, 0.75f, 0.14f, 0.69f, 0.0f, 0.67f, 0.55f, 0.97f, 0.82f, 0.7f, 1.0f, 0.42f, 0.84f, 0.67f, 0.3f, 0.47f, 0.5f},
	{0.75f, 0.51f, 0.29f, 0.0f, 0.49f, 0.5f, 0.55f, 0.16f, 0.69f, 0.08f, 0.2f, 0.76f, 0.29f, 0.76f, 1.0f, 0.46f, 0.8f, 1.0f, 0.39f, 0.79f, 0.27f, 0.0f, 0.68f, 0.5f},
	{0.0f, 0.5f, 0.53f, 0.0f, 0.13f, 0.39f, 0.38f, 0.74f, 0.54f, 0.2f, 0.0f, 0.0f, 0.55f, 0.52f, 0.31f, 0.0f, 0.17f, 0.73f, 0.28f, 0.87f, 0.24f, 0.0f, 0.29f, 0.5f},
	{0.5f, 0.77f, 0.52f, 0.0f, 0.35f, 0.5f, 0.44f, 0.5f, 0.65f, 0.16f, 0.0f, 0.0f, 0.0f, 0.18f, 0.0f, 0.0f, 0.75f, 0.8f, 0.0f, 0.81f, 0.49f, 0.0f, 0.44f, 0.5f},
	{0.89f, 0.91f, 0.37f, 0.0f, 0.35f, 0.5f, 0.51f, 0.62f, 0.54f, 0.0f, 0.0f, 0.0f, 0.37f, 0.0f, 1.0f, 0.04f, 0.08f, 0.72f, 0.04f, 0.77f, 0.49f, 0.0f, 0.58f, 0.5f},
	{1.0f, 0.51f, 0.51f, 0.37f, 0.0f, 0.5f, 0.51f, 0.1f, 0.5f, 0.11f, 0.5f, 0.0f, 0.0f, 0.0f, 0.25f, 0.35f, 0.65f, 0.65f, 0.32f, 0.79f, 0.49f, 0.2f, 0.35f, 0.5f},
	{0.0f, 0.51f, 0.51f, 0.82f, 0.06f, 0.5f, 0.57f, 0.0f, 0.32f, 0.15f, 0.5f, 0.21f, 0.15f, 0.0f, 0.25f, 0.24f, 0.6f, 0.8f, 0.1f, 0.75f, 0.55f, 0.25f, 0.69f, 0.5f},
	{0.12f, 0.9f, 0.67f, 0.0f, 0.35f, 0.5f, 0.5f, 0.21f, 0.29f, 0.12f, 0.6f, 0.0f, 0.35f, 0.36f, 0.25f, 0.08f, 0.5f, 1.0f, 0.27f, 0.83f, 0.51f, 0.1f, 0.25f, 0.5f},
	{0.43f, 0.76f, 0.23f, 0.0f, 0.28f, 0.36f, 0.5f, 0.0f, 0.59f, 0.0f, 0.5f, 0.24f, 0.16f, 0.91f, 0.08f, 0.17f, 0.5f, 0.8f, 0.45f, 0.81f, 0.5f, 0.0f, 0.58f, 0.5f},
	{0.4f, 0.51f, 0.25f, 0.0f, 0.3f, 0.28f, 0.39f, 0.15f, 0.75f, 0.0f, 0.5f, 0.39f, 0.3f, 0.82f, 0.25f, 0.33f, 0.74f, 0.76f, 0.41f, 0.81f, 0.47f, 0.23f, 0.5f, 0.5f},
	{0.68f, 0.5f, 0.93f, 0.0f, 0.31f, 0.62f, 0.26f, 0.07f, 0.85f, 0.0f, 0.66f, 0.0f, 0.83f, 0.0f, 0.05f, 0.0f, 0.75f, 0.54f, 0.32f, 0.76f, 0.37f, 0.29f, 0.56f, 0.5f},
	{1.0f, 0.27f, 0.22f, 0.0f, 0.35f, 0.5f, 0.82f, 0.13f, 0.75f, 0.0f, 0.0f, 0.24f, 0.3f, 0.88f, 0.34f, 0.0f, 0.5f, 1.0f, 0.48f, 0.71f, 0.37f, 0.0f, 0.35f, 0.5f},
	{0.76f, 0.51f, 0.35f, 0.0f, 0.49f, 0.5f, 0.87f, 0.67f, 1.0f, 0.32f, 0.09f, 0.95f, 0.56f, 0.72f, 1.0f, 0.04f, 0.76f, 0.11f, 0.46f, 0.88f, 0.72f, 0.0f, 0.38f, 0.5f},
	{0.75f, 0.51f, 0.24f, 0.45f, 0.16f, 0.48f, 0.38f, 0.58f, 0.75f, 0.16f, 0.81f, 0.0f, 0.3f, 0.4f, 0.31f, 0.37f, 0.5f, 1.0f, 0.54f, 0.85f, 0.83f, 0.43f, 0.46f, 0.5f},
	{0.31f, 0.51f, 0.43f, 0.0f, 0.35f, 0.5f, 0.34f, 0.26f, 0.53f, 0.0f, 0.63f, 0.0f, 0.22f, 0.0f, 0.39f, 0.0f, 0.8f, 0.0f, 0.44f, 0.81f, 0.51f, 0.0f, 0.5f, 0.5f},
	{0.72f, 0.82f, 1.0f, 0.0f, 0.35f, 0.5f, 0.37f, 0.47f, 0.54f, 0.0f, 0.5f, 0.0f, 0.45f, 0.0f, 0.39f, 0.0f, 0.39f, 0.0f, 0.48f, 0.81f, 0.6f, 0.0f, 0.71f, 0.5f},
	{0.81f, 0.76f, 0.19f, 0.0f, 0.18f, 0.7f, 0.4f, 0.3f, 0.54f, 0.17f, 0.4f, 0.0f, 0.42f, 0.23f, 0.47f, 0.12f, 0.48f, 0.0f, 0.49f, 0.53f, 0.36f, 0.34f, 0.56f, 0.5f},       
	{0.57f, 0.49f, 0.31f, 0.0f, 0.35f, 0.5f, 0.46f, 0.0f, 0.68f, 0.0f, 0.5f, 0.46f, 0.3f, 1.0f, 0.23f, 0.3f, 0.5f, 1.0f, 0.31f, 1.0f, 0.38f, 0.0f, 0.5f, 0.5f},
	{0.0f, 0.25f, 0.5f, 0.0f, 0.35f, 0.5f, 0.08f, 0.36f, 0.69f, 1.0f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.96f, 0.5f, 1.0f, 0.92f, 0.97f, 0.5f, 1.0f, 0.0f, 0.5f},
	{0.0f, 0.25f, 0.5f, 0.0f, 0.35f, 0.5f, 0.16f, 0.85f, 0.5f, 0.28f, 0.5f, 0.37f, 0.3f, 0.0f, 0.25f, 0.89f, 0.5f, 1.0f, 0.89f, 0.24f, 0.5f, 1.0f, 1.0f, 0.5f},
	{1.0f, 0.37f, 0.51f, 0.0f, 0.35f, 0.5f, 0.0f, 1.0f, 0.97f, 0.0f, 0.5f, 0.02f, 0.2f, 0.0f, 0.2f, 0.0f, 0.46f, 0.0f, 0.3f, 0.81f, 0.5f, 0.78f, 0.48f, 0.5f},
	{0.0f, 0.25f, 0.5f, 0.0f, 0.76f, 0.94f, 0.3f, 0.33f, 0.76f, 0.0f, 0.68f, 0.0f, 0.59f, 0.0f, 0.59f, 0.1f, 0.5f, 0.0f, 0.5f, 0.81f, 0.5f, 0.7f, 0.0f, 0.5f},
	{0.5f, 0.41f, 0.23f, 0.45f, 0.77f, 0.0f, 0.4f, 0.65f, 0.95f, 0.0f, 0.5f, 0.33f, 0.5f, 0.0f, 0.25f, 0.0f, 0.7f, 0.65f, 0.18f, 0.32f, 1.0f, 0.0f, 0.06f, 0.5f},
};

//-----------------------------------------------------------------------------
JX10Processor::JX10Processor ()
: currentProgram (0)
{
	setControllerClass (JX10Controller::uid);
	allocParameters (NPARAMS);
}

//-----------------------------------------------------------------------------
JX10Processor::~JX10Processor ()
{
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API JX10Processor::initialize (FUnknown* context)
{
	tresult res = Base::initialize (context);
	if (res == kResultTrue)
	{
		addEventInput (USTRING("MIDI in"), 1);
		addAudioOutput (USTRING("Stereo Out"), SpeakerArr::kStereo);

		const float* newParams = programParams[0];
		if (newParams)
		{
			for (int32 i = 0; i < NPARAMS; i++)
				params[i] = newParams[i];
		}

		//initialise...
		for(int32 v=0; v<synthData.numVoices; v++) 
		{
			memset (&synthData.voice[v], 0, sizeof (VOICE));
			synthData.voice[v].dp   = synthData.voice[v].dp2   = 1.0f;
			synthData.voice[v].saw  = synthData.voice[v].p     = synthData.voice[v].p2    = 0.0f;
			synthData.voice[v].env  = synthData.voice[v].envd  = synthData.voice[v].envl  = 0.0f;
			synthData.voice[v].fenv = synthData.voice[v].fenvd = synthData.voice[v].fenvl = 0.0f;
			synthData.voice[v].f0   = synthData.voice[v].f1    = synthData.voice[v].f2    = 0.0f;
			synthData.voice[v].note = 0;
		}
		lfo = modwhl = filtwhl = press = fzip = 0.0f; 
		rezwhl = pbend = ipbend = 1.0f;
		volume = 0.0005f;
		K = mode = lastnote = synthData.sustain = synthData.activevoices = 0;
		noise = 22222;
		
		recalculate ();
	}
	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API JX10Processor::terminate ()
{
	return Base::terminate ();
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API JX10Processor::setActive (TBool state)
{
	if (state)
	{
		synthData.init ();
		for (auto& v : synthData.voice)
			clearVoice (v);
		recalculate ();
	}

	return Base::setActive (state);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API JX10Processor::setProcessing (TBool state)
{
	if (state)
	{
		synthData.init ();
		synthData.clearEvents ();
	}

	Base::setProcessing (state);
	return kResultTrue;
}

//-----------------------------------------------------------------------------
void JX10Processor::setParameter (ParamID index, ParamValue newValue, int32 sampleOffset)
{
	if (index < NPARAMS)
		Base::setParameter (index, newValue, sampleOffset);
	else if (index == BaseController::kPresetParam) // program change
	{
		auto newProgramIndex = std::min<int32> (kNumPrograms - 1, (int32)(newValue * kNumPrograms));
		if (currentProgram != newProgramIndex)
		{
			currentProgram = newProgramIndex;
			const float* newParams = programParams[currentProgram];
			if (newParams)
			{
				for (int32 i = 0; i < NPARAMS; i++)
					params[i] = newParams[i];
				recalculate ();
			}
		}
	}
	else if (index == BaseController::kModWheelParam) // mod wheel
	{
		newValue *= 127.;
		modwhl = static_cast<float> (0.000005f * (newValue*newValue));
	}
	else if (index == BaseController::kPitchBendParam) // pitch bend
	{
		if (newValue <= 1)
			newValue = (newValue - 0.5) * 0x2000;
		ipbend = (float)exp (0.000014102 * (double)newValue);
		pbend = 1.0f / ipbend;
	}
	else if (index == BaseController::kBreathParam)
	{
		newValue *= 127.;
		filtwhl = static_cast<float> (0.02f * newValue);
	}
	else if (index == BaseController::kCtrler3Param)
	{
		newValue *= 127.;
		filtwhl = static_cast<float> (-0.03f * newValue);
	}
	else if (index == BaseController::kExpressionParam)
	{
		newValue *= 127.;
		rezwhl = static_cast<float> (0.0065f * (154 - newValue));
	}
	else if (index == BaseController::kAftertouchParam)
	{
		newValue *= 127.;
		press = static_cast<float> (0.00001f * (newValue * newValue));
	}
}

//-----------------------------------------------------------------------------
void JX10Processor::setCurrentProgram (Steinberg::uint32 val)
{
	if (val < kNumPrograms)
		currentProgram = val;
}

//-----------------------------------------------------------------------------
void JX10Processor::setCurrentProgramNormalized (ParamValue val)
{
	setCurrentProgram (std::min<int32> (kNumPrograms - 1, (int32)(val * kNumPrograms)));
}

//-----------------------------------------------------------------------------
void JX10Processor::doProcessing (ProcessData& data)
{
	int32 sampleFrames = data.numSamples;
	
	float* out1 = data.outputs[0].channelBuffers32[0];
	float* out2 = data.outputs[0].channelBuffers32[1];

	int32 frame=0, frames, v;
	float oL, oR, e, vib, pwm, pb=pbend, ipb=ipbend, gl=glide;
	float x, y, hpf=0.997f, min=1.0f, w=0.0f, ww=noisemix;
	float ff, fe=filtenv, fq=filtq * rezwhl, fx=1.97f-0.85f*fq, fz=fzip;
	int32 k=K;
	Steinberg::uint32 r;

	vib = (float)sin(lfo);
	ff = filtf + filtwhl + (filtlfo + press) * vib; //have to do again here as way that
	pwm = 1.0f + vib * (modwhl + pwmdep);           //below triggers on k was too cheap!
	vib = 1.0f + vib * (modwhl + vibrato);

	synthData.eventPos = 0;
	if (synthData.activevoices > 0 || synthData.hasEvents ())
	{    
		while (frame<sampleFrames)
		{
			frames = synthData.events[synthData.eventPos].sampleOffset;
			if (frames>sampleFrames) frames = sampleFrames;
			frames -= frame;
			frame += frames;

			while (--frames>=0)
			{
				VOICE *V = synthData.voice.data ();
				oL = oR = 0.0f;

				noise = (noise * 196314165) + 907633515;
				r = (noise & 0x7FFFFF) + 0x40000000; //generate noise + fast convert to float
				w = *(float *)&r;
				w = ww * (w - 3.0f);

				if (--k<0)
				{
					lfo += dlfo;
					if (lfo>PI) lfo -= TWOPI;
					vib = (float)sin(lfo);
					ff = filtf + filtwhl + (filtlfo + press) * vib;
					pwm = 1.0f + vib * (modwhl + pwmdep);
					vib = 1.0f + vib * (modwhl + vibrato);
					k = KMAX;
				}

				for(v=0; v<synthData.numVoices; v++)  //for each voice
				{ 
					e = V->env;
					if (e > SILENCE)
					{ //Sinc-Loop Oscillator
						x = V->p + V->dp;
						if (x > min) 
						{
							if (x > V->pmax) 
							{ 
								x = V->pmax + V->pmax - x;  
								V->dp = -V->dp; 
							}
							V->p = x;
							x = V->sin0 * V->sinx - V->sin1; //sine osc
							V->sin1 = V->sin0;
							V->sin0 = x;
							x = x / V->p;
						}
						else
						{ 
							V->p = x = - x;  
							V->dp = V->period * vib * pb * V->snaPitchbend; //set period for next cycle
							V->pmax = (float)floor(0.5f + V->dp) - 0.5f;
							V->dc = -0.5f * V->lev / V->pmax;
							V->pmax *= PI;
							V->dp = V->pmax / V->dp;
							V->sin0 = V->lev * (float)sin(x);
							V->sin1 = V->lev * (float)sin(x - V->dp);
							V->sinx = 2.0f * (float)cos(V->dp);
							if (x*x > .1f) x = V->sin0 / x; else x = V->lev; //was 0.01f;
						}

						y = V->p2 + V->dp2; //osc2
						if (y > min) 
						{ 
							if (y > V->pmax2) 
							{ 
								y = V->pmax2 + V->pmax2 - y;  
								V->dp2 = -V->dp2; 
							}
							V->p2 = y;
							y = V->sin02 * V->sinx2 - V->sin12;
							V->sin12 = V->sin02;
							V->sin02 = y;
							y = y / V->p2;
						}
						else
						{
							V->p2 = y = - y;  
							V->dp2 = V->period * V->detune * pwm * pb * V->snaPitchbend;
							V->pmax2 = (float)floor(0.5f + V->dp2) - 0.5f;
							V->dc2 = -0.5f * V->lev2 / V->pmax2;
							V->pmax2 *= PI;
							V->dp2 = V->pmax2 / V->dp2;
							V->sin02 = V->lev2 * (float)sin(y);
							V->sin12 = V->lev2 * (float)sin(y - V->dp2);
							V->sinx2 = 2.0f * (float)cos(V->dp2);
							if (y*y > .1f) y = V->sin02 / y; else y = V->lev2;
						}
						V->saw = V->saw * hpf + V->dc + x - V->dc2 - y;  //integrated sinc = saw
						x = V->saw + w;
						V->env += V->envd * (V->envl - V->env);

						if (k==KMAX) //filter freq update at LFO rate
						{
							if ((V->env+V->envl)>3.0f) { V->envd=dec; V->envl=sus; } //envelopes
							V->fenv += V->fenvd * (V->fenvl - V->fenv);
							if ((V->fenv+V->fenvl)>3.0f) { V->fenvd=fdec; V->fenvl=fsus; }

							fz += 0.005f * (ff - fz); //filter zipper noise filter
							y = V->fc * (float)exp (fz + fe * V->fenv) * ipb; //filter cutoff
							if (y<0.005f) y =0.005f;
							V->ff = y;

							V->period += gl * (V->target - V->period); //glide
							if (V->target < V->period) V->period += gl * (V->target - V->period);
						}

						if (V->ff > fx) V->ff = fx; //stability limit

						V->f0 += V->ff * V->f1; //state-variable filter
						V->f1 -= V->ff * (V->f0 + fq * V->f1 - x - V->f2);
						V->f1 -= 0.2f * V->f1 * V->f1 * V->f1; //soft limit  //was 0.08f
						V->f2 = x;

						float oneSample = V->env * V->f0 * V->snaVolume;
						oL += oneSample * V->snaPanLeft;
						oR += oneSample * V->snaPanRight;
					}
					V++;
				}

				*out1++ = oL;
				*out2++ = oR;
			}

			if (frame<sampleFrames)
			{
				noteEvent (synthData.events[synthData.eventPos]);
				++synthData.eventPos;
			}
		}

		synthData.activevoices = synthData.numVoices;
		for(v=0; v<synthData.numVoices; v++)
		{
			if (synthData.voice[v].env<SILENCE)  //choke voices
			{
				synthData.voice[v].env = synthData.voice[v].envl = 0.0f;
				synthData.voice[v].f0 = synthData.voice[v].f1 = synthData.voice[v].f2 = 0.0f;
				synthData.activevoices--;
			}
		}
	}
	else
	{
		// complete empty block
		memset (out1, 0, sampleFrames * sizeof (float));
		memset (out2, 0, sampleFrames * sizeof (float));
	}
	fzip = fz;
	K = k;
}

//-----------------------------------------------------------------------------
void JX10Processor::preProcess ()
{
	synthData.clearEvents ();
}

//-----------------------------------------------------------------------------
void JX10Processor::processEvent (const Event& e)
{
	synthData.processEvent (e);
}

//-----------------------------------------------------------------------------
void JX10Processor::noteEvent (const Event& event)
{
	float p, l=100.0f; //louder than any envelope!
	int32  v=0, tmp, held=0;

	bool polyMode = (mode < 3);
	bool _glide = !(mode == 0 || mode == 3);
	bool legato = (mode == 1 || mode == 5);

	if (event.type == Event::kNoteOnEvent)
	{
		auto& note = event.noteOn;
		float velocity = note.velocity * 127;
		if (veloff) velocity = 80;

		if (!polyMode) //monophonic
		{
			if (synthData.voice[0].noteID != EndNoteID) //legato pitch change
			{
				for(tmp= (synthData.numVoices-1); tmp>0; tmp--)  //queue any held notes
				{
					synthData.voice[tmp].note = synthData.voice[tmp - 1].note;
					synthData.voice[tmp].noteID = synthData.voice[tmp - 1].noteID;
				}
				p = tune * (float)exp (-0.05776226505 * ((double)note.pitch + ANALOG * (double)v));
				while (p<3.0f || (p * detune)<3.0f) p += p;
				synthData.voice[0].target = p;
				if ((_glide)==0) synthData.voice[v].period = p;
				synthData.voice[0].fc = (float)exp (filtvel * (float)(velocity - 64)) / p;
				synthData.voice[0].env += SILENCE + SILENCE; ///was missed out below if returned?
				synthData.voice[0].note = note.pitch;
				synthData.voice[0].noteID = note.noteId;
				synthData.voice[0].snaVolume = 1.f;
				synthData.voice[0].snaPanLeft = synthData.voice[v].snaPanRight = 1.f;
				synthData.voice[0].snaPitchbend = 1.f;
				return;
			}
		}
		else //polyphonic 
		{
			for(tmp=0; tmp<synthData.numVoices; tmp++)  //replace quietest voice not in attack
			{
				if (synthData.voice[tmp].note > 0) held++;
				if (synthData.voice[tmp].env<l && synthData.voice[tmp].envl<2.0f) { l=synthData.voice[tmp].env;  v=tmp; }
			}
		}  
		p = tune * (float)exp (-0.05776226505 * ((double)note.pitch + ANALOG * (double)v));
		while (p<3.0f || (p * detune)<3.0f) p += p;
		synthData.voice[v].target = p;
		synthData.voice[v].detune = detune;

		tmp = 0;
		if (_glide || legato)
		{
			if ((_glide) || held) tmp = note.pitch - lastnote; //glide
		}
		synthData.voice[v].period = p * (float)pow (1.059463094359, (double)tmp - glidedisp);
		if (synthData.voice[v].period<3.0f) synthData.voice[v].period = 3.0f; //limit min period

		synthData.voice[v].note = lastnote = note.pitch;
		synthData.voice[v].noteID = note.noteId;

		synthData.voice[v].fc = (float)exp (filtvel * (float)(velocity - 64)) / p; //filter tracking

		synthData.voice[v].lev = voltrim * volume * (0.004f * (float)((velocity + 64) * (velocity + 64)) - 8.0f);
		synthData.voice[v].lev2 = synthData.voice[v].lev * oscmix;

		if (params[20]<0.5f) //force 180 deg phase difference for PWM
		{
			if (synthData.voice[v].dp>0.0f)
			{
				p = synthData.voice[v].pmax + synthData.voice[v].pmax - synthData.voice[v].p;
				synthData.voice[v].dp2 = -synthData.voice[v].dp;
			}
			else
			{
				p = synthData.voice[v].p;
				synthData.voice[v].dp2 = synthData.voice[v].dp;
			}
			synthData.voice[v].p2 = synthData.voice[v].pmax2 = p + PI * synthData.voice[v].period;

			synthData.voice[v].dc2 = 0.0f;
			synthData.voice[v].sin02 = synthData.voice[v].sin12 = synthData.voice[v].sinx2 = 0.0f;
		}

		if (!polyMode) //monophonic retriggering
		{
			synthData.voice[v].env += SILENCE + SILENCE;
		}
		else
		{
			//if (params[15] < 0.28f) 
			//{
			//  voice[v].f0 = voice[v].f1 = voice[v].f2 = 0.0f; //reset filter
			//  voice[v].env = SILENCE + SILENCE;
			//  voice[v].fenv = 0.0f;
			//}
			//else 
			synthData.voice[v].env += SILENCE + SILENCE; //anti-glitching trick
		}
		synthData.voice[v].envl  = 2.0f;
		synthData.voice[v].envd  = att;
		synthData.voice[v].fenvl = 2.0f;
		synthData.voice[v].fenvd = fatt;
		synthData.voice[v].snaVolume = 1.f;
		synthData.voice[v].snaPanLeft = synthData.voice[v].snaPanRight = 1.f;
		synthData.voice[v].snaPitchbend = 1.f;
	}
	else //note off
	{
		auto& note = event.noteOff;
		if ((!polyMode) && (synthData.voice[0].noteID==note.noteId)) //monophonic (and current note)
		{
			for(v= (synthData.numVoices-1); v>0; v--) 
			{
				if (synthData.voice[v].noteID!=EndNoteID) held = v; //any other notes queued?
			}
			if (held>0)
			{
				synthData.voice[v].note = synthData.voice[held].note;
				synthData.voice[v].noteID  = synthData.voice[held].noteID;
				clearVoice (synthData.voice[held]);

				p = tune * (float)exp (-0.05776226505 * ((double)synthData.voice[v].note + ANALOG * (double)v));
				while (p<3.0f || (p * detune)<3.0f) p += p;
				synthData.voice[v].target = p;
				if ((_glide || legato)==0) synthData.voice[v].period = p;
				synthData.voice[v].fc = 1.0f / p;
			}
			else
			{
				clearVoice (synthData.voice[v]);
			}
		}
		else //polyphonic
		{
			for(v=0; v<synthData.numVoices; v++) if (synthData.voice[v].noteID==note.noteId) //any voices playing that note?
			{
				if (synthData.sustain==0)
				{
					clearVoice (synthData.voice[v]);
				}
				else synthData.voice[v].note = SustainNoteID;
			}
		}
	}
}

//------------------------------------------------------------------------
void JX10Processor::clearVoice (VOICE& v)
{
	v.envl  = 0.0f;
	v.envd  = rel;
	v.fenvl = 0.0f;
	v.fenvd = frel;
	v.note  = 0;
	v.noteID  = EndNoteID;
}

//-----------------------------------------------------------------------------
void JX10Processor::recalculate ()
{
	double ifs = 1.0 / getSampleRate ();

	mode = std::min<int32>(5, static_cast<int32> (params[3] * 6));
	noisemix = static_cast<float> (params[21] * params[21]);
	voltrim = static_cast<float> ((3.2f - params[0] - 1.5f * noisemix) * (1.5f - 0.5f * params[7]));
	noisemix *= 0.06f;
	oscmix = static_cast<float> (params[0]);

	semi = (float)floor(48.0f * params[1]) - 24.0f;
	cent = static_cast<float> (15.876f * params[2] - 7.938f);
	cent = 0.1f * (float)floor(cent * cent * cent);
	detune = (float)pow (1.059463094359f, - semi - 0.01f * cent);
	tune = static_cast<float> (-23.376f - 2.0f * params[23] - 12.0f * (float)floor(params[22] * 4.9));
	tune = static_cast<float> (getSampleRate ()) * powf (1.059463094359f, tune);

	vibrato = pwmdep = static_cast<float> (0.2f * (params[20] - 0.5f) * (params[20] - 0.5f));
	if (params[20]<0.5f) vibrato = 0.0f;

	lfoHz = (float)exp (7.0f * params[19] - 4.0f);
	dlfo = lfoHz * (float)(ifs * TWOPI * KMAX); 

	filtf = static_cast<float> (8.0f * params[6] - 1.5f);
	filtq  = static_cast<float> ((1.0f - params[7]) * (1.0f - params[7])); ////// + 0.02f;
	filtlfo = static_cast<float> (2.5f * params[9] * params[9]);
	filtenv = static_cast<float> (12.0f * params[8] - 6.0f);
	filtvel = static_cast<float> (0.1f * params[10] - 0.05f);
	if (params[10]<0.05f) { veloff = 1; filtvel = 0; } else veloff = 0;

	att = 1.0f - (float)exp (-ifs * exp (5.5 - 7.5 * params[15]));
	dec = 1.0f - (float)exp (-ifs * exp (5.5 - 7.5 * params[16]));
	sus = static_cast<float> (params[17]);
	rel = 1.0f - (float)exp (-ifs * exp (5.5 - 7.5 * params[18]));
	if (params[18]<0.01f) rel = 0.1f; //extra fast release

	ifs *= KMAX; //lower update rate...

	fatt = 1.0f - (float)exp (-ifs * exp (5.5 - 7.5 * params[11]));
	fdec = 1.0f - (float)exp (-ifs * exp (5.5 - 7.5 * params[12]));
	fsus = static_cast<float> (params[13] * params[13]);
	frel = 1.0f - (float)exp (-ifs * exp (5.5 - 7.5 * params[14]));

	if (params[4]<0.02f) glide = 1.0f; else
	glide = 1.0f - (float)exp (-ifs * exp (6.0 - 7.0 * params[4]));
	glidedisp = static_cast<float> (6.604f * params[5] - 3.302f);
	glidedisp *= glidedisp * glidedisp;
}

}}} // namespaces

