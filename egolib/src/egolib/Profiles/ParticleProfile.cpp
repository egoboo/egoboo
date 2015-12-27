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
#include "egolib/Audio/AudioSystem.hpp"
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/fileutil.h"

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

void dynalight_info_t::reset() 
{
    mode = 0;
    on = 0;
    level = 0.0f;
    level_add = 0.0f;
    falloff = 0.0f;
    falloff_add = 0.0f;
}

ParticleProfile::ParticleProfile() :

    // Spawning.
    soundspawn(-1),
    force(false),
    newtargetonspawn(false),
    needtarget(false),
    startontarget(false),

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
    damageType(DamageType::DAMAGE_DIRECT),
    dazeTime(0),
    grogTime(0),

    // Hitting.
    _intellectDamageBonus(false),
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
    dynalight(),
    type(e_sprite_mode::SPRITE_SOLID),
    image_max(0),
    image_stt(0),
    image_add(),
    rotate_pair(),
    rotate_add(0),
    size_base(0),
    size_add(0),
    facingadd(0),
    orientation(prt_ori_t::ORIENTATION_B),  // make the orientation the normal billboarded orientation

    _comment(),
    _particleEffectBits(),

    _spawnFacing(),
    _spawnPositionOffsetXY(),
    _spawnPositionOffsetZ(),
    _spawnVelocityOffsetXY(),
    _spawnVelocityOffsetZ()
{
    _particleEffectBits[DAMFX_TURN] = true;
    dynalight.reset();
}

ParticleProfile::~ParticleProfile()
{
    //dtor
}

std::shared_ptr<ParticleProfile> ParticleProfile::readFromFile(const std::string& pathname)
{
    char cTmp;

    ReadContext ctxt(pathname);
    if (!ctxt.ensureOpen()) {
        return nullptr;
    }

    std::shared_ptr<ParticleProfile> profile = std::make_shared<ParticleProfile>();

    // set up the EGO_PROFILE_STUFF
    profile->_name = pathname;

    // read the 1 line comment at the top of the file
    profile->_comment = ctxt.readSingleLineComment();

    // General data
    profile->force = vfs_get_next_bool(ctxt);

    switch(Ego::toupper(vfs_get_next_printable(ctxt)))
    {
        case 'L': profile->type = SPRITE_LIGHT; break;
        case 'S': profile->type = SPRITE_SOLID; break;
        case 'T': profile->type = SPRITE_ALPHA; break;
    }

    profile->image_stt = vfs_get_next_int(ctxt);
    profile->image_max = vfs_get_next_int(ctxt);
    profile->image_add.base = vfs_get_next_int(ctxt);
    profile->image_add.rand = vfs_get_next_int(ctxt);
    profile->image_add.base /= EGO_ANIMATION_FRAMERATE_SCALING;
    profile->image_add.rand /= EGO_ANIMATION_FRAMERATE_SCALING;
    profile->rotate_pair.base = vfs_get_next_int(ctxt);
    profile->rotate_pair.rand = vfs_get_next_int(ctxt);
    profile->rotate_add = vfs_get_next_int(ctxt);
    profile->size_base = vfs_get_next_int(ctxt);
    profile->size_add = vfs_get_next_int(ctxt);
    profile->spdlimit = vfs_get_next_float(ctxt);
    profile->facingadd = vfs_get_next_int(ctxt);

    // override the base rotation
    if (profile->image_stt < EGO_ANIMATION_MULTIPLIER && prt_u != prt_direction[profile->image_stt])
    {
        profile->rotate_pair.base = prt_direction[profile->image_stt];
    }

    // Ending conditions
    profile->end_water = vfs_get_next_bool(ctxt);
    profile->end_bump = vfs_get_next_bool(ctxt);
    profile->end_ground = vfs_get_next_bool(ctxt);
    profile->end_lastframe = vfs_get_next_bool(ctxt);
    profile->end_time = vfs_get_next_int(ctxt);

    // Collision data
    profile->dampen = vfs_get_next_float(ctxt);
    profile->bump_money = vfs_get_next_int(ctxt);
    profile->bump_size = vfs_get_next_int(ctxt);
    profile->bump_height = vfs_get_next_int(ctxt);

    vfs_get_next_range(ctxt, &(profile->damage));
    profile->damageType = vfs_get_next_damage_type(ctxt);

    // Lighting data
    cTmp = vfs_get_next_printable(ctxt);
    if ('T' == Ego::toupper(cTmp)) profile->dynalight.mode = DYNA_MODE_ON;
    else if ('L' == Ego::toupper(cTmp)) profile->dynalight.mode = DYNA_MODE_LOCAL;
    else profile->dynalight.mode = DYNA_MODE_OFF;

    profile->dynalight.level = vfs_get_next_float(ctxt);
    profile->dynalight.falloff = vfs_get_next_int(ctxt);

    // Initial spawning of this particle
    profile->_spawnFacing.base = vfs_get_next_int(ctxt);
    profile->_spawnFacing.rand = vfs_get_next_int(ctxt);

    profile->_spawnPositionOffsetXY.base = vfs_get_next_int(ctxt);
    profile->_spawnPositionOffsetXY.rand = vfs_get_next_int(ctxt);

    profile->_spawnPositionOffsetZ.base = vfs_get_next_int(ctxt);
    profile->_spawnPositionOffsetZ.rand = vfs_get_next_int(ctxt);

    profile->_spawnVelocityOffsetXY.base = vfs_get_next_int(ctxt);
    profile->_spawnVelocityOffsetXY.rand = vfs_get_next_int(ctxt);

    profile->_spawnVelocityOffsetZ.base = vfs_get_next_int(ctxt);
    profile->_spawnVelocityOffsetZ.rand = vfs_get_next_int(ctxt);

    // Continuous spawning of other particles
    profile->contspawn._delay = vfs_get_next_int(ctxt);
    profile->contspawn._amount = vfs_get_next_int(ctxt);
    profile->contspawn._facingAdd = vfs_get_next_int(ctxt);
    profile->contspawn._lpip = vfs_get_next_local_particle_profile_ref(ctxt);

    // End spawning of other particles
    profile->endspawn._amount = vfs_get_next_int(ctxt);
    profile->endspawn._facingAdd = vfs_get_next_int(ctxt);
    profile->endspawn._lpip = vfs_get_next_local_particle_profile_ref(ctxt);

    // Bump spawning of attached particles
    profile->bumpspawn._amount = vfs_get_next_int(ctxt);
    profile->bumpspawn._facingAdd = 0; // @to add.
    profile->bumpspawn._lpip = vfs_get_next_local_particle_profile_ref(ctxt);

    // Random stuff  !!!BAD!!! Not complete
    profile->dazeTime = vfs_get_next_nat(ctxt);
    profile->grogTime = vfs_get_next_nat(ctxt);
    profile->spawnenchant = vfs_get_next_bool(ctxt);

    profile->cause_roll = vfs_get_next_bool(ctxt);  // !!Cause roll
    profile->cause_pancake = vfs_get_next_bool(ctxt);

    profile->needtarget = vfs_get_next_bool(ctxt);
    profile->targetcaster = vfs_get_next_bool(ctxt);
    profile->startontarget = vfs_get_next_bool(ctxt);
    profile->onlydamagefriendly = vfs_get_next_bool(ctxt);

    profile->soundspawn = vfs_get_next_int(ctxt);

    profile->end_sound = vfs_get_next_int(ctxt);

    profile->friendlyfire = vfs_get_next_bool(ctxt);

    profile->hateonly = vfs_get_next_bool(ctxt);

    profile->newtargetonspawn = vfs_get_next_bool(ctxt);

    profile->targetangle = vfs_get_next_int(ctxt) >> 1;
    profile->homing = vfs_get_next_bool(ctxt);

    profile->homingfriction = vfs_get_next_float(ctxt);
    profile->homingaccel = vfs_get_next_float(ctxt);
    profile->rotatetoface = vfs_get_next_bool(ctxt);

    ctxt.skipToColon(false);  // !!Respawn on hit is unused

    profile->manaDrain = vfs_get_next_ufp8(ctxt);
    profile->lifeDrain = vfs_get_next_ufp8(ctxt);

    // assume default end_wall
    profile->end_wall = profile->end_ground;

    // assume default damfx
    if (profile->homing) profile->_particleEffectBits.reset();

    // Read expansions
    while (!ctxt.is(ReadContext::Traits::endOfInput()))
    {
        if (ctxt.isWhiteSpace()) {
            ctxt.skipWhiteSpaces();
            continue;
        } else if (ctxt.isNewLine()) {
            ctxt.skipNewLines();
            continue;
        } else if (ctxt.is('/')) {
            ctxt.readSingleLineComment(); /// @todo Add and use ReadContext::skipSingleLineComment().
            continue;
        } else  if (ctxt.is(':')) {
            ctxt.next();
            IDSZ2 idsz = ctxt.readIDSZ();

            switch(idsz.toUint32())
            {
                case IDSZ2::caseLabel('N', 'O', 'N', 'E'):
                    profile->_particleEffectBits[DAMFX_NONE] = ctxt.readIntegerLiteral() != 0;
                break;

                case IDSZ2::caseLabel('T', 'U', 'R', 'N'):
                    profile->_particleEffectBits[DAMFX_TURN] = ctxt.readIntegerLiteral() != 0;
                break;

                case IDSZ2::caseLabel('A', 'R', 'M', 'O'):
                    profile->_particleEffectBits[DAMFX_ARMO] = ctxt.readIntegerLiteral() != 0;
                break;

                case IDSZ2::caseLabel('B', 'L', 'O', 'C'):
                    profile->_particleEffectBits[DAMFX_NBLOC] = ctxt.readIntegerLiteral() != 0;
                break;

                case IDSZ2::caseLabel('A', 'R', 'R', 'O'):
                    profile->_particleEffectBits[DAMFX_ARRO] = ctxt.readIntegerLiteral() != 0;
                break;

                case IDSZ2::caseLabel('T', 'I', 'M', 'E'):
                    profile->_particleEffectBits[DAMFX_TIME] = ctxt.readIntegerLiteral() != 0;
                break;

                case IDSZ2::caseLabel('Z', 'S', 'P', 'D'):  
                    profile->zaimspd = ctxt.readRealLiteral();
                break;

                case IDSZ2::caseLabel('F', 'S', 'N', 'D'):  
                    profile->end_sound_floor = ctxt.readIntegerLiteral();
                break;

                case IDSZ2::caseLabel('W', 'S', 'N', 'D'):  
                    profile->end_sound_wall = ctxt.readIntegerLiteral();
                break;

                case IDSZ2::caseLabel('W', 'E', 'N', 'D'):  
                    profile->end_wall = (0 != ctxt.readIntegerLiteral());
                break;

                case IDSZ2::caseLabel('P', 'U', 'S', 'H'):  
                    profile->allowpush = (0 != ctxt.readIntegerLiteral());
                break;

                case IDSZ2::caseLabel('D', 'L', 'E', 'V'):  
                    profile->dynalight.level_add = ctxt.readIntegerLiteral() / 1000.0f;
                break;

                case IDSZ2::caseLabel('D', 'R', 'A', 'D'):  
                    profile->dynalight.falloff_add = ctxt.readIntegerLiteral() / 1000.0f;
                break;

                case IDSZ2::caseLabel('I', 'D', 'A', 'M'):  
                    profile->_intellectDamageBonus = (0 != ctxt.readIntegerLiteral());
                break;

                case IDSZ2::caseLabel('G', 'R', 'A', 'V'):  
                    profile->ignore_gravity = (0 != ctxt.readIntegerLiteral());
                break;

                case IDSZ2::caseLabel('O', 'R', 'N', 'T'):
                    switch (Ego::toupper(ctxt.readPrintable()))
                    {
                        case 'X': profile->orientation = prt_ori_t::ORIENTATION_X; break;  // put particle up along the world or body-fixed x-axis
                        case 'Y': profile->orientation = prt_ori_t::ORIENTATION_Y; break;  // put particle up along the world or body-fixed y-axis
                        case 'Z': profile->orientation = prt_ori_t::ORIENTATION_Z; break;  // put particle up along the world or body-fixed z-axis
                        case 'V': profile->orientation = prt_ori_t::ORIENTATION_V; break;  // vertical, like a candle
                        case 'H': profile->orientation = prt_ori_t::ORIENTATION_H; break;  // horizontal, like a plate
                        case 'B': profile->orientation = prt_ori_t::ORIENTATION_B; break;  // billboard
                    }
                    while (ctxt.isAlpha()) {
                        ctxt.next();
                    }
                break;

                default:
                    throw Id::LexicalErrorException(__FILE__, __LINE__, Id::Location(ctxt._loadName, ctxt._lineNumber),
                                                    std::string("Unknown IDSZ type parsed: ") + idsz.toString());
                break;
            }
        } else {
            throw Id::LexicalErrorException(__FILE__, __LINE__, Id::Location(ctxt._loadName, ctxt._lineNumber),
                                            "expected `:`, comment, whitespace, newline or end of input");
        }
    }

    // Limit the end_sound index.
    profile->end_sound = Ego::Math::constrain<int8_t>(profile->end_sound, INVALID_SOUND_ID, MAX_WAVE);

    // Limit the soundspawn index.
    profile->soundspawn = Ego::Math::constrain<int8_t>(profile->soundspawn, INVALID_SOUND_ID, MAX_WAVE);

    return profile;
}

bool ParticleProfile::hasBit(const ParticleDamageEffectBits bit) const
{
    if(bit == NR_OF_DAMFX_BITS) return false; //should never happen
    return _particleEffectBits[bit];
}

const IPair& ParticleProfile::getSpawnFacing() const
{
    return _spawnFacing;
}

const IPair& ParticleProfile::getSpawnPositionOffsetXY() const
{
    return _spawnPositionOffsetXY;
}

const IPair& ParticleProfile::getSpawnPositionOffsetZ() const
{
    return _spawnPositionOffsetZ;
}

const IPair& ParticleProfile::getSpawnVelocityOffsetXY() const
{
    return _spawnVelocityOffsetXY;
}

const IPair& ParticleProfile::getSpawnVelocityOffsetZ() const
{
    return _spawnVelocityOffsetZ;
}
