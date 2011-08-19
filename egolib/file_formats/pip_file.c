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

/// @file file_formats/pip_file.c
/// @brief Routines for reading and writing the particle profile file "part*.txt"
/// @details

#include "pip_file.h"

#include "../vfs.h"
#include "../fileutil.h"

// includes for egoboo constants
#include "../../game/sound.h"                 // for INVALID_SOUND

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
pip_t * pip_init( pip_t * ppip )
{
    if ( NULL == ppip ) return ppip;

    // clear the pip
    BLANK_STRUCT_PTR( ppip )

    ppip->end_sound       = INVALID_SOUND;
    ppip->end_sound_floor = INVALID_SOUND;
    ppip->end_sound_wall  = INVALID_SOUND;
    ppip->damfx           = DAMFX_TURN;

    ppip->allowpush = btrue;

    ppip->orientation = ORIENTATION_B;  // make the orientation the normal billboarded orientation
    ppip->type        = SPRITE_SOLID;

    return ppip;
}

//--------------------------------------------------------------------------------------------
pip_t * load_one_pip_file_vfs( const char *szLoadName, pip_t * ppip )
{
    /// @author ZZ
    /// @details This function loads a particle template, returning bfalse if the file wasn't
    ///    found

    vfs_FILE* fileread;
    IDSZ idsz;
    char cTmp;

    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread )
    {
        return NULL;
    }

    pip_init( ppip );

    // set up the EGO_PROFILE_STUFF
    strncpy( ppip->name, szLoadName, SDL_arraysize( ppip->name ) );
    ppip->loaded = btrue;

    // read the 1 line comment at the top of the file
    vfs_gets( ppip->comment, SDL_arraysize( ppip->comment ) - 1, fileread );

    // rewind the file
    vfs_seek( fileread, 0 );

    // General data
    ppip->force = vfs_get_next_bool( fileread );

    cTmp = vfs_get_next_char( fileread );
    if ( 'L' == toupper( cTmp ) )  ppip->type = SPRITE_LIGHT;
    else if ( 'S' == toupper( cTmp ) )  ppip->type = SPRITE_SOLID;
    else if ( 'T' == toupper( cTmp ) )  ppip->type = SPRITE_ALPHA;

    ppip->image_base = vfs_get_next_int( fileread );
    ppip->numframes = vfs_get_next_int( fileread );
    ppip->image_add.base = vfs_get_next_int( fileread );
    ppip->image_add.rand = vfs_get_next_int( fileread );
    ppip->rotate_pair.base = vfs_get_next_int( fileread );
    ppip->rotate_pair.rand = vfs_get_next_int( fileread );
    ppip->rotate_add = vfs_get_next_int( fileread );
    ppip->size_base = vfs_get_next_int( fileread );
    ppip->size_add = vfs_get_next_int( fileread );
    ppip->spdlimit = vfs_get_next_float( fileread );
    ppip->facingadd = vfs_get_next_int( fileread );

    // override the base rotation
    if ( ppip->image_base < 256 && prt_u != prt_direction[ ppip->image_base ] )
    {
        ppip->rotate_pair.base = prt_direction[ ppip->image_base ];
    };

    // Ending conditions
    ppip->end_water     = vfs_get_next_bool( fileread );
    ppip->end_bump      = vfs_get_next_bool( fileread );
    ppip->end_ground    = vfs_get_next_bool( fileread );
    ppip->end_lastframe = vfs_get_next_bool( fileread );
    ppip->end_time      = vfs_get_next_int( fileread );

    // Collision data
    ppip->dampen     = vfs_get_next_float( fileread );
    ppip->bump_money  = vfs_get_next_int( fileread );
    ppip->bump_size   = vfs_get_next_int( fileread );
    ppip->bump_height = vfs_get_next_int( fileread );

    vfs_get_next_range( fileread, &( ppip->damage ) );
    ppip->damagetype = vfs_get_next_damage_type( fileread );

    // Lighting data
    cTmp = vfs_get_next_char( fileread );
    if ( 'T' == toupper( cTmp ) ) ppip->dynalight.mode = DYNA_MODE_ON;
    else if ( 'L' == toupper( cTmp ) ) ppip->dynalight.mode = DYNA_MODE_LOCAL;
    else                             ppip->dynalight.mode = DYNA_MODE_OFF;

    ppip->dynalight.level   = vfs_get_next_float( fileread );
    ppip->dynalight.falloff = vfs_get_next_int( fileread );

    // Initial spawning of this particle
    ppip->facing_pair.base    = vfs_get_next_int( fileread );
    ppip->facing_pair.rand    = vfs_get_next_int( fileread );
    ppip->spacing_hrz_pair.base = vfs_get_next_int( fileread );
    ppip->spacing_hrz_pair.rand = vfs_get_next_int( fileread );
    ppip->spacing_vrt_pair.base  = vfs_get_next_int( fileread );
    ppip->spacing_vrt_pair.rand  = vfs_get_next_int( fileread );
    ppip->vel_hrz_pair.base     = vfs_get_next_int( fileread );
    ppip->vel_hrz_pair.rand     = vfs_get_next_int( fileread );
    ppip->vel_vrt_pair.base      = vfs_get_next_int( fileread );
    ppip->vel_vrt_pair.rand      = vfs_get_next_int( fileread );

    // Continuous spawning of other particles
    ppip->contspawn_delay      = vfs_get_next_int( fileread );
    ppip->contspawn_amount    = vfs_get_next_int( fileread );
    ppip->contspawn_facingadd = vfs_get_next_int( fileread );
    ppip->contspawn_lpip       = vfs_get_next_int( fileread );

    // End spawning of other particles
    ppip->endspawn_amount    = vfs_get_next_int( fileread );
    ppip->endspawn_facingadd = vfs_get_next_int( fileread );
    ppip->endspawn_lpip       = vfs_get_next_int( fileread );

    // Bump spawning of attached particles
    ppip->bumpspawn_amount = vfs_get_next_int( fileread );
    ppip->bumpspawn_lpip    = vfs_get_next_int( fileread );

    // Random stuff  !!!BAD!!! Not complete
    ppip->daze_time    = vfs_get_next_int( fileread );
    ppip->grog_time    = vfs_get_next_int( fileread );
    ppip->spawnenchant = vfs_get_next_bool( fileread );

    ppip->cause_roll    = vfs_get_next_bool( fileread );  // !!Cause roll
    ppip->cause_pancake = vfs_get_next_bool( fileread );

    ppip->needtarget         = vfs_get_next_bool( fileread );
    ppip->targetcaster       = vfs_get_next_bool( fileread );
    ppip->startontarget      = vfs_get_next_bool( fileread );
    ppip->onlydamagefriendly = vfs_get_next_bool( fileread );

    ppip->soundspawn = vfs_get_next_int( fileread );

    ppip->end_sound = vfs_get_next_int( fileread );

    ppip->friendlyfire = vfs_get_next_bool( fileread );

    ppip->hateonly = vfs_get_next_bool( fileread );

    ppip->newtargetonspawn = vfs_get_next_bool( fileread );

    ppip->targetangle = vfs_get_next_int( fileread ) >> 1;
    ppip->homing      = vfs_get_next_bool( fileread );

    ppip->homingfriction = vfs_get_next_float( fileread );
    ppip->homingaccel    = vfs_get_next_float( fileread );
    ppip->rotatetoface   = vfs_get_next_bool( fileread );

    goto_colon_vfs( NULL, fileread, bfalse );  // !!Respawn on hit is unused

    ppip->manadrain         = vfs_get_next_ufp8( fileread );
    ppip->lifedrain         = vfs_get_next_ufp8( fileread );

    // assume default end_wall
    ppip->end_wall = ppip->end_ground;

    // assume default damfx
    if ( ppip->homing )  ppip->damfx = DAMFX_NONE;

    // Read expansions
    while ( goto_colon_vfs( NULL, fileread, btrue ) )
    {
        idsz = vfs_get_idsz( fileread );

        if ( idsz == MAKE_IDSZ( 'T', 'U', 'R', 'N' ) )       SET_BIT( ppip->damfx, DAMFX_NONE );        //ZF> This line doesnt do anything?
        else if ( idsz == MAKE_IDSZ( 'A', 'R', 'M', 'O' ) )  SET_BIT( ppip->damfx, DAMFX_ARMO );
        else if ( idsz == MAKE_IDSZ( 'B', 'L', 'O', 'C' ) )  SET_BIT( ppip->damfx, DAMFX_NBLOC );
        else if ( idsz == MAKE_IDSZ( 'A', 'R', 'R', 'O' ) )  SET_BIT( ppip->damfx, DAMFX_ARRO );
        else if ( idsz == MAKE_IDSZ( 'T', 'I', 'M', 'E' ) )  SET_BIT( ppip->damfx, DAMFX_TIME );
        else if ( idsz == MAKE_IDSZ( 'Z', 'S', 'P', 'D' ) )  ppip->zaimspd = vfs_get_float( fileread );
        else if ( idsz == MAKE_IDSZ( 'F', 'S', 'N', 'D' ) )  ppip->end_sound_floor = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'W', 'S', 'N', 'D' ) )  ppip->end_sound_wall = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'W', 'E', 'N', 'D' ) )  ppip->end_wall = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'P', 'U', 'S', 'H' ) )  ppip->allowpush = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'D', 'L', 'E', 'V' ) )  ppip->dynalight.level_add = vfs_get_int( fileread ) / 1000.0f;
        else if ( idsz == MAKE_IDSZ( 'D', 'R', 'A', 'D' ) )  ppip->dynalight.falloff_add = vfs_get_int( fileread ) / 1000.0f;
        else if ( idsz == MAKE_IDSZ( 'I', 'D', 'A', 'M' ) )  ppip->intdamagebonus = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'W', 'D', 'A', 'M' ) )  ppip->wisdamagebonus = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'G', 'R', 'A', 'V' ) )  ppip->ignore_gravity = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'O', 'R', 'N', 'T' ) )
        {
            char cTmp = vfs_get_first_letter( fileread );
            switch ( toupper( cTmp ) )
            {
                case 'X': ppip->orientation = ORIENTATION_X; break;  // put particle up along the world or body-fixed x-axis
                case 'Y': ppip->orientation = ORIENTATION_Y; break;  // put particle up along the world or body-fixed y-axis
                case 'Z': ppip->orientation = ORIENTATION_Z; break;  // put particle up along the world or body-fixed z-axis
                case 'V': ppip->orientation = ORIENTATION_V; break;  // vertical, like a candle
                case 'H': ppip->orientation = ORIENTATION_H; break;  // horizontal, like a plate
                case 'B': ppip->orientation = ORIENTATION_B; break;  // billboard
            }
        }
    }

    vfs_close( fileread );

    return ppip;
}
