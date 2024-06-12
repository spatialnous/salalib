# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

target_sources(salalib
    PRIVATE
        ntfp.cpp
        dxfp.cpp
        mapinfodata.cpp
        tigerp.cpp
    PUBLIC
        dxfp.h
        ntfp.h
        mapinfodata.h
        tigerp.h)
