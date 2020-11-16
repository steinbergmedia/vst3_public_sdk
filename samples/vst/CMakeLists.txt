
if(ANDROID)
    add_subdirectory(adelay)
    return()
endif()

include(SMTG_AddSubDirectories)

smtg_add_subdirectories()
