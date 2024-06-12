// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Human readable(ish) metagraph version changes

const int VERSION_ALWAYS_RECORD_BINDISTANCES = 440;

// Current metagraph version
const int METAGRAPH_VERSION = VERSION_ALWAYS_RECORD_BINDISTANCES;

///////////////////////////////////////////////////////////////////////////////

const unsigned int SALA_SELECTED_COLOR = 0x00FFFF77;
const unsigned int SALA_HIGHLIGHTED_COLOR = 0x0077FFFF;

///////////////////////////////////////////////////////////////////////////////

// Parse errors for MapInfo:

enum { MINFO_OK, MINFO_HEADER, MINFO_TABLE, MINFO_MIFPARSE, MINFO_OBJROWS, MINFO_MULTIPLE };

///////////////////////////////////////////////////////////////////////////////
