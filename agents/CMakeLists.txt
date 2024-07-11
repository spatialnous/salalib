# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

target_sources(salalib
    PUBLIC
        agent.h
        agentprogram.h
        agentga.h
        agentanalysis.h
    PRIVATE
        agent.cpp
        agentprogram.cpp
        agentga.cpp
        agentanalysis.cpp
)

