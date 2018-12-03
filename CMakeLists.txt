
cmake_minimum_required (VERSION 3.4.3)

project(PublicSDK)

set(sdk_sources
    source/common/pluginview.cpp
    source/common/pluginview.h
    source/main/pluginfactoryvst3.cpp
    source/main/pluginfactoryvst3.h
    source/vst/utility/ringbuffer.h
    source/vst/vstaudioeffect.cpp
    source/vst/vstaudioeffect.h
    source/vst/vstbus.cpp
    source/vst/vstbus.h
    source/vst/vstbypassprocessor.cpp
    source/vst/vstbypassprocessor.h
    source/vst/vstcomponent.cpp
    source/vst/vstcomponent.h
    source/vst/vstcomponentbase.cpp
    source/vst/vstcomponentbase.h
    source/vst/vsteditcontroller.cpp
    source/vst/vsteditcontroller.h
    source/vst/vsteventshelper.h
    source/vst/vstinitiids.cpp
    source/vst/vstnoteexpressiontypes.cpp
    source/vst/vstnoteexpressiontypes.h
    source/vst/vstparameters.cpp
    source/vst/vstparameters.h
    source/vst/vstpresetfile.cpp
    source/vst/vstpresetfile.h
    source/vst/vstrepresentation.cpp
    source/vst/vstrepresentation.h
)

set(pluginterfaces_sources
    ${pluginterfaces_SOURCE_DIR}/base/conststringtable.cpp
    ${pluginterfaces_SOURCE_DIR}/base/conststringtable.h
    ${pluginterfaces_SOURCE_DIR}/base/coreiids.cpp
    ${pluginterfaces_SOURCE_DIR}/base/falignpop.h
    ${pluginterfaces_SOURCE_DIR}/base/falignpush.h
    ${pluginterfaces_SOURCE_DIR}/base/fplatform.h
    ${pluginterfaces_SOURCE_DIR}/base/fstrdefs.h
    ${pluginterfaces_SOURCE_DIR}/base/ftypes.h
    ${pluginterfaces_SOURCE_DIR}/base/funknown.cpp
    ${pluginterfaces_SOURCE_DIR}/base/funknown.h
    ${pluginterfaces_SOURCE_DIR}/base/futils.h
    ${pluginterfaces_SOURCE_DIR}/base/fvariant.h
    ${pluginterfaces_SOURCE_DIR}/base/geoconstants.h
    ${pluginterfaces_SOURCE_DIR}/base/ibstream.h
    ${pluginterfaces_SOURCE_DIR}/base/icloneable.h
    ${pluginterfaces_SOURCE_DIR}/base/ierrorcontext.h
    ${pluginterfaces_SOURCE_DIR}/base/ipersistent.h
    ${pluginterfaces_SOURCE_DIR}/base/ipluginbase.h
    ${pluginterfaces_SOURCE_DIR}/base/istringresult.h
    ${pluginterfaces_SOURCE_DIR}/base/iupdatehandler.h
    ${pluginterfaces_SOURCE_DIR}/base/keycodes.h
    ${pluginterfaces_SOURCE_DIR}/base/pluginbasefwd.h
    ${pluginterfaces_SOURCE_DIR}/base/smartpointer.h
	${pluginterfaces_SOURCE_DIR}/base/typesizecheck.h
    ${pluginterfaces_SOURCE_DIR}/base/ucolorspec.h
    ${pluginterfaces_SOURCE_DIR}/base/ustring.cpp
    ${pluginterfaces_SOURCE_DIR}/base/ustring.h
)

set(vst_includes
    ${pluginterfaces_SOURCE_DIR}/gui/iplugview.h
    ${pluginterfaces_SOURCE_DIR}/gui/iplugviewcontentscalesupport.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstattributes.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstaudioprocessor.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstautomationstate.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstchannelcontextinfo.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstcomponent.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstcontextmenu.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivsteditcontroller.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstevents.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivsthostapplication.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstinterappaudio.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstmessage.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstmidicontrollers.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstmidilearn.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstnoteexpression.h	
    ${pluginterfaces_SOURCE_DIR}/vst/ivstparameterchanges.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstphysicalui.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstplugview.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstprefetchablesupport.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstprocesscontext.h	
    ${pluginterfaces_SOURCE_DIR}/vst/ivstpluginterfacesupport.h
    ${pluginterfaces_SOURCE_DIR}/vst/ivstrepresentation.h	
    ${pluginterfaces_SOURCE_DIR}/vst/ivstunits.h
    ${pluginterfaces_SOURCE_DIR}/vst/vstpresetkeys.h
    ${pluginterfaces_SOURCE_DIR}/vst/vstpshpack4.h
    ${pluginterfaces_SOURCE_DIR}/vst/vsttypes.h
)

add_library(sdk STATIC 
    ${sdk_sources}
    ${pluginterfaces_sources}
    ${vst_includes}
)
target_link_libraries(sdk PRIVATE base)
smtg_setup_universal_binary(sdk)

# iOS target
if(SMTG_MAC AND XCODE AND SMTG_IOS_DEVELOPMENT_TEAM)
    add_library(sdk_ios STATIC 
        ${sdk_sources}
        ${pluginterfaces_sources}
        ${vst_includes}
    )
    smtg_set_platform_ios(sdk_ios)
    set_target_properties(sdk_ios PROPERTIES ${SDK_IDE_LIBS_FOLDER})
    target_link_libraries(sdk_ios PRIVATE base_ios)
endif()

source_group("public.sdk"   FILES ${sdk_sources})
source_group("vst"          FILES ${vst_includes})
source_group("base"         FILES ${pluginterfaces_sources})
