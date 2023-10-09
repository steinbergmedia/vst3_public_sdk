
if(NOT SMTG_ENABLE_VSTGUI_SUPPORT)
    return()
endif()

cmake_minimum_required(VERSION 3.15.0)

project(smtg-vst3-dataexchange
    VERSION ${vstsdk_VERSION}.0
    DESCRIPTION "Steinberg VST 3 Data Exchange example"
)

smtg_add_vst3plugin(dataexchange 
    source/factory.cpp
    source/controller.cpp
    source/processor.cpp
    source/shared.h
    source/version.h
    source/waveformview.h
    source/waveformview.cpp
    source/3rdparty/nanovg/src/nanovg.h
    source/3rdparty/nanovg/src/nanovg.c
)

smtg_target_add_plugin_resources(dataexchange
    RESOURCES
        resource/editor.uidesc
)

target_link_libraries(dataexchange
    PRIVATE
        vstgui_support
)

target_include_directories(dataexchange PRIVATE source/3rdparty/nanovg/src/)

smtg_target_setup_as_vst3_example(dataexchange)

if(SMTG_MAC)

	target_include_directories(dataexchange PRIVATE source/3rdparty/MetalNanoVG/src/)

	target_sources(dataexchange PRIVATE 
		source/waveformview_metal.mm
		source/3rdparty/MetalNanoVG/src/nanovg_mtl_shaders.metal
		source/3rdparty/MetalNanoVG/src/nanovg_mtl.h
		source/3rdparty/MetalNanoVG/src/nanovg_mtl.m
	)

    set_source_files_properties(
        "source/3rdparty/MetalNanoVG/src/nanovg_mtl.m" PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )

	target_link_libraries(dataexchange PRIVATE
		"-framework Metal"
		"-framework QuartzCore"
		"-framework CoreAudio"
	)

endif(SMTG_MAC)

if(SMTG_WIN)

	target_include_directories(dataexchange PRIVATE source/3rdparty/D3D11NanoVG/)

	target_sources(dataexchange PRIVATE 
		source/waveformview_direct3d.cpp
		source/3rdparty/D3D11NanoVG/D3D11PixelShader.h
		source/3rdparty/D3D11NanoVG/D3D11PixelShaderAA.h
#		source/3rdparty/D3D11NanoVG/D3D11PixelShader.hlsl
#		source/3rdparty/D3D11NanoVG/D3D11PixelShaderAA.hlsl
#		source/3rdparty/D3D11NanoVG/D3D11PixelShaderNoAA.hlsl
#		source/3rdparty/D3D11NanoVG/D3D11VertexShader.hlsl
		source/3rdparty/D3D11NanoVG/D3D11VertexShader.h
		source/3rdparty/D3D11NanoVG/nanovg_d3d11.h
	)

endif(SMTG_WIN)
