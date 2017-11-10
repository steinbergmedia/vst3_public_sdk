  
if (SMTG_ENABLE_USE_OF_JACK)

include(FindJack)
  
if(${LIBJACK_FOUND})
	set(audiohost_sources
		source/audiohost.cpp
 		source/audiohost.h
 		source/usediids.cpp
 		source/platform/appinit.h
 		source/platform/iapplication.h
 		source/media/audioclient.h
 		source/media/audioclient.cpp
 		source/media/imediaserver.h
 		source/media/iparameterclient.h
 		source/media/miditovst.h
 		source/media/jack/jackclient.cpp
 		${SDK_ROOT}/public.sdk/source/vst/vstinitiids.cpp
 		${SDK_ROOT}/public.sdk/source/vst/hosting/processdata.cpp
 		${SDK_ROOT}/public.sdk/source/vst/hosting/processdata.h
 		${SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.cpp
 		${SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.h
 		${SDK_ROOT}/public.sdk/source/vst/hosting/hostclasses.cpp
 		${SDK_ROOT}/public.sdk/source/vst/hosting/hostclasses.h
 		${SDK_ROOT}/public.sdk/source/vst/hosting/module.cpp
 		${SDK_ROOT}/public.sdk/source/vst/hosting/module.h
 		${SDK_ROOT}/public.sdk/source/vst/hosting/optional.h
 		${SDK_ROOT}/public.sdk/source/vst/hosting/stringconvert.cpp
 		${SDK_ROOT}/public.sdk/source/vst/hosting/stringconvert.h
 		${SDK_ROOT}/public.sdk/source/vst/hosting/uid.h
 		${SDK_ROOT}/public.sdk/source/vst/hosting/eventlist.cpp
 		${SDK_ROOT}/public.sdk/source/vst/hosting/eventlist.h
 		${SDK_ROOT}/public.sdk/source/vst/hosting/parameterchanges.cpp
 		${SDK_ROOT}/public.sdk/source/vst/hosting/parameterchanges.h )

	if(APPLE)
 		set(audiohost_sources 
			${audiohost_sources}
			${SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm)
		set(audiohost_PLATFORM_LIBS "-framework Cocoa")
		get_filename_component(InfoPlistFile "resource/Info.plist" ABSOLUTE)
		set(APP_PROPERTIES
  			MACOSX_BUNDLE TRUE
			MACOSX_BUNDLE_INFO_PLIST ${InfoPlistFile} )
	elseif(LINUX)
		set(audiohost_sources
			${audiohost_sources}
			${SDK_ROOT}/public.sdk/source/vst/hosting/module_linux.cpp )
		set(audiohost_PLATFORM_LIBS)
		set(APP_PROPERTIES)
	elseif(WIN)
		set(audiohost_sources
			${audiohost_sources}
			${SDK_ROOT}/public.sdk/source/vst/hosting/module_win32.cpp )
		set(audiohost_PLATFORM_LIBS)
		set(APP_PROPERTIES	LINK_FLAGS "/SUBSYSTEM:console"	)
	endif()

	set(target audiohost)

	add_executable(${target} ${audiohost_sources})
	set_target_properties(${target} PROPERTIES ${SDK_IDE_HOSTING_EXAMPLES_FOLDER})
	target_link_libraries(${target} PRIVATE base sdk ${audiohost_PLATFORM_LIBS} ${LIBJACK_LIBRARY})
	target_include_directories(${target} PRIVATE ${LIBJACK_INCLUDE_DIR})
	if (APP_PROPERTIES)
  		set_target_properties(${target} PROPERTIES ${APP_PROPERTIES})
	endif()
else()
	message(STATUS "LIBJACK_FOUND is not set.")  
endif()

endif()
