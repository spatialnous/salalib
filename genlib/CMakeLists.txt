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
        p2dpoly.h
        pafmath.h
        pflipper.h
        readwritehelpers.h
        simplematrix.h
        stringutils.h
        xmlparse.h
    PUBLIC
        bsptree.cpp
        p2dpoly.cpp
        pafmath.cpp
        stringutils.cpp
        xmlparse.cpp
)
