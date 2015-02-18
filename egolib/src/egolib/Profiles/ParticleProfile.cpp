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

#define EGOLIB_PROFILES_PRIVATE 1
#include "egolib/Profiles/ParticleProfile.hpp"

particle_direction_t prt_direction[256] =
{
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_l, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_l, prt_v, prt_v, prt_v, prt_v, prt_l, prt_l, prt_l, prt_r, prt_r, prt_r, prt_r, prt_r,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_l, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_l, prt_l, prt_l, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_l, prt_l, prt_l, prt_l, prt_l,
    prt_u, prt_u, prt_u, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_l, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_l, prt_u, prt_u, prt_u, prt_u,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_u, prt_u, prt_u, prt_u
};

pip_t *pip_t::init()
{
    // clear the pip
    BLANK_STRUCT_PTR(this);

    this->loaded = false;
    this->name[0] = '\0';

    this->end_sound = -1;
    this->end_sound_floor = -1;
    this->end_sound_wall = -1;
    this->damfx = DAMFX_TURN;

    this->allowpush = true;

    this->orientation = ORIENTATION_B;  // make the orientation the normal billboarded orientation
    this->type = SPRITE_SOLID;

    return this;
}
