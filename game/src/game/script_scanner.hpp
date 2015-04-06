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
#pragma once

#include "game/egoboo_typedef.h"

#define MAX_OPCODE 1024 ///< Number of lines in AICODES.TXT

/// The description of a single pre-defined egoscript token
struct token_t
{
    int iLine;                       ///< Line number
    int iIndex;
    char cType;                      ///< Constant, Variable, Function, etc.
    int iValue;                      ///< Integer value

    size_t szWord_length;
    STRING szWord;                   ///< The text representation

    static token_t *ctor(token_t *self);
    static void dtor(token_t *self);
};
