
cmake_minimum_required(VERSION 3.15.0)

project(smtg-vst3-utf16name
    VERSION ${vstsdk_VERSION}.0
    DESCRIPTION "Steinberg VST 3 UTF16Name example"
)

smtg_add_vst3plugin(utf16name     
    source/version.h
    source/utf16namecids.h
    source/utf16nameprocessor.h
    source/utf16nameprocessor.cpp
    source/utf16namecontroller.h
    source/utf16namecontroller.cpp
    source/utf16nameentry.cpp
)

smtg_target_setup_as_vst3_example(utf16name)
