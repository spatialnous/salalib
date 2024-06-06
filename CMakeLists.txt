set(salalib salalib)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4 /EHsc")
endif()

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

add_library(${salalib} STATIC ${salalib_SRCS} ${vgamodules_SRCS} ${axialmodules_SRCS} ${segmmodules_SRCS} ${parsers_SRCS})

add_subdirectory(vgamodules)
add_subdirectory(axialmodules)
add_subdirectory(segmmodules)
add_subdirectory(parsers)
add_subdirectory(agents)



