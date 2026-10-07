# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2026 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

target_sources(salalib
    PRIVATE
        bsptree.hpp
        comm.hpp
        containerutils.hpp
        exceptions.hpp
        point2f.hpp
        point3f.hpp
        edgeu.hpp
        event2f.hpp
        region4f.hpp
        line4f.hpp
        simpleline.hpp
        pafmath.hpp
        pflipper.hpp
        readwritehelpers.hpp
        simplematrix.hpp
        stringutils.hpp
        xmlparse.hpp

        bsptree.cpp
        point2f.cpp
        edgeu.cpp
        region4f.cpp
        line4f.cpp
        pafmath.cpp
        stringutils.cpp
        xmlparse.cpp
)
