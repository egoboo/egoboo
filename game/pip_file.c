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

/* Egoboo - pip_file.c
 * routines for reading and writing the particle profile file "part*.txt"
 */

#include "pip_file.h"

#include "egoboo_vfs.h"
#include "egoboo_fileutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enum e_particle_direction
{
    prt_v = 0x0000,    // particle is vertical on the bitmap
    prt_d = 0x2000,    // particle is diagonal (rotated 45 degrees to the right = 8192)
    prt_h = 0x4000,    // particle is horizontal (rotated 90 degrees = 16384)
    prt_u = 0xFFFF,    // particle is of unknown orientation
};
typedef enum e_particle_direction particle_direction_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
particle_direction_t prt_direction[256] =
{
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_d, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_d, prt_v, prt_v, prt_v, prt_v, prt_d, prt_d, prt_v, prt_d, prt_d, prt_d, prt_d, prt_d,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_d, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_d, prt_d, prt_d, prt_d,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_d, prt_d, prt_d, prt_d, prt_v, prt_v, prt_v, prt_v,
    prt_u, prt_u, prt_u, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_d, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_d, prt_u, prt_u, prt_u, prt_u,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_u, prt_u, prt_u, prt_u
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void pip_init( pip_t * ppip )
{
    if( NULL == ppip ) return;

    // clear the pip
    memset( ppip, 0, sizeof(pip_t) );

    /* ppip->zaimspd = 0; */
    ppip->soundfloor = -1;
    ppip->soundwall  = -1;
    ppip->endwall    = ppip->endground;
    ppip->damfx      = DAMFX_TURN;

    ppip->allowpush = btrue;
    /* ppip->dynalight_falloffadd = 0; */
    /* ppip->dynalight_leveladd = 0; */
    /* ppip->intdamagebonus = bfalse; */
    /* ppip->wisdamagebonus = bfalse; */

    ppip->orientation = ORIENTATION_B;  // make the orientation the normal billboarded orientation
    ppip->type        = PRTSOLIDSPRITE;

}

//--------------------------------------------------------------------------------------------
pip_t * load_one_pip_file( const char *szLoadName, pip_t * ppip )
{
    // ZZ> This function loads a particle template, returning bfalse if the file wasn't
    //    found

    vfs_FILE* fileread;
    IDSZ idsz;
    char cTmp;

    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread )
    {
        return NULL;
    }

    pip_init( ppip );

    strncpy( ppip->name, szLoadName, SDL_arraysize(ppip->name) );
    ppip->loaded = btrue;

    // General data
    ppip->force = fget_next_bool( fileread );

    cTmp = fget_next_char( fileread );
         if ( 'L' == toupper(cTmp) )  ppip->type = PRTLIGHTSPRITE;
    else if ( 'S' == toupper(cTmp) )  ppip->type = PRTSOLIDSPRITE;
    else if ( 'T' == toupper(cTmp) )  ppip->type = PRTALPHASPRITE;

    ppip->imagebase = fget_next_int( fileread );
    ppip->numframes = fget_next_int( fileread );
    ppip->imageadd = fget_next_int( fileread );
    ppip->imageaddrand = fget_next_int( fileread );
    ppip->rotate_pair.base = fget_next_int( fileread );
    ppip->rotate_pair.rand = fget_next_int( fileread );
    ppip->rotateadd = fget_next_int( fileread );
    ppip->sizebase = fget_next_int( fileread );
    ppip->sizeadd = fget_next_int( fileread );
    ppip->spdlimit = fget_next_float( fileread );
    ppip->facingadd = fget_next_int( fileread );

    // override the base rotation
    if ( 0xFFFF != prt_direction[ ppip->imagebase ] )
    {
        ppip->rotate_pair.base = prt_direction[ ppip->imagebase ];
    };

    // Ending conditions
    ppip->endwater = fget_next_bool( fileread );
    ppip->endbump = fget_next_bool( fileread );
    ppip->endground = fget_next_bool( fileread );
    ppip->endlastframe = fget_next_bool( fileread );

    ppip->time = fget_next_int( fileread );

    // Collision data
    ppip->dampen     = fget_next_float( fileread );
    ppip->bumpmoney  = fget_next_int( fileread );
    ppip->bumpsize   = fget_next_int( fileread );
    ppip->bumpheight = fget_next_int( fileread );

    fget_next_pair( fileread ); ppip->damage = pair;
    ppip->damagetype = fget_next_damage_type( fileread );

    // Lighting data
    cTmp = fget_next_char( fileread );
         if ( 'T' == toupper(cTmp) ) ppip->dynalight_mode = DYNAON;
    else if ( 'L' == toupper(cTmp) ) ppip->dynalight_mode = DYNALOCAL;
    else                             ppip->dynalight_mode = DYNAOFF;

    ppip->dynalight_level = fget_next_float( fileread );
    ppip->dynalight_falloff = fget_next_int( fileread );

    // Initial spawning of this particle
    ppip->facing_pair.base    = fget_next_int( fileread );
    ppip->facing_pair.rand    = fget_next_int( fileread );
    ppip->xyspacing_pair.base = fget_next_int( fileread );
    ppip->xyspacing_pair.rand = fget_next_int( fileread );
    ppip->zspacing_pair.base  = fget_next_int( fileread );
    ppip->zspacing_pair.rand  = fget_next_int( fileread );
    ppip->xyvel_pair.base     = fget_next_int( fileread );
    ppip->xyvel_pair.rand     = fget_next_int( fileread );
    ppip->zvel_pair.base      = fget_next_int( fileread );
    ppip->zvel_pair.rand      = fget_next_int( fileread );

    // Continuous spawning of other particles
    ppip->contspawntime      = fget_next_int( fileread );
    ppip->contspawnamount    = fget_next_int( fileread );
    ppip->contspawnfacingadd = fget_next_int( fileread );
    ppip->contspawnpip       = fget_next_int( fileread );

    // End spawning of other particles
    ppip->endspawnamount    = fget_next_int( fileread );
    ppip->endspawnfacingadd = fget_next_int( fileread );
    ppip->endspawnpip       = fget_next_int( fileread );

    // Bump spawning of attached particles
    ppip->bumpspawnamount = fget_next_int( fileread );
    ppip->bumpspawnpip    = fget_next_int( fileread );

    // Random stuff  !!!BAD!!! Not complete
    ppip->dazetime     = fget_next_int( fileread );
    ppip->grogtime     = fget_next_int( fileread );
    ppip->spawnenchant = fget_next_bool( fileread );

    goto_colon( NULL, fileread, bfalse );  // !!Cause roll

    ppip->causepancake = fget_next_bool( fileread );

    ppip->needtarget         = fget_next_bool( fileread );
    ppip->targetcaster       = fget_next_bool( fileread );
    ppip->startontarget      = fget_next_bool( fileread );
    ppip->onlydamagefriendly = fget_next_bool( fileread );

    ppip->soundspawn = fget_next_int( fileread );

    ppip->soundend = fget_next_int( fileread );

    ppip->friendlyfire = fget_next_bool( fileread );

    ppip->hateonly = fget_next_bool( fileread );           //TODO: not implemented yet

    ppip->newtargetonspawn = fget_next_bool( fileread );

    ppip->targetangle = fget_next_int( fileread ) >> 1;
    ppip->homing = fget_next_bool( fileread );

    ppip->homingfriction = fget_next_float( fileread );
    ppip->homingaccel = fget_next_float( fileread );
    ppip->rotatetoface = fget_next_bool( fileread );

    // Clear expansions...
    ppip->endwall = ppip->endground;
    if ( ppip->homing )  ppip->damfx = DAMFX_NONE;

    // Read expansions
    while ( goto_colon( NULL, fileread, btrue ) )
    {
        idsz = fget_idsz( fileread );

             if ( idsz == MAKE_IDSZ( 'T', 'U', 'R', 'N' ) )  ppip->damfx  = DAMFX_NONE;
        else if ( idsz == MAKE_IDSZ( 'A', 'R', 'M', 'O' ) )  ppip->damfx |= DAMFX_ARMO;
        else if ( idsz == MAKE_IDSZ( 'B', 'L', 'O', 'C' ) )  ppip->damfx |= DAMFX_NBLOC;
        else if ( idsz == MAKE_IDSZ( 'A', 'R', 'R', 'O' ) )  ppip->damfx |= DAMFX_ARRO;
        else if ( idsz == MAKE_IDSZ( 'T', 'I', 'M', 'E' ) )  ppip->damfx |= DAMFX_TIME;
        else if ( idsz == MAKE_IDSZ( 'Z', 'S', 'P', 'D' ) )  ppip->zaimspd = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'F', 'S', 'N', 'D' ) )  ppip->soundfloor = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'W', 'S', 'N', 'D' ) )  ppip->soundwall = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'W', 'E', 'N', 'D' ) )  ppip->endwall = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'P', 'U', 'S', 'H' ) )  ppip->allowpush = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'D', 'L', 'E', 'V' ) )  ppip->dynalight_leveladd = fget_int( fileread ) / 1000.0f;
        else if ( idsz == MAKE_IDSZ( 'D', 'R', 'A', 'D' ) )  ppip->dynalight_falloffadd = fget_int( fileread ) / 1000.0f;
        else if ( idsz == MAKE_IDSZ( 'I', 'D', 'A', 'M' ) )  ppip->intdamagebonus = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'W', 'D', 'A', 'M' ) )  ppip->wisdamagebonus = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'O', 'R', 'N', 'T' ) )
        {
            char cTmp = fget_first_letter( fileread );
            switch ( toupper(cTmp) )
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
