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

dynalight_info_t::dynalight_info_t() :
    mode(0), on(0),
    level(0.0f), level_add(0.0f),
    falloff(0.0f), falloff_add(0.0f)
{
}

void dynalight_info_t::init() 
{
    mode = 0;
    on = 0;
    level = 0.0f;
    level_add = 0.0f;
    falloff = 0.0f;
    falloff_add = 0.0f;
}

pip_t::pip_t() :
    // comment - see below
    // Initial spawning of this particle.
    facing_pair(),
    spacing_hrz_pair(),
    spacing_vrt_pair(),
    vel_hrz_pair(),
    vel_vrt_pair(),
    // Spawning.
    force(false),
    newtargetonspawn(false),
    needtarget(false),
    startontarget(false),
    soundspawn(-1),
    // Ending conditions.
    end_time(0),
    end_water(false),
    end_bump(false),
    end_ground(false),
    end_wall(false),
    end_lastframe(false),
    // Ending sounds.
    end_sound(-1),
    end_sound_floor(-1),
    end_sound_wall(-1),
    // What/how to spawn continuously.
    contspawn(),
    // What/how to spawn at the end.
    endspawn(),
    // What/how to spawn when bumped.
    bumpspawn(),
    // Bumping of particle into particles/objects.
    bump_money(0),
    bump_size(0),
    bump_height(0),
    damage(),
    damageType(DamageType::DAMAGE_NONE),
    dazeTime(),
    grogTime(),
    damfx(DAMFX_TURN),
    // Hitting.
    // damageBoni - see below
    spawnenchant(false),
    onlydamagefriendly(false),
    friendlyfire(false),
    hateonly(false),
    cause_roll(false),
    cause_pancake(false),
    // Drains.
    lifeDrain(0),
    manaDrain(0),
    // Homing.
    homing(false),
    targetangle(0.0f),
    homingaccel(0.0f),
    homingfriction(0.0f),
    zaimspd(0.0f),
    rotatetoface(false),
    targetcaster(false),
    // Physics.
    spdlimit(0.0f),
    dampen(0.0f),
    allowpush(true),
    ignore_gravity(false),
    // Visual aspects.
    type(e_sprite_mode::SPRITE_SOLID),
    numframes(0),
    image_base(0),
    image_add(),
    rotate_pair(),
    rotate_add(0),
    size_base(0),
    size_add(0),
    facingadd(0),
    orientation(ORIENTATION_B),
    dynalight()

{
    damageBoni._intelligence = false;
    damageBoni._wisdom = false;
    // Metadata.
    comment[0] = '\0';
}

pip_t::~pip_t()
{
}

pip_t *pip_t::init()
{
    this->AbstractProfile::init();

    // Metadata.
    strcpy(comment, "");

    // Ending conditions,
    end_time = 0;
    end_water = false;
    end_bump = false;
    end_ground = false;
    end_wall = false;
    end_lastframe = false;

    // Ending sounds.
    end_sound = -1;
    end_sound_floor = -1;
    end_sound_wall = -1;

    // Initial spawning of this particle.
    facing_pair.init();
    spacing_hrz_pair.init();
    spacing_vrt_pair.init();
    vel_hrz_pair.init();
    vel_vrt_pair.init();

    // Hitting.
    damage.init();
    damageType = DamageType::DAMAGE_NONE;
    damageBoni._intelligence = 0;
    damageBoni._wisdom = 0;
    damfx = DAMFX_TURN;
    spawnenchant = false;
    onlydamagefriendly = false;
    friendlyfire = false;
    hateonly = false;
    cause_roll = false;
    cause_pancake = false;
    dazeTime = 0;
    grogTime = 0;

    // What/how to spawn continuously.
    contspawn.init();
    // What/how to spawn at the end.
    endspawn.init();
    // What/how to spawn when bumped.
    bumpspawn.init();
    
    // Spawning.
    force = false;
    newtargetonspawn = false;
    needtarget = false;
    startontarget = false;
    soundspawn = -1;

    // Physics.
    spdlimit = 0.0f;
    dampen = 0.0f;
    allowpush = true;
    ignore_gravity = false;

    // Visual aspects.
    type = e_sprite_mode::SPRITE_SOLID;
    numframes = 0;
    image_base = 0;
    image_add.init();
    rotate_pair.init();
    rotate_add = 0;
    size_base = 0;
    size_add = 0;
    facingadd = 0;
    orientation = ORIENTATION_B;  // make the orientation the normal billboarded orientation
    type = SPRITE_SOLID;
    dynalight.init();

    return this;
}
