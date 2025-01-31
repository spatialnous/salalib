# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

target_sources(salalib
    PRIVATE
        bsptree.h
        comm.h
        containerutils.h
        exceptions.h
        point2f.h
        point3f.h
        edgeu.h
        event2f.h
        region4f.h
        line4f.h
        simpleline.h
        regiontree.h
        poly.h
        pafmath.h
        pflipper.h
        readwritehelpers.h
        simplematrix.h
        stringutils.h
        xmlparse.h
    PUBLIC
        bsptree.cpp
        point2f.cpp
        edgeu.cpp
        region4f.cpp
        line4f.cpp
        regiontree.cpp
        poly.cpp
        pafmath.cpp
        stringutils.cpp
        xmlparse.cpp
)
