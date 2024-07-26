
if(SMTG_ENABLE_VST3_HOSTING_EXAMPLES)
 
    include(SMTG_FindJack)
    if(${LIBJACK_FOUND})
        set(audiohost_sources
            "${SDK_ROOT}/public.sdk/samples/vst-hosting/editorhost/source/platform/iapplication.h"
            "${SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.cpp"
            "${SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.h"
            source/audiohost.cpp
            source/audiohost.h
            source/media/audioclient.cpp
            source/media/audioclient.h
            source/media/imediaserver.h
            source/media/iparameterclient.h
            source/media/jack/jackclient.cpp
            source/media/miditovst.h
            source/platform/appinit.h
            source/usediids.cpp
        )

        if(SMTG_MAC)
            set(audiohost_sources 
                ${audiohost_sources}
                "${SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm"
            )
             set_source_files_properties(
                "${SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm" PROPERTIES
                COMPILE_FLAGS "-fobjc-arc"
            )
           set(audiohost_PLATFORM_LIBS "-framework CoreFoundation")
            get_filename_component(InfoPlistFile "resource/Info.plist" ABSOLUTE)
            set(APP_PROPERTIES
                MACOSX_BUNDLE TRUE
                MACOSX_BUNDLE_INFO_PLIST ${InfoPlistFile}
            )
        elseif(SMTG_LINUX)
            set(audiohost_sources
                ${audiohost_sources}
                "${SDK_ROOT}/public.sdk/source/vst/hosting/module_linux.cpp"
            )
            set(audiohost_PLATFORM_LIBS)
            set(APP_PROPERTIES)
        elseif(SMTG_WIN)
            set(audiohost_sources
                ${audiohost_sources}
                "${SDK_ROOT}/public.sdk/source/vst/hosting/module_win32.cpp"
            )
            set(audiohost_PLATFORM_LIBS)
            if(MSVC)
                set(APP_PROPERTIES
                    LINK_FLAGS
                        "/SUBSYSTEM:console"
                )
            endif(MSVC)
        endif(SMTG_MAC)

        set(target audiohost)

        add_executable(${target} ${audiohost_sources})
        set_target_properties(${target}
            PROPERTIES
                ${SDK_IDE_HOSTING_EXAMPLES_FOLDER}
        )
        target_compile_features(${target}
            PUBLIC
                cxx_std_17
        )
        target_link_libraries(${target} 
            PRIVATE
                sdk_hosting
                ${audiohost_PLATFORM_LIBS}
                ${LIBJACK_LIBRARY}
        )
        target_include_directories(${target}
            PRIVATE
                ${LIBJACK_INCLUDE_DIR}
        )
        if (APP_PROPERTIES)
            set_target_properties(${target}
                PROPERTIES
                    ${APP_PROPERTIES}
            )
        endif(APP_PROPERTIES)
    else()
        message(STATUS "[SMTG] LIBJACK_FOUND is not set.")  
    endif(${LIBJACK_FOUND})

endif(SMTG_ENABLE_VST3_HOSTING_EXAMPLES)
