# -*- cmake -*-

project(Meta7)

include(00-Common)
include(Meta7)
include(OpenSSL)

include_directories(
    ${META7_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    )

set(META7_SOURCE_FILES
    Meta7.cpp
    dsaparam.cpp
    dhparam.cpp
    )
    
set(META7_HEADER_FILES
    CMakeLists.txt

    Meta7.h
    )

set_source_files_properties($[META7_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND Meta7_SOURCE_FILES ${META7_HEADER_FILES})

add_library (Meta7 ${META7_SOURCE_FILES})
