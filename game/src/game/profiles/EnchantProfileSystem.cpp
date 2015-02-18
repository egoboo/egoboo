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

#define GAME_PROFILES_PRIVATE 1
#include "game/profiles/EnchantProfileSystem.hpp"
#include "egolib/Audio/AudioSystem.hpp"

_AbstractProfileSystem<eve_t, EVE_REF, INVALID_EVE_REF, MAX_EVE, EnchantProfileReader> EveStack("enchant", "/debug/enchant_profile_usage.txt");

EVE_REF EveStack_load_one(const char *loadName, const EVE_REF _override)
{
    return EveStack.load_one(loadName, _override);
}

void EveStack_release_all()
{
    EveStack.release_all();
}

void EveStack_init_all()
{
    EveStack.init_all();
}

bool EveStack_release_one(const EVE_REF ref)
{
    return EveStack.release_one(ref);
}

EVE_REF EveStack_get_free()
{
    return EveStack.get_free();
}
