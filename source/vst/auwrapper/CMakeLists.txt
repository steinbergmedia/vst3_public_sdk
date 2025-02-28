if(SMTG_MAC)
    if (XCODE AND SMTG_ENABLE_AUV2_BUILDS)

		option(SMTG_AUWRAPPER_ACTIVATE_ONLY_DEFAULT_ACTIVE_BUSES 
			"Activate only the buses that have the kDefaultActive flag set in the AUWrapper. This may not work on some hosts because they never activate a bus later." 
			OFF
		)

		string(RANDOM LENGTH 20 CocoaId)
		file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/au/aucocoaclassprefix.h"
			CONTENT "#define SMTG_AUCocoaUIBase_CLASS_NAME SMTG_AUCocoaUIBase_${CocoaId}"
		)

        set(target au_wrapper)
        set(${target}_sources
            aucarbonview.mm
            aucarbonview.h
            aucocoaview.mm
            aucocoaview.h
            auwrapper.mm
            auwrapper.h
            NSDataIBStream.mm
            NSDataIBStream.h
        )
        add_library(${target}
            STATIC
                ${${target}_sources}
        )
        smtg_target_setup_universal_binary(${target})
        set_target_properties(${target}
            PROPERTIES
                ${SDK_IDE_LIBS_FOLDER}
        )
		target_compile_features(${target}
			PUBLIC
				cxx_std_17
		)

		if(SMTG_AUWRAPPER_ACTIVATE_ONLY_DEFAULT_ACTIVE_BUSES)
			target_compile_definitions(${target} PRIVATE SMTG_AUWRAPPER_ACTIVATE_ONLY_DEFAULT_ACTIVE_BUSES)
		endif()

        target_link_libraries(${target} 
            PRIVATE 
                sdk_hosting 
                "-framework AudioUnit" "-framework CoreMIDI"
                "-framework AudioToolbox"
                "-framework CoreFoundation"
                "-framework Carbon" 
                "-framework Cocoa" 
                "-framework CoreAudio"
        )
        target_include_directories(${target}
            PRIVATE 
                "${CMAKE_CURRENT_BINARY_DIR}/au/"
        )

		if(NOT ${SMTG_COREAUDIO_SDK_PATH} STREQUAL "")
			target_sources(${target} PRIVATE
	            ausdk.mm
			)
			target_include_directories(${target}
				PRIVATE 
					"${SMTG_COREAUDIO_SDK_PATH}/**"
			)
		elseif(NOT ${SMTG_AUDIOUNIT_SDK_PATH} STREQUAL "")
            target_compile_definitions(${target} PRIVATE SMTG_AUWRAPPER_USES_AUSDK)
## Adding the xcodeproj will crash Xcode when closing and reopening the cmake generated project
#			target_sources(${target} PRIVATE
#	            "${SMTG_AUDIOUNIT_SDK_PATH}/AudioUnitSDK.xcodeproj"
#			)
			target_include_directories(${target}
				PRIVATE 
					"${SMTG_AUDIOUNIT_SDK_PATH}/include/**"
			)
			target_link_libraries(${target}
				PRIVATE
					AudioUnitSDK
			)
		else()
			message(${SMTG_AUDIOUNIT_SDK_PATH})
			message(FATAL_ERROR "The option SMTG_ENABLE_AUV2_BUILDS is set but the audio unit SDK paths are not set")
		endif()

    else()
        message("[SMTG] * To enable building the AudioUnit wrapper, you need to use the Xcode generator and set SMTG_COREAUDIO_SDK_PATH to the path of your installation of the CoreAudio SDK!")
    endif(XCODE AND SMTG_ENABLE_AUV2_BUILDS)
endif(SMTG_MAC)