# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(salalib salalib)

if (NOT FORCE_GLOBAL_COMPILE_WARNINGS)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
        "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # Common GCC and Clang warnings
        set(COMPILE_WARNINGS -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            -Wcast-align -Wpacked -Wsuggest-override -Wextra-semi
            -Wunused-macros -Wmissing-noreturn -Wmissing-declarations
            -Wold-style-cast -Weffc++ -Wpadded -Warray-bounds
            # yes eventually
            -Wno-float-equal -Wno-double-promotion
            -Wno-switch-enum
            # maybe
            -Wno-non-virtual-dtor -Wno-cast-qual
            -Wno-format-nonliteral
            )
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            # Clang-specific
            set(COMPILE_WARNINGS "${COMPILE_WARNINGS}"
                -Wunreachable-code-break -Wextra-semi-stmt
                -Wreserved-identifier -Wmissing-prototypes -Wattributes
                -Wsuggest-destructor-override -Wreserved-macro-identifier
                # yes eventually
                # -Wunsafe-buffer-usage
                # maybe
                # -Wno-exit-time-destructors -Wno-deprecated-copy-with-user-provided-dtor
                # -Wno-weak-vtables -Wno-global-constructors
                # no
                # -Wno-c++98-compat
            )
            # enable clang-tidy
            find_program(CLANG_TIDY_COMMAND NAMES clang-tidy)
            if(CLANG_TIDY_COMMAND)
                set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
            else()
                message(WARNING "clang-tidy not found!")
            endif()
        else()
            # GCC-specific
            set(COMPILE_WARNINGS "${COMPILE_WARNINGS}"
                # explicitly disabled
                -Wno-attributes
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

set(salalib_HDRS
    alllinemap.hpp
    axialminimiser.hpp
    geometrygenerators.hpp
    isegment.hpp
    linkutils.hpp
    pafcolor.hpp
    shapemap.hpp
    analysisresult.hpp
    axialpolygons.hpp
    gridproperties.hpp
    isovistdef.hpp
    mapconverter.hpp
    pixelref.hpp
    attributetable.hpp
    connector.hpp
    ianalysis.hpp
    isovist.hpp
    mgraph_consts.hpp
    pointmap.hpp
    spacepix.hpp
    attributetablehelpers.hpp
    displayparams.hpp
    iaxial.hpp
    metagraph.hpp
    metagraphreadwrite.hpp
    point.hpp
    sparksieve2.hpp
    attributetableindex.hpp
    entityparsing.hpp
    importtypedefs.hpp
    layermanager.hpp
    ngraph.hpp
    salaprogram.hpp
    tidylines.hpp
    attributetableview.hpp
    fileproperties.hpp
    importutils.hpp
    layermanagerimpl.hpp
    radiustype.hpp
    analysistype.hpp
    shapegraph.hpp
    shapemapgroupdata.hpp
    tolerances.hpp
    ianalysis.hpp
    salashape.hpp
    salaedgeu.hpp
    shaperef.hpp
    salaevent.hpp
    pushvalues.hpp
    attributemap.hpp
    exportutils.hpp
    bspnodetree.hpp
    isovistutils.hpp
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



