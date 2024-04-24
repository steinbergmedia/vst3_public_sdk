
cmake_minimum_required(VERSION 3.15.0)

project(smtg-vst3-remapparamid
    VERSION ${vstsdk_VERSION}.0
    DESCRIPTION "Steinberg VST 3 Remap ParamID example"
)

set(COMPATIBILITY_FILE resource/vst_compatibility.json)

smtg_add_vst3plugin(remap-paramid     
    MODULEINFO_COMPATIBILITY
        ${COMPATIBILITY_FILE}
    source/version.h
    source/remapparamidcids.h
    source/remapparamidprocessor.h
    source/remapparamidprocessor.cpp
    source/remapparamidcontroller.h
    source/remapparamidcontroller.cpp
    source/remapparamidentry.cpp
    README.md
)

smtg_target_setup_as_vst3_example(remap-paramid)
