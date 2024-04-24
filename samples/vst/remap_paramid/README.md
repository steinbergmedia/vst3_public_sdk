
# Test Remap ParamID

## Introduction

As part of the [VST 3 Plug-ins Examples](https://steinbergmedia.github.io/vst3_dev_portal/pages/What+is+the+VST+3+SDK/Plug-in+Examples.html), [Test Remap ParamID](https://steinbergmedia.github.io/vst3_dev_portal/pages/What+is+the+VST+3+SDK/Plug-in+Examples.html?#test-remap-paramid) is a simple FX plug-in that demonstrates how a **VST 3** Plug-in could replace another one.
In this example, it could replace the [AGain](https://steinbergmedia.github.io/vst3_dev_portal/pages/What+is+the+VST+3+SDK/Plug-in+Examples.html?#again) plug-in when it is not available. It illustrates the use of the interface [IRemapParamID](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Change+History/3.7.11/IRemapParamID.html) (for mapping [Test Remap ParamID](https://steinbergmedia.github.io/vst3_dev_portal/pages/What+is+the+VST+3+SDK/Plug-in+Examples.html?#test-remap-paramid) parameters to AGain plug-in parameters) and the [module info](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/VST+Module+Architecture/ModuleInfo-JSON.html) with its compatibility field.

> See also: [Online Documentation](https://steinbergmedia.github.io/vst3_dev_portal/pages/What+is+the+VST+3+SDK/Plug-in+Examples.html#test-remap-paramid).

## Getting Started

This plug-in is part of the VST 3 SDK package. It is created with the VST 3 SDK root project.

> See the top-level README of the VST 3 SDK: https://github.com/steinbergmedia/vst3sdk.git

## Getting Help

* Read through the SDK documentation on the **[VST 3 Developer Portal](https://steinbergmedia.github.io/vst3_dev_portal/pages/index.html)**
* Ask some real people in the official **[VST 3 Developer Forum](https://forums.steinberg.net/c/developer/103)**
