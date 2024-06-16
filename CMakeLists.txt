# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(salalib salalib)

if (NOT FORCE_GLOBAL_COMPILE_WARNINGS)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
        "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(COMPILE_WARNINGS -Wall -Wextra -Wpedantic)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(COMPILE_WARNINGS /W4 /EHsc)
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
    spacepixfile.h
    attributetable.h
    connector.h
    ianalysis.h
    isovist.h
    mgraph_consts.h
    pointdata.h
    spacepix.h
    attributetablehelpers.h
    displayparams.h
    iaxial.h
    ivga.h
    mgraph.h
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
    options.h
    shapegraph.h
    tolerances.h
)

set(salalib_SRCS
    shapegraph.cpp
    connector.cpp
    isovist.cpp
    mgraph.cpp
    ngraph.cpp
    pointdata.cpp
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
    spacepixfile.cpp
    alllinemap.cpp
    axialminimiser.cpp
    axialpolygons.cpp
    tidylines.cpp
    mapconverter.cpp
    importutils.cpp
    attributetableindex.cpp
    ianalysis.h)

add_compile_definitions(_DEPTHMAP SALALIB_LIBRARY)

# The headers are added to the library primarily so that they appear as part
# of this particular project to IDEs that open it
add_library(${salalib} STATIC ${salalib_HDRS} ${salalib_SRCS}
    ${vgamodules_SRCS} ${axialmodules_SRCS} ${segmmodules_SRCS} ${parsers_SRCS})

target_compile_options(${salalib} PRIVATE ${COMPILE_WARNINGS})

add_subdirectory(vgamodules)
add_subdirectory(axialmodules)
add_subdirectory(segmmodules)
add_subdirectory(parsers)
add_subdirectory(agents)



