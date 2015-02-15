//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file    game/egoboo_typedef.h
/// @details Base type definitions and config options.

#pragma once

#include "egolib/egolib.h"

#include "game/egoboo_config.h"

#include "egolib/Ref.hpp"

DECLARE_REF(CHR_REF);
#define INVALID_CHR_REF ((CHR_REF)MAX_CHR)

DECLARE_REF(TEAM_REF);
/// @todo TEAM_REF has no corresponding INVALID_TEAM_REF value.

DECLARE_REF(EVE_REF);
#define INVALID_EVE_REF ((EVE_REF)MAX_EVE)

DECLARE_REF(ENC_REF);
#define INVALID_ENC_REF ((ENC_REF)MAX_ENC)

DECLARE_REF(MAD_REF);
#define INVALID_MAD_REF ((MAD_REF)MAX_MAD)

DECLARE_REF(PLA_REF);
/// @todo PLA_REF has no corresponding INVALID_PLA_REF value.

DECLARE_REF(PIP_REF);
#define INVALID_PIP_REF ((PIP_REF)MAX_PIP)

DECLARE_REF(PRT_REF);
#define INVALID_PRT_REF ((PRT_REF)MAX_PRT)

DECLARE_REF(PASS_REF);
/// @todo PASS_REF has no correspnding INVALID_PASS_REF value.

DECLARE_REF(PRO_REF);
#define INVALID_PRO_REF ((PRO_REF)0xFFFF)

DECLARE_REF(TX_REF);
#define INVALID_TX_REF ((TX_REF)TX_COUNT)

DECLARE_REF(MNU_TX_REF);
/// @todo MNU_TX_REF has no corresponding INVALID_MNU_TX_REF value.

DECLARE_REF(BBOARD_REF);
#define INVALID_BBOARD_REF ((BBOARD_REF)MAX_BBOARD)

DECLARE_REF(MOD_REF);
#define INVALID_MOD_REF ((MOD_REF)MAX_MODULE)
