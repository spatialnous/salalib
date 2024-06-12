# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

target_sources(salalib
    PUBLIC
    	agent.h 
    	agentprogram.h 
    	agentset.h 
    	agentengine.h 
    	agentga.h 
    	agenthelpers.h
    PRIVATE
    	agent.cpp 
    	agentprogram.cpp 
    	agentset.cpp 
    	agentengine.cpp 
    	agentga.cpp)

