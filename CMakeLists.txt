set(genlib genlib)

#set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wconversion")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4 /EHsc")
endif()

set(genlib_HDRS
    bsptree.h
    comm.h
    containerutils.h
    exceptions.h
    linreg.h
    p2dpoly.h
    pafmath.h
    pflipper.h
    readwritehelpers.h
    simplematrix.h
    stringutils.h
    xmlparse.h
)
set(genlib_SRCS
    bsptree.cpp  
    p2dpoly.cpp  
    pafmath.cpp  
    stringutils.cpp  
    xmlparse.cpp)

# The headers are added to the library primarily so that they appear as part
# of this particular project to IDEs that open it
add_compile_definitions(GENLIB_LIBRARY)

add_library(${genlib} STATIC ${genlib_HDRS} ${genlib_SRCS})
