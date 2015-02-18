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
#include "game/profiles/ParticleProfileSystem.hpp"
#include "egolib/Audio/AudioSystem.hpp"

_AbstractProfileSystem<pip_t, PIP_REF, INVALID_PIP_REF, MAX_PIP, ParticleProfileReader> PipStack("particle", "/debug/particle_profile_usage.txt");

PIP_REF PipStack_load_one(const char *loadName, const PIP_REF _override)
{
    return PipStack.load_one(loadName, _override);
}

void PipStack_release_all()
{
    PipStack.release_all();
}

void PipStack_init_all()
{
    PipStack.init_all();
}

bool PipStack_release_one(const PIP_REF ref)
{
    return PipStack.release_one(ref);
}

PIP_REF PipStack_get_free()
{
    return PipStack.get_free();
}
