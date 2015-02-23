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
#if !defined(EGOLIB_PROFILES_PRIVATE) || EGOLIB_PROFILES_PRIVATE != 1
#error(do not include directly, include `egolib/Profiles/_Include.hpp` instead)
#endif

#include "egolib/Profiles/_AbstractProfileSystem.hpp"
#include "egolib/Profiles/EnchantProfile.hpp"
#include "egolib/Profiles/EnchantProfileReader.hpp"

extern _AbstractProfileSystem<eve_t, EVE_REF, INVALID_EVE_REF, MAX_EVE, EnchantProfileReader> EveStack;

#define VALID_EVE_RANGE(ref) (EveStack.isValidRange(ref))
#define LOADED_EVE(ref) (EveStack.isLoaded(ref))
