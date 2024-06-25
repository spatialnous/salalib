# SPDX-FileCopyrightText: 2017-2018 Christian Sailer
# SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

target_sources(salalib
    PRIVATE
       vgaisovist.cpp 
       vgaangular.cpp
       vgametric.cpp
       vgathroughvision.cpp
       vgavisuallocal.cpp
       vgavisualglobal.cpp
       vgaangulardepth.cpp
       vgametricdepth.cpp
       vgavisualglobaldepth.cpp
       vgaangularopenmp.cpp
       vgametricopenmp.cpp
       vgavisualglobalopenmp.cpp
       vgavisuallocaladjmatrix.cpp
       vgavisuallocalopenmp.cpp
    PUBLIC
       vgaangular.h
       vgametric.h
       vgavisualglobal.h
       vgaangulardepth.h
       vgametricdepth.h
       vgavisualglobaldepth.h
       vgaisovist.h
       vgathroughvision.h
       vgavisuallocal.h
       vgaangularopenmp.h
       vgametricopenmp.h
       vgavisualglobalopenmp.h
       vgavisuallocaladjmatrix.h
       vgavisuallocalopenmp.h
)
