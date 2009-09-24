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

/* Egoboo - particle.c
* Manages particle systems.
*/

#include "particle.h"
#include "enchant.h"
#include "char.h"
#include "mad.h"
#include "profile.h"

#include "log.h"
#include "sound.h"
#include "camera.h"
#include "mesh.h"
#include "game.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float            sprite_list_u[MAXPARTICLEIMAGE][2];        // Texture coordinates
float            sprite_list_v[MAXPARTICLEIMAGE][2];

Uint16           maxparticles = 512;                            // max number of particles

DECLARE_STACK( ACCESS_TYPE_NONE, pip_t, PipStack );
DECLARE_LIST ( ACCESS_TYPE_NONE, prt_t, PrtList );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int prt_count_free()
{
    return PrtList.free_count;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_free_one( Uint16 iprt )
{
    // ZZ> This function sticks a particle back on the free particle stack

    bool_t retval;

    if ( !VALID_PRT_RANGE(iprt) ) return bfalse;

    // particle "destructor"
    // sets all boolean values to false, incluting the "on" flag
    memset( PrtList.lst + iprt, 0, sizeof(prt_t) );

#if defined(DEBUG)
    {
        int cnt;
        // determine whether this texture is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < PrtList.free_count; cnt++ )
        {
            if ( iprt == PrtList.free_ref[cnt] ) return bfalse;
        }
    }
#endif

    // push it on the free stack
    retval = bfalse;
    if ( PrtList.free_count < TOTAL_MAX_PRT )
    {
        PrtList.free_ref[PrtList.free_count] = iprt;
        PrtList.free_count++;
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void play_particle_sound( Uint16 particle, Sint8 sound )
{
    // This function plays a sound effect for a particle

    prt_t * pprt;

    if ( !VALID_PRT(particle) ) return;
    pprt = PrtList.lst + particle;

    if ( sound >= 0 && sound < MAX_WAVE )
    {
        if ( VALID_PRO( pprt->iprofile ) )
        {
            sound_play_chunk( pprt->pos, pro_get_chunk(pprt->iprofile, sound) );
        }
        else
        {
            sound_play_chunk( pprt->pos, g_wavelist[sound] );
        }
    }
}

//--------------------------------------------------------------------------------------------
void free_one_particle_in_game( Uint16 particle )
{
    // ZZ> This function sticks a particle back on the free particle stack and
    //    plays the sound associated with the particle

    if ( VALID_PRT( particle) )
    {
        Uint16 child;
        prt_t * pprt = PrtList.lst + particle;

        if ( pprt->spawncharacterstate != SPAWNNOCHARACTER )
        {
            child = spawn_one_character( pprt->pos, pprt->iprofile, pprt->team, 0, pprt->facing, NULL, MAX_CHR );
            if ( VALID_CHR(child) )
            {
                chr_get_pai(child)->state = pprt->spawncharacterstate;
                chr_get_pai(child)->owner = pprt->chr;
            }
        }

        if ( VALID_PIP(pprt->pip) )
        {
            play_particle_sound( particle, PipStack.lst[pprt->pip].soundend );
        }

    }

    PrtList_free_one( particle );
}

//--------------------------------------------------------------------------------------------
Uint16 PrtList_get_free()
{
    // ZZ> This function returns the next free particle or TOTAL_MAX_PRT if there are none

    Uint16 retval = TOTAL_MAX_PRT;

    if ( PrtList.free_count > 0 )
    {
        PrtList.free_count--;
        retval = PrtList.free_ref[PrtList.free_count];
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int get_free_particle( int force )
{
    // ZZ> This function gets an unused particle.  If all particles are in use
    //    and force is set, it grabs the first unimportant one.  The particle
    //    index is the return value
    int particle;

    // Return maxparticles if we can't find one
    particle = TOTAL_MAX_PRT;
    if ( 0 == PrtList.free_count )
    {
        if ( force )
        {
            // Gotta find one, so go through the list and replace a unimportant one
            particle = 0;

            while ( particle < maxparticles )
            {
                if ( PrtList.lst[particle].bumpsize == 0 )
                {
                    // Found one
                    return particle;
                }

                particle++;
            }
        }
    }
    else
    {
        if ( force || PrtList.free_count > ( maxparticles / 4 ) )
        {
            // Just grab the next one
            particle = PrtList_get_free();
        }
    }

    return (particle >= maxparticles) ? TOTAL_MAX_PRT : particle;
}

//--------------------------------------------------------------------------------------------
void prt_init( prt_t * pprt )
{
    if( NULL == pprt ) return;

    memset( pprt, 0, sizeof(prt_t) );

    /* pprt->floor_level = 0; */
    pprt->spawncharacterstate = SPAWNNOCHARACTER;

    // Lighting and sound
    /* pprt->dynalight_on = bfalse; */

    // Image data
    /* pprt->image = 0; */

    // "no lifetime" = "eternal"
    /* pprt->is_eternal = bfalse; */
    pprt->time      = (Uint32)~0;

}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_particle( float x, float y, float z,
                           Uint16 facing, Uint16 iprofile, Uint16 ipip,
                           Uint16 characterattach, Uint16 vrt_offset, Uint8 team,
                           Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget )
{
    // ZZ> This function spawns a new particle, and returns the number of that particle
    int iprt, velocity;
    GLvector3 vel;
    float tvel;
    int offsetfacing = 0, newrand;
    prt_t * pprt;
    pip_t * ppip;
    Uint32 prt_lifetime;

    // Convert from local ipip to global ipip
    if ( ipip < MAX_PIP_PER_PROFILE && VALID_PRO(iprofile) )
    {
        ipip = pro_get_ipip(iprofile, ipip);
    }

    if ( INVALID_PIP(ipip) )
    {
        log_warning( "spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
            ipip,
            characterorigin, VALID_CHR(characterorigin) ? ChrList.lst[characterorigin].name : "INVALID",
            iprofile, VALID_PRO(iprofile) ? ProList.lst[iprofile].name : "INVALID" );
        return TOTAL_MAX_PRT;
    }
    ppip = PipStack.lst + ipip;

    iprt = get_free_particle( ppip->force );
    if ( !VALID_PRT_RANGE(iprt) )
    {
        log_warning( "spawn_one_particle() - cannot allocate a particle owner == %d(\"%s\"), pip == %d(\"%s\"), profile == %d(\"%s\")\n",
            characterorigin, VALID_CHR(characterorigin) ? ChrList.lst[characterorigin].name : "INVALID",
            ipip, VALID_PIP(ipip) ? PipStack.lst[ipip].name : "INVALID",
            iprofile, VALID_PRO(iprofile) ? ProList.lst[iprofile].name : "INVALID" );
        return TOTAL_MAX_PRT;
    }
    pprt = PrtList.lst + iprt;

    // clear out all data
    prt_init( pprt );

    // Necessary data for any part
    strncpy( pprt->name, ppip->name, SDL_arraysize(pprt->name) );
    pprt->on = btrue;

    pprt->pip = ipip;
    pprt->iprofile = iprofile;
    pprt->inview = bfalse;
    pprt->team = team;
    pprt->chr = characterorigin;
    pprt->damagetype = ppip->damagetype;

    // Lighting and sound
    if ( multispawn == 0 )
    {
        pprt->dynalight_on = ppip->dynalight_mode;
        if ( ppip->dynalight_mode == DYNALOCAL )
        {
            pprt->dynalight_on = bfalse;
        }
    }

    pprt->dynalight_level = ppip->dynalight_level;
    pprt->dynalight_falloff = ppip->dynalight_falloff;

    // Set character attachments ( characterattach==MAX_CHR means none )
    pprt->attachedtocharacter = characterattach;
    pprt->vrt_off = vrt_offset;

    // Correct facing
    facing += ppip->facing_pair.base;

    // Targeting...
    vel.z = 0;
    z = z + generate_randmask( ppip->zspacing_pair.base, ppip->zspacing_pair.rand ) - ( ppip->zspacing_pair.rand >> 1 );
    velocity = generate_randmask( ppip->xyvel_pair.base, ppip->xyvel_pair.rand );
    pprt->target = oldtarget;
    if ( ppip->newtargetonspawn )
    {
        if ( ppip->targetcaster )
        {
            // Set the target to the caster
            pprt->target = characterorigin;
        }
        else
        {
            // Find a target
            pprt->target = get_particle_target( x, y, z, facing, ipip, team, characterorigin, oldtarget );
            if ( pprt->target != MAX_CHR && !ppip->homing )
            {
                facing -= glouseangle;
            }

            // Correct facing for dexterity...
            offsetfacing = 0;
            if ( ChrList.lst[characterorigin].dexterity < PERFECTSTAT )
            {
                // Correct facing for randomness
                offsetfacing  = generate_randmask( -(ppip->facing_pair.rand >> 1), ppip->facing_pair.rand);
                offsetfacing -= ppip->facing_pair.rand >> 1;
                offsetfacing  = ( offsetfacing * ( PERFECTSTAT - ChrList.lst[characterorigin].dexterity ) ) / PERFECTSTAT;  // Divided by PERFECTSTAT
            }
            if ( pprt->target != MAX_CHR && ppip->zaimspd != 0 )
            {
                // These aren't velocities...  This is to do aiming on the Z axis
                if ( velocity > 0 )
                {
                    vel.x = ChrList.lst[pprt->target].pos.x - x;
                    vel.y = ChrList.lst[pprt->target].pos.y - y;
                    tvel = SQRT( vel.x * vel.x + vel.y * vel.y ) / velocity;  // This is the number of steps...
                    if ( tvel > 0 )
                    {
                        vel.z = ( ChrList.lst[pprt->target].pos.z + ( ChrList.lst[pprt->target].bump.height * 0.5f ) - z ) / tvel;  // This is the vel.z alteration
                        if ( vel.z < -( ppip->zaimspd >> 1 ) ) vel.z = -( ppip->zaimspd >> 1 );
                        if ( vel.z > ppip->zaimspd ) vel.z = ppip->zaimspd;
                    }
                }
            }
        }

        // Does it go away?
        if ( pprt->target == MAX_CHR && ppip->needtarget )
        {
            free_one_particle_in_game( iprt );
            return maxparticles;
        }

        // Start on top of target
        if ( pprt->target != MAX_CHR && ppip->startontarget )
        {
            x = ChrList.lst[pprt->target].pos.x;
            y = ChrList.lst[pprt->target].pos.y;
        }
    }
    else
    {
        // Correct facing for randomness
        offsetfacing = generate_randmask( 0,  ppip->facing_pair.rand );
        offsetfacing -= ppip->facing_pair.rand >> 1;
    }

    facing += offsetfacing;
    pprt->facing = facing;
    facing = facing >> 2;

    // Location data from arguments
    newrand = generate_randmask( ppip->xyspacing_pair.base, ppip->xyspacing_pair.rand );
    x += turntocos[( facing+8192 ) & TRIG_TABLE_MASK] * newrand;
    y += turntosin[( facing+8192 ) & TRIG_TABLE_MASK] * newrand;
    x = CLIP(x, 0, PMesh->info.edge_x - 2);
    y = CLIP(y, 0, PMesh->info.edge_y - 2);

    pprt->pos.x = x;
    pprt->pos.y = y;
    pprt->pos.z = z;

    // Velocity data
    vel.x = turntocos[( facing+8192 ) & TRIG_TABLE_MASK] * velocity;
    vel.y = turntosin[( facing+8192 ) & TRIG_TABLE_MASK] * velocity;
    vel.z += generate_randmask( ppip->zvel_pair.base, ppip->zvel_pair.rand ) - ( ppip->zvel_pair.rand >> 1 );
    pprt->vel = vel;

    // Template values
    pprt->bumpsize = ppip->bumpsize;
    pprt->bumpsizebig = pprt->bumpsize * SQRT_TWO;
    pprt->bumpheight = ppip->bumpheight;
    pprt->type = ppip->type;

    // Image data
    pprt->rotate = generate_randmask( ppip->rotate_pair.base, ppip->rotate_pair.rand );
    pprt->rotateadd = ppip->rotateadd;
    pprt->size = ppip->sizebase;
    pprt->sizeadd = ppip->sizeadd;
    pprt->imageadd = generate_randmask( ppip->imageadd , ppip->imageaddrand );
    pprt->imagestt = INT_TO_FP8( ppip->imagebase );
    pprt->imagemax = INT_TO_FP8( ppip->numframes );
    prt_lifetime = ppip->time;
    if ( ppip->endlastframe && pprt->imageadd != 0 )
    {
        if ( ppip->time == 0 )
        {
            // Part time is set to 1 cycle
            int frames = ( pprt->imagemax / pprt->imageadd ) - 1;
            prt_lifetime = frames;
        }
        else
        {
            // Part time is used to give number of cycles
            int frames = ( ( pprt->imagemax / pprt->imageadd ) - 1 );
            prt_lifetime = ppip->time * frames;
        }
    }

    // "no lifetime" = "eternal"
    if ( 0 == prt_lifetime )
    {
        pprt->is_eternal = btrue;
    }
    else
    {
        pprt->time = frame_all + prt_lifetime;
    }

    // Set onwhichfan...
    pprt->onwhichfan   = mesh_get_tile( PMesh, pprt->pos.x, pprt->pos.y );
    pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );

    // Damage stuff
    pprt->damage.base = ppip->damage.base;
    pprt->damage.rand = ppip->damage.rand;

    // Spawning data
    pprt->spawntime = ppip->contspawntime;
    if ( pprt->spawntime != 0 )
    {
        pprt->spawntime = 1;
        if ( pprt->attachedtocharacter != MAX_CHR )
        {
            pprt->spawntime++; // Because attachment takes an update before it happens
        }
    }

    // Sound effect
    play_particle_sound( iprt, ppip->soundspawn );

    return iprt;
}

//--------------------------------------------------------------------------------------------
Uint8 __prthitawall( Uint16 particle )
{
    // ZZ> This function returns nonzero if the particle hit a wall

    Uint8  retval = MPDFX_IMPASS | MPDFX_WALL;
    Uint32 fan;

    pip_t * ppip;
    prt_t * pprt;

    if( INVALID_PRT(particle) ) return retval;
    pprt = PrtList.lst + particle;

    ppip = prt_get_ppip(particle);

    fan = mesh_get_tile( PMesh, pprt->pos.x, pprt->pos.y );
    if ( VALID_TILE(PMesh, fan) )
    {
        if ( ppip->bumpmoney )
        {
            retval = mesh_test_fx(PMesh, fan, MPDFX_IMPASS | MPDFX_WALL );
        }
        else
        {
            retval = mesh_test_fx(PMesh, fan, MPDFX_IMPASS);
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void move_particles( void )
{
    // ZZ> This is the particle physics function

    int tnc;
    Uint16 cnt;
    Uint16 facing, ipip, particle;
    float level;

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        pip_t * ppip;
        prt_t * pprt;

        if ( !PrtList.lst[cnt].on ) continue;
        pprt = PrtList.lst + cnt;

        if ( pprt->is_hidden ) continue;

        pprt->onwhichfan   = mesh_get_tile ( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->floor_level  = mesh_get_level( PMesh, pprt->pos.x, pprt->pos.y );

        // To make it easier
        ipip = pprt->pip;
        if ( INVALID_PIP( ipip ) ) continue;
        ppip = PipStack.lst + ipip;

        // Animate particle
        pprt->image = pprt->image + pprt->imageadd;
        if ( pprt->image >= pprt->imagemax ) pprt->image = 0;

        pprt->rotate += pprt->rotateadd;
        if ( ( (int)pprt->size + (int)pprt->sizeadd ) > (int)0x0000FFFF ) pprt->size = 0xFFFF;
        else if ( ( pprt->size + pprt->sizeadd ) < 0 ) pprt->size = 0;
        else pprt->size += pprt->sizeadd;

        // Change dyna light values
        pprt->dynalight_level += ppip->dynalight_leveladd;
        pprt->dynalight_falloff += ppip->dynalight_falloffadd;

        // Make it sit on the floor...  Shift is there to correct for sprite size
        level = pprt->floor_level + (FP8_TO_INT( pprt->size ) >> 1);

        // Check floor collision and do iterative physics
        if ( ( pprt->pos.z < level && pprt->vel.z < 0.1f ) || ( pprt->pos.z < level - PRTLEVELFIX ) )
        {
            pprt->pos.z = level;
            pprt->vel.x = pprt->vel.x * noslipfriction;
            pprt->vel.y = pprt->vel.y * noslipfriction;
            if ( ppip->endground )
            {
                pprt->time  = frame_all + 1;
                pprt->poofme = btrue;
            }
            if ( pprt->vel.z < 0 )
            {
                if ( pprt->vel.z > -STOPBOUNCINGPART )
                {
                    // Make it not bounce
                    pprt->pos.z -= 0.0001f;
                }
                else
                {
                    // Make it bounce
                    pprt->vel.z = -pprt->vel.z * ppip->dampen;
                    // Play the sound for hitting the floor [FSND]
                    play_particle_sound( cnt, ppip->soundfloor );
                }
            }
        }
        else if ( INVALID_CHR( pprt->attachedtocharacter ) )
        {
            pprt->pos.x += pprt->vel.x;
            if ( __prthitawall( cnt ) )
            {
                // Play the sound for hitting a wall [WSND]
                play_particle_sound( cnt, ppip->soundwall );
                pprt->pos.x -= pprt->vel.x;
                pprt->vel.x = ( -pprt->vel.x * ppip->dampen );
                if ( ppip->endwall )
                {
                    pprt->time  = frame_all + 1;
                    pprt->poofme = btrue;
                }
                else
                {
                    // Change facing
                    facing = pprt->facing;
                    if ( facing < 32768 )
                    {
                        facing -= FACE_NORTH;
                        facing = ~facing;
                        facing += FACE_NORTH;
                    }
                    else
                    {
                        facing -= FACE_SOUTH;
                        facing = ~facing;
                        facing += FACE_SOUTH;
                    }

                    pprt->facing = facing;
                }
            }

            pprt->pos.y += pprt->vel.y;
            if ( __prthitawall( cnt ) )
            {
                pprt->pos.y -= pprt->vel.y;
                pprt->vel.y = ( -pprt->vel.y * ppip->dampen );
                if ( ppip->endwall )
                {
                    pprt->time  = frame_all + 1;
                    pprt->poofme = btrue;
                }
                else
                {
                    // Change facing
                    facing = pprt->facing;
                    if ( facing < 16384 || facing > 49152 )
                    {
                        facing = ~facing;
                    }
                    else
                    {
                        facing -= FACE_EAST;
                        facing = ~facing;
                        facing += FACE_EAST;
                    }

                    pprt->facing = facing;
                }
            }

            pprt->pos.z += pprt->vel.z;
            pprt->vel.z += gravity;
        }

        // Do homing
        if ( ppip->homing && VALID_CHR( pprt->target ) )
        {
            if ( !ChrList.lst[pprt->target].alive )
            {
                pprt->time  = frame_all + 1;
                pprt->poofme = btrue;
            }
            else
            {
                if ( INVALID_CHR( pprt->attachedtocharacter ) )
                {
                    pprt->vel.x = ( pprt->vel.x + ( ( ChrList.lst[pprt->target].pos.x - pprt->pos.x ) * ppip->homingaccel ) ) * ppip->homingfriction;
                    pprt->vel.y = ( pprt->vel.y + ( ( ChrList.lst[pprt->target].pos.y - pprt->pos.y ) * ppip->homingaccel ) ) * ppip->homingfriction;
                    pprt->vel.z = ( pprt->vel.z + ( ( ChrList.lst[pprt->target].pos.z + ( ChrList.lst[pprt->target].bump.height * 0.5f ) - pprt->pos.z ) * ppip->homingaccel ) );

                }
                if ( ppip->rotatetoface )
                {
                    // Turn to face target
                    facing = ATAN2( ChrList.lst[pprt->target].pos.y - pprt->pos.y, ChrList.lst[pprt->target].pos.x - pprt->pos.x ) * 0xFFFF / ( TWO_PI );
                    facing += 32768;
                    pprt->facing = facing;
                }
            }
        }

        // Do speed limit on Z
        if ( pprt->vel.z < -ppip->spdlimit )  pprt->vel.z = -ppip->spdlimit;

        // Spawn new particles if continually spawning
        if ( ppip->contspawnamount > 0 )
        {
            pprt->spawntime--;
            if ( pprt->spawntime == 0 )
            {
                pprt->spawntime = ppip->contspawntime;
                facing = pprt->facing;
                tnc = 0;

                while ( tnc < ppip->contspawnamount )
                {
                    particle = spawn_one_particle( pprt->pos.x, pprt->pos.y, pprt->pos.z,
                                                   facing, pprt->iprofile, ppip->contspawnpip,
                                                   MAX_CHR, GRIP_LAST, pprt->team, pprt->chr, tnc, pprt->target );
                    if ( PipStack.lst[pprt->pip].facingadd != 0 && particle != TOTAL_MAX_PRT )
                    {
                        // Hack to fix velocity
                        PrtList.lst[particle].vel.x += pprt->vel.x;
                        PrtList.lst[particle].vel.y += pprt->vel.y;
                    }

                    facing += ppip->contspawnfacingadd;
                    tnc++;
                }
            }
        }

        // Check underwater
        if ( pprt->pos.z < water.douse_level && ppip->endwater && VALID_TILE(PMesh, pprt->onwhichfan) && (0 != mesh_test_fx( PMesh, pprt->onwhichfan, MPDFX_WATER )) )
        {
            // Splash for particles is just a ripple
            spawn_one_particle( pprt->pos.x, pprt->pos.y, water.surface_level,
                                0, MAX_PROFILE, RIPPLE, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, 0, MAX_CHR );

            // Check for disaffirming character
            if ( VALID_CHR( pprt->attachedtocharacter ) && pprt->chr == pprt->attachedtocharacter )
            {
                // Disaffirm the whole character
                disaffirm_attached_particles( pprt->attachedtocharacter );
            }
            else
            {
                // Just destroy the particle
                //                   free_one_particle_in_game(cnt);
                pprt->time  = frame_all + 1;
                pprt->poofme = btrue;
            }
        }

        // Spawn new particles if time for old one is up
        if ( pprt->poofme || ( !pprt->is_eternal && frame_all >= pprt->time ) )
        {
            facing = pprt->facing;

            for ( tnc = 0; tnc < ppip->endspawnamount; tnc++ )
            {
                spawn_one_particle( pprt->pos.x - pprt->vel.x, pprt->pos.y - pprt->vel.y, pprt->pos.z,
                                    facing, pprt->iprofile, ppip->endspawnpip,
                                    MAX_CHR, GRIP_LAST, pprt->team, pprt->chr, tnc, pprt->target );
                facing += ppip->endspawnfacingadd;
                tnc++;
            }

            free_one_particle_in_game( cnt );
        }

        pprt->facing += ppip->facingadd;
    }
}

//--------------------------------------------------------------------------------------------
void PrtList_free_all()
{
    // ZZ> This function resets the particle allocation lists

    int cnt;

    // free all the particles
    PrtList.free_count = 0;
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        // reuse this code
        PrtList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void setup_particles()
{
    // ZZ> This function sets up particle data
    int cnt;
    double x, y;

    // Image coordinates on the big particle bitmap
    for ( cnt = 0; cnt < MAXPARTICLEIMAGE; cnt++ )
    {
        x = cnt & 15;
        y = cnt >> 4;
        sprite_list_u[cnt][0] = (float)( ( 0.05f + x ) / 16.0f );
        sprite_list_u[cnt][1] = (float)( ( 0.95f + x ) / 16.0f );
        sprite_list_v[cnt][0] = (float)( ( 0.05f + y ) / 16.0f );
        sprite_list_v[cnt][1] = (float)( ( 0.95f + y ) / 16.0f );
    }

    // Reset the allocation table
    PrtList_free_all();
}

//--------------------------------------------------------------------------------------------
void spawn_bump_particles( Uint16 character, Uint16 particle )
{
    // ZZ> This function is for catching characters on fire and such

    int cnt;
    Sint16 x, y, z;
    Uint32 distance, bestdistance;
    Uint16 frame;
    Uint16 facing, bestvertex;
    Uint16 amount;
    Uint16 vertices;
    Uint16 direction;
    float fsin, fcos;
    pip_t * ppip;
    chr_t * pchr;
    mad_t * pmad;
    prt_t * pprt;
    cap_t * pcap;

    if ( INVALID_PRT(particle) ) return;
    pprt = PrtList.lst + particle;

    if ( INVALID_PIP(pprt->pip) ) return;
    ppip = PipStack.lst + pprt->pip;

    // no point in going on, is there?
    if ( 0 == ppip->bumpspawnamount && !ppip->spawnenchant ) return;
    amount = ppip->bumpspawnamount;

    if ( INVALID_CHR(character) ) return;
    pchr = ChrList.lst + character;

    pmad = chr_get_pmad( character );
    if ( NULL == pmad ) return;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return;

    // Only damage if hitting from proper direction
    direction = ( ATAN2( pprt->vel.y, pprt->vel.x ) + PI ) * 0xFFFF / TWO_PI;
    direction = pchr->turn_z - direction + 32768;

    // Check that direction
    if ( !is_invictus_direction( direction, character, ppip->damfx) )
    {
        vertices = pmad->md2_data.vertices;

        // Spawn new enchantments
        if ( ppip->spawnenchant )
        {
            spawn_one_enchant( pprt->chr, character, MAX_CHR, MAX_ENC, pprt->iprofile );
        }

        // Spawn particles
        if ( amount != 0 && !pcap->resistbumpspawn && !pchr->invictus && vertices != 0 && ( pchr->damagemodifier[pprt->damagetype]&DAMAGESHIFT ) < 3 )
        {

            if ( amount == 1 )
            {
                // A single particle ( arrow? ) has been stuck in the character...
                // Find best vertex to attach to

                bestvertex = 0;
                bestdistance = 1 << 31;         //Really high number

                z = -pchr->pos.z + pprt->pos.z + RAISE;
                facing = pprt->facing - pchr->turn_z - FACE_NORTH;
                facing = facing >> 2;
                fsin = turntosin[facing & TRIG_TABLE_MASK ];
                fcos = turntocos[facing & TRIG_TABLE_MASK ];
                x = -8192 * fsin;
                y =  8192 * fcos;
                z = z << 10;/// pchr->scale;
                frame = pmad->md2_data.framestart;

                for ( cnt = 0; cnt < amount; cnt++ )
                {
                    distance = ABS( x - Md2FrameList[frame].vrtx[vertices-cnt-1] ) + ABS( y - Md2FrameList[frame].vrty[vertices-cnt-1] ) + ( ABS( z - Md2FrameList[frame].vrtz[vertices-cnt-1] ) );
                    if ( distance < bestdistance )
                    {
                        bestdistance = distance;
                        bestvertex = cnt;
                    }
                }

                spawn_one_particle( pchr->pos.x, pchr->pos.y, pchr->pos.z, 0, pprt->iprofile, ppip->bumpspawnpip,
                                    character, bestvertex + 1, pprt->team, pprt->chr, cnt, character );
            }

            else
            {
                //Multiple particles are attached to character

                amount = ( amount * vertices ) >> 5;  // Correct amount for size of character

                for ( cnt = 0; cnt < amount; cnt++ )
                {
                    int irand = RANDIE;
                    spawn_one_particle( pchr->pos.x, pchr->pos.y, pchr->pos.z, 0, pprt->iprofile, ppip->bumpspawnpip,
                                        character, irand % vertices, pprt->team, pprt->chr, cnt, character );
                }
            }

        }
    }
}

//--------------------------------------------------------------------------------------------
int prt_is_over_water( Uint16 cnt )
{
    // This function returns btrue if the particle is over a water tile
    Uint32 fan;

    if ( INVALID_PRT(cnt) ) return bfalse;

    fan = mesh_get_tile( PMesh, PrtList.lst[cnt].pos.x, PrtList.lst[cnt].pos.y );
    if ( VALID_TILE(PMesh, fan) )
    {
        if ( 0 != mesh_test_fx( PMesh, fan, MPDFX_WATER ) )  return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 PipStack_get_free()
{
    Uint16 retval = MAX_PIP;

    if( PipStack.count < MAX_PIP )
    {
        retval = PipStack.count;
        PipStack.count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int load_one_particle_profile( const char *szLoadName, Uint16 pip_override )
{
    // ZZ> This function loads a particle template, returning bfalse if the file wasn't
    //    found

    Uint16  ipip;
    pip_t * ppip;

    ipip = MAX_PIP;
    if( VALID_PIP_RANGE(pip_override) )
    {
        release_one_pip(pip_override);
        ipip = pip_override;
    }
    else
    {
        ipip = PipStack_get_free();
    }

    if ( !VALID_PIP_RANGE(ipip) )
    {
        return MAX_PIP;
    }
    ppip = PipStack.lst + ipip;

    if( NULL == load_one_pip_file( szLoadName, ppip ) )
    {
        return MAX_PIP;
    }

    ppip->soundend = CLIP(ppip->soundend, -1, MAX_WAVE);
    ppip->soundspawn = CLIP(ppip->soundspawn, -1, MAX_WAVE);
//   if ( ppip->dynalight_falloff > MAXFALLOFF && PMod->rtscontrol )  ppip->dynalight_falloff = MAXFALLOFF;

    return ipip;
}

//--------------------------------------------------------------------------------------------
void reset_particles( const char* modname )
{
    // ZZ> This resets all particle data and reads in the coin and water particles

    STRING newloadname;
    char *loadpath;

    release_all_local_pips();
    release_all_pip();

    // Load in the standard global particles ( the coins for example )
    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "1money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, COIN1 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "5money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, COIN5 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "25money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, COIN25 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "100money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, COIN100 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    // Load module specific information
    make_newloadname( modname, "gamedat" SLASH_STR "weather4.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname, WEATHER4 ) )
    {
        log_error( "Data file was not found! (%s)\n", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "weather5.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname, WEATHER5 ) )
    {
        log_error( "Data file was not found! (%s)\n", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "splash.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname, SPLASH ) )
    {
        if (cfg.dev_mode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "splash.txt";
        if ( MAX_PIP == load_one_particle_profile( loadpath, SPLASH ) )
        {
            log_error( "Data file was not found! (%s)\n", loadpath );
        }
    }

    make_newloadname( modname, "gamedat" SLASH_STR "ripple.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname, RIPPLE ) )
    {
        if (cfg.dev_mode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "ripple.txt";
        if ( MAX_PIP == load_one_particle_profile( loadpath, RIPPLE ) )
        {
            log_error( "Data file was not found! (%s)\n", loadpath );
        }
    }

    // This is also global...
    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "defend.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, DEFEND ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    PipStack.count = DEFEND;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint16  prt_get_ipip( Uint16 iprt )
{
    prt_t * pprt;

    if( INVALID_PRT(iprt) ) return MAX_PIP;
    pprt = PrtList.lst + iprt;

    if( INVALID_PIP(pprt->pip) ) return MAX_PIP;

    return pprt->pip;
}

//--------------------------------------------------------------------------------------------
pip_t * prt_get_ppip( Uint16 iprt )
{
    prt_t * pprt;

    if( INVALID_PRT(iprt) ) return NULL;
    pprt = PrtList.lst + iprt;

    if( INVALID_PIP(pprt->pip) ) return NULL;

    return PipStack.lst + pprt->pip;
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void init_all_pip()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        memset( PipStack.lst + cnt, 0, sizeof(pip_t) );
    }
}

//--------------------------------------------------------------------------------------------
void release_all_pip()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        release_one_pip( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool_t release_one_pip( Uint16 ipip )
{
    pip_t * ppip;

    if( !VALID_PIP_RANGE(ipip) ) return bfalse;
    ppip = PipStack.lst + ipip;

    if( !ppip->loaded ) return btrue;

    memset( ppip, 0, sizeof(pip_t) );

    ppip->loaded  = bfalse;
    ppip->name[0] = '\0';

    return btrue;
}
