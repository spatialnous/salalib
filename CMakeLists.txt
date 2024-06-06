set(genlib genlib)

#set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wconversion")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4 /EHsc")
endif()

set(genlib_SRCS
    bsptree.cpp  
    p2dpoly.cpp  
    pafmath.cpp  
    stringutils.cpp  
    xmlparse.cpp)

add_compile_definitions(GENLIB_LIBRARY)

add_library(${genlib} STATIC ${genlib_SRCS})
