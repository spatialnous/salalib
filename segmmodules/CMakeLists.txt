# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

target_sources(salalib
    PRIVATE
        segmangular.cpp
        segmmetric.cpp
        segmtopological.cpp
        segmtulip.cpp
        segmtopologicalpd.cpp
        segmmetricpd.cpp
        segmtulipdepth.cpp
        segmmetricshortestpath.cpp
        segmtopologicalshortestpath.cpp
        segmtulipshortestpath.cpp
    PUBLIC
        segmangular.hpp
        segmmetric.hpp
        segmtopological.hpp
        segmtulip.hpp
        segmhelpers.hpp
        segmmetricpd.hpp
        segmtopologicalpd.hpp
        segmtulipdepth.hpp
        segmmetricshortestpath.hpp
        segmtopologicalshortestpath.hpp
        segmtulipshortestpath.hpp
    )
