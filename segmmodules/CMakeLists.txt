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
        segmangular.h
        segmmetric.h
        segmtopological.h
        segmtulip.h
        segmhelpers.h
        segmmetricpd.h
        segmtopologicalpd.h
        segmtulipdepth.h
        segmmetricshortestpath.h
        segmtopologicalshortestpath.h
        segmtulipshortestpath.h)
