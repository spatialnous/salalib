# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

target_sources(salalib
    PRIVATE
        dxfp.hpp
        ntfp.hpp
        mapinfodata.hpp
        tigerp.hpp

        ntfp.cpp
        dxfp.cpp
        mapinfodata.cpp
        tigerp.cpp
)
