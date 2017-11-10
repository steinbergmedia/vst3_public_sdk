
set(adelay_sources
	source/adelaycontroller.cpp
	source/adelaycontroller.h
	source/adelayids.h
	source/adelayprocessor.cpp
	source/adelayprocessor.h
	source/exampletest.cpp
	source/exampletest.h
	source/factory.cpp
	source/version.h
)

set(target adelay)
smtg_add_vst3plugin(${target} ${SDK_ROOT} ${adelay_sources})
set_target_properties(${target} PROPERTIES ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER})
target_link_libraries(${target} PRIVATE base sdk)
if(MAC)
	smtg_set_bundle(${target} INFOPLIST "${CMAKE_CURRENT_LIST_DIR}/resource/Info.plist" PREPROCESS)
elseif(WIN)
	target_sources(${target} PRIVATE resource/adelay.rc)
endif()
