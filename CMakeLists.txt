# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(genlib genlib)

if (NOT FORCE_GLOBAL_COMPILE_WARNINGS)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
        "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(COMPILE_WARNINGS -Wall -Wextra -Wpedantic -Wconversion)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(COMPILE_WARNINGS /W4 /EHsc)
    endif()
endif()

set(genlib_HDRS
    bsptree.h
    comm.h
    containerutils.h
    exceptions.h
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

# Allows enabling M_PI for MSVC
add_compile_definitions(_USE_MATH_DEFINES)

add_library(${genlib} STATIC ${genlib_HDRS} ${genlib_SRCS})

target_compile_options(${genlib} PRIVATE ${COMPILE_WARNINGS})
