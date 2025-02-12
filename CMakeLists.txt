# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(salalib salalib)

#set(CMAKE_CXX_CLANG_TIDY "clang-tidy")

if (NOT FORCE_GLOBAL_COMPILE_WARNINGS)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
        "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # Common GCC and Clang warnings
        set(COMPILE_WARNINGS -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            -Wcast-align -Wpacked -Wsuggest-override -Wextra-semi
            -Wunused-macros -Wmissing-noreturn -Wmissing-declarations
            -Wold-style-cast
            # yes eventually
            -Wno-float-equal -Wno-double-promotion -Wno-reserved-macro-identifier
            -Wno-switch-enum -Wno-effc++
            # maybe
            -Wno-global-constructors -Wno-weak-vtables -Wno-non-virtual-dtor
            -Wno-cast-qual -Wno-deprecated-copy-with-user-provided-dtor
            -Wno-exit-time-destructors -Wno-format-nonliteral
            # no
            -Wno-c++98-compat)
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            # Clang-specific
            set(COMPILE_WARNINGS "${COMPILE_WARNINGS}"
                -Wunreachable-code-break -Wextra-semi-stmt
                -Wreserved-identifier -Wmissing-prototypes
                -Wsuggest-destructor-override
            )
        endif()

    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        add_compile_options(/EHsc /W4 /WX)

    endif()
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # Disable floating-point expression contraction to get the
    # same results on x86 and arm64
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffp-contract=off")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:strict")
endif()

option(WITH_EXPLICIT_PADDING "Enable explicit padding in structs" ON)
if(WITH_EXPLICIT_PADDING)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
        "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(COMPILE_WARNINGS "${COMPILE_WARNINGS}" -Wpadded)
        add_definitions(-DUSE_EXPLICIT_PADDING)
    endif()
endif()

set(salalib_HDRS
    alllinemap.h
    axialminimiser.h
    geometrygenerators.h
    isegment.h
    linkutils.h
    pafcolor.h
    shapemap.h
    analysisresult.h
    axialpolygons.h
    gridproperties.h
    isovistdef.h
    mapconverter.h
    pixelref.h
    attributetable.h
    connector.h
    ianalysis.h
    isovist.h
    mgraph_consts.h
    pointmap.h
    spacepix.h
    attributetablehelpers.h
    displayparams.h
    iaxial.h
    metagraph.h
    metagraphreadwrite.h
    point.h
    sparksieve2.h
    attributetableindex.h
    entityparsing.h
    importtypedefs.h
    layermanager.h
    ngraph.h
    salaprogram.h
    tidylines.h
    attributetableview.h
    fileproperties.h
    importutils.h
    layermanagerimpl.h
    radiustype.h
    analysistype.h
    shapegraph.h
    shapemapgroupdata.h
    tolerances.h
    ianalysis.h
    salashape.h
    salaedgeu.h
    shaperef.h
    salaevent.h
    pushvalues.h
    attributemap.h
    exportutils.h
    bspnodetree.h
    isovistutils.h
)

set(salalib_SRCS
    shapegraph.cpp
    connector.cpp
    isovist.cpp
    metagraphreadwrite.cpp
    ngraph.cpp
    pointmap.cpp
    salaprogram.cpp
    shapemap.cpp
    spacepix.cpp
    sparksieve2.cpp
    entityparsing.cpp
    linkutils.cpp
    gridproperties.cpp
    attributetable.cpp
    layermanagerimpl.cpp
    attributetableview.cpp
    geometrygenerators.cpp
    point.cpp
    pafcolor.cpp
    alllinemap.cpp
    axialminimiser.cpp
    axialpolygons.cpp
    tidylines.cpp
    mapconverter.cpp
    importutils.cpp
    attributetableindex.cpp
    salashape.cpp
    shapemapgroupdata.cpp
    pushvalues.cpp
    exportutils.cpp
)

add_compile_definitions(_DEPTHMAP SALALIB_LIBRARY)

# Allows enabling M_PI for MSVC
add_compile_definitions(_USE_MATH_DEFINES)

# The headers are added to the library primarily so that they appear as part
# of this particular project to IDEs that open it
add_library(${salalib} STATIC ${salalib_HDRS} ${salalib_SRCS}
    ${vgamodules_SRCS} ${axialmodules_SRCS} ${segmmodules_SRCS} ${parsers_SRCS})

target_compile_options(${salalib} PRIVATE ${COMPILE_WARNINGS})

if (NOT DISABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(${salalib} PUBLIC OpenMP::OpenMP_CXX)
    endif()
endif()

add_subdirectory(genlib)
add_subdirectory(vgamodules)
add_subdirectory(axialmodules)
add_subdirectory(segmmodules)
add_subdirectory(parsers)
add_subdirectory(agents)



