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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - char.c
 */

#include "char.h"
#include "enchant.h"
#include "log.h"
#include "script.h"
#include "menu.h"
#include "sound.h"
#include "camera.h"
#include "input.h"
#include "particle.h"
#include "file_common.h"
#include "Md2.h"
#include "passage.h"

#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo_math.h"
#include "egoboo.h"

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define BUY  0
#define SELL 1

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
chop_data_t chop = {0, 0};

static int            numfreechr = 0;             // For allocation
static Uint16         freechrlist[MAXCHR];        //

team_t TeamList[MAXTEAM];

int   importobject;
cap_t CapList[MAXMODEL];

chr_t ChrList[MAXCHR];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int chr_count_free()
{
    return numfreechr;
}

//--------------------------------------------------------------------------------------------
void flash_character_height( Uint16 character, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high )
{
    // ZZ> This function sets a character's lighting depending on vertex height...
    //     Can make feet dark and head light...
    int cnt;
    Uint16 frame;
    Sint16 z;

    frame = ChrList[character].frame;

    for ( cnt = 0; cnt < MadList[ChrList[character].model].transvertices; cnt++  )
    {
        z = Md2FrameList[frame].vrtz[cnt];
        if ( z < low )
        {
            ChrList[character].vrta[cnt] = valuelow;
        }
        else
        {
            if ( z > high )
            {
                ChrList[character].vrta[cnt] = valuehigh;
            }
            else
            {
                ChrList[character].vrta[cnt] = ( valuehigh * ( z - low ) / ( high - low ) ) +
                                               ( valuelow * ( high - z ) / ( high - low ) );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders()
{
    // ZZ> This function keeps weapons near their holders
    int cnt, character;

    // !!!BAD!!!  May need to do 3 levels of attachment...
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( ChrList[cnt].on )
        {
            character = ChrList[cnt].attachedto;
            if ( character == MAXCHR )
            {
                // Keep inventory with character
                if ( !ChrList[cnt].inpack )
                {
                    character = ChrList[cnt].nextinpack;

                    while ( character != MAXCHR )
                    {
                        ChrList[character].xpos = ChrList[cnt].xpos;
                        ChrList[character].ypos = ChrList[cnt].ypos;
                        ChrList[character].zpos = ChrList[cnt].zpos;
                        // Copy olds to make SendMessageNear work
                        ChrList[character].oldx = ChrList[cnt].xpos;
                        ChrList[character].oldy = ChrList[cnt].ypos;
                        ChrList[character].oldz = ChrList[cnt].zpos;
                        character = ChrList[character].nextinpack;
                    }
                }
            }
            else
            {
                // Keep in hand weapons with character
                if ( ChrList[character].matrixvalid && ChrList[cnt].matrixvalid )
                {
                    ChrList[cnt].xpos = ChrList[cnt].matrix.CNV( 3, 0 );
                    ChrList[cnt].ypos = ChrList[cnt].matrix.CNV( 3, 1 );
                    ChrList[cnt].zpos = ChrList[cnt].matrix.CNV( 3, 2 );
                }
                else
                {
                    ChrList[cnt].xpos = ChrList[character].xpos;
                    ChrList[cnt].ypos = ChrList[character].ypos;
                    ChrList[cnt].zpos = ChrList[character].zpos;
                }

                ChrList[cnt].turnleftright = ChrList[character].turnleftright;

                // Copy this stuff ONLY if it's a weapon, not for mounts
                if ( ChrList[character].transferblend && ChrList[cnt].isitem )
                {

                    // Items become partially invisible in hands of players
                    if ( ChrList[character].isplayer && ChrList[character].alpha != 255 )
                        ChrList[cnt].alpha = 128;
                    else
                    {
                        // Only if not naturally transparent
                        if ( CapList[ChrList[cnt].model].alpha == 255 )
                            ChrList[cnt].alpha = ChrList[character].alpha;
                        else ChrList[cnt].alpha = CapList[ChrList[cnt].model].alpha;
                    }

                    //Do light too
                    if ( ChrList[character].isplayer && ChrList[character].light != 255 )
                        ChrList[cnt].light = 128;
                    else
                    {
                        // Only if not naturally transparent
                        if ( CapList[ChrList[cnt].model].light == 255 )
                            ChrList[cnt].light = ChrList[character].light;
                        else ChrList[cnt].light = CapList[ChrList[cnt].model].light;
                    }
                }
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix( Uint16 cnt )
{
    // ZZ> This function sets one character's matrix
    Uint16 tnc;
    ChrList[cnt].matrixvalid = btrue;
    if ( ChrList[cnt].overlay )
    {
        // Overlays are kept with their target...
        tnc = ChrList[cnt].ai.target;
        ChrList[cnt].xpos = ChrList[tnc].xpos;
        ChrList[cnt].ypos = ChrList[tnc].ypos;
        ChrList[cnt].zpos = ChrList[tnc].zpos;
        ChrList[cnt].matrix.CNV( 0, 0 ) = ChrList[tnc].matrix.CNV( 0, 0 );
        ChrList[cnt].matrix.CNV( 0, 1 ) = ChrList[tnc].matrix.CNV( 0, 1 );
        ChrList[cnt].matrix.CNV( 0, 2 ) = ChrList[tnc].matrix.CNV( 0, 2 );
        ChrList[cnt].matrix.CNV( 0, 3 ) = ChrList[tnc].matrix.CNV( 0, 3 );
        ChrList[cnt].matrix.CNV( 1, 0 ) = ChrList[tnc].matrix.CNV( 1, 0 );
        ChrList[cnt].matrix.CNV( 1, 1 ) = ChrList[tnc].matrix.CNV( 1, 1 );
        ChrList[cnt].matrix.CNV( 1, 2 ) = ChrList[tnc].matrix.CNV( 1, 2 );
        ChrList[cnt].matrix.CNV( 1, 3 ) = ChrList[tnc].matrix.CNV( 1, 3 );
        ChrList[cnt].matrix.CNV( 2, 0 ) = ChrList[tnc].matrix.CNV( 2, 0 );
        ChrList[cnt].matrix.CNV( 2, 1 ) = ChrList[tnc].matrix.CNV( 2, 1 );
        ChrList[cnt].matrix.CNV( 2, 2 ) = ChrList[tnc].matrix.CNV( 2, 2 );
        ChrList[cnt].matrix.CNV( 2, 3 ) = ChrList[tnc].matrix.CNV( 2, 3 );
        ChrList[cnt].matrix.CNV( 3, 0 ) = ChrList[tnc].matrix.CNV( 3, 0 );
        ChrList[cnt].matrix.CNV( 3, 1 ) = ChrList[tnc].matrix.CNV( 3, 1 );
        ChrList[cnt].matrix.CNV( 3, 2 ) = ChrList[tnc].matrix.CNV( 3, 2 );
        ChrList[cnt].matrix.CNV( 3, 3 ) = ChrList[tnc].matrix.CNV( 3, 3 );
    }
    else
    {
        ChrList[cnt].matrix = ScaleXYZRotateXYZTranslate( ChrList[cnt].fat, ChrList[cnt].fat, ChrList[cnt].fat,
                              ChrList[cnt].turnleftright >> 2,
                              ( ( Uint16 ) ( ChrList[cnt].turnmapud + 32768 ) ) >> 2,
                              ( ( Uint16 ) ( ChrList[cnt].turnmaplr + 32768 ) ) >> 2,
                              ChrList[cnt].xpos, ChrList[cnt].ypos, ChrList[cnt].zpos );
    }
}

//--------------------------------------------------------------------------------------------
void free_one_character( Uint16 character )
{
    // ZZ> This function sticks a character back on the free character stack
    int cnt;

    freechrlist[numfreechr] = character;
    numfreechr++;

    // Remove from stat list
    if ( ChrList[character].staton )
    {
        ChrList[character].staton = bfalse;
        cnt = 0;

        while ( cnt < numstat )
        {
            if ( statlist[cnt] == character )
            {
                cnt++;

                while ( cnt < numstat )
                {
                    statlist[cnt-1] = statlist[cnt];
                    cnt++;
                }

                numstat--;
            }

            cnt++;
        }
    }

    // Make sure everyone knows it died
    if ( ChrList[character].alive && !CapList[ChrList[character].model].invictus )
    {
        TeamList[ChrList[character].baseteam].morale--;
    }

    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( ChrList[cnt].on )
        {
            if ( ChrList[cnt].ai.target == character )
            {
                ChrList[cnt].ai.alert |= ALERTIF_TARGETKILLED;
                ChrList[cnt].ai.target = cnt;
            }
            if ( TeamList[ChrList[cnt].team].leader == character )
            {
                ChrList[cnt].ai.alert |= ALERTIF_LEADERKILLED;
            }
        }

        cnt++;
    }
    if ( TeamList[ChrList[character].team].leader == character )
    {
        TeamList[ChrList[character].team].leader = NOLEADER;
    }

    ChrList[character].on = bfalse;
    ChrList[character].alive = bfalse;
    ChrList[character].inpack = bfalse;
}

//--------------------------------------------------------------------------------------------
void free_inventory( Uint16 character )
{
    // ZZ> This function frees every item in the character's inventory
    int cnt, next;

    cnt = ChrList[character].nextinpack;
    while ( cnt < MAXCHR )
    {
        next = ChrList[cnt].nextinpack;
        free_one_character( cnt );
        cnt = next;
    }
}

//--------------------------------------------------------------------------------------------
void attach_particle_to_character( Uint16 particle, Uint16 character, int grip )
{
    // ZZ> This function sets one particle's position to be attached to a character.
    //     It will kill the particle if the character is no longer around
    Uint16 vertex, model, frame, lastframe;
    Uint8 lip;
    float flip;
    float pointx = 0;
    float pointy = 0;
    float pointz = 0;

    // Check validity of attachment
    if ( !ChrList[character].on || ChrList[character].inpack )
    {
        PrtList[particle].time = 1;
        return;
    }

    // Do we have a matrix???
    if ( ChrList[character].matrixvalid )// mesh.mem.inrenderlist[ChrList[character].onwhichfan])
    {
        // Transform the weapon grip from model to world space
        model = ChrList[character].model;
        frame = ChrList[character].frame;
        lastframe = ChrList[character].lastframe;
        lip = ChrList[character].lip >> 6;
        flip = lip / 4.0f;

        if ( grip == GRIP_ORIGIN )
        {
            PrtList[particle].xpos = ChrList[character].matrix.CNV( 3, 0 );
            PrtList[particle].ypos = ChrList[character].matrix.CNV( 3, 1 );
            PrtList[particle].zpos = ChrList[character].matrix.CNV( 3, 2 );
            return;
        }

        vertex = MadList[model].vertices - grip;

        // Calculate grip point locations with linear interpolation and other silly things
        pointx = Md2FrameList[lastframe].vrtx[vertex] + (Md2FrameList[frame].vrtx[vertex] - Md2FrameList[lastframe].vrtx[vertex]) * flip;
        pointy = Md2FrameList[lastframe].vrty[vertex] + (Md2FrameList[frame].vrty[vertex] - Md2FrameList[lastframe].vrty[vertex]) * flip;
        pointz = Md2FrameList[lastframe].vrtz[vertex] + (Md2FrameList[frame].vrtz[vertex] - Md2FrameList[lastframe].vrtz[vertex]) * flip;

        // Do the transform
        PrtList[particle].xpos = ( pointx * ChrList[character].matrix.CNV( 0, 0 ) +
                                   pointy * ChrList[character].matrix.CNV( 1, 0 ) +
                                   pointz * ChrList[character].matrix.CNV( 2, 0 ) );
        PrtList[particle].ypos = ( pointx * ChrList[character].matrix.CNV( 0, 1 ) +
                                   pointy * ChrList[character].matrix.CNV( 1, 1 ) +
                                   pointz * ChrList[character].matrix.CNV( 2, 1 ) );
        PrtList[particle].zpos = ( pointx * ChrList[character].matrix.CNV( 0, 2 ) +
                                   pointy * ChrList[character].matrix.CNV( 1, 2 ) +
                                   pointz * ChrList[character].matrix.CNV( 2, 2 ) );

        PrtList[particle].xpos += ChrList[character].matrix.CNV( 3, 0 );
        PrtList[particle].ypos += ChrList[character].matrix.CNV( 3, 1 );
        PrtList[particle].zpos += ChrList[character].matrix.CNV( 3, 2 );
    }
    else
    {
        // No matrix, so just wing it...
        PrtList[particle].xpos = ChrList[character].xpos;
        PrtList[particle].ypos = ChrList[character].ypos;
        PrtList[particle].zpos = ChrList[character].zpos;
    }
}

//--------------------------------------------------------------------------------------------
void make_one_weapon_matrix( Uint16 iweap )
{
    // ZZ> This function sets one weapon's matrix, based on who it's attached to
    int    cnt, tnc, vertex;
    Uint16 ichr, ichr_model, ichr_frame, ichr_lastframe;
    Uint8  ichr_lip;
    float  ichr_flip;
    float  pointx[GRIP_VERTS], pointy[GRIP_VERTS], pointz[GRIP_VERTS];
    float  nupointx[GRIP_VERTS], nupointy[GRIP_VERTS], nupointz[GRIP_VERTS];
    int    iweappoints;

    // make sure that we are attached to a valid character
    ichr = ChrList[iweap].attachedto;
    if (ichr >= MAXCHR || !ChrList[ichr].on) return;

    // make sure that the matrix is invalid incase of an error
    ChrList[iweap].matrixvalid = bfalse;

    // Transform the weapon grip from model space to world space
    ichr_model = ChrList[ichr].model;
    ichr_frame = ChrList[ichr].frame;
    ichr_lastframe = ChrList[ichr].lastframe;
    ichr_lip = ChrList[ichr].lip >> 6;
    ichr_flip = ichr_lip / 4.0f;

    iweappoints = 0;
    for (cnt = 0; cnt < GRIP_VERTS; cnt++)
    {
        if (0xFFFF != ChrList[iweap].weapongrip[cnt])
        {
            iweappoints++;
        }
    }
    if (0 == iweappoints)
    {
        // punt! attach to origin
        pointx[0] = ChrList[0].xpos;
        pointy[0] = ChrList[0].ypos;
        pointz[0] = ChrList[0].zpos;
        iweappoints = 1;
    }
    else
    {
        // Calculate grip point locations with linear interpolation and other silly things
        for (cnt = 0; cnt < GRIP_VERTS; cnt++ )
        {
            vertex = ChrList[iweap].weapongrip[cnt];
            if (0xFFFF == vertex) continue;

            // Calculate grip point locations with linear interpolation and other silly things
            pointx[cnt] = Md2FrameList[ichr_lastframe].vrtx[vertex] + (Md2FrameList[ichr_frame].vrtx[vertex] - Md2FrameList[ichr_lastframe].vrtx[vertex]) * ichr_flip;
            pointy[cnt] = Md2FrameList[ichr_lastframe].vrty[vertex] + (Md2FrameList[ichr_frame].vrty[vertex] - Md2FrameList[ichr_lastframe].vrty[vertex]) * ichr_flip;
            pointz[cnt] = Md2FrameList[ichr_lastframe].vrtz[vertex] + (Md2FrameList[ichr_frame].vrtz[vertex] - Md2FrameList[ichr_lastframe].vrtz[vertex]) * ichr_flip;
        }
    }

    for ( tnc = 0; tnc < iweappoints; tnc++ )
    {
        // Do the transform
        nupointx[tnc] = ( pointx[tnc] * ChrList[ichr].matrix.CNV( 0, 0 ) +
                          pointy[tnc] * ChrList[ichr].matrix.CNV( 1, 0 ) +
                          pointz[tnc] * ChrList[ichr].matrix.CNV( 2, 0 ) );
        nupointy[tnc] = ( pointx[tnc] * ChrList[ichr].matrix.CNV( 0, 1 ) +
                          pointy[tnc] * ChrList[ichr].matrix.CNV( 1, 1 ) +
                          pointz[tnc] * ChrList[ichr].matrix.CNV( 2, 1 ) );
        nupointz[tnc] = ( pointx[tnc] * ChrList[ichr].matrix.CNV( 0, 2 ) +
                          pointy[tnc] * ChrList[ichr].matrix.CNV( 1, 2 ) +
                          pointz[tnc] * ChrList[ichr].matrix.CNV( 2, 2 ) );

        nupointx[tnc] += ChrList[ichr].matrix.CNV( 3, 0 );
        nupointy[tnc] += ChrList[ichr].matrix.CNV( 3, 1 );
        nupointz[tnc] += ChrList[ichr].matrix.CNV( 3, 2 );
    }

    if (1 == iweappoints)
    {
        // attach to single point
        ChrList[iweap].matrix = ScaleXYZRotateXYZTranslate(ChrList[iweap].fat, ChrList[iweap].fat, ChrList[iweap].fat,
                                ChrList[iweap].turnleftright >> 2,
                                ( ( Uint16 ) ( ChrList[iweap].turnmapud + 32768 ) ) >> 2,
                                ( ( Uint16 ) ( ChrList[iweap].turnmaplr + 32768 ) ) >> 2,
                                nupointx[0], nupointy[0], nupointz[0]);

        ChrList[iweap].matrixvalid = btrue;
    }
    else if (4 == iweappoints)
    {
        // Calculate weapon's matrix based on positions of grip points
        // chrscale is recomputed at time of attachment
        ChrList[iweap].matrix = FourPoints( nupointx[0], nupointy[0], nupointz[0],
                                            nupointx[1], nupointy[1], nupointz[1],
                                            nupointx[2], nupointy[2], nupointz[2],
                                            nupointx[3], nupointy[3], nupointz[3],
                                            ChrList[iweap].fat );
        ChrList[iweap].matrixvalid = btrue;
    }
}

//--------------------------------------------------------------------------------------------
void make_character_matrices()
{
    // ZZ> This function makes all of the character's matrices
    int cnt, tnc;

    // Forget about old matrices
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        ChrList[cnt].matrixvalid = bfalse;
        cnt++;
    }

    // Do base characters
    tnc = 0;

    while ( tnc < MAXCHR )
    {
        if ( ChrList[tnc].attachedto == MAXCHR && ChrList[tnc].on )  // Skip weapons for now
        {
            make_one_character_matrix( tnc );
        }

        tnc++;
    }

    // Do first level of attachments
    tnc = 0;

    while ( tnc < MAXCHR )
    {
        if ( ChrList[tnc].attachedto != MAXCHR && ChrList[tnc].on )
        {
            if ( ChrList[ChrList[tnc].attachedto].attachedto == MAXCHR )
            {
                make_one_weapon_matrix( tnc );
            }
        }

        tnc++;
    }

    // Do second level of attachments
    tnc = 0;

    while ( tnc < MAXCHR )
    {
        if ( ChrList[tnc].attachedto != MAXCHR && ChrList[tnc].on )
        {
            if ( ChrList[ChrList[tnc].attachedto].attachedto != MAXCHR )
            {
                make_one_weapon_matrix( tnc );
            }
        }

        tnc++;
    }
}

//--------------------------------------------------------------------------------------------
int get_free_character()
{
    // ZZ> This function gets an unused character and returns its index
    int character;
    if ( numfreechr == 0 )
    {
        // Return MAXCHR if we can't find one
        return MAXCHR;
    }
    else
    {
        // Just grab the next one
        numfreechr--;
        character = freechrlist[numfreechr];
    }

    return character;
}

//--------------------------------------------------------------------------------------------
void free_all_characters()
{
    // ZZ> This function resets the character allocation list
    local_noplayers = btrue;
    numfreechr = 0;

    while ( numfreechr < MAXCHR )
    {
        ChrList[numfreechr].on = bfalse;
        ChrList[numfreechr].alive = bfalse;
        ChrList[numfreechr].inpack = bfalse;
        ChrList[numfreechr].numinpack = 0;
        ChrList[numfreechr].nextinpack = MAXCHR;
        ChrList[numfreechr].staton = bfalse;
        ChrList[numfreechr].matrixvalid = bfalse;
        freechrlist[numfreechr] = numfreechr;
        numfreechr++;
    }

    numpla = 0;
    local_numlpla = 0;
    numstat = 0;
}

//--------------------------------------------------------------------------------------------
Uint8 __chrhitawall( Uint16 character )
{
    // ZZ> This function returns nonzero if the character hit a wall that the
    //     character is not allowed to cross

    Uint8 passtl, passtr, passbr, passbl;
    float x, y, bs;
    float fx, fy;
    Uint32 itile;

    y = ChrList[character].ypos;  x = ChrList[character].xpos;  bs = ChrList[character].bumpsize >> 1;

    fx = x - bs; fy = y - bs;
    passtl = MESHFX_IMPASS;
    itile  = mesh_get_tile( fx, fy );
    if ( INVALID_TILE != itile )
    {
        passtl = mesh.mem.tile_list[itile].fx;
    }

    fx = x + bs; fy = y - bs;
    passtr = MESHFX_IMPASS;
    itile  = mesh_get_tile( fx, fy );
    if ( INVALID_TILE != itile )
    {
        passtr = mesh.mem.tile_list[itile].fx;
    }

    fx = x - bs; fy = y + bs;
    passbl = MESHFX_IMPASS;
    itile  = mesh_get_tile( fx, fy );
    if ( INVALID_TILE != itile )
    {
        passbl = mesh.mem.tile_list[itile].fx;
    }

    fx = x + bs; fy = y + bs;
    passbr = MESHFX_IMPASS;
    itile  = mesh_get_tile( fx, fy );
    if ( INVALID_TILE != itile )
    {
        passbr = mesh.mem.tile_list[itile].fx;
    }

    return ( passtl | passtr | passbr | passbl ) & ChrList[character].stoppedby;
}

//--------------------------------------------------------------------------------------------
void reset_character_accel( Uint16 character )
{
    // ZZ> This function fixes a character's max acceleration
    Uint16 enchant;
    if ( character != MAXCHR )
    {
        if ( ChrList[character].on )
        {
            // Okay, remove all acceleration enchants
            enchant = ChrList[character].firstenchant;

            while ( enchant < MAXENCHANT )
            {
                remove_enchant_value( enchant, ADDACCEL );
                enchant = EncList[enchant].nextenchant;
            }

            // Set the starting value
            ChrList[character].maxaccel = CapList[ChrList[character].model].maxaccel[ChrList[character].skin];
            // Put the acceleration enchants back on
            enchant = ChrList[character].firstenchant;

            while ( enchant < MAXENCHANT )
            {
                add_enchant_value( enchant, ADDACCEL, EncList[enchant].eve );
                enchant = EncList[enchant].nextenchant;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void detach_character_from_mount( Uint16 character, Uint8 ignorekurse,
                                  Uint8 doshop )
{
    // ZZ> This function drops an item
    Uint16 mount, hand, enchant, cnt, passage, owner = NOOWNER;
    bool_t inshop;
    int loc;
    float price;

    // Make sure the character is valid
    if ( character == MAXCHR )
        return;

    // Make sure the character is mounted
    mount = ChrList[character].attachedto;
    if ( mount >= MAXCHR )
        return;

    // Make sure both are still around
    if ( !ChrList[character].on || !ChrList[mount].on )
        return;

    // Don't allow living characters to drop kursed weapons
    if ( !ignorekurse && ChrList[character].iskursed && ChrList[mount].alive && ChrList[character].isitem )
    {
        ChrList[character].ai.alert |= ALERTIF_NOTDROPPED;
        return;
    }

    // Figure out which hand it's in
    hand = ChrList[character].inwhichhand;

    // Rip 'em apart
    ChrList[character].attachedto = MAXCHR;
    if ( ChrList[mount].holdingwhich[0] == character )
        ChrList[mount].holdingwhich[0] = MAXCHR;
    if ( ChrList[mount].holdingwhich[1] == character )
        ChrList[mount].holdingwhich[1] = MAXCHR;

    if ( ChrList[character].alive )
    {
        // play the falling animation...
        chr_play_action( character, ACTIONJB + hand, bfalse );
    }
    else if ( ChrList[character].action < ACTIONKA || ChrList[character].action > ACTIONKD )
    {
        // play the "killed" animation...
        chr_play_action( character, ACTIONKA + hand, bfalse );
        ChrList[character].keepaction = btrue;
    }

    // Set the positions
    if ( ChrList[character].matrixvalid )
    {
        ChrList[character].xpos = ChrList[character].matrix.CNV( 3, 0 );
        ChrList[character].ypos = ChrList[character].matrix.CNV( 3, 1 );
        ChrList[character].zpos = ChrList[character].matrix.CNV( 3, 2 );
    }
    else
    {
        ChrList[character].xpos = ChrList[mount].xpos;
        ChrList[character].ypos = ChrList[mount].ypos;
        ChrList[character].zpos = ChrList[mount].zpos;
    }

    // Make sure it's not dropped in a wall...
    if ( __chrhitawall( character ) )
    {
        ChrList[character].xpos = ChrList[mount].xpos;
        ChrList[character].ypos = ChrList[mount].ypos;
    }

    // Check for shop passages
    inshop = bfalse;
    if ( ChrList[character].isitem && numshoppassage != 0 && doshop )
    {
        //This is a hack that makes spellbooks in shops cost correctly
        if (ChrList[mount].isshopitem) ChrList[character].isshopitem = btrue;

        cnt = 0;

        while ( cnt < numshoppassage )
        {
            passage = shoppassage[cnt];
            loc = ChrList[character].xpos;
            loc = loc >> 7;
            if ( loc >= passtlx[passage] && loc <= passbrx[passage] )
            {
                loc = ChrList[character].ypos;
                loc = loc >> 7;
                if ( loc >= passtly[passage] && loc <= passbry[passage] )
                {
                    inshop = btrue;
                    owner = shopowner[cnt];
                    cnt = numshoppassage;  // Finish loop
                    if ( owner == NOOWNER )
                    {
                        // The owner has died!!!
                        inshop = bfalse;
                    }
                }
            }

            cnt++;
        }
        if ( inshop )
        {
            // Give the mount its money back, alert the shop owner
            price = (float) CapList[ChrList[character].model].skincost[0];
            if ( CapList[ChrList[character].model].isstackable )
            {
                price = price * ChrList[character].ammo;
            }

            // Reduce value depending on charges left
            else if (CapList[ChrList[character].model].isranged && ChrList[character].ammo < ChrList[character].ammomax)
            {
                if (ChrList[character].ammo == 0)
                {
                    price /= 2;
                }
                else price -= ((ChrList[character].ammomax - ChrList[character].ammo) * ((float)(price / ChrList[character].ammomax))) / 2;
            }

            //Items spawned within shops are more valuable
            if (!ChrList[character].isshopitem) price *= 0.5;

            ChrList[mount].money += (Sint16) price;
            ChrList[owner].money -= (Sint16) price;
            if ( ChrList[owner].money < 0 )  ChrList[owner].money = 0;
            if ( ChrList[mount].money > MAXMONEY )  ChrList[mount].money = MAXMONEY;

            ChrList[owner].ai.alert |= ALERTIF_ORDERED;
            ChrList[owner].ai.order = (Uint32) price;  // Tell owner how much...
            ChrList[owner].ai.rank  = BUY;  // 0 for buying an item
        }
    }

    // Make sure it works right
    ChrList[character].hitready = btrue;
    if ( inshop )
    {
        // Drop straight down to avoid theft
        ChrList[character].xvel = 0;
        ChrList[character].yvel = 0;
    }
    else
    {
        ChrList[character].xvel = ChrList[mount].xvel;
        ChrList[character].yvel = ChrList[mount].yvel;
    }

    ChrList[character].zvel = DROPZVEL;

    // Turn looping off
    ChrList[character].loopaction = bfalse;

    // Reset the team if it is a mount
    if ( ChrList[mount].ismount )
    {
        ChrList[mount].team = ChrList[mount].baseteam;
        ChrList[mount].ai.alert |= ALERTIF_DROPPED;
    }

    ChrList[character].team = ChrList[character].baseteam;
    ChrList[character].ai.alert |= ALERTIF_DROPPED;

    // Reset transparency
    if ( ChrList[character].isitem && ChrList[mount].transferblend )
    {
        // Okay, reset transparency
        enchant = ChrList[character].firstenchant;

        while ( enchant < MAXENCHANT )
        {
            unset_enchant_value( enchant, SETALPHABLEND );
            unset_enchant_value( enchant, SETLIGHTBLEND );
            enchant = EncList[enchant].nextenchant;
        }

        ChrList[character].alpha = ChrList[character].basealpha;
        ChrList[character].light = CapList[ChrList[character].model].light;
        enchant = ChrList[character].firstenchant;

        while ( enchant < MAXENCHANT )
        {
            set_enchant_value( enchant, SETALPHABLEND, EncList[enchant].eve );
            set_enchant_value( enchant, SETLIGHTBLEND, EncList[enchant].eve );
            enchant = EncList[enchant].nextenchant;
        }
    }

    // Set twist
    ChrList[character].turnmaplr = 32768;
    ChrList[character].turnmapud = 32768;
}
//--------------------------------------------------------------------------------------------
void reset_character_alpha( Uint16 character )
{
    // ZZ> This function fixes an item's transparency
    Uint16 enchant, mount;
    if ( character != MAXCHR )
    {
        mount = ChrList[character].attachedto;
        if ( ChrList[character].on && mount != MAXCHR && ChrList[character].isitem && ChrList[mount].transferblend )
        {
            // Okay, reset transparency
            enchant = ChrList[character].firstenchant;

            while ( enchant < MAXENCHANT )
            {
                unset_enchant_value( enchant, SETALPHABLEND );
                unset_enchant_value( enchant, SETLIGHTBLEND );
                enchant = EncList[enchant].nextenchant;
            }

            ChrList[character].alpha = ChrList[character].basealpha;
            ChrList[character].light = CapList[ChrList[character].model].light;
            enchant = ChrList[character].firstenchant;

            while ( enchant < MAXENCHANT )
            {
                set_enchant_value( enchant, SETALPHABLEND, EncList[enchant].eve );
                set_enchant_value( enchant, SETLIGHTBLEND, EncList[enchant].eve );
                enchant = EncList[enchant].nextenchant;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void attach_character_to_mount( Uint16 character, Uint16 mount, Uint16 grip )
{
    // ZZ> This function attaches one character to another ( the mount )
    //     at either the left or right grip
    int i, tnc, slot;

    // Make sure both are still around
    if ( !ChrList[character].on || !ChrList[mount].on || ChrList[character].inpack || ChrList[mount].inpack )
        return;

    // Figure out which slot this grip relates to
    if ( grip == GRIP_LEFT )
    {
        slot = SLOT_LEFT;
    }
    else
    {
        slot = SLOT_RIGHT;
    }

    // Make sure the the slot is valid
    if ( !CapList[ChrList[mount].model].gripvalid[slot] )
        return;

    // Put 'em together
    ChrList[character].inwhichhand    = slot;
    ChrList[character].attachedto     = mount;
    ChrList[mount].holdingwhich[slot] = character;

    tnc = MadList[ChrList[mount].model].vertices - grip;

    for (i = 0; i < GRIP_VERTS; i++)
    {
        if (tnc + i < MadList[ChrList[mount].model].vertices )
        {
            ChrList[character].weapongrip[i] = i + tnc;
        }
        else
        {
            ChrList[character].weapongrip[i] = 0xFFFF;
        }
    }

    // catually make position of the object coincide with its actual held position
    make_one_weapon_matrix( character );

    ChrList[character].xpos = ChrList[character].matrix.CNV( 3, 0 );
    ChrList[character].ypos = ChrList[character].matrix.CNV( 3, 1 );
    ChrList[character].zpos = ChrList[character].matrix.CNV( 3, 2 );

    ChrList[character].inwater = bfalse;
    ChrList[character].jumptime = JUMPDELAY * 4;

    // Run the held animation
    if ( ChrList[mount].ismount && grip == GRIP_ONLY )
    {
        // Riding mount
        chr_play_action( character, ACTIONMI, btrue );
        ChrList[character].loopaction = btrue;
    }
    else if ( ChrList[character].alive )
    {
        chr_play_action( character, ACTIONMM + slot, bfalse );
        if ( ChrList[character].isitem )
        {
            // Item grab
            ChrList[character].keepaction = btrue;
        }
    }

    // Set the team
    if ( ChrList[character].isitem )
    {
        ChrList[character].team = ChrList[mount].team;

        // Set the alert
        if ( ChrList[character].alive )
        {
            ChrList[character].ai.alert |= ALERTIF_GRABBED;
        }
    }

    if ( ChrList[mount].ismount )
    {
        ChrList[mount].team = ChrList[character].team;

        // Set the alert
        if ( !ChrList[mount].isitem && ChrList[mount].alive )
        {
            ChrList[mount].ai.alert |= ALERTIF_GRABBED;
        }
    }

    // It's not gonna hit the floor
    ChrList[character].hitready = bfalse;
}

//--------------------------------------------------------------------------------------------
static Uint16 pack_has_a_stack( Uint16 item, Uint16 character )
{
    // ZZ> This function looks in the character's pack for an item similar
    //     to the one given.  If it finds one, it returns the similar item's
    //     index number, otherwise it returns MAXCHR.
    Uint16 inpack, id;
    bool_t allok;
    if ( CapList[ChrList[item].model].isstackable )
    {
        inpack = ChrList[character].nextinpack;
        allok = bfalse;

        while ( inpack != MAXCHR && !allok )
        {
            allok = btrue;
            if ( ChrList[inpack].model != ChrList[item].model )
            {
                if ( !CapList[ChrList[inpack].model].isstackable )
                {
                    allok = bfalse;
                }
                if ( ChrList[inpack].ammomax != ChrList[item].ammomax )
                {
                    allok = bfalse;
                }

                id = 0;

                while ( id < IDSZ_COUNT && allok )
                {
                    if ( CapList[ChrList[inpack].model].idsz[id] != CapList[ChrList[item].model].idsz[id] )
                    {
                        allok = bfalse;
                    }

                    id++;
                }
            }
            if ( !allok )
            {
                inpack = ChrList[inpack].nextinpack;
            }
        }
        if ( allok )
        {
            return inpack;
        }
    }

    return MAXCHR;
}

//--------------------------------------------------------------------------------------------
void pack_add_item( Uint16 item, Uint16 character )
{
    // ZZ> This function puts one character inside the other's pack
    Uint16 oldfirstitem, newammo, stack;

    // Make sure everything is hunkydori
    if ( ( !ChrList[item].on ) || ( !ChrList[character].on ) || ChrList[item].inpack || ChrList[character].inpack ||
            ChrList[character].isitem )
        return;

    stack = pack_has_a_stack( item, character );
    if ( stack != MAXCHR )
    {
        // We found a similar, stackable item in the pack
        if ( ChrList[item].nameknown || ChrList[stack].nameknown )
        {
            ChrList[item].nameknown = btrue;
            ChrList[stack].nameknown = btrue;
        }
        if ( CapList[ChrList[item].model].usageknown || CapList[ChrList[stack].model].usageknown )
        {
            CapList[ChrList[item].model].usageknown = btrue;
            CapList[ChrList[stack].model].usageknown = btrue;
        }

        newammo = ChrList[item].ammo + ChrList[stack].ammo;
        if ( newammo <= ChrList[stack].ammomax )
        {
            // All transfered, so kill the in hand item
            ChrList[stack].ammo = newammo;
            if ( ChrList[item].attachedto != MAXCHR )
            {
                detach_character_from_mount( item, btrue, bfalse );
            }

            free_one_character( item );
        }
        else
        {
            // Only some were transfered,
            ChrList[item].ammo = ChrList[item].ammo + ChrList[stack].ammo - ChrList[stack].ammomax;
            ChrList[stack].ammo = ChrList[stack].ammomax;
            ChrList[character].ai.alert |= ALERTIF_TOOMUCHBAGGAGE;
        }
    }
    else
    {
        // Make sure we have room for another item
        if ( ChrList[character].numinpack >= MAXNUMINPACK )
        {
            ChrList[character].ai.alert |= ALERTIF_TOOMUCHBAGGAGE;
            return;
        }

        // Take the item out of hand
        if ( ChrList[item].attachedto != MAXCHR )
        {
            detach_character_from_mount( item, btrue, bfalse );
            ChrList[item].ai.alert &= ( ~ALERTIF_DROPPED );
        }

        // Remove the item from play
        ChrList[item].hitready = bfalse;
        ChrList[item].inpack = btrue;

        // Insert the item into the pack as the first one
        oldfirstitem = ChrList[character].nextinpack;
        ChrList[character].nextinpack = item;
        ChrList[item].nextinpack = oldfirstitem;
        ChrList[character].numinpack++;
        if ( CapList[ChrList[item].model].isequipment )
        {
            // AtLastWaypoint doubles as PutAway
            ChrList[item].ai.alert |= ALERTIF_ATLASTWAYPOINT;
        }
    }

    return;
}

//--------------------------------------------------------------------------------------------
Uint16 pack_get_item( Uint16 character, Uint16 grip, Uint8 ignorekurse )
{
    // ZZ> This function takes the last item in the character's pack and puts
    //     it into the designated hand.  It returns the item number or MAXCHR.
    Uint16 item, nexttolastitem;

    // Make sure everything is hunkydori
    if ( ( !ChrList[character].on ) || ChrList[character].inpack || ChrList[character].isitem || ChrList[character].nextinpack == MAXCHR )
        return MAXCHR;
    if ( ChrList[character].numinpack == 0 )
        return MAXCHR;

    // Find the last item in the pack
    nexttolastitem = character;
    item = ChrList[character].nextinpack;

    while ( ChrList[item].nextinpack != MAXCHR )
    {
        nexttolastitem = item;
        item = ChrList[item].nextinpack;
    }

    // Figure out what to do with it
    if ( ChrList[item].iskursed && ChrList[item].isequipped && !ignorekurse )
    {
        // Flag the last item as not removed
        ChrList[item].ai.alert |= ALERTIF_NOTPUTAWAY;  // Doubles as IfNotTakenOut
        // Cycle it to the front
        ChrList[item].nextinpack = ChrList[character].nextinpack;
        ChrList[nexttolastitem].nextinpack = MAXCHR;
        ChrList[character].nextinpack = item;
        if ( character == nexttolastitem )
        {
            ChrList[item].nextinpack = MAXCHR;
        }

        return MAXCHR;
    }
    else
    {
        // Remove the last item from the pack
        ChrList[item].inpack = bfalse;
        ChrList[item].isequipped = bfalse;
        ChrList[nexttolastitem].nextinpack = MAXCHR;
        ChrList[character].numinpack--;
        ChrList[item].team = ChrList[character].team;

        // Attach the item to the character's hand
        attach_character_to_mount( item, character, grip );
        ChrList[item].ai.alert &= ( ~ALERTIF_GRABBED );
        ChrList[item].ai.alert |= ( ALERTIF_TAKENOUT );
    }

    return item;
}

//--------------------------------------------------------------------------------------------
void drop_keys( Uint16 character )
{
    // ZZ> This function drops all keys ( [KEYA] to [KEYZ] ) that are in a character's
    //     inventory ( Not hands ).
    Uint16 item, lastitem, nextitem, direction, cosdir;
    IDSZ testa, testz;
    if ( character < MAXCHR )
    {
        if ( ChrList[character].on )
        {
            if ( ChrList[character].zpos > -2 ) // Don't lose keys in pits...
            {
                // The IDSZs to find
                testa = Make_IDSZ( "KEYA" );  // [KEYA]
                testz = Make_IDSZ( "KEYZ" );  // [KEYZ]

                lastitem = character;
                item = ChrList[character].nextinpack;

                while ( item != MAXCHR )
                {
                    nextitem = ChrList[item].nextinpack;
                    if ( item != character )  // Should never happen...
                    {
                        if ( ( CapList[ChrList[item].model].idsz[IDSZ_PARENT] >= testa &&
                                CapList[ChrList[item].model].idsz[IDSZ_PARENT] <= testz ) ||
                                ( CapList[ChrList[item].model].idsz[IDSZ_TYPE] >= testa &&
                                  CapList[ChrList[item].model].idsz[IDSZ_TYPE] <= testz ) )
                        {
                            // We found a key...
                            ChrList[item].inpack = bfalse;
                            ChrList[item].isequipped = bfalse;
                            ChrList[lastitem].nextinpack = nextitem;
                            ChrList[item].nextinpack = MAXCHR;
                            ChrList[character].numinpack--;
                            ChrList[item].attachedto = MAXCHR;
                            ChrList[item].ai.alert |= ALERTIF_DROPPED;
                            ChrList[item].hitready = btrue;

                            direction = RANDIE;
                            ChrList[item].turnleftright = direction + 32768;
                            cosdir = direction + 16384;
                            ChrList[item].level = ChrList[character].level;
                            ChrList[item].xpos = ChrList[character].xpos;
                            ChrList[item].ypos = ChrList[character].ypos;
                            ChrList[item].zpos = ChrList[character].zpos;
                            ChrList[item].xvel = turntocos[direction>>2] * DROPXYVEL;
                            ChrList[item].yvel = turntosin[direction>>2] * DROPXYVEL;
                            ChrList[item].zvel = DROPZVEL;
                            ChrList[item].team = ChrList[item].baseteam;
                        }
                        else
                        {
                            lastitem = item;
                        }
                    }

                    item = nextitem;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void drop_all_items( Uint16 character )
{
    // ZZ> This function drops all of a character's items
    Uint16 item, direction, diradd;
    if ( character < MAXCHR )
    {
        if ( ChrList[character].on )
        {
            detach_character_from_mount( ChrList[character].holdingwhich[0], btrue, bfalse );
            detach_character_from_mount( ChrList[character].holdingwhich[1], btrue, bfalse );
            if ( ChrList[character].numinpack > 0 )
            {
                direction = ChrList[character].turnleftright + 32768;
                diradd = 0xFFFF / ChrList[character].numinpack;

                while ( ChrList[character].numinpack > 0 )
                {
                    item = pack_get_item( character, GRIP_LEFT, bfalse );
                    if ( item < MAXCHR )
                    {
                        detach_character_from_mount( item, btrue, btrue );
                        ChrList[item].hitready = btrue;
                        ChrList[item].ai.alert |= ALERTIF_DROPPED;
                        ChrList[item].xpos = ChrList[character].xpos;
                        ChrList[item].ypos = ChrList[character].ypos;
                        ChrList[item].zpos = ChrList[character].zpos;
                        ChrList[item].level = ChrList[character].level;
                        ChrList[item].turnleftright = direction + 32768;
                        ChrList[item].xvel = turntocos[direction>>2] * DROPXYVEL;
                        ChrList[item].yvel = turntosin[direction>>2] * DROPXYVEL;
                        ChrList[item].zvel = DROPZVEL;
                        ChrList[item].team = ChrList[item].baseteam;
                    }

                    direction += diradd;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t character_grab_stuff( Uint16 chara, int grip, Uint8 people )
{
    // ZZ> This function makes the character pick up an item if there's one around
    float xa, ya, za, xb, yb, zb, dist;
    int charb, slot;
    Uint16 vertex, model, frame, passage, cnt, owner = NOOWNER;
    float pointx, pointy, pointz;
    bool_t inshop;
    int loc;
    float price;

    // Make life easier
    model = ChrList[chara].model;
    slot = ( grip / GRIP_VERTS ) - 1;  // 0 is left, 1 is right

    // Make sure the character doesn't have something already, and that it has hands
    if ( ChrList[chara].holdingwhich[slot] != MAXCHR || !CapList[model].gripvalid[slot] )
        return bfalse;

    // Do we have a matrix???
    if ( ChrList[chara].matrixvalid )// mesh.mem.inrenderlist[ChrList[chara].onwhichfan])
    {
        // Transform the weapon grip from model to world space
        frame = ChrList[chara].frame;
        vertex = MadList[model].vertices - grip;

        // Calculate grip point locations
        pointx = Md2FrameList[frame].vrtx[vertex];/// ChrList[cnt].scale;
        pointy = Md2FrameList[frame].vrty[vertex];/// ChrList[cnt].scale;
        pointz = Md2FrameList[frame].vrtz[vertex];/// ChrList[cnt].scale;

        // Do the transform
        xa = ( pointx * ChrList[chara].matrix.CNV( 0, 0 ) +
               pointy * ChrList[chara].matrix.CNV( 1, 0 ) +
               pointz * ChrList[chara].matrix.CNV( 2, 0 ) );
        ya = ( pointx * ChrList[chara].matrix.CNV( 0, 1 ) +
               pointy * ChrList[chara].matrix.CNV( 1, 1 ) +
               pointz * ChrList[chara].matrix.CNV( 2, 1 ) );
        za = ( pointx * ChrList[chara].matrix.CNV( 0, 2 ) +
               pointy * ChrList[chara].matrix.CNV( 1, 2 ) +
               pointz * ChrList[chara].matrix.CNV( 2, 2 ) );
        xa += ChrList[chara].matrix.CNV( 3, 0 );
        ya += ChrList[chara].matrix.CNV( 3, 1 );
        za += ChrList[chara].matrix.CNV( 3, 2 );
    }
    else
    {
        // Just wing it
        xa = ChrList[chara].xpos;
        ya = ChrList[chara].ypos;
        za = ChrList[chara].zpos;
    }

    // Go through all characters to find the best match
    for ( charb = 0; charb < MAXCHR; charb++ )
    {
        if ( !ChrList[charb].on ) continue;

        if ( ChrList[charb].inpack ) continue;              // pickpocket not allowed yet
        if ( MAXCHR != ChrList[charb].attachedto) continue; // disarm not allowed yet

        if ( ChrList[charb].weight > ChrList[chara].weight + ChrList[chara].strength ) continue; // reasonable carrying capacity

        // people == btrue allows you to pick up living non-items
        // people == false allows you to pick up living (functioning) items
        if ( ChrList[charb].alive && (people == ChrList[charb].isitem) ) continue;

        // do not pick up your mount
        if ( ChrList[charb].holdingwhich[0] == chara || ChrList[charb].holdingwhich[1] == chara ) continue;

        xb = ChrList[charb].xpos;
        yb = ChrList[charb].ypos;
        zb = ChrList[charb].zpos;

        // First check absolute value diamond
        xb = ABS( xa - xb );
        yb = ABS( ya - yb );
        zb = ABS( za - zb );
        dist = xb + yb;

        if ( dist < GRABSIZE && zb < GRABSIZE )
        {
            // Check for shop
            inshop = bfalse;
            if ( ChrList[charb].isitem && numshoppassage > 0 )
            {
                for ( cnt = 0; cnt < numshoppassage; cnt++ )
                {
                    passage = shoppassage[cnt];

                    loc = ChrList[charb].xpos;
                    loc = loc >> 7;
                    if ( loc >= passtlx[passage] && loc <= passbrx[passage] )
                    {
                        loc = ChrList[charb].ypos;
                        loc = loc >> 7;
                        if ( loc >= passtly[passage] && loc <= passbry[passage] )
                        {
                            // if there is NOOWNER, someone has been murdered!
                            owner = shopowner[cnt];
                            inshop = (owner != NOOWNER);
                            break;
                        }
                    }
                }

                if ( inshop )
                {
                    // Pay the shop owner, or don't allow grab...
                    if ( ChrList[chara].isitem || ( ChrList[chara].alpha < INVISIBLE) )
                    {
                        // Pets can try to steal in addition to invisible characters
                        STRING text;
                        inshop = bfalse;
                        snprintf( text, sizeof(text), "%s stole something! (%s)", ChrList[chara].name, CapList[ChrList[charb].model].classname );
                        debug_message( text );

                        // Check if it was detected. 50% chance +2% per pet DEX and -2% per shopkeeper wisdom
                        if (ChrList[owner].canseeinvisible || generate_number( 1, 100 ) - ( ChrList[chara].dexterity >> 7 ) + ( ChrList[owner].wisdom >> 7 ) > 50 )
                        {
                            snprintf( text, sizeof(text), "%s was detected!!", ChrList[chara].name );
                            debug_message( text );
                            ChrList[owner].ai.alert |= ALERTIF_ORDERED;
                            ChrList[owner].ai.order = STOLEN;
                            ChrList[owner].ai.rank  = 3;
                        }
                    }
                    else
                    {
                        ChrList[owner].ai.alert |= ALERTIF_ORDERED;
                        price = (float) CapList[ChrList[charb].model].skincost[0];
                        if ( CapList[ChrList[charb].model].isstackable )
                        {
                            price = price * ChrList[charb].ammo;
                        }

                        // Reduce value depending on charges left
                        else if (CapList[ChrList[charb].model].isranged && ChrList[charb].ammo < ChrList[charb].ammomax)
                        {
                            if (ChrList[charb].ammo == 0) price /= 2;
                            else price -= ((ChrList[charb].ammomax - ChrList[charb].ammo) * ((float)(price / ChrList[charb].ammomax))) / 2;
                        }

                        //Items spawned in shops are more valuable
                        if (!ChrList[charb].isshopitem) price *= 0.5;

                        ChrList[owner].ai.order = (Uint32) price;  // Tell owner how much...
                        if ( ChrList[chara].money >= price )
                        {
                            // Okay to buy
                            ChrList[owner].ai.rank = SELL;  // 1 for selling an item
                            ChrList[chara].money  -= (Sint16) price;  // Skin 0 cost is price
                            ChrList[owner].money  += (Sint16) price;
                            if ( ChrList[owner].money > MAXMONEY )  ChrList[owner].money = MAXMONEY;

                            inshop = bfalse;
                        }
                        else
                        {
                            // Don't allow purchase
                            ChrList[owner].ai.rank = 2;  // 2 for "you can't afford that"
                            inshop = btrue;
                        }
                    }
                }
            }

            if ( !inshop )
            {
                // Stick 'em together and quit
                attach_character_to_mount( charb, chara, grip );
                if ( people )
                {
                    // Do a slam animation...  ( Be sure to drop!!! )
                    chr_play_action( chara, ACTIONMC + slot, bfalse );
                }
                return btrue;
            }
            else
            {
                // Lift the item a little and quit...
                ChrList[charb].zvel = DROPZVEL;
                ChrList[charb].hitready = btrue;
                ChrList[charb].ai.alert |= ALERTIF_DROPPED;
                break;
            }
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void character_swipe( Uint16 cnt, Uint8 slot )
{
    // ZZ> This function spawns an attack particle
    int weapon, particle, spawngrip, thrown;
    Uint8 action;
    Uint16 tTmp;
    float dampen;
    float x, y, z, velocity;

    weapon = ChrList[cnt].holdingwhich[slot];
    spawngrip = GRIP_LAST;
    action = ChrList[cnt].action;

    // See if it's an unarmed attack...
    if ( weapon == MAXCHR )
    {
        weapon = cnt;
        spawngrip = (slot + 1) * GRIP_VERTS;  // 0 -> GRIP_LEFT, 1 -> GRIP_RIGHT
    }
    if ( weapon != cnt && ( ( CapList[ChrList[weapon].model].isstackable && ChrList[weapon].ammo > 1 ) || ( action >= ACTIONFA && action <= ACTIONFD ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation
        x = ChrList[cnt].xpos;
        y = ChrList[cnt].ypos;
        z = ChrList[cnt].zpos;
        thrown = spawn_one_character( x, y, z, ChrList[weapon].model, ChrList[cnt].team, 0, ChrList[cnt].turnleftright, ChrList[weapon].name, MAXCHR );
        if ( thrown < MAXCHR )
        {
            ChrList[thrown].iskursed = bfalse;
            ChrList[thrown].ammo = 1;
            ChrList[thrown].ai.alert |= ALERTIF_THROWN;
            velocity = ChrList[cnt].strength / ( ChrList[thrown].weight * THROWFIX );
            velocity += MINTHROWVELOCITY;
            if ( velocity > MAXTHROWVELOCITY )
            {
                velocity = MAXTHROWVELOCITY;
            }

            tTmp = ChrList[cnt].turnleftright >> 2;
            ChrList[thrown].xvel += turntocos[( tTmp+8192 )&TRIG_TABLE_MASK] * velocity;
            ChrList[thrown].yvel += turntosin[( tTmp+8192 )&TRIG_TABLE_MASK] * velocity;
            ChrList[thrown].zvel = DROPZVEL;
            if ( ChrList[weapon].ammo <= 1 )
            {
                // Poof the item
                detach_character_from_mount( weapon, btrue, bfalse );
                free_one_character( weapon );
            }
            else
            {
                ChrList[weapon].ammo--;
            }
        }
    }
    else
    {
        // Spawn an attack particle
        if ( ChrList[weapon].ammomax == 0 || ChrList[weapon].ammo != 0 )
        {
            if ( ChrList[weapon].ammo > 0 && !CapList[ChrList[weapon].model].isstackable )
            {
                ChrList[weapon].ammo--;  // Ammo usage
            }

            // HERE
            if ( CapList[ChrList[weapon].model].attackprttype != -1 )
            {
                particle = spawn_one_particle( ChrList[weapon].xpos, ChrList[weapon].ypos, ChrList[weapon].zpos, ChrList[cnt].turnleftright, ChrList[weapon].model, CapList[ChrList[weapon].model].attackprttype, weapon, spawngrip, ChrList[cnt].team, cnt, 0, MAXCHR );
                if ( particle != TOTALMAXPRT )
                {
                    if ( !CapList[ChrList[weapon].model].attackattached )
                    {
                        // Detach the particle
                        if ( !PipList[PrtList[particle].pip].startontarget || PrtList[particle].target == MAXCHR )
                        {
                            attach_particle_to_character( particle, weapon, spawngrip );
                            // Correct Z spacing base, but nothing else...
                            PrtList[particle].zpos += PipList[PrtList[particle].pip].zspacingbase;
                        }

                        PrtList[particle].attachedtocharacter = MAXCHR;

                        // Don't spawn in walls
                        if ( __prthitawall( particle ) )
                        {
                            PrtList[particle].xpos = ChrList[weapon].xpos;
                            PrtList[particle].ypos = ChrList[weapon].ypos;
                            if ( __prthitawall( particle ) )
                            {
                                PrtList[particle].xpos = ChrList[cnt].xpos;
                                PrtList[particle].ypos = ChrList[cnt].ypos;
                            }
                        }
                    }
                    else
                    {
                        // Attached particles get a strength bonus for reeling...
                        dampen = REELBASE + ( ChrList[cnt].strength / REEL );
                        PrtList[particle].xvel = PrtList[particle].xvel * dampen;
                        PrtList[particle].yvel = PrtList[particle].yvel * dampen;
                        PrtList[particle].zvel = PrtList[particle].zvel * dampen;
                    }

                    // Initial particles get a strength bonus, which may be 0.00f
                    PrtList[particle].damagebase += ( ChrList[cnt].strength * CapList[ChrList[weapon].model].strengthdampen );
                    // Initial particles get an enchantment bonus
                    PrtList[particle].damagebase += ChrList[weapon].damageboost;
                    // Initial particles inherit damage type of weapon
                    PrtList[particle].damagetype = ChrList[weapon].damagetargettype;
                }
            }
        }
        else
        {
            ChrList[weapon].ammoknown = btrue;
        }
    }
}

//--------------------------------------------------------------------------------------------
void drop_money( Uint16 character, Uint16 money )
{
    // ZZ> This function drops some of a character's money
    Uint16 huns, tfives, fives, ones, cnt;
    if ( money > ChrList[character].money )  money = ChrList[character].money;
    if ( money > 0 && ChrList[character].zpos > -2 )
    {
        ChrList[character].money = ChrList[character].money - money;
        huns = money / 100;  money -= ( huns << 7 ) - ( huns << 5 ) + ( huns << 2 );
        tfives = money / 25;  money -= ( tfives << 5 ) - ( tfives << 3 ) + tfives;
        fives = money / 5;  money -= ( fives << 2 ) + fives;
        ones = money;

        for ( cnt = 0; cnt < ones; cnt++ )
        {
            spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos,  ChrList[character].zpos, 0, MAXMODEL, COIN1, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        for ( cnt = 0; cnt < fives; cnt++ )
        {
            spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos,  ChrList[character].zpos, 0, MAXMODEL, COIN5, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        for ( cnt = 0; cnt < tfives; cnt++ )
        {
            spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos,  ChrList[character].zpos, 0, MAXMODEL, COIN25, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        for ( cnt = 0; cnt < huns; cnt++ )
        {
            spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos,  ChrList[character].zpos, 0, MAXMODEL, COIN100, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        ChrList[character].damagetime = DAMAGETIME;  // So it doesn't grab it again
    }
}

//--------------------------------------------------------------------------------------------
void call_for_help( Uint16 character )
{
    // ZZ> This function issues a call for help to all allies
    Uint8 team;
    Uint16 cnt;

    team = ChrList[character].team;
    TeamList[team].sissy = character;

    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( ChrList[cnt].on && cnt != character && !TeamList[ChrList[cnt].team].hatesteam[team] )
        {
            ChrList[cnt].ai.alert |= ALERTIF_CALLEDFORHELP;
        }
    }
}

//--------------------------------------------------------------------------------------------
Uint32 xp_for_next_level(Uint16 character)
{
    Uint32 curlevel;
    Uint16 profile;
    Uint32 xpneeded = (Uint32)(~0);
    if ( !ChrList[character].on ) return xpneeded;

    profile  = ChrList[character].model;
    if (profile == MAXMODEL) return xpneeded;

    // Do level ups and stat changes
    curlevel = ChrList[character].experiencelevel;
    if ( curlevel + 1 < MAXLEVEL )
    {
        xpneeded = CapList[profile].experienceforlevel[curlevel+1];
    }
    else
    {
        xpneeded = CapList[profile].experienceforlevel[MAXLEVEL - 1];
        xpneeded += ( ( curlevel + 1 ) * ( curlevel + 1 ) * ( curlevel + 1 ) * 15 );
        xpneeded -= ( ( MAXLEVEL - 1 ) * ( MAXLEVEL - 1 ) * ( MAXLEVEL - 1 ) * 15 );
    }

    return xpneeded;
}

//--------------------------------------------------------------------------------------------
void do_level_up( Uint16 character )
{
    // BB > level gains are done here, but only once a second

    Uint8 curlevel;
    int number;
    Uint16 profile;
    STRING text;
    if (character >= MAXCHR || !ChrList[character].on) return;

    profile = ChrList[character].model;
    if ( profile >= MAXMODEL ) return;

    // Do level ups and stat changes
    curlevel = ChrList[character].experiencelevel;
    if ( curlevel + 1 < 20 )
    {
        Uint32 xpcurrent, xpneeded;

        xpcurrent = ChrList[character].experience;
        xpneeded  = xp_for_next_level(character);
        if ( xpcurrent >= xpneeded )
        {
            // do the level up
            ChrList[character].experiencelevel++;
            xpneeded  = xp_for_next_level(character);

            // The character is ready to advance...
            if ( ChrList[character].isplayer )
            {
                snprintf( text, sizeof(text), "%s gained a level!!!", ChrList[character].name );
                debug_message( text );
                sound_play_chunk( gCamera.trackx, gCamera.tracky, g_wavelist[GSND_LEVELUP] );
            }

            // Size
            ChrList[character].sizegoto += CapList[profile].sizeperlevel * 0.5f;  // Limit this?
            ChrList[character].sizegototime += SIZETIME;

            // Strength
            number = generate_number( CapList[profile].strengthperlevelbase, CapList[profile].strengthperlevelrand );
            number = number + ChrList[character].strength;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList[character].strength = number;

            // Wisdom
            number = generate_number( CapList[profile].wisdomperlevelbase, CapList[profile].wisdomperlevelrand );
            number = number + ChrList[character].wisdom;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList[character].wisdom = number;

            // Intelligence
            number = generate_number( CapList[profile].intelligenceperlevelbase, CapList[profile].intelligenceperlevelrand );
            number = number + ChrList[character].intelligence;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList[character].intelligence = number;

            // Dexterity
            number = generate_number( CapList[profile].dexterityperlevelbase, CapList[profile].dexterityperlevelrand );
            number = number + ChrList[character].dexterity;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList[character].dexterity = number;

            // Life
            number = generate_number( CapList[profile].lifeperlevelbase, CapList[profile].lifeperlevelrand );
            number = number + ChrList[character].lifemax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            ChrList[character].life += ( number - ChrList[character].lifemax );
            ChrList[character].lifemax = number;

            // Mana
            number = generate_number( CapList[profile].manaperlevelbase, CapList[profile].manaperlevelrand );
            number = number + ChrList[character].manamax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            ChrList[character].mana += ( number - ChrList[character].manamax );
            ChrList[character].manamax = number;

            // Mana Return
            number = generate_number( CapList[profile].manareturnperlevelbase, CapList[profile].manareturnperlevelrand );
            number = number + ChrList[character].manareturn;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList[character].manareturn = number;

            // Mana Flow
            number = generate_number( CapList[profile].manaflowperlevelbase, CapList[profile].manaflowperlevelrand );
            number = number + ChrList[character].manaflow;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList[character].manaflow = number;
        }
    }
}

//--------------------------------------------------------------------------------------------
void give_experience( Uint16 character, int amount, Uint8 xptype, bool_t override_invictus )
{
    // ZZ> This function gives a character experience

    int newamount;
    int profile;
    if (amount == 0) return;
    if ( !ChrList[character].invictus || override_invictus )
    {
        // Figure out how much experience to give
        profile = ChrList[character].model;
        newamount = amount;
        if ( xptype < XP_COUNT )
        {
            newamount = amount * CapList[profile].experiencerate[xptype];
        }

        //Intelligence and slightly wisdom increases xp gained (0,5% per int and 0,25% per wisdom above 10)
        newamount = newamount * (1 + ((float)FP8_TO_INT(ChrList[character].intelligence - 2560) / 200))
                    + (1 + ((float)FP8_TO_INT(ChrList[character].wisdom - 2560) / 400));

        ChrList[character].experience += newamount;
    }
}

//--------------------------------------------------------------------------------------------
void give_team_experience( Uint8 team, int amount, Uint8 xptype )
{
    // ZZ> This function gives a character experience, and pawns off level gains to
    //     another function
    Uint16 cnt;

    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( ChrList[cnt].team == team && ChrList[cnt].on )
        {
            give_experience( cnt, amount, xptype, bfalse );
        }
    }
}

//--------------------------------------------------------------------------------------------
void resize_characters()
{
    // ZZ> This function makes the characters get bigger or smaller, depending
    //     on their sizegoto and sizegototime
    int cnt = 0;
    bool_t willgetcaught;
    float newsize;

    while ( cnt < MAXCHR )
    {
        if ( ChrList[cnt].on && ChrList[cnt].sizegototime && ChrList[cnt].sizegoto != ChrList[cnt].fat )
        {
            int bump_increase;

            bump_increase = ( ChrList[cnt].sizegoto - ChrList[cnt].fat ) * 0.10f * ChrList[cnt].bumpsize;

            // Make sure it won't get caught in a wall
            willgetcaught = bfalse;
            if ( ChrList[cnt].sizegoto > ChrList[cnt].fat )
            {
                ChrList[cnt].bumpsize += bump_increase;

                if ( __chrhitawall( cnt ) )
                {
                    willgetcaught = btrue;
                }

                ChrList[cnt].bumpsize -= bump_increase;
            }

            // If it is getting caught, simply halt growth until later
            if ( !willgetcaught )
            {
                // Figure out how big it is
                ChrList[cnt].sizegototime--;

                newsize = ChrList[cnt].sizegoto;
                if ( ChrList[cnt].sizegototime > 0 )
                {
                    newsize = ( ChrList[cnt].fat * 0.90f ) + ( newsize * 0.10f );
                }

                // Make it that big...
                ChrList[cnt].fat   = newsize;
                ChrList[cnt].shadowsize = ChrList[cnt].shadowsizesave * newsize;
                ChrList[cnt].bumpsize = ChrList[cnt].bumpsizesave * newsize;
                ChrList[cnt].bumpsizebig = ChrList[cnt].bumpsizebigsave * newsize;
                ChrList[cnt].bumpheight = ChrList[cnt].bumpheightsave * newsize;

                if ( CapList[ChrList[cnt].model].weight == 0xFF )
                {
                    ChrList[cnt].weight = 0xFFFF;
                }
                else
                {
                    int itmp = CapList[ChrList[cnt].model].weight * ChrList[cnt].fat * ChrList[cnt].fat * ChrList[cnt].fat;
                    ChrList[cnt].weight = MIN( itmp, 0xFFFE );
                }
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void export_one_character_name( const char *szSaveName, Uint16 character )
{
    // ZZ> This function makes the naming.txt file for the character
    FILE* filewrite;
    int profile;
    char cTmp;
    int cnt, tnc;

    // Can it export?
    profile = ChrList[character].model;
    filewrite = fopen( szSaveName, "w" );
    if ( filewrite )
    {
        cnt = 0;
        cTmp = ChrList[character].name[0];
        cnt++;

        while ( cnt < MAXCAPNAMESIZE && cTmp != 0 )
        {
            fprintf( filewrite, ":" );
            tnc = 0;

            while ( tnc < 8 && cTmp != 0 )
            {
                if ( cTmp == ' ' )
                {
                    fprintf( filewrite, "_" );
                }
                else
                {
                    fprintf( filewrite, "%c", cTmp );
                }

                cTmp = ChrList[character].name[cnt];
                tnc++;
                cnt++;
            }

            fprintf( filewrite, "\n" );
            fprintf( filewrite, ":STOP\n\n" );
        }

        fclose( filewrite );
    }
}

//--------------------------------------------------------------------------------------------
void export_one_character_profile( const char *szSaveName, Uint16 character )
{
    // ZZ> This function creates a data.txt file for the given character.
    //     it is assumed that all enchantments have been done away with
    FILE* filewrite;
    int profile;
    int damagetype, skin;
    char types[10] = "SCPHEFIZ";
    char codes[4];

    // General stuff
    profile = ChrList[character].model;

    // Open the file
    filewrite = fopen( szSaveName, "w" );
    if ( filewrite )
    {
        // Real general data
        fprintf( filewrite, "Slot number    : -1\n" );  // -1 signals a flexible load thing
        funderf( filewrite, "Class name     : ", CapList[profile].classname );
        ftruthf( filewrite, "Uniform light  : ", CapList[profile].uniformlit );
        fprintf( filewrite, "Maximum ammo   : %d\n", CapList[profile].ammomax );
        fprintf( filewrite, "Current ammo   : %d\n", ChrList[character].ammo );
        fgendef( filewrite, "Gender         : ", ChrList[character].gender );
        fprintf( filewrite, "\n" );

        // Object stats
        fprintf( filewrite, "Life color     : %d\n", ChrList[character].lifecolor );
        fprintf( filewrite, "Mana color     : %d\n", ChrList[character].manacolor );
        fprintf( filewrite, "Life           : %4.2f\n", ChrList[character].lifemax / 256.0f );
        fpairof( filewrite, "Life up        : ", CapList[profile].lifeperlevelbase, CapList[profile].lifeperlevelrand );
        fprintf( filewrite, "Mana           : %4.2f\n", ChrList[character].manamax / 256.0f );
        fpairof( filewrite, "Mana up        : ", CapList[profile].manaperlevelbase, CapList[profile].manaperlevelrand );
        fprintf( filewrite, "Mana return    : %4.2f\n", ChrList[character].manareturn / 256.0f );
        fpairof( filewrite, "Mana return up : ", CapList[profile].manareturnperlevelbase, CapList[profile].manareturnperlevelrand );
        fprintf( filewrite, "Mana flow      : %4.2f\n", ChrList[character].manaflow / 256.0f );
        fpairof( filewrite, "Mana flow up   : ", CapList[profile].manaflowperlevelbase, CapList[profile].manaflowperlevelrand );
        fprintf( filewrite, "STR            : %4.2f\n", ChrList[character].strength / 256.0f );
        fpairof( filewrite, "STR up         : ", CapList[profile].strengthperlevelbase, CapList[profile].strengthperlevelrand );
        fprintf( filewrite, "WIS            : %4.2f\n", ChrList[character].wisdom / 256.0f );
        fpairof( filewrite, "WIS up         : ", CapList[profile].wisdomperlevelbase, CapList[profile].wisdomperlevelrand );
        fprintf( filewrite, "INT            : %4.2f\n", ChrList[character].intelligence / 256.0f );
        fpairof( filewrite, "INT up         : ", CapList[profile].intelligenceperlevelbase, CapList[profile].intelligenceperlevelrand );
        fprintf( filewrite, "DEX            : %4.2f\n", ChrList[character].dexterity / 256.0f );
        fpairof( filewrite, "DEX up         : ", CapList[profile].dexterityperlevelbase, CapList[profile].dexterityperlevelrand );
        fprintf( filewrite, "\n" );

        // More physical attributes
        fprintf( filewrite, "Size           : %4.2f\n", ChrList[character].sizegoto );
        fprintf( filewrite, "Size up        : %4.2f\n", CapList[profile].sizeperlevel );
        fprintf( filewrite, "Shadow size    : %d\n", CapList[profile].shadowsize );
        fprintf( filewrite, "Bump size      : %d\n", CapList[profile].bumpsize );
        fprintf( filewrite, "Bump height    : %d\n", CapList[profile].bumpheight );
        fprintf( filewrite, "Bump dampen    : %4.2f\n", CapList[profile].bumpdampen );
        fprintf( filewrite, "Weight         : %d\n", CapList[profile].weight );
        fprintf( filewrite, "Jump power     : %4.2f\n", CapList[profile].jump );
        fprintf( filewrite, "Jump number    : %d\n", CapList[profile].jumpnumber );
        fprintf( filewrite, "Sneak speed    : %d\n", CapList[profile].sneakspd );
        fprintf( filewrite, "Walk speed     : %d\n", CapList[profile].walkspd );
        fprintf( filewrite, "Run speed      : %d\n", CapList[profile].runspd );
        fprintf( filewrite, "Fly to height  : %d\n", CapList[profile].flyheight );
        fprintf( filewrite, "Flashing AND   : %d\n", CapList[profile].flashand );
        fprintf( filewrite, "Alpha blending : %d\n", CapList[profile].alpha );
        fprintf( filewrite, "Light blending : %d\n", CapList[profile].light );
        ftruthf( filewrite, "Transfer blend : ", CapList[profile].transferblend );
        fprintf( filewrite, "Sheen          : %d\n", CapList[profile].sheen );
        ftruthf( filewrite, "Phong mapping  : ", CapList[profile].enviro );
        fprintf( filewrite, "Texture X add  : %4.2f\n", CapList[profile].uoffvel / (float)0xFFFF );
        fprintf( filewrite, "Texture Y add  : %4.2f\n", CapList[profile].voffvel / (float)0xFFFF );
        ftruthf( filewrite, "Sticky butt    : ", CapList[profile].stickybutt );
        fprintf( filewrite, "\n" );

        // Invulnerability data
        ftruthf( filewrite, "Invictus       : ", CapList[profile].invictus );
        fprintf( filewrite, "NonI facing    : %d\n", CapList[profile].nframefacing );
        fprintf( filewrite, "NonI angle     : %d\n", CapList[profile].nframeangle );
        fprintf( filewrite, "I facing       : %d\n", CapList[profile].iframefacing );
        fprintf( filewrite, "I angle        : %d\n", CapList[profile].iframeangle );
        fprintf( filewrite, "\n" );

        // Skin defenses
        fprintf( filewrite, "Base defense   : %3d %3d %3d %3d\n", 255 - CapList[profile].defense[0], 255 - CapList[profile].defense[1],
                 255 - CapList[profile].defense[2], 255 - CapList[profile].defense[3] );
        damagetype = 0;

        while ( damagetype < DAMAGE_COUNT )
        {
            fprintf( filewrite, "%c damage shift : %3d %3d %3d %3d\n", types[damagetype],
                     CapList[profile].damagemodifier[damagetype][0]&DAMAGESHIFT,
                     CapList[profile].damagemodifier[damagetype][1]&DAMAGESHIFT,
                     CapList[profile].damagemodifier[damagetype][2]&DAMAGESHIFT,
                     CapList[profile].damagemodifier[damagetype][3]&DAMAGESHIFT );
            damagetype++;
        }

        damagetype = 0;

        while ( damagetype < DAMAGE_COUNT )
        {
            skin = 0;

            while ( skin < MAXSKIN )
            {
                codes[skin] = 'F';
                if ( CapList[profile].damagemodifier[damagetype][skin]&DAMAGEINVERT )
                    codes[skin] = 'T';
                if ( CapList[profile].damagemodifier[damagetype][skin]&DAMAGECHARGE )
                    codes[skin] = 'C';

                skin++;
            }

            fprintf( filewrite, "%c damage code  : %3c %3c %3c %3c\n", types[damagetype], codes[0], codes[1], codes[2], codes[3] );
            damagetype++;
        }

        fprintf( filewrite, "Acceleration   : %3.0f %3.0f %3.0f %3.0f\n", CapList[profile].maxaccel[0]*80,
                 CapList[profile].maxaccel[1]*80,
                 CapList[profile].maxaccel[2]*80,
                 CapList[profile].maxaccel[3]*80 );
        fprintf( filewrite, "\n" );

        // Experience and level data
        fprintf( filewrite, "EXP for 2nd    : %d\n", CapList[profile].experienceforlevel[1] );
        fprintf( filewrite, "EXP for 3rd    : %d\n", CapList[profile].experienceforlevel[2] );
        fprintf( filewrite, "EXP for 4th    : %d\n", CapList[profile].experienceforlevel[3] );
        fprintf( filewrite, "EXP for 5th    : %d\n", CapList[profile].experienceforlevel[4] );
        fprintf( filewrite, "EXP for 6th    : %d\n", CapList[profile].experienceforlevel[5] );
        fprintf( filewrite, "Starting EXP   : %d\n", ChrList[character].experience );
        fprintf( filewrite, "EXP worth      : %d\n", CapList[profile].experienceworth );
        fprintf( filewrite, "EXP exchange   : %5.3f\n", CapList[profile].experienceexchange );
        fprintf( filewrite, "EXPSECRET      : %4.2f\n", CapList[profile].experiencerate[0] );
        fprintf( filewrite, "EXPQUEST       : %4.2f\n", CapList[profile].experiencerate[1] );
        fprintf( filewrite, "EXPDARE        : %4.2f\n", CapList[profile].experiencerate[2] );
        fprintf( filewrite, "EXPKILL        : %4.2f\n", CapList[profile].experiencerate[3] );
        fprintf( filewrite, "EXPMURDER      : %4.2f\n", CapList[profile].experiencerate[4] );
        fprintf( filewrite, "EXPREVENGE     : %4.2f\n", CapList[profile].experiencerate[5] );
        fprintf( filewrite, "EXPTEAMWORK    : %4.2f\n", CapList[profile].experiencerate[6] );
        fprintf( filewrite, "EXPROLEPLAY    : %4.2f\n", CapList[profile].experiencerate[7] );
        fprintf( filewrite, "\n" );

        // IDSZ identification tags
        undo_idsz( CapList[profile].idsz[IDSZ_PARENT] );
        fprintf( filewrite, "IDSZ Parent    : [%s]\n", idsz_string );
        undo_idsz( CapList[profile].idsz[IDSZ_TYPE] );
        fprintf( filewrite, "IDSZ Type      : [%s]\n", idsz_string );
        undo_idsz( CapList[profile].idsz[IDSZ_SKILL] );
        fprintf( filewrite, "IDSZ Skill     : [%s]\n", idsz_string );
        undo_idsz( CapList[profile].idsz[IDSZ_SPECIAL] );
        fprintf( filewrite, "IDSZ Special   : [%s]\n", idsz_string );
        undo_idsz( CapList[profile].idsz[IDSZ_HATE] );
        fprintf( filewrite, "IDSZ Hate      : [%s]\n", idsz_string );
        undo_idsz( CapList[profile].idsz[IDSZ_VULNERABILITY] );
        fprintf( filewrite, "IDSZ Vulnie    : [%s]\n", idsz_string );
        fprintf( filewrite, "\n" );

        // Item and damage flags
        ftruthf( filewrite, "Is an item     : ", CapList[profile].isitem );
        ftruthf( filewrite, "Is a mount     : ", CapList[profile].ismount );
        ftruthf( filewrite, "Is stackable   : ", CapList[profile].isstackable );
        ftruthf( filewrite, "Name known     : ", ChrList[character].nameknown );
        ftruthf( filewrite, "Usage known    : ", CapList[profile].usageknown );
        ftruthf( filewrite, "Is exportable  : ", CapList[profile].cancarrytonextmodule );
        ftruthf( filewrite, "Requires skill : ", CapList[profile].needskillidtouse );
        ftruthf( filewrite, "Is platform    : ", CapList[profile].platform );
        ftruthf( filewrite, "Collects money : ", CapList[profile].cangrabmoney );
        ftruthf( filewrite, "Can open stuff : ", CapList[profile].canopenstuff );
        fprintf( filewrite, "\n" );

        // Other item and damage stuff
        fdamagf( filewrite, "Damage type    : ", CapList[profile].damagetargettype );
        factiof( filewrite, "Attack type    : ", CapList[profile].weaponaction );
        fprintf( filewrite, "\n" );

        // Particle attachments
        fprintf( filewrite, "Attached parts : %d\n", CapList[profile].attachedprtamount );
        fdamagf( filewrite, "Reaffirm type  : ", CapList[profile].attachedprtreaffirmdamagetype );
        fprintf( filewrite, "Particle type  : %d\n", CapList[profile].attachedprttype );
        fprintf( filewrite, "\n" );

        // Character hands
        ftruthf( filewrite, "Left valid     : ", CapList[profile].gripvalid[0] );
        ftruthf( filewrite, "Right valid    : ", CapList[profile].gripvalid[1] );
        fprintf( filewrite, "\n" );

        // Particle spawning on attack
        ftruthf( filewrite, "Part on weapon : ", CapList[profile].attackattached );
        fprintf( filewrite, "Part type      : %d\n", CapList[profile].attackprttype );
        fprintf( filewrite, "\n" );

        // Particle spawning for GoPoof
        fprintf( filewrite, "Poof amount    : %d\n", CapList[profile].gopoofprtamount );
        fprintf( filewrite, "Facing add     : %d\n", CapList[profile].gopoofprtfacingadd );
        fprintf( filewrite, "Part type      : %d\n", CapList[profile].gopoofprttype );
        fprintf( filewrite, "\n" );

        // Particle spawning for blud
        ftruthf( filewrite, "Blud valid    : ", CapList[profile].bludvalid );
        fprintf( filewrite, "Part type      : %d\n", CapList[profile].bludprttype );
        fprintf( filewrite, "\n" );

        // Extra stuff
        ftruthf( filewrite, "Waterwalking   : ", CapList[profile].waterwalk );
        fprintf( filewrite, "Bounce dampen  : %5.3f\n", CapList[profile].dampen );
        fprintf( filewrite, "\n" );

        // More stuff
        fprintf( filewrite, "Life healing   : %5.3f\n", CapList[profile].lifeheal / 256.0f );
        fprintf( filewrite, "Mana cost      : %5.3f\n", CapList[profile].manacost / 256.0f );
        fprintf( filewrite, "Life return    : %d\n", CapList[profile].lifereturn );
        fprintf( filewrite, "Stopped by     : %d\n", CapList[profile].stoppedby );
        funderf( filewrite, "Skin 0 name    : ", CapList[profile].skinname[0] );
        funderf( filewrite, "Skin 1 name    : ", CapList[profile].skinname[1] );
        funderf( filewrite, "Skin 2 name    : ", CapList[profile].skinname[2] );
        funderf( filewrite, "Skin 3 name    : ", CapList[profile].skinname[3] );
        fprintf( filewrite, "Skin 0 cost    : %d\n", CapList[profile].skincost[0] );
        fprintf( filewrite, "Skin 1 cost    : %d\n", CapList[profile].skincost[1] );
        fprintf( filewrite, "Skin 2 cost    : %d\n", CapList[profile].skincost[2] );
        fprintf( filewrite, "Skin 3 cost    : %d\n", CapList[profile].skincost[3] );
        fprintf( filewrite, "STR dampen     : %5.3f\n", CapList[profile].strengthdampen );
        fprintf( filewrite, "\n" );

        // Another memory lapse
        ftruthf( filewrite, "No rider attak : ", btrue - CapList[profile].ridercanattack );
        ftruthf( filewrite, "Can be dazed   : ", CapList[profile].canbedazed );
        ftruthf( filewrite, "Can be grogged : ", CapList[profile].canbegrogged );
        fprintf( filewrite, "NOT USED       : 0\n" );
        fprintf( filewrite, "NOT USED       : 0\n" );
        ftruthf( filewrite, "Can see invisi : ", CapList[profile].canseeinvisible );
        fprintf( filewrite, "Kursed chance  : %d\n", ChrList[character].iskursed*100 );
        fprintf( filewrite, "Footfall sound : %d\n", CapList[profile].soundindex[SOUND_FOOTFALL] );
        fprintf( filewrite, "Jump sound     : %d\n", CapList[profile].soundindex[SOUND_JUMP] );
        fprintf( filewrite, "\n" );

        // Expansions
        if ( CapList[profile].skindressy&1 )
            fprintf( filewrite, ":[DRES] 0\n" );
        if ( CapList[profile].skindressy&2 )
            fprintf( filewrite, ":[DRES] 1\n" );
        if ( CapList[profile].skindressy&4 )
            fprintf( filewrite, ":[DRES] 2\n" );
        if ( CapList[profile].skindressy&8 )
            fprintf( filewrite, ":[DRES] 3\n" );
        if ( CapList[profile].resistbumpspawn )
            fprintf( filewrite, ":[STUK] 0\n" );
        if ( CapList[profile].istoobig )
            fprintf( filewrite, ":[PACK] 0\n" );
        if ( !CapList[profile].reflect )
            fprintf( filewrite, ":[VAMP] 1\n" );
        if ( CapList[profile].alwaysdraw )
            fprintf( filewrite, ":[DRAW] 1\n" );
        if ( CapList[profile].isranged )
            fprintf( filewrite, ":[RANG] 1\n" );
        if ( CapList[profile].hidestate != NOHIDE )
            fprintf( filewrite, ":[HIDE] %d\n", CapList[profile].hidestate );
        if ( CapList[profile].isequipment )
            fprintf( filewrite, ":[EQUI] 1\n" );
        if ( CapList[profile].bumpsizebig == ( CapList[profile].bumpsize << 1 ) )
            fprintf( filewrite, ":[SQUA] 1\n" );
        if ( CapList[profile].icon != CapList[profile].usageknown )
            fprintf( filewrite, ":[ICON] %d\n", CapList[profile].icon );
        if ( CapList[profile].forceshadow )
            fprintf( filewrite, ":[SHAD] 1\n" );
        if ( CapList[profile].ripple == CapList[profile].isitem )
            fprintf( filewrite, ":[RIPP] %d\n", CapList[profile].ripple );
        if ( CapList[profile].isvaluable != -1 )
            fprintf( filewrite, ":[VALU] %d\n", CapList[profile].isvaluable );

        //Basic stuff that is always written
        fprintf( filewrite, ":[GOLD] %d\n", ChrList[character].money );
        fprintf( filewrite, ":[PLAT] %d\n", CapList[profile].canuseplatforms );
        fprintf( filewrite, ":[SKIN] %d\n", ChrList[character].skin );
        fprintf( filewrite, ":[CONT] %d\n", ChrList[character].ai.content );
        fprintf( filewrite, ":[STAT] %d\n", ChrList[character].ai.state );
        fprintf( filewrite, ":[LEVL] %d\n", ChrList[character].experiencelevel );

        //Copy all skill expansions
        fprintf( filewrite, ":[SHPR] %d\n", ChrList[character].shieldproficiency );
        if ( ChrList[character].canuseadvancedweapons )
            fprintf( filewrite, ":[AWEP] 1\n" );
        if ( ChrList[character].canjoust )
            fprintf( filewrite, ":[JOUS] 1\n" );
        if ( ChrList[character].candisarm )
            fprintf( filewrite, ":[DISA] 1\n" );
        if ( CapList[profile].canseekurse )
            fprintf( filewrite, ":[CKUR] 1\n" );
        if ( ChrList[character].canusepoison )
            fprintf( filewrite, ":[POIS] 1\n" );
        if ( ChrList[character].canread )
            fprintf( filewrite, ":[READ] 1\n" );
        if ( ChrList[character].canbackstab )
            fprintf( filewrite, ":[STAB] 1\n" );
        if ( ChrList[character].canusedivine )
            fprintf( filewrite, ":[HMAG] 1\n" );
        if ( ChrList[character].canusearcane )
            fprintf( filewrite, ":[WMAG] 1\n" );
        if ( ChrList[character].canusetech )
            fprintf( filewrite, ":[TECH] 1\n" );

        //The end
        fclose( filewrite );
    }
}

//--------------------------------------------------------------------------------------------
void export_one_character_skin( const char *szSaveName, Uint16 character )
{
    // ZZ> This function creates a skin.txt file for the given character.
    FILE* filewrite;
    int profile;

    // General stuff
    profile = ChrList[character].model;

    // Open the file
    filewrite = fopen( szSaveName, "w" );
    if ( filewrite )
    {
        fprintf( filewrite, "//This file is used only by the import menu\n" );
        fprintf( filewrite, ": %d\n", ChrList[character].skin );
        fclose( filewrite );
    }
}

//--------------------------------------------------------------------------------------------
int load_one_character_profile( const char *szLoadName )
{
    // ZZ> This function fills a character profile with data from data.txt, returning
    // the object slot that the profile was stuck into.  It may cause the program
    // to abort if bad things happen.
    FILE* fileread;
    Sint16 object = -1;
    int iTmp;
    float fTmp;
    char cTmp;
    Uint8 damagetype, level, xptype;
    int idsz_cnt;
    IDSZ idsz, test;

    // Open the file
    fileread = fopen( szLoadName, "r" );
    if ( fileread != NULL )
    {
        parse_filename = szLoadName;  //For debugging goto_colon()

        // Read in the object slot
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); object = iTmp;
        if ( object < 0 )
        {
            if ( importobject < 0 )
            {
                log_warning( "Object slot number cannot be negative (%s)\n", szLoadName );
            }
            else
            {
                object = importobject;
            }
        }

        // Make sure global objects don't load over existing models
        if ( MadList[object].used )
        {
            if ( object == SPELLBOOK ) log_error( "Object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, szLoadName );
            else if (overrideslots)  log_error( "Object slot %i used twice (%s)\n", object, szLoadName );
            else return -1;   //Stop, we don't want to override it
        }

        MadList[object].used = btrue;

        // clear out the sounds
        for ( iTmp = 0; iTmp < SOUND_COUNT; iTmp++ )
        {
            CapList[object].soundindex[iTmp] = -1;
        }

        // Read in the real general data
        goto_colon( fileread );  fget_name( fileread, CapList[object].classname, sizeof(CapList[object].classname) );

        // Light cheat
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].uniformlit = bfalse;
        if ( cTmp == 'T' || cTmp == 't' || GL_FLAT == shading )  CapList[object].uniformlit = btrue;

        // Ammo
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].ammomax = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].ammo = iTmp;
        // Gender
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].gender = GENOTHER;
        if ( cTmp == 'F' || cTmp == 'f' )  CapList[object].gender = GENFEMALE;
        if ( cTmp == 'M' || cTmp == 'm' )  CapList[object].gender = GENMALE;
        if ( cTmp == 'R' || cTmp == 'r' )  CapList[object].gender = GENRANDOM;

        // Read in the object stats
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].lifecolor = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].manacolor = iTmp;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].lifebase = pairbase;  CapList[object].liferand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].lifeperlevelbase = pairbase;  CapList[object].lifeperlevelrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].manabase = pairbase;  CapList[object].manarand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].manaperlevelbase = pairbase;  CapList[object].manaperlevelrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].manareturnbase = pairbase;  CapList[object].manareturnrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].manareturnperlevelbase = pairbase;  CapList[object].manareturnperlevelrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].manaflowbase = pairbase;  CapList[object].manaflowrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].manaflowperlevelbase = pairbase;  CapList[object].manaflowperlevelrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].strengthbase = pairbase;  CapList[object].strengthrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].strengthperlevelbase = pairbase;  CapList[object].strengthperlevelrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].wisdombase = pairbase;  CapList[object].wisdomrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].wisdomperlevelbase = pairbase;  CapList[object].wisdomperlevelrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].intelligencebase = pairbase;  CapList[object].intelligencerand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].intelligenceperlevelbase = pairbase;  CapList[object].intelligenceperlevelrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].dexteritybase = pairbase;  CapList[object].dexterityrand = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        CapList[object].dexterityperlevelbase = pairbase;  CapList[object].dexterityperlevelrand = pairrand;

        // More physical attributes
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].size = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].sizeperlevel = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].shadowsize = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].bumpsize = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].bumpheight = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].bumpdampen = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].weight = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].jump = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].jumpnumber = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].sneakspd = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].walkspd = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].runspd = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].flyheight = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].flashand = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].alpha = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].light = iTmp;
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].transferblend = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].transferblend = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].sheen = iTmp;
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].enviro = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].enviro = btrue;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].uoffvel = fTmp * 0xFFFF;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].voffvel = fTmp * 0xFFFF;
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].stickybutt = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].stickybutt = btrue;

        // Invulnerability data
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].invictus = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].invictus = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].nframefacing = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].nframeangle = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].iframefacing = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].iframeangle = iTmp;

        // Resist burning and stuck arrows with nframe angle of 1 or more
        if ( CapList[object].nframeangle > 0 )
        {
            if ( CapList[object].nframeangle == 1 )
            {
                CapList[object].nframeangle = 0;
            }
        }

        // Skin defenses ( 4 skins )
        goto_colon( fileread );
        fscanf( fileread, "%d", &iTmp );  CapList[object].defense[0] = 255 - iTmp;
        fscanf( fileread, "%d", &iTmp );  CapList[object].defense[1] = 255 - iTmp;
        fscanf( fileread, "%d", &iTmp );  CapList[object].defense[2] = 255 - iTmp;
        fscanf( fileread, "%d", &iTmp );  CapList[object].defense[3] = 255 - iTmp;

        for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
        {
            goto_colon( fileread );
            fscanf( fileread, "%d", &iTmp );  CapList[object].damagemodifier[damagetype][0] = iTmp;
            fscanf( fileread, "%d", &iTmp );  CapList[object].damagemodifier[damagetype][1] = iTmp;
            fscanf( fileread, "%d", &iTmp );  CapList[object].damagemodifier[damagetype][2] = iTmp;
            fscanf( fileread, "%d", &iTmp );  CapList[object].damagemodifier[damagetype][3] = iTmp;
        }

        for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
        {
            goto_colon( fileread );

            cTmp = fget_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  CapList[object].damagemodifier[damagetype][0] |= DAMAGEINVERT;
            if ( cTmp == 'C' || cTmp == 'c' )  CapList[object].damagemodifier[damagetype][0] |= DAMAGECHARGE;

            cTmp = fget_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  CapList[object].damagemodifier[damagetype][1] |= DAMAGEINVERT;
            if ( cTmp == 'C' || cTmp == 'c' )  CapList[object].damagemodifier[damagetype][1] |= DAMAGECHARGE;

            cTmp = fget_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  CapList[object].damagemodifier[damagetype][2] |= DAMAGEINVERT;
            if ( cTmp == 'C' || cTmp == 'c' )  CapList[object].damagemodifier[damagetype][2] |= DAMAGECHARGE;

            cTmp = fget_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  CapList[object].damagemodifier[damagetype][3] |= DAMAGEINVERT;
            if ( cTmp == 'C' || cTmp == 'c' )  CapList[object].damagemodifier[damagetype][3] |= DAMAGECHARGE;
        }

        goto_colon( fileread );
        fscanf( fileread, "%f", &fTmp );  CapList[object].maxaccel[0] = fTmp / 80.0f;
        fscanf( fileread, "%f", &fTmp );  CapList[object].maxaccel[1] = fTmp / 80.0f;
        fscanf( fileread, "%f", &fTmp );  CapList[object].maxaccel[2] = fTmp / 80.0f;
        fscanf( fileread, "%f", &fTmp );  CapList[object].maxaccel[3] = fTmp / 80.0f;

        // Experience and level data
        CapList[object].experienceforlevel[0] = 0;

        for ( level = 1; level < MAXLEVEL; level++ )
        {
            goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].experienceforlevel[level] = iTmp;
        }

        goto_colon( fileread );  read_pair( fileread );
        pairbase = pairbase >> 8;
        pairrand = pairrand >> 8;
        if ( pairrand < 1 )  pairrand = 1;

        CapList[object].experiencebase = pairbase;
        CapList[object].experiencerand = pairrand;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].experienceworth = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].experienceexchange = fTmp;

        for ( xptype = 0; xptype < XP_COUNT; xptype++ )
        {
            goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].experiencerate[xptype] = fTmp + 0.001f;
        }

        // IDSZ tags
        for ( idsz_cnt = 0; idsz_cnt < IDSZ_COUNT; idsz_cnt++ )
        {
            goto_colon( fileread );  iTmp = fget_idsz( fileread );  CapList[object].idsz[idsz_cnt] = iTmp;
        }

        // Item and damage flags
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].isitem = bfalse;  CapList[object].ripple = btrue;
        if ( cTmp == 'T' || cTmp == 't' )  { CapList[object].isitem = btrue; CapList[object].ripple = bfalse; }

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].ismount = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].ismount = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].isstackable = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].isstackable = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].nameknown = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].nameknown = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].usageknown = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].usageknown = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].cancarrytonextmodule = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].cancarrytonextmodule = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].needskillidtouse = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].needskillidtouse = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].platform = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].platform = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].cangrabmoney = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].cangrabmoney = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].canopenstuff = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].canopenstuff = btrue;

        // More item and damage stuff
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        if ( cTmp == 'S' || cTmp == 's' )  CapList[object].damagetargettype = DAMAGE_SLASH;
        if ( cTmp == 'C' || cTmp == 'c' )  CapList[object].damagetargettype = DAMAGE_CRUSH;
        if ( cTmp == 'P' || cTmp == 'p' )  CapList[object].damagetargettype = DAMAGE_POKE;
        if ( cTmp == 'H' || cTmp == 'h' )  CapList[object].damagetargettype = DAMAGE_HOLY;
        if ( cTmp == 'E' || cTmp == 'e' )  CapList[object].damagetargettype = DAMAGE_EVIL;
        if ( cTmp == 'F' || cTmp == 'f' )  CapList[object].damagetargettype = DAMAGE_FIRE;
        if ( cTmp == 'I' || cTmp == 'i' )  CapList[object].damagetargettype = DAMAGE_ICE;
        if ( cTmp == 'Z' || cTmp == 'z' )  CapList[object].damagetargettype = DAMAGE_ZAP;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].weaponaction = action_which( cTmp );

        // Particle attachments
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].attachedprtamount = iTmp;
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        if ( cTmp == 'N' || cTmp == 'n' )  CapList[object].attachedprtreaffirmdamagetype = DAMAGENULL;
        if ( cTmp == 'S' || cTmp == 's' )  CapList[object].attachedprtreaffirmdamagetype = DAMAGE_SLASH;
        if ( cTmp == 'C' || cTmp == 'c' )  CapList[object].attachedprtreaffirmdamagetype = DAMAGE_CRUSH;
        if ( cTmp == 'P' || cTmp == 'p' )  CapList[object].attachedprtreaffirmdamagetype = DAMAGE_POKE;
        if ( cTmp == 'H' || cTmp == 'h' )  CapList[object].attachedprtreaffirmdamagetype = DAMAGE_HOLY;
        if ( cTmp == 'E' || cTmp == 'e' )  CapList[object].attachedprtreaffirmdamagetype = DAMAGE_EVIL;
        if ( cTmp == 'F' || cTmp == 'f' )  CapList[object].attachedprtreaffirmdamagetype = DAMAGE_FIRE;
        if ( cTmp == 'I' || cTmp == 'i' )  CapList[object].attachedprtreaffirmdamagetype = DAMAGE_ICE;
        if ( cTmp == 'Z' || cTmp == 'z' )  CapList[object].attachedprtreaffirmdamagetype = DAMAGE_ZAP;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].attachedprttype = iTmp;

        // Character hands
        CapList[object].gripvalid[0] = bfalse;
        CapList[object].gripvalid[1] = bfalse;
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].gripvalid[0] = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].gripvalid[1] = btrue;

        // Attack order ( weapon )
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].attackattached = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].attackattached = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].attackprttype = iTmp;

        // GoPoof
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].gopoofprtamount = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].gopoofprtfacingadd = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].gopoofprttype = iTmp;

        // Blud
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].bludvalid = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].bludvalid = btrue;
        if ( cTmp == 'U' || cTmp == 'u' )  CapList[object].bludvalid = ULTRABLUDY;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].bludprttype = iTmp;

        // Stuff I forgot
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].waterwalk = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].waterwalk = btrue;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].dampen = fTmp;

        // More stuff I forgot
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].lifeheal = fTmp * 256;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].manacost = fTmp * 256;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].lifereturn = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].stoppedby = iTmp | MESHFX_IMPASS;
        goto_colon( fileread );  fget_name( fileread, CapList[object].skinname[0], sizeof(CapList[object].skinname[0]) );
        goto_colon( fileread );  fget_name( fileread, CapList[object].skinname[1], sizeof(CapList[object].skinname[1]) );
        goto_colon( fileread );  fget_name( fileread, CapList[object].skinname[2], sizeof(CapList[object].skinname[2]) );
        goto_colon( fileread );  fget_name( fileread, CapList[object].skinname[3], sizeof(CapList[object].skinname[3]) );
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].skincost[0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].skincost[1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].skincost[2] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  CapList[object].skincost[3] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  CapList[object].strengthdampen = fTmp;

        // Another memory lapse
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        CapList[object].ridercanattack = btrue;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].ridercanattack = bfalse;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );  // Can be dazed
        CapList[object].canbedazed = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].canbedazed = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );  // Can be grogged
        CapList[object].canbegrogged = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].canbegrogged = btrue;

        goto_colon( fileread );  // !!!BAD!!! Life add
        goto_colon( fileread );  // !!!BAD!!! Mana add
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );  // Can see invisible
        CapList[object].canseeinvisible = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  CapList[object].canseeinvisible = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Chance of kursed
        CapList[object].kursechance = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Footfall sound
        CapList[object].soundindex[SOUND_FOOTFALL] = CLIP(iTmp, -1, MAXWAVE);
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Jump sound
        CapList[object].soundindex[SOUND_JUMP] = CLIP(iTmp, -1, MAXWAVE);

        // Clear expansions...
        CapList[object].skindressy = bfalse;
        CapList[object].resistbumpspawn = bfalse;
        CapList[object].istoobig = bfalse;
        CapList[object].reflect = btrue;
        CapList[object].alwaysdraw = bfalse;
        CapList[object].isranged = bfalse;
        CapList[object].hidestate = NOHIDE;
        CapList[object].isequipment = bfalse;
        CapList[object].bumpsizebig = CapList[object].bumpsize + ( CapList[object].bumpsize >> 1 );
        CapList[object].canseekurse = bfalse;
        CapList[object].money = 0;
        CapList[object].icon = CapList[object].usageknown;
        CapList[object].forceshadow = bfalse;
        CapList[object].skinoverride = NOSKINOVERRIDE;
        CapList[object].contentoverride = 0;
        CapList[object].stateoverride = 0;
        CapList[object].leveloverride = 0;
        CapList[object].canuseplatforms = !CapList[object].platform;
        CapList[object].isvaluable = 0;

        //Skills
        CapList[object].canuseadvancedweapons = 0;
        CapList[object].canjoust = 0;
        CapList[object].canusetech = 0;
        CapList[object].canusedivine = 0;
        CapList[object].canusearcane = 0;
        CapList[object].shieldproficiency = 0;
        CapList[object].candisarm = 0;
        CapList[object].canbackstab = 0;
        CapList[object].canusepoison = 0;
        CapList[object].canread = 0;

        // Read expansions
        while ( goto_colon_yesno( fileread ) )
        {
            idsz = fget_idsz( fileread );
            fscanf( fileread, "%c%d", &cTmp, &iTmp );

            test = Make_IDSZ( "DRES" );  // [DRES]
            if ( idsz == test )  CapList[object].skindressy |= 1 << iTmp;

            test = Make_IDSZ( "GOLD" );  // [GOLD]
            if ( idsz == test )  CapList[object].money = iTmp;

            test = Make_IDSZ( "STUK" );  // [STUK]
            if ( idsz == test )  CapList[object].resistbumpspawn = 1 - iTmp;

            test = Make_IDSZ( "PACK" );  // [PACK]
            if ( idsz == test )  CapList[object].istoobig = 1 - iTmp;

            test = Make_IDSZ( "VAMP" );  // [VAMP]
            if ( idsz == test )  CapList[object].reflect = 1 - iTmp;

            test = Make_IDSZ( "DRAW" );  // [DRAW]
            if ( idsz == test )  CapList[object].alwaysdraw = iTmp;

            test = Make_IDSZ( "RANG" );  // [RANG]
            if ( idsz == test )  CapList[object].isranged = iTmp;

            test = Make_IDSZ( "HIDE" );  // [HIDE]
            if ( idsz == test )  CapList[object].hidestate = iTmp;

            test = Make_IDSZ( "EQUI");  // [EQUI]
            if ( idsz == test )  CapList[object].isequipment = iTmp;

            test = Make_IDSZ( "SQUA");  // [SQUA]
            if ( idsz == test )  CapList[object].bumpsizebig = CapList[object].bumpsize << 1;

            test = Make_IDSZ( "ICON" );  // [ICON]
            if ( idsz == test )  CapList[object].icon = iTmp;

            test = Make_IDSZ( "SHAD" );  // [SHAD]
            if ( idsz == test )  CapList[object].forceshadow = iTmp;

            test = Make_IDSZ( "CKUR" );  // [CKUR]
            if ( idsz == test )  CapList[object].canseekurse = iTmp;

            test = Make_IDSZ( "SKIN" );  // [SKIN]
            if ( idsz == test )  CapList[object].skinoverride = iTmp & 3;

            test = Make_IDSZ( "CONT" );  // [CONT]
            if ( idsz == test )  CapList[object].contentoverride = iTmp;

            test = Make_IDSZ( "STAT" );  // [STAT]
            if ( idsz == test )  CapList[object].stateoverride = iTmp;

            test = Make_IDSZ( "LEVL" );  // [LEVL]
            if ( idsz == test )  CapList[object].leveloverride = iTmp;

            test = Make_IDSZ( "PLAT" );  // [PLAT]
            if ( idsz == test )  CapList[object].canuseplatforms = iTmp;

            test = Make_IDSZ( "RIPP" );  // [RIPP]
            if ( idsz == test )  CapList[object].ripple = iTmp;

            test = Make_IDSZ( "VALU" );  // [VALU]
            if ( idsz == test ) CapList[object].isvaluable = iTmp;

            //Read Skills
            test = Make_IDSZ( "AWEP" );  // [AWEP]
            if ( idsz == test )  CapList[object].canuseadvancedweapons = iTmp;

            test = Make_IDSZ( "SHPR" );  // [SHPR]
            if ( idsz == test )  CapList[object].shieldproficiency = iTmp;

            test = Make_IDSZ( "JOUS" );  // [JOUS]
            if ( idsz == test )  CapList[object].canjoust = iTmp;

            test = Make_IDSZ( "WMAG" );  // [WMAG]
            if ( idsz == test )  CapList[object].canusearcane = iTmp;

            test = Make_IDSZ( "HMAG" );  // [HMAG]
            if ( idsz == test )  CapList[object].canusedivine = iTmp;

            test = Make_IDSZ( "TECH" );  // [TECH]
            if ( idsz == test )  CapList[object].canusetech = iTmp;

            test = Make_IDSZ( "DISA" );  // [DISA]
            if ( idsz == test )  CapList[object].candisarm = iTmp;

            test = Make_IDSZ( "STAB" );  // [STAB]
            if ( idsz == test )  CapList[object].canbackstab = iTmp;

            test = Make_IDSZ( "POIS" );  // [POIS]
            if ( idsz == test )  CapList[object].canusepoison = iTmp;

            test = Make_IDSZ( "READ" );  // [READ]
            if ( idsz == test )  CapList[object].canread = iTmp;
        }

        fclose( fileread );
    }
    else
    {
        // The data file wasn't found
        log_error( "DATA.TXT was not found! (%s)\n", szLoadName );
    }

    return object;
}

//--------------------------------------------------------------------------------------------
void damage_character( Uint16 character, Uint16 direction,
                       int damagebase, int damagerand, Uint8 damagetype, Uint8 team,
                       Uint16 attacker, Uint16 effects, bool_t ignoreinvincible )
{
    // ZZ> This function calculates and applies damage to a character.  It also
    //     sets alerts and begins actions.  Blocking and frame invincibility
    //     are done here too.  Direction is 0 if the attack is coming head on,
    //     16384 if from the right, 32768 if from the back, 49152 if from the
    //     left.

    int tnc;
    Uint16 action;
    int damage, basedamage;
    Uint16 experience, model, left, right;
    if ( ChrList[character].alive && damagebase >= 0 && damagerand >= 1 )
    {
        // Lessen damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
        // This can also be used to lessen effectiveness of healing
        damage = damagebase + ( rand() % damagerand );
        basedamage = damage;
        damage = damage >> ( ChrList[character].damagemodifier[damagetype] & DAMAGESHIFT );

        // Allow charging (Invert damage to mana)
        if ( ChrList[character].damagemodifier[damagetype]&DAMAGECHARGE )
        {
            ChrList[character].mana += damage;
            if ( ChrList[character].mana > ChrList[character].manamax )
            {
                ChrList[character].mana = ChrList[character].manamax;
            }

            return;
        }

        // Invert damage to heal
        if ( ChrList[character].damagemodifier[damagetype]&DAMAGEINVERT )
            damage = -damage;

        // Remember the damage type
        ChrList[character].ai.damagetypelast = damagetype;
        ChrList[character].ai.directionlast = direction;

        // Do it already
        if ( damage > 0 )
        {
            // Only damage if not invincible
            if ( 0 == ChrList[character].damagetime && ( !ChrList[character].invictus || ignoreinvincible ) )
            {
                model = ChrList[character].model;

                if ( 0 == ( effects&DAMFXBLOC ) )
                {
                    // Only damage if hitting from proper direction
                    if ( Md2FrameList[ChrList[character].frame].framefx&MADFXINVICTUS )
                    {
                        // I Frame...
                        direction -= CapList[model].iframefacing;
                        left = ( ~CapList[model].iframeangle );
                        right = CapList[model].iframeangle;

                        // Check for shield
                        if ( ChrList[character].action >= ACTIONPA && ChrList[character].action <= ACTIONPD )
                        {
                            // Using a shield?
                            if ( ChrList[character].action < ACTIONPC )
                            {
                                // Check left hand
                                if ( ChrList[character].holdingwhich[0] != MAXCHR )
                                {
                                    left = ( ~CapList[ChrList[ChrList[character].holdingwhich[0]].model].iframeangle );
                                    right = CapList[ChrList[ChrList[character].holdingwhich[0]].model].iframeangle;
                                }
                            }
                            else
                            {
                                // Check right hand
                                if ( ChrList[character].holdingwhich[1] != MAXCHR )
                                {
                                    left = ( ~CapList[ChrList[ChrList[character].holdingwhich[1]].model].iframeangle );
                                    right = CapList[ChrList[ChrList[character].holdingwhich[1]].model].iframeangle;
                                }
                            }
                        }
                    }
                    else
                    {
                        // N Frame
                        direction -= CapList[model].nframefacing;
                        left = ( ~CapList[model].nframeangle );
                        right = CapList[model].nframeangle;
                    }

                    // Check that direction
                    if ( direction > left || direction < right )
                    {
                        damage = 0;
                    }
                }

                if ( damage != 0 )
                {
                    if ( effects&DAMFXARMO )
                    {
                        ChrList[character].life -= damage;
                    }
                    else
                    {
                        ChrList[character].life -= FP8_MUL( damage, ChrList[character].defense );
                    }
                    if ( basedamage > HURTDAMAGE )
                    {
                        // Call for help if below 1/2 life
                        /*if(ChrList[character].life < (ChrList[character].lifemax>>1))
                            call_for_help(character);*/

                        // Spawn blud particles
                        if ( CapList[model].bludvalid && ( damagetype < DAMAGE_HOLY || CapList[model].bludvalid == ULTRABLUDY ) )
                        {
                            spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos,
                                                ChrList[character].turnleftright + direction, ChrList[character].model, CapList[model].bludprttype,
                                                MAXCHR, GRIP_LAST, ChrList[character].team, character, 0, MAXCHR );
                        }

                        // Set attack alert if it wasn't an accident
                        if ( team == DAMAGETEAM )
                        {
                            ChrList[character].ai.attacklast = MAXCHR;
                        }
                        else
                        {
                            // Don't alert the character too much if under constant fire
                            if ( ChrList[character].carefultime == 0 )
                            {
                                // Don't let characters chase themselves...  That would be silly
                                if ( attacker != character )
                                {
                                    ChrList[character].ai.alert |= ALERTIF_ATTACKED;
                                    ChrList[character].ai.attacklast = attacker;
                                    ChrList[character].carefultime = CAREFULTIME;
                                }
                            }
                        }
                    }

                    // Taking damage action
                    action = ACTIONHA;
                    if ( ChrList[character].life < 0 )
                    {
                        // Character has died
                        ChrList[character].alive = bfalse;
                        disenchant_character( character );
                        ChrList[character].waskilled = btrue;
                        ChrList[character].keepaction = btrue;
                        ChrList[character].life = -1;
                        ChrList[character].platform = btrue;
                        ChrList[character].bumpdampen = ChrList[character].bumpdampen / 2.0f;
                        action = ACTIONKA;
                        // Give kill experience
                        experience = CapList[model].experienceworth + ( ChrList[character].experience * CapList[model].experienceexchange );
                        if ( attacker < MAXCHR )
                        {
                            // Set target
                            ChrList[character].ai.target = attacker;
                            if ( team == DAMAGETEAM )  ChrList[character].ai.target = character;
                            if ( team == NULLTEAM )  ChrList[character].ai.target = character;

                            // Award direct kill experience
                            if ( TeamList[ChrList[attacker].team].hatesteam[ChrList[character].team] )
                            {
                                give_experience( attacker, experience, XP_KILLENEMY, bfalse );
                            }

                            // Check for hated
                            if ( CapList[ChrList[attacker].model].idsz[IDSZ_HATE] == CapList[model].idsz[IDSZ_PARENT] ||
                                    CapList[ChrList[attacker].model].idsz[IDSZ_HATE] == CapList[model].idsz[IDSZ_TYPE] )
                            {
                                give_experience( attacker, experience, XP_KILLHATED, bfalse );
                            }
                        }

                        // Clear all shop passages that it owned...
                        tnc = 0;

                        while ( tnc < numshoppassage )
                        {
                            if ( shopowner[tnc] == character )
                            {
                                shopowner[tnc] = NOOWNER;
                            }

                            tnc++;
                        }

                        // Let the other characters know it died
                        tnc = 0;

                        while ( tnc < MAXCHR )
                        {
                            if ( ChrList[tnc].on && ChrList[tnc].alive )
                            {
                                if ( ChrList[tnc].ai.target == character )
                                {
                                    ChrList[tnc].ai.alert |= ALERTIF_TARGETKILLED;
                                }
                                if ( !TeamList[ChrList[tnc].team].hatesteam[team] && ( TeamList[ChrList[tnc].team].hatesteam[ChrList[character].team] ) )
                                {
                                    // All allies get team experience, but only if they also hate the dead guy's team
                                    give_experience( tnc, experience, XP_TEAMKILL, bfalse );
                                }
                            }

                            tnc++;
                        }

                        // Check if it was a leader
                        if ( TeamList[ChrList[character].team].leader == character )
                        {
                            // It was a leader, so set more alerts
                            tnc = 0;

                            while ( tnc < MAXCHR )
                            {
                                if ( ChrList[tnc].on && ChrList[tnc].team == ChrList[character].team )
                                {
                                    // All folks on the leaders team get the alert
                                    ChrList[tnc].ai.alert |= ALERTIF_LEADERKILLED;
                                }

                                tnc++;
                            }

                            // The team now has no leader
                            TeamList[ChrList[character].team].leader = NOLEADER;
                        }

                        detach_character_from_mount( character, btrue, bfalse );

                        //Play the death animation
                        action += ( rand() & 3 );
                        chr_play_action( character, action, bfalse );

                        //If it's a player, let it die properly before enabling respawn
                        if (ChrList[character].isplayer) revivetimer = ONESECOND; //1 second

                        // Afford it one last thought if it's an AI
                        TeamList[ChrList[character].baseteam].morale--;
                        ChrList[character].team = ChrList[character].baseteam;
                        ChrList[character].ai.alert |= ALERTIF_KILLED;
                        ChrList[character].sparkle = NOSPARKLE;
                        ChrList[character].ai.timer = frame_wld + 1;  // No timeout...

                        let_character_think( character );
                    }
                    else
                    {
                        if ( basedamage > HURTDAMAGE )
                        {
                            action += ( rand() & 3 );
                            chr_play_action( character, action, bfalse );

                            // Make the character invincible for a limited time only
                            if ( !( effects & DAMFXTIME ) )
                                ChrList[character].damagetime = DAMAGETIME;
                        }
                    }
                }
                else
                {
                    // Spawn a defend particle
                    spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, ChrList[character].turnleftright, MAXMODEL, DEFEND, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                    ChrList[character].damagetime    = DEFENDTIME;
                    ChrList[character].ai.alert     |= ALERTIF_BLOCKED;
                    ChrList[character].ai.attacklast = attacker;     // For the ones attacking a shield
                }
            }
        }
        else if ( damage < 0 )
        {
            ChrList[character].life -= damage;
            if ( ChrList[character].life > ChrList[character].lifemax )  ChrList[character].life = ChrList[character].lifemax;

            // Isssue an alert
            ChrList[character].ai.alert |= ALERTIF_HEALED;
            ChrList[character].ai.attacklast = attacker;
            if ( team != DAMAGETEAM )
            {
                ChrList[character].ai.attacklast = MAXCHR;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void kill_character( Uint16 character, Uint16 killer )
{
    // ZZ> This function kills a character...  MAXCHR killer for accidental death
    Uint8 modifier;
    if ( ChrList[character].alive )
    {
        ChrList[character].damagetime = 0;
        ChrList[character].life = 1;
        modifier = ChrList[character].damagemodifier[DAMAGE_CRUSH];
        ChrList[character].damagemodifier[DAMAGE_CRUSH] = 1;
        if ( killer != MAXCHR )
        {
            damage_character( character, 0, 512, 1, DAMAGE_CRUSH, ChrList[killer].team, killer, DAMFXARMO | DAMFXBLOC, btrue );
        }
        else
        {
            damage_character( character, 0, 512, 1, DAMAGE_CRUSH, DAMAGETEAM, ChrList[character].ai.bumplast, DAMFXARMO | DAMFXBLOC, btrue );
        }

        ChrList[character].damagemodifier[DAMAGE_CRUSH] = modifier;
    }
}

//--------------------------------------------------------------------------------------------
void spawn_poof( Uint16 character, Uint16 profile )
{
    // ZZ> This function spawns a character poof
    Uint16 sTmp;
    Uint16 origin;
    int iTmp;

    sTmp = ChrList[character].turnleftright;
    iTmp = 0;
    origin = ChrList[character].ai.owner;

    while ( iTmp < CapList[profile].gopoofprtamount )
    {
        spawn_one_particle( ChrList[character].oldx, ChrList[character].oldy, ChrList[character].oldz,
                            sTmp, profile, CapList[profile].gopoofprttype,
                            MAXCHR, GRIP_LAST, ChrList[character].team, origin, iTmp, MAXCHR );
        sTmp += CapList[profile].gopoofprtfacingadd;
        iTmp++;
    }
}

//--------------------------------------------------------------------------------------------
char * chop_create( Uint16 profile )
{
    // ZZ> This function generates a random name
    int read, write, section, mychop;
    char cTmp;

    static char buffer[MAXCAPNAMESIZE];// The name returned by the function

    if ( 0 == CapList[profile].chop_sectionsize[0] )
    {
        strcpy(buffer, "Blah");
    }
    else
    {
        write = 0;

        for ( section = 0; section < MAXSECTION; section++ )
        {
            if ( 0 != CapList[profile].chop_sectionsize[section] )
            {
                mychop = CapList[profile].chop_sectionstart[section] + ( rand() % CapList[profile].chop_sectionsize[section] );

                read = chop.start[mychop];
                cTmp = chop.buffer[read];
                while ( cTmp != 0 && write < MAXCAPNAMESIZE - 1 )
                {
                    buffer[write] = cTmp;
                    write++;
                    read++;
                    cTmp = chop.buffer[read];
                }
            }
        }
        if ( write >= MAXCAPNAMESIZE ) write = MAXCAPNAMESIZE - 1;

        buffer[write] = '\0';
    }

    return buffer;
}

//--------------------------------------------------------------------------------------------
void init_ai_state( ai_state_t * pself, Uint16 index, Uint16 profile, Uint16 model, Uint16 rank )
{
    int tnc;
    if ( NULL == pself || index >= MAXCHR ) return;

    // clear out everything
    memset( pself, 0, sizeof(ai_state_t) );

    pself->index      = index;
    pself->type       = MadList[model].ai;
    pself->alert      = ALERTIF_SPAWNED;
    pself->state      = CapList[profile].stateoverride;
    pself->content    = CapList[profile].contentoverride;
    pself->passage    = 0;
    pself->target     = index;
    pself->owner      = index;
    pself->child      = index;
    pself->target_old = pself->target;
    pself->poof_time  = -1;

    pself->timer      = 0;

    pself->bumplast   = index;
    pself->attacklast = MAXCHR;
    pself->hitlast    = index;

    pself->rank       = rank;
    pself->order      = 0;

    pself->wp_tail = 0;
    pself->wp_head = 0;
    for ( tnc = 0; tnc < MAXSTOR; tnc++ )
    {
        pself->x[tnc] = 0;
        pself->y[tnc] = 0;
    }
}

//--------------------------------------------------------------------------------------------
int spawn_one_character( float x, float y, float z, Uint16 profile, Uint8 team,
                         Uint8 skin, Uint16 facing, const char *name, int override )
{
    // ZZ> This function spawns a character and returns the character's index number
    //     if it worked, MAXCHR otherwise

    Uint16 ichr;
    int tnc;

    if ( profile >= MAXMODEL )
    {
        return MAXCHR;
    }

    if ( !MadList[profile].used )
    {
        if ( profile > importamount * 9 )
        {
            log_warning( "spawn_one_character() - trying to spawn using invalid profile %d\n", profile );
        }

        return MAXCHR;
    }

    // allocate a new character
    ichr = MAXCHR;
    if ( override < MAXCHR )
    {
        ichr = get_free_character();
        if ( ichr != override )
        {
            // Picked the wrong one, so put this one back and find the right one

            for ( tnc = 0; tnc < MAXCHR; tnc++ )
            {
                if ( freechrlist[tnc] == override )
                {
                    freechrlist[tnc] = ichr;
                    break;
                }
            }

            ichr = override;
        }

        if ( MAXCHR == ichr )
        {
            log_warning( "spawn_one_character() - failed to override a character? character %d already spawned? \n", override );
            return ichr;
        }
    }
    else
    {
        ichr = get_free_character();
        if ( MAXCHR == ichr )
        {
            log_warning( "spawn_one_character() - failed to allocate a new character\n" );
            return ichr;
        }
    }

    if ( MAXCHR == ichr )
    {
        log_warning( "spawn_one_character() - failed to spawn character\n" );
        return ichr;
    }

    // Make sure the team is valid
    team = MIN( team, MAXTEAM - 1 );

    // IMPORTANT!!!
    ChrList[ichr].indolist = bfalse;
    ChrList[ichr].isequipped = bfalse;
    ChrList[ichr].sparkle = NOSPARKLE;
    ChrList[ichr].overlay = bfalse;
    ChrList[ichr].missilehandler = ichr;

    // sound stuff...  copy from the cap
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        ChrList[ichr].soundindex[tnc] = CapList[profile].soundindex[tnc];
    }

    // Set up model stuff
    ChrList[ichr].reloadtime = 0;
    ChrList[ichr].inwhichhand = SLOT_LEFT;
    ChrList[ichr].waskilled = bfalse;
    ChrList[ichr].inpack = bfalse;
    ChrList[ichr].nextinpack = MAXCHR;
    ChrList[ichr].numinpack = 0;
    ChrList[ichr].model = profile;
    ChrList[ichr].basemodel = profile;
    ChrList[ichr].stoppedby = CapList[profile].stoppedby;
    ChrList[ichr].lifeheal = CapList[profile].lifeheal;
    ChrList[ichr].manacost = CapList[profile].manacost;
    ChrList[ichr].inwater = bfalse;
    ChrList[ichr].nameknown = CapList[profile].nameknown;
    ChrList[ichr].ammoknown = CapList[profile].nameknown;
    ChrList[ichr].hitready = btrue;
    ChrList[ichr].boretime = BORETIME;
    ChrList[ichr].carefultime = CAREFULTIME;
    ChrList[ichr].canbecrushed = bfalse;
    ChrList[ichr].damageboost = 0;
    ChrList[ichr].icon = CapList[profile].icon;

    // Enchant stuff
    ChrList[ichr].firstenchant = MAXENCHANT;
    ChrList[ichr].undoenchant = MAXENCHANT;
    ChrList[ichr].canseeinvisible = CapList[profile].canseeinvisible;
    ChrList[ichr].canchannel = bfalse;
    ChrList[ichr].missiletreatment = MISNORMAL;
    ChrList[ichr].missilecost = 0;

    //Skillz
    ChrList[ichr].canjoust = CapList[profile].canjoust;
    ChrList[ichr].canuseadvancedweapons = CapList[profile].canuseadvancedweapons;
    ChrList[ichr].shieldproficiency = CapList[profile].shieldproficiency;
    ChrList[ichr].canusedivine = CapList[profile].canusedivine;
    ChrList[ichr].canusearcane = CapList[profile].canusearcane;
    ChrList[ichr].canusetech = CapList[profile].canusetech;
    ChrList[ichr].candisarm = CapList[profile].candisarm;
    ChrList[ichr].canbackstab = CapList[profile].canbackstab;
    ChrList[ichr].canusepoison = CapList[profile].canusepoison;
    ChrList[ichr].canread = CapList[profile].canread;
    ChrList[ichr].canseekurse = CapList[profile].canseekurse;

    // Kurse state
    ChrList[ichr].iskursed = ( ( rand() % 100 ) < CapList[profile].kursechance );
    if ( !CapList[profile].isitem )  ChrList[ichr].iskursed = bfalse;

    // Ammo
    ChrList[ichr].ammomax = CapList[profile].ammomax;
    ChrList[ichr].ammo = CapList[profile].ammo;

    // Gender
    ChrList[ichr].gender = CapList[profile].gender;
    if ( ChrList[ichr].gender == GENRANDOM )  ChrList[ichr].gender = GENFEMALE + ( rand() & 1 );

    ChrList[ichr].isplayer = bfalse;
    ChrList[ichr].islocalplayer = bfalse;

    // AI stuff
    init_ai_state( &(ChrList[ichr].ai), ichr, profile, ChrList[ichr].model, TeamList[team].morale );

    // Team stuff
    ChrList[ichr].team = team;
    ChrList[ichr].baseteam = team;
    if ( !CapList[profile].invictus )  TeamList[team].morale++;

    // Firstborn becomes the leader
    if ( TeamList[team].leader == NOLEADER )
    {
        TeamList[team].leader = ichr;
    }

    // Skin
    if ( CapList[profile].skinoverride != NOSKINOVERRIDE )
    {
        skin = CapList[profile].skinoverride % MAXSKIN;
    }
    if ( skin >= MadList[profile].skins )
    {
        skin = 0;
        if ( MadList[profile].skins > 1 )
        {
            skin = rand() % MadList[profile].skins;
        }
    }

    ChrList[ichr].skin    = skin;
    ChrList[ichr].texture = MadList[profile].skinstart + skin;

    // Life and Mana
    ChrList[ichr].alive = btrue;
    ChrList[ichr].lifecolor = CapList[profile].lifecolor;
    ChrList[ichr].manacolor = CapList[profile].manacolor;
    ChrList[ichr].lifemax = generate_number( CapList[profile].lifebase, CapList[profile].liferand );
    ChrList[ichr].life = ChrList[ichr].lifemax;
    ChrList[ichr].lifereturn = CapList[profile].lifereturn;
    ChrList[ichr].manamax = generate_number( CapList[profile].manabase, CapList[profile].manarand );
    ChrList[ichr].manaflow = generate_number( CapList[profile].manaflowbase, CapList[profile].manaflowrand );
    ChrList[ichr].manareturn = generate_number( CapList[profile].manareturnbase, CapList[profile].manareturnrand );
    ChrList[ichr].mana = ChrList[ichr].manamax;

    // SWID
    ChrList[ichr].strength = generate_number( CapList[profile].strengthbase, CapList[profile].strengthrand );
    ChrList[ichr].wisdom = generate_number( CapList[profile].wisdombase, CapList[profile].wisdomrand );
    ChrList[ichr].intelligence = generate_number( CapList[profile].intelligencebase, CapList[profile].intelligencerand );
    ChrList[ichr].dexterity = generate_number( CapList[profile].dexteritybase, CapList[profile].dexterityrand );

    // Damage
    ChrList[ichr].defense = CapList[profile].defense[skin];
    ChrList[ichr].reaffirmdamagetype = CapList[profile].attachedprtreaffirmdamagetype;
    ChrList[ichr].damagetargettype = CapList[profile].damagetargettype;
    tnc = 0;

    while ( tnc < DAMAGE_COUNT )
    {
        ChrList[ichr].damagemodifier[tnc] = CapList[profile].damagemodifier[tnc][skin];
        tnc++;
    }

    //latches
    ChrList[ichr].latchx = 0;
    ChrList[ichr].latchy = 0;
    ChrList[ichr].latchbutton = 0;

    ChrList[ichr].turnmode = TURNMODEVELOCITY;

    // Flags
    ChrList[ichr].stickybutt = CapList[profile].stickybutt;
    ChrList[ichr].openstuff = CapList[profile].canopenstuff;
    ChrList[ichr].transferblend = CapList[profile].transferblend;
    ChrList[ichr].enviro = CapList[profile].enviro;
    ChrList[ichr].waterwalk = CapList[profile].waterwalk;
    ChrList[ichr].platform = CapList[profile].platform;
    ChrList[ichr].isitem = CapList[profile].isitem;
    ChrList[ichr].invictus = CapList[profile].invictus;
    ChrList[ichr].ismount = CapList[profile].ismount;
    ChrList[ichr].cangrabmoney = CapList[profile].cangrabmoney;

    // Jumping
    ChrList[ichr].jump = CapList[profile].jump;
    ChrList[ichr].jumpnumber = 0;
    ChrList[ichr].jumpnumberreset = CapList[profile].jumpnumber;
    ChrList[ichr].jumptime = JUMPDELAY;

    // Other junk
    ChrList[ichr].flyheight = CapList[profile].flyheight;
    ChrList[ichr].maxaccel = CapList[profile].maxaccel[skin];
    ChrList[ichr].alpha = ChrList[ichr].basealpha = CapList[profile].alpha;
    ChrList[ichr].light = CapList[profile].light;
    ChrList[ichr].flashand = CapList[profile].flashand;
    ChrList[ichr].sheen = CapList[profile].sheen;
    ChrList[ichr].dampen = CapList[profile].dampen;

    // Character size and bumping
    ChrList[ichr].fat = CapList[profile].size;
    ChrList[ichr].sizegoto = ChrList[ichr].fat;
    ChrList[ichr].sizegototime = 0;
    ChrList[ichr].shadowsize = CapList[profile].shadowsize * ChrList[ichr].fat;
    ChrList[ichr].bumpsize = CapList[profile].bumpsize * ChrList[ichr].fat;
    ChrList[ichr].bumpsizebig = CapList[profile].bumpsizebig * ChrList[ichr].fat;
    ChrList[ichr].bumpheight = CapList[profile].bumpheight * ChrList[ichr].fat;

    ChrList[ichr].shadowsizesave = CapList[profile].shadowsize;
    ChrList[ichr].bumpsizesave = CapList[profile].bumpsize;
    ChrList[ichr].bumpsizebigsave = CapList[profile].bumpsizebig;
    ChrList[ichr].bumpheightsave = CapList[profile].bumpheight;

    ChrList[ichr].bumpdampen = CapList[profile].bumpdampen;
    if ( CapList[profile].weight == 0xFF )
    {
        ChrList[ichr].weight = 0xFFFF;
    }
    else
    {
        int itmp = CapList[profile].weight * ChrList[ichr].fat * ChrList[ichr].fat * ChrList[ichr].fat;
        ChrList[ichr].weight = MIN( itmp, 0xFFFE );
    }

    // Grip info
    ChrList[ichr].attachedto = MAXCHR;
    ChrList[ichr].holdingwhich[0] = MAXCHR;
    ChrList[ichr].holdingwhich[1] = MAXCHR;

    // Image rendering
    ChrList[ichr].uoffset = 0;
    ChrList[ichr].voffset = 0;
    ChrList[ichr].uoffvel = CapList[profile].uoffvel;
    ChrList[ichr].voffvel = CapList[profile].voffvel;
    ChrList[ichr].redshift = 0;
    ChrList[ichr].grnshift = 0;
    ChrList[ichr].blushift = 0;

    // Movement
    ChrList[ichr].sneakspd = CapList[profile].sneakspd;
    ChrList[ichr].walkspd = CapList[profile].walkspd;
    ChrList[ichr].runspd = CapList[profile].runspd;

    // Set up position
    ChrList[ichr].xpos = x;
    ChrList[ichr].ypos = y;
    ChrList[ichr].oldx = x;
    ChrList[ichr].oldy = y;
    ChrList[ichr].turnleftright = facing;
    ChrList[ichr].lightturnleftright = 0;
    ChrList[ichr].onwhichfan   = mesh_get_tile(x, y);
    ChrList[ichr].onwhichblock = mesh_get_block( x, y );
    ChrList[ichr].level = get_level( ChrList[ichr].xpos, ChrList[ichr].ypos, ChrList[ichr].waterwalk ) + RAISE;
    if ( z < ChrList[ichr].level ) z = ChrList[ichr].level;

    ChrList[ichr].zpos = z;
    ChrList[ichr].oldz = z;
    ChrList[ichr].xstt = ChrList[ichr].xpos;
    ChrList[ichr].ystt = ChrList[ichr].ypos;
    ChrList[ichr].zstt = ChrList[ichr].zpos;
    ChrList[ichr].xvel = 0;
    ChrList[ichr].yvel = 0;
    ChrList[ichr].zvel = 0;
    ChrList[ichr].turnmaplr = 32768;  // These two mean on level surface
    ChrList[ichr].turnmapud = 32768;

    // action stuff
    ChrList[ichr].actionready = btrue;
    ChrList[ichr].keepaction = bfalse;
    ChrList[ichr].loopaction = bfalse;
    ChrList[ichr].action = ACTIONDA;
    ChrList[ichr].nextaction = ACTIONDA;
    ChrList[ichr].lip = 0;
    ChrList[ichr].frame = MadList[ChrList[ichr].model].framestart;
    ChrList[ichr].lastframe = ChrList[ichr].frame;

    ChrList[ichr].holdingweight = 0;

    // Timers set to 0
    ChrList[ichr].grogtime = 0;
    ChrList[ichr].dazetime = 0;

    // Money is added later
    ChrList[ichr].money = CapList[profile].money;

    // Name the character
    if ( name == NULL )
    {
        // Generate a random name
        sprintf( ChrList[ichr].name, "%s", chop_create( profile ) );
    }
    else
    {
        // A name has been given
        tnc = 0;

        while ( tnc < MAXCAPNAMESIZE - 1 )
        {
            ChrList[ichr].name[tnc] = name[tnc];
            tnc++;
        }

        ChrList[ichr].name[tnc] = 0;
    }

    // Set up initial fade in lighting
    for ( tnc = 0; tnc < MadList[ChrList[ichr].model].transvertices; tnc++ )
    {
        ChrList[ichr].vrta[tnc] = 0;
    }

    // Particle attachments
    tnc = 0;
    while ( tnc < CapList[profile].attachedprtamount )
    {
        spawn_one_particle( ChrList[ichr].xpos, ChrList[ichr].ypos, ChrList[ichr].zpos,
                            0, ChrList[ichr].model, CapList[profile].attachedprttype,
                            ichr, GRIP_LAST + tnc, ChrList[ichr].team, ichr, tnc, MAXCHR );
        tnc++;
    }

    ChrList[ichr].reaffirmdamagetype = CapList[profile].attachedprtreaffirmdamagetype;

    // Experience
    tnc = generate_number( CapList[profile].experiencebase, CapList[profile].experiencerand );
    if ( tnc > MAXXP ) tnc = MAXXP;

    ChrList[ichr].experience = tnc;
    ChrList[ichr].experiencelevel = CapList[profile].leveloverride;

    //Items that are spawned inside shop passages are more expensive than normal
    if (CapList[profile].isvaluable)
    {
        ChrList[ichr].isshopitem = btrue;
    }
    else
    {
        ChrList[ichr].isshopitem = bfalse;
        if (ChrList[ichr].isitem && !ChrList[ichr].inpack && ChrList[ichr].attachedto == MAXCHR)
        {
            float tlx, tly, brx, bry;
            Uint16 passage = 0;
            float bumpsize;

            bumpsize = ChrList[ichr].bumpsize;

            while (passage < numpassage)
            {
                // Passage area
                tlx = ( passtlx[passage] << 7 ) - CLOSETOLERANCE;
                tly = ( passtly[passage] << 7 ) - CLOSETOLERANCE;
                brx = ( ( passbrx[passage] + 1 ) << 7 ) + CLOSETOLERANCE;
                bry = ( ( passbry[passage] + 1 ) << 7 ) + CLOSETOLERANCE;

                //Check if the character is inside that passage
                if ( ChrList[ichr].xpos > tlx - bumpsize && ChrList[ichr].xpos < brx + bumpsize )
                {
                    if ( ChrList[ichr].ypos > tly - bumpsize && ChrList[ichr].ypos < bry + bumpsize )
                    {
                        //Yep, flag as valuable (does not export)
                        ChrList[ichr].isshopitem = btrue;
                        break;
                    }
                }

                passage++;
            }
        }
    }

    ChrList[ichr].on = btrue;

    return ichr;
}

//--------------------------------------------------------------------------------------------
void respawn_character( Uint16 character )
{
    // ZZ> This function respawns a character
    Uint16 item;
    if ( ChrList[character].alive ) return;

    spawn_poof( character, ChrList[character].model );
    disaffirm_attached_particles( character );
    ChrList[character].alive = btrue;
    ChrList[character].boretime = BORETIME;
    ChrList[character].carefultime = CAREFULTIME;
    ChrList[character].life = ChrList[character].lifemax;
    ChrList[character].mana = ChrList[character].manamax;
    ChrList[character].xpos = ChrList[character].xstt;
    ChrList[character].ypos = ChrList[character].ystt;
    ChrList[character].zpos = ChrList[character].zstt;
    ChrList[character].xvel = 0;
    ChrList[character].yvel = 0;
    ChrList[character].zvel = 0;
    ChrList[character].team = ChrList[character].baseteam;
    ChrList[character].canbecrushed = bfalse;
    ChrList[character].turnmaplr = 32768;  // These two mean on level surface
    ChrList[character].turnmapud = 32768;
    if ( TeamList[ChrList[character].team].leader == NOLEADER )  TeamList[ChrList[character].team].leader = character;
    if ( !ChrList[character].invictus )  TeamList[ChrList[character].baseteam].morale++;

    ChrList[character].actionready = btrue;
    ChrList[character].keepaction = bfalse;
    ChrList[character].loopaction = bfalse;
    ChrList[character].action = ACTIONDA;
    ChrList[character].nextaction = ACTIONDA;
    ChrList[character].lip = 0;
    ChrList[character].frame = MadList[ChrList[character].model].framestart;
    ChrList[character].lastframe = ChrList[character].frame;
    ChrList[character].platform = CapList[ChrList[character].model].platform;
    ChrList[character].flyheight = CapList[ChrList[character].model].flyheight;
    ChrList[character].bumpdampen = CapList[ChrList[character].model].bumpdampen;
    ChrList[character].bumpsize = CapList[ChrList[character].model].bumpsize * ChrList[character].fat;
    ChrList[character].bumpsizebig = CapList[ChrList[character].model].bumpsizebig * ChrList[character].fat;
    ChrList[character].bumpheight = CapList[ChrList[character].model].bumpheight * ChrList[character].fat;

    ChrList[character].bumpsizesave = CapList[ChrList[character].model].bumpsize;
    ChrList[character].bumpsizebigsave = CapList[ChrList[character].model].bumpsizebig;
    ChrList[character].bumpheightsave = CapList[ChrList[character].model].bumpheight;

    ChrList[character].ai.alert = 0;
    ChrList[character].ai.target = character;
    ChrList[character].ai.timer  = 0;

    ChrList[character].grogtime = 0;
    ChrList[character].dazetime = 0;
    reaffirm_attached_particles( character );

    // Let worn items come back
    for ( item = ChrList[character].nextinpack; item < MAXCHR; item = ChrList[item].nextinpack )
    {
        if ( ChrList[item].on && ChrList[item].isequipped )
        {
            ChrList[item].isequipped = bfalse;
            ChrList[item].ai.alert |= ALERTIF_ATLASTWAYPOINT;  // doubles as PutAway
        }
    }

}

//--------------------------------------------------------------------------------------------
Uint16 change_armor( Uint16 character, Uint16 skin )
{
    // ZZ> This function changes the armor of the character

    Uint16 enchant, sTmp;
    int iTmp;

    // Remove armor enchantments
    enchant = ChrList[character].firstenchant;

    while ( enchant < MAXENCHANT )
    {
        unset_enchant_value( enchant, SETSLASHMODIFIER );
        unset_enchant_value( enchant, SETCRUSHMODIFIER );
        unset_enchant_value( enchant, SETPOKEMODIFIER );
        unset_enchant_value( enchant, SETHOLYMODIFIER );
        unset_enchant_value( enchant, SETEVILMODIFIER );
        unset_enchant_value( enchant, SETFIREMODIFIER );
        unset_enchant_value( enchant, SETICEMODIFIER );
        unset_enchant_value( enchant, SETZAPMODIFIER );
        enchant = EncList[enchant].nextenchant;
    }

    // Change the skin
    sTmp = ChrList[character].model;
    if ( skin > MadList[sTmp].skins )  skin = 0;

    ChrList[character].skin    = skin;
    ChrList[character].texture = MadList[sTmp].skinstart + skin;

    // Change stats associated with skin
    ChrList[character].defense = CapList[sTmp].defense[skin];
    iTmp = 0;

    while ( iTmp < DAMAGE_COUNT )
    {
        ChrList[character].damagemodifier[iTmp] = CapList[sTmp].damagemodifier[iTmp][skin];
        iTmp++;
    }

    ChrList[character].maxaccel = CapList[sTmp].maxaccel[skin];

    // Reset armor enchantments
    // These should really be done in reverse order ( Start with last enchant ), but
    // I don't care at this point !!!BAD!!!
    enchant = ChrList[character].firstenchant;

    while ( enchant < MAXENCHANT )
    {
        set_enchant_value( enchant, SETSLASHMODIFIER, EncList[enchant].eve );
        set_enchant_value( enchant, SETCRUSHMODIFIER, EncList[enchant].eve );
        set_enchant_value( enchant, SETPOKEMODIFIER, EncList[enchant].eve );
        set_enchant_value( enchant, SETHOLYMODIFIER, EncList[enchant].eve );
        set_enchant_value( enchant, SETEVILMODIFIER, EncList[enchant].eve );
        set_enchant_value( enchant, SETFIREMODIFIER, EncList[enchant].eve );
        set_enchant_value( enchant, SETICEMODIFIER, EncList[enchant].eve );
        set_enchant_value( enchant, SETZAPMODIFIER, EncList[enchant].eve );
        add_enchant_value( enchant, ADDACCEL, EncList[enchant].eve );
        add_enchant_value( enchant, ADDDEFENSE, EncList[enchant].eve );
        enchant = EncList[enchant].nextenchant;
    }

    return skin;
}

//--------------------------------------------------------------------------------------------
void change_character( Uint16 ichr, Uint16 profile, Uint8 skin,
                       Uint8 leavewhich )
{
    // ZZ> This function polymorphs a character, changing stats, dropping weapons
    int tnc, enchant;
    Uint16 sTmp, item;

    profile = profile % MAXMODEL;
    if ( MadList[profile].used )
    {
        // Drop left weapon
        sTmp = ChrList[ichr].holdingwhich[0];
        if ( sTmp != MAXCHR && ( !CapList[profile].gripvalid[0] || CapList[profile].ismount ) )
        {
            detach_character_from_mount( sTmp, btrue, btrue );
            if ( ChrList[ichr].ismount )
            {
                ChrList[sTmp].zvel = DISMOUNTZVEL;
                ChrList[sTmp].zpos += DISMOUNTZVEL;
                ChrList[sTmp].jumptime = JUMPDELAY;
            }
        }

        // Drop right weapon
        sTmp = ChrList[ichr].holdingwhich[1];
        if ( sTmp != MAXCHR && !CapList[profile].gripvalid[1] )
        {
            detach_character_from_mount( sTmp, btrue, btrue );
            if ( ChrList[ichr].ismount )
            {
                ChrList[sTmp].zvel = DISMOUNTZVEL;
                ChrList[sTmp].zpos += DISMOUNTZVEL;
                ChrList[sTmp].jumptime = JUMPDELAY;
            }
        }

        // Remove particles
        disaffirm_attached_particles( ichr );

        // Remove enchantments
        if ( leavewhich == LEAVEFIRST )
        {
            // Remove all enchantments except top one
            enchant = ChrList[ichr].firstenchant;
            if ( enchant != MAXENCHANT )
            {
                while ( EncList[enchant].nextenchant != MAXENCHANT )
                {
                    remove_enchant( EncList[enchant].nextenchant );
                }
            }
        }

        if ( leavewhich == LEAVENONE )
        {
            // Remove all enchantments
            disenchant_character( ichr );
        }

        // Stuff that must be set
        ChrList[ichr].model = profile;
        ChrList[ichr].stoppedby = CapList[profile].stoppedby;
        ChrList[ichr].lifeheal = CapList[profile].lifeheal;
        ChrList[ichr].manacost = CapList[profile].manacost;

        // Ammo
        ChrList[ichr].ammomax = CapList[profile].ammomax;
        ChrList[ichr].ammo = CapList[profile].ammo;

        // Gender
        if ( CapList[profile].gender != GENRANDOM )  // GENRANDOM means keep old gender
        {
            ChrList[ichr].gender = CapList[profile].gender;
        }

        for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
        {
            ChrList[ichr].soundindex[tnc] = CapList[profile].soundindex[tnc];
        }

        // AI stuff
        ChrList[ichr].ai.type = MadList[profile].ai;
        ChrList[ichr].ai.state = 0;
        ChrList[ichr].ai.timer = 0;

        ChrList[ichr].latchx = 0;
        ChrList[ichr].latchy = 0;
        ChrList[ichr].latchbutton = 0;
        ChrList[ichr].turnmode = TURNMODEVELOCITY;
        // Flags
        ChrList[ichr].stickybutt = CapList[profile].stickybutt;
        ChrList[ichr].openstuff = CapList[profile].canopenstuff;
        ChrList[ichr].transferblend = CapList[profile].transferblend;
        ChrList[ichr].enviro = CapList[profile].enviro;
        ChrList[ichr].platform = CapList[profile].platform;
        ChrList[ichr].isitem = CapList[profile].isitem;
        ChrList[ichr].invictus = CapList[profile].invictus;
        ChrList[ichr].ismount = CapList[profile].ismount;
        ChrList[ichr].cangrabmoney = CapList[profile].cangrabmoney;
        ChrList[ichr].jumptime = JUMPDELAY;
        // Character size and bumping
        ChrList[ichr].shadowsize = (Uint8)(CapList[profile].shadowsize * ChrList[ichr].fat);
        ChrList[ichr].bumpsize = (Uint8) (CapList[profile].bumpsize * ChrList[ichr].fat);
        ChrList[ichr].bumpsizebig = CapList[profile].bumpsizebig * ChrList[ichr].fat;
        ChrList[ichr].bumpheight = CapList[profile].bumpheight * ChrList[ichr].fat;

        ChrList[ichr].shadowsizesave = CapList[profile].shadowsize;
        ChrList[ichr].bumpsizesave = CapList[profile].bumpsize;
        ChrList[ichr].bumpsizebigsave = CapList[profile].bumpsizebig;
        ChrList[ichr].bumpheightsave = CapList[profile].bumpheight;

        ChrList[ichr].bumpdampen = CapList[profile].bumpdampen;

        if ( CapList[profile].weight == 0xFF )
        {
            ChrList[ichr].weight = 0xFFFF;
        }
        else
        {
            int itmp = CapList[profile].weight * ChrList[ichr].fat * ChrList[ichr].fat * ChrList[ichr].fat;
            ChrList[ichr].weight = MIN( itmp, 0xFFFE );
        }

        // Character scales...  Magic numbers
        if ( ChrList[ichr].attachedto != MAXCHR )
        {
            int i;
            Uint16 iholder = ChrList[ichr].attachedto;
            tnc = MadList[ChrList[iholder].model].vertices - (ChrList[ichr].inwhichhand + 1) * GRIP_VERTS;

            for (i = 0; i < GRIP_VERTS; i++)
            {
                if (tnc + i < MadList[ChrList[iholder].model].vertices )
                {
                    ChrList[ichr].weapongrip[i] = tnc + i;
                }
                else
                {
                    ChrList[ichr].weapongrip[i] = 0xFFFF;
                }
            }
        }

        item = ChrList[ichr].holdingwhich[0];
        if ( item != MAXCHR )
        {
            int i;

            tnc = MadList[ChrList[ichr].model].vertices - GRIP_LEFT;

            for (i = 0; i < GRIP_VERTS; i++)
            {
                if (tnc + i < MadList[ChrList[ichr].model].vertices )
                {
                    ChrList[item].weapongrip[i] = i + tnc;
                }
                else
                {
                    ChrList[item].weapongrip[i] = 0xFFFF;
                }
            }
        }

        item = ChrList[ichr].holdingwhich[1];
        if ( item != MAXCHR )
        {
            int i;

            tnc = MadList[ChrList[ichr].model].vertices - GRIP_RIGHT;

            for (i = 0; i < GRIP_VERTS; i++)
            {
                if (tnc + i < MadList[ChrList[ichr].model].vertices )
                {
                    ChrList[item].weapongrip[i] = i + tnc;
                }
                else
                {
                    ChrList[item].weapongrip[i] = 0xFFFF;
                }
            }
        }

        // Image rendering
        ChrList[ichr].uoffset = 0;
        ChrList[ichr].voffset = 0;
        ChrList[ichr].uoffvel = CapList[profile].uoffvel;
        ChrList[ichr].voffvel = CapList[profile].voffvel;

        // Movement
        ChrList[ichr].sneakspd = CapList[profile].sneakspd;
        ChrList[ichr].walkspd = CapList[profile].walkspd;
        ChrList[ichr].runspd = CapList[profile].runspd;

        // AI and action stuff
        ChrList[ichr].actionready = btrue;
        ChrList[ichr].keepaction = bfalse;
        ChrList[ichr].loopaction = bfalse;
        ChrList[ichr].action = ACTIONDA;
        ChrList[ichr].nextaction = ACTIONDA;
        ChrList[ichr].lip = 0;
        ChrList[ichr].frame = MadList[profile].framestart;
        ChrList[ichr].lastframe = ChrList[ichr].frame;
        ChrList[ichr].holdingweight = 0;

        // Set the skin
        change_armor( ichr, skin );

        // Reaffirm them particles...
        ChrList[ichr].reaffirmdamagetype = CapList[profile].attachedprtreaffirmdamagetype;
        reaffirm_attached_particles( ichr );

        // Set up initial fade in lighting
        for ( tnc = 0; tnc < MadList[ChrList[ichr].model].transvertices; tnc++ )
        {
            ChrList[ichr].vrta[tnc] = 0;
        }
    }
}

//--------------------------------------------------------------------------------------------
/*Uint16 get_target_in_block( int x, int y, Uint16 character, char items,
                            char friends, char enemies, char dead, char seeinvisible, IDSZ idsz,
                            char excludeid )
{
  // ZZ> This is a good little helper, that returns != MAXCHR if a suitable target
  //     was found
  int cnt;
  Uint16 charb;
  Uint32 fanblock;
  Uint8 team;
  if ( x >= 0 && x < meshbloksx && y >= 0 && y < meshbloksy )
  {
    team = ChrList[character].team;
    fanblock = mesh_get_block_int(&mesh, x,y);
    charb = bumplist[fanblock].chr;
    cnt = 0;
    while ( cnt < bumplist[fanblock].chrnum )
    {
      if ( dead != ChrList[charb].alive && ( seeinvisible || ( ChrList[charb].alpha > INVISIBLE && ChrList[charb].light > INVISIBLE ) ) )
      {
        if ( ( enemies && TeamList[team].hatesteam[ChrList[charb].team] && !ChrList[charb].invictus ) ||
             ( items && ChrList[charb].isitem ) ||
             ( friends && ChrList[charb].baseteam == team ) )
        {
          if ( charb != character && ChrList[character].attachedto != charb )
          {
            if ( !ChrList[charb].isitem || items )
            {
              if ( idsz != IDSZ_NONE )
              {
                if ( CapList[ChrList[charb].model].idsz[IDSZ_PARENT] == idsz ||
                     CapList[ChrList[charb].model].idsz[IDSZ_TYPE] == idsz )
                {
                  if ( !excludeid ) return charb;
                }
                else
                {
                  if ( excludeid )  return charb;
                }
              }
              else
              {
                return charb;
              }
            }
          }
        }
      }
      charb = ChrList[charb].bumpnext;
      cnt++;
    }
  }
  return MAXCHR;
}*/

//--------------------------------------------------------------------------------------------
/*Uint16 get_nearby_target( Uint16 character, char items,
                          char friends, char enemies, char dead, IDSZ idsz )
{
  // ZZ> This function finds a nearby target, or it returns MAXCHR if it can't find one
  int x, y;
  char seeinvisible;
  seeinvisible = ChrList[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )ChrList[character].xpos ) >> 9;
  y = ( ( int )ChrList[character].ypos ) >> 9;
  return get_target_in_block( x, y, character, items, friends, enemies, dead, seeinvisible, idsz, 0 );
}*/

//--------------------------------------------------------------------------------------------
Uint8 cost_mana( Uint16 character, int amount, Uint16 killer )
{
    // ZZ> This function takes mana from a character ( or gives mana ),
    //     and returns btrue if the character had enough to pay, or bfalse
    //     otherwise
    int iTmp;

    iTmp = ChrList[character].mana - amount;
    if ( iTmp < 0 )
    {
        ChrList[character].mana = 0;
        if ( ChrList[character].canchannel )
        {
            ChrList[character].life += iTmp;
            if ( ChrList[character].life <= 0 )
            {
                kill_character( character, (killer == MAXCHR) ? character : killer );
            }

            return btrue;
        }

        return bfalse;
    }
    else
    {
        ChrList[character].mana = iTmp;
        if ( iTmp > ChrList[character].manamax )
        {
            ChrList[character].mana = ChrList[character].manamax;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
/*Uint16 find_distant_target( Uint16 character, int maxdistance )
{
  // ZZ> This function finds a target, or it returns MAXCHR if it can't find one...
  //     maxdistance should be the square of the actual distance you want to use
  //     as the cutoff...
  int cnt, distance, xdistance, ydistance;
  Uint8 team;

  team = ChrList[character].team;
  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( ChrList[cnt].on )
    {
      if ( ChrList[cnt].attachedto == MAXCHR && !ChrList[cnt].inpack )
      {
        if ( TeamList[team].hatesteam[ChrList[cnt].team] && ChrList[cnt].alive && !ChrList[cnt].invictus )
        {
          if ( ChrList[character].canseeinvisible || ( ChrList[cnt].alpha > INVISIBLE && ChrList[cnt].light > INVISIBLE ) )
          {
            xdistance = (int) (ChrList[cnt].xpos - ChrList[character].xpos);
            ydistance = (int) (ChrList[cnt].ypos - ChrList[character].ypos);
            distance = xdistance * xdistance + ydistance * ydistance;
            if ( distance < maxdistance )
            {
              return cnt;
            }
          }
        }
      }
    }
    cnt++;
  }
  return MAXCHR;
}*/

//--------------------------------------------------------------------------------------------
void switch_team( Uint16 character, Uint8 team )
{
    // ZZ> This function makes a character join another team...
    if ( team < MAXTEAM )
    {
        if ( !ChrList[character].invictus )
        {
            TeamList[ChrList[character].baseteam].morale--;
            TeamList[team].morale++;
        }
        if ( ( !ChrList[character].ismount || ChrList[character].holdingwhich[0] == MAXCHR ) &&
                ( !ChrList[character].isitem || ChrList[character].attachedto == MAXCHR ) )
        {
            ChrList[character].team = team;
        }

        ChrList[character].baseteam = team;
        if ( TeamList[team].leader == NOLEADER )
        {
            TeamList[team].leader = character;
        }
    }
}

//--------------------------------------------------------------------------------------------
/*void get_nearest_in_block( int x, int y, Uint16 character, char items,
                           char friends, char enemies, char dead, char seeinvisible, IDSZ idsz )
{
  // ZZ> This is a good little helper
  float distance, xdis, ydis;
  int cnt;
  Uint8 team;
  Uint16 charb;
  Uint32 fanblock;
  if ( x >= 0 && x < meshbloksx && y >= 0 && y < meshbloksy )
  {
    team = ChrList[character].team;
    fanblock = mesh_get_block_int(&mesh, x,y);
    charb = bumplist[fanblock].chr;
    cnt = 0;
    while ( cnt < bumplist[fanblock].chrnum )
    {
      if ( dead != ChrList[charb].alive && ( seeinvisible || ( ChrList[charb].alpha > INVISIBLE && ChrList[charb].light > INVISIBLE ) ) )
      {
        if ( ( enemies && TeamList[team].hatesteam[ChrList[charb].team] ) ||
             ( items && ChrList[charb].isitem ) ||
             ( friends && ChrList[charb].team == team ) ||
             ( friends && enemies ) )
        {
          if ( charb != character && ChrList[character].attachedto != charb && ChrList[charb].attachedto == MAXCHR && !ChrList[charb].inpack )
          {
            if ( !ChrList[charb].invictus || items )
            {
              if ( idsz != IDSZ_NONE )
              {
                if ( CapList[ChrList[charb].model].idsz[IDSZ_PARENT] == idsz ||
                     CapList[ChrList[charb].model].idsz[IDSZ_TYPE] == idsz )
                {
                  xdis = ChrList[character].xpos - ChrList[charb].xpos;
                  ydis = ChrList[character].ypos - ChrList[charb].ypos;
                  xdis = xdis * xdis;
                  ydis = ydis * ydis;
                  distance = xdis + ydis;
                  if ( distance < globaldistance )
                  {
                    globalnearest = charb;
                    globaldistance = distance;
                  }
                }
              }
              else
              {
                xdis = ChrList[character].xpos - ChrList[charb].xpos;
                ydis = ChrList[character].ypos - ChrList[charb].ypos;
                xdis = xdis * xdis;
                ydis = ydis * ydis;
                distance = xdis + ydis;
                if ( distance < globaldistance )
                {
                  globalnearest = charb;
                  globaldistance = distance;
                }
              }
            }
          }
        }
      }
      charb = ChrList[charb].bumpnext;
      cnt++;
    }
  }
  return;
}*/

//--------------------------------------------------------------------------------------------
/*Uint16 get_nearest_target( Uint16 character, char items,
                           char friends, char enemies, char dead, IDSZ idsz )
{
  // ZZ> This function finds a target, or it returns MAXCHR if it can't find one
  int x, y;
  char seeinvisible;
  seeinvisible = ChrList[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )ChrList[character].xpos ) >> 9;
  y = ( ( int )ChrList[character].ypos ) >> 9;

  globalnearest = MAXCHR;
  globaldistance = 999999;
  get_nearest_in_block( x, y, character, items, friends, enemies, dead, seeinvisible, idsz );

  get_nearest_in_block( x - 1, y, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x + 1, y, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz );

  get_nearest_in_block( x - 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x + 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x - 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x + 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz );
  return globalnearest;
}*/

//--------------------------------------------------------------------------------------------
/*Uint16 get_wide_target( Uint16 character, char items,
                        char friends, char enemies, char dead, IDSZ idsz, char excludeid )
{
  // ZZ> This function finds a target, or it returns MAXCHR if it can't find one
  int x, y;
  Uint16 enemy;
  char seeinvisible;
  seeinvisible = ChrList[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )ChrList[character].xpos ) >> 9;
  y = ( ( int )ChrList[character].ypos ) >> 9;
  enemy = get_target_in_block( x, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;

  enemy = get_target_in_block( x - 1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x + 1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;

  enemy = get_target_in_block( x - 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x + 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x - 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x + 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  return enemy;
}*/

//--------------------------------------------------------------------------------------------
void issue_clean( Uint16 character )
{
    // ZZ> This function issues a clean up order to all teammates
    Uint8 team;
    Uint16 cnt;

    team = ChrList[character].team;
    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( !ChrList[cnt].on || team != ChrList[cnt].team ) continue;

        ChrList[cnt].ai.timer  = frame_wld + 2;  // Don't let it think too much...
        ChrList[cnt].ai.alert |= ALERTIF_CLEANEDUP;
    }
}

//--------------------------------------------------------------------------------------------
int restock_ammo( Uint16 character, IDSZ idsz )
{
    // ZZ> This function restocks the characters ammo, if it needs ammo and if
    //     either its parent or type idsz match the given idsz.  This
    //     function returns the amount of ammo given.
    int amount, model;

    amount = 0;
    if ( character < MAXCHR )
    {
        if ( ChrList[character].on )
        {
            model = ChrList[character].model;
            if ( CapList[model].idsz[IDSZ_PARENT] == idsz || CapList[model].idsz[IDSZ_TYPE] == idsz )
            {
                if ( ChrList[character].ammo < ChrList[character].ammomax )
                {
                    amount = ChrList[character].ammomax - ChrList[character].ammo;
                    ChrList[character].ammo = ChrList[character].ammomax;
                }
            }
        }
    }

    return amount;
}

//--------------------------------------------------------------------------------------------
bool_t add_quest_idsz( const char *whichplayer, IDSZ idsz )
{
    /// @details ZF@> This function writes a IDSZ (With quest level 0) into a player quest.txt file, returns btrue if succeeded

    FILE *filewrite;
    STRING newloadname;

    // Only add quest IDSZ if it doesnt have it already
    if (check_player_quest(whichplayer, idsz) >= QUEST_BEATEN) return bfalse;

    // Try to open the file in read and append mode
    snprintf(newloadname, sizeof(newloadname), "players/%s/quest.txt", get_file_path(whichplayer) );
    filewrite = fopen( newloadname, "a" );
    if ( !filewrite )
    {
        //Create the file if it does not exist
        filewrite = fopen( newloadname, "w" );
        if (!filewrite)
        {
            log_warning("Cannot write to %s!\n", newloadname);
            return bfalse;
        }

        fprintf( filewrite, "//This file keeps order of all the quests for the player (%s)\n", whichplayer);
        fprintf( filewrite, "//The number after the IDSZ shows the quest level. -1 means it is completed.");
    }

    fprintf( filewrite, "\n:[%4s] 0", undo_idsz( idsz ));
    fclose( filewrite );

    return btrue;
}

//--------------------------------------------------------------------------------------------
Sint16 modify_quest_idsz( const char *whichplayer, IDSZ idsz, Sint16 adjustment )
{
    /// @details ZF@> This function increases or decreases a Quest IDSZ quest level by the amount determined in
    ///     adjustment. It then returns the current quest level it now has.
    ///     It returns QUEST_NONE if failed and if the adjustment is 0, the quest is marked as beaten...

    FILE *filewrite, *fileread;
    STRING newloadname, copybuffer;
    IDSZ newidsz;
    Sint8 NewQuestLevel = QUEST_NONE, QuestLevel;

    //Now check each expansion until we find correct IDSZ
    if (check_player_quest(whichplayer, idsz) <= QUEST_BEATEN || adjustment == 0)  return NewQuestLevel;
    else
    {
        // modify the CData.quest_file
        char ctmp;

        // create a "tmp_*" copy of the file
        snprintf( newloadname, sizeof( newloadname ), "players/%s/quest.txt", get_file_path(whichplayer));
        snprintf( copybuffer, sizeof( copybuffer ), "players/%s/tmp_quest.txt", whichplayer);
        fs_copyFile( newloadname, copybuffer );

        // open the tmp file for reading and overwrite the original file
        fileread  = fopen( copybuffer, "r" );
        filewrite = fopen( newloadname, "w" );

        //Something went wrong
        if (!fileread || !filewrite)
        {
            log_warning("Could not modify quest IDSZ (%s).\n", newloadname);
            return QUEST_NONE;
        }

        // read the tmp file line-by line
        while ( !feof(fileread) )
        {
            ctmp = fgetc(fileread);
            ungetc(ctmp, fileread);
            if ( ctmp == '/' )
            {
                // copy comments exactly
                fcopy_line(fileread, filewrite);
            }
            else if ( goto_colon_yesno( fileread ) )
            {
                // scan the line for quest info
                newidsz = fget_idsz( fileread );
                fget_first_letter( fileread );      //Skip the ] bracket
                QuestLevel = fget_int( fileread );

                // modify it
                if ( newidsz == idsz )
                {
                    QuestLevel += adjustment;
                    if (QuestLevel < 0) QuestLevel = 0;   //Don't get negative

                    NewQuestLevel = QuestLevel;
                }

                fprintf(filewrite, "\n:[%s] %i", undo_idsz(newidsz), QuestLevel);
            }
        }
    }

    //clean it up
    fclose( fileread );
    fclose( filewrite );
    fs_deleteFile( copybuffer );

    return NewQuestLevel;
}

//--------------------------------------------------------------------------------------------
char * undo_idsz( IDSZ idsz )
{
    // ZZ> This function takes an integer and makes a text IDSZ out of it.

    static char value_string[5] = {"NONE"};
    if ( idsz == IDSZ_NONE )
    {
        sprintf( idsz_string, "NONE" );
        snprintf( value_string, sizeof( value_string ), "NONE" );
    }
    else
    {
        idsz_string[0] = ( ( idsz >> 15 ) & 31 ) + 'A';
        idsz_string[1] = ( ( idsz >> 10 ) & 31 ) + 'A';
        idsz_string[2] = ( ( idsz >> 5 ) & 31 ) + 'A';
        idsz_string[3] = ( ( idsz ) & 31 ) + 'A';
        idsz_string[4] = 0;

        //Bad! both function return and return to global variable!
        value_string[0] = (( idsz >> 15 ) & 31 ) + 'A';
        value_string[1] = (( idsz >> 10 ) & 31 ) + 'A';
        value_string[2] = (( idsz >> 5 ) & 31 ) + 'A';
        value_string[3] = (( idsz ) & 31 ) + 'A';
        value_string[4] = 0;
    }

    return value_string;
}

//--------------------------------------------------------------------------------------------
Sint16 check_player_quest( const char *whichplayer, IDSZ idsz )
{
    /// @details ZF@> This function checks if the specified player has the IDSZ in his or her quest.txt
    /// and returns the quest level of that specific quest (Or QUEST_NONE if it is not found, QUEST_BEATEN if it is finished)

    FILE *fileread;
    STRING newloadname;
    IDSZ newidsz;
    bool_t foundidsz = bfalse;
    Sint8 result = QUEST_NONE;

    snprintf( newloadname, sizeof(newloadname), "players/%s/quest.txt", get_file_path(whichplayer) );
    fileread = fopen( newloadname, "r" );
    if ( NULL == fileread ) return result;

    //Always return "true" for [NONE] IDSZ checks
    if (idsz == IDSZ_NONE) result = QUEST_BEATEN;

    // Check each expansion
    while ( !foundidsz && goto_colon_yesno( fileread ) )
    {
        newidsz = fget_idsz( fileread );
        if ( newidsz == idsz )
        {
            foundidsz = btrue;
            fget_first_letter( fileread );   //Skip the ] bracket
            result = fget_int( fileread );  //Read value behind colon and IDSZ
        }
    }

    fclose( fileread );

    return result;
}

//--------------------------------------------------------------------------------------------
int check_skills( Uint16 who, IDSZ whichskill )
{
    // @details ZF@> This checks if the specified character has the required skill. Returns the level
    // of the skill. Also checks Skill expansions.

    bool_t result = bfalse;

    // First check the character Skill ID matches
    // Then check for expansion skills too.
    if ( CapList[ChrList[who].model].idsz[IDSZ_SKILL]  == whichskill ) result = btrue;
    else if ( Make_IDSZ( "AWEP" ) == whichskill ) result = ChrList[who].canuseadvancedweapons;
    else if ( Make_IDSZ( "CKUR" ) == whichskill ) result = ChrList[who].canseekurse;
    else if ( Make_IDSZ( "JOUS" ) == whichskill ) result = ChrList[who].canjoust;
    else if ( Make_IDSZ( "SHPR" ) == whichskill ) result = ChrList[who].shieldproficiency;
    else if ( Make_IDSZ( "TECH" ) == whichskill ) result = ChrList[who].canusetech;
    else if ( Make_IDSZ( "WMAG" ) == whichskill ) result = ChrList[who].canusearcane;
    else if ( Make_IDSZ( "HMAG" ) == whichskill ) result = ChrList[who].canusedivine;
    else if ( Make_IDSZ( "DISA" ) == whichskill ) result = ChrList[who].candisarm;
    else if ( Make_IDSZ( "STAB" ) == whichskill ) result = ChrList[who].canbackstab;
    else if ( Make_IDSZ( "POIS" ) == whichskill ) result = ChrList[who].canusepoison;
    else if ( Make_IDSZ( "READ" ) == whichskill ) result = ChrList[who].canread;

    return result;
}

//--------------------------------------------------------------------------------------------
void move_characters( void )
{
    // ZZ> This function handles character physics
    Uint16 cnt;
    Uint32 mapud, maplr;
    Uint8 twist, actionready;
    Uint8 speed, framelip, allowedtoattack;
    float level, friction;
    float dvx, dvy, dvmax;
    Uint16 action, weapon, mount, item;
    int distance, volume;
    bool_t watchtarget, grounded;

    // Move every character
    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( !ChrList[cnt].on || ChrList[cnt].inpack ) continue;

        grounded = bfalse;

        // Down that ol' damage timer
        ChrList[cnt].damagetime -= ( ChrList[cnt].damagetime != 0 );

        // Character's old location
        ChrList[cnt].oldx = ChrList[cnt].xpos;
        ChrList[cnt].oldy = ChrList[cnt].ypos;
        ChrList[cnt].oldz = ChrList[cnt].zpos;
        ChrList[cnt].oldturn = ChrList[cnt].turnleftright;
        //            if(ChrList[cnt].attachedto!=MAXCHR)
        //            {
        //                ChrList[cnt].turnleftright = ChrList[ChrList[cnt].attachedto].turnleftright;
        //                if(ChrList[cnt].indolist==bfalse)
        //                {
        //                    ChrList[cnt].xpos = ChrList[ChrList[cnt].attachedto].xpos;
        //                    ChrList[cnt].ypos = ChrList[ChrList[cnt].attachedto].ypos;
        //                    ChrList[cnt].zpos = ChrList[ChrList[cnt].attachedto].zpos;
        //                }
        //            }

        // Texture movement
        ChrList[cnt].uoffset += ChrList[cnt].uoffvel;
        ChrList[cnt].voffset += ChrList[cnt].voffvel;
        if ( ChrList[cnt].alive )
        {
            if ( ChrList[cnt].attachedto == MAXCHR )
            {
                // Character latches for generalized movement
                dvx = ChrList[cnt].latchx;
                dvy = ChrList[cnt].latchy;

                // Reverse movements for daze
                if ( ChrList[cnt].dazetime > 0 )
                {
                    dvx = -dvx;
                    dvy = -dvy;
                }

                // Switch x and y for daze
                if ( ChrList[cnt].grogtime > 0 )
                {
                    dvmax = dvx;
                    dvx = dvy;
                    dvy = dvmax;
                }

                // Get direction from the DESIRED change in velocity
                if ( ChrList[cnt].turnmode == TURNMODEWATCH )
                {
                    if ( ( ABS( dvx ) > WATCHMIN || ABS( dvy ) > WATCHMIN ) )
                    {
                        ChrList[cnt].turnleftright = terp_dir( ChrList[cnt].turnleftright, ( ATAN2( dvy, dvx ) + PI ) * 0xFFFF / ( TWO_PI ) );
                    }
                }

                // Face the target
                watchtarget = ( ChrList[cnt].turnmode == TURNMODEWATCHTARGET );
                if ( watchtarget )
                {
                    if ( cnt != ChrList[cnt].ai.target )
                        ChrList[cnt].turnleftright = terp_dir( ChrList[cnt].turnleftright, ( ATAN2( ChrList[ChrList[cnt].ai.target].ypos - ChrList[cnt].ypos, ChrList[ChrList[cnt].ai.target].xpos - ChrList[cnt].xpos ) + PI ) * 0xFFFF / ( TWO_PI ) );
                }
                if ( Md2FrameList[ChrList[cnt].frame].framefx&MADFXSTOP )
                {
                    dvx = 0;
                    dvy = 0;
                }
                else
                {
                    // Limit to max acceleration
                    dvmax = ChrList[cnt].maxaccel;
                    if ( dvx < -dvmax ) dvx = -dvmax;
                    if ( dvx > dvmax ) dvx = dvmax;
                    if ( dvy < -dvmax ) dvy = -dvmax;
                    if ( dvy > dvmax ) dvy = dvmax;

                    ChrList[cnt].xvel += dvx;
                    ChrList[cnt].yvel += dvy;
                }

                // Get direction from ACTUAL change in velocity
                if ( ChrList[cnt].turnmode == TURNMODEVELOCITY )
                {
                    if ( dvx < -TURNSPD || dvx > TURNSPD || dvy < -TURNSPD || dvy > TURNSPD )
                    {
                        if ( ChrList[cnt].isplayer )
                        {
                            // Players turn quickly
                            ChrList[cnt].turnleftright = terp_dir_fast( ChrList[cnt].turnleftright, ( ATAN2( dvy, dvx ) + PI ) * 0xFFFF / ( TWO_PI ) );
                        }
                        else
                        {
                            // AI turn slowly
                            ChrList[cnt].turnleftright = terp_dir( ChrList[cnt].turnleftright, ( ATAN2( dvy, dvx ) + PI ) * 0xFFFF / ( TWO_PI ) );
                        }
                    }
                }

                // Otherwise make it spin
                else if ( ChrList[cnt].turnmode == TURNMODESPIN )
                {
                    ChrList[cnt].turnleftright += SPINRATE;
                }
            }

            // Character latches for generalized buttons
            if ( ChrList[cnt].latchbutton != 0 )
            {
                if ( 0 != (ChrList[cnt].latchbutton & LATCHBUTTON_JUMP) )
                {
                    if ( ChrList[cnt].attachedto != MAXCHR && ChrList[cnt].jumptime == 0 )
                    {
                        int ijump;

                        detach_character_from_mount( cnt, btrue, btrue );
                        ChrList[cnt].jumptime = JUMPDELAY;
                        ChrList[cnt].zvel = DISMOUNTZVEL;
                        if ( ChrList[cnt].flyheight != 0 )
                            ChrList[cnt].zvel = DISMOUNTZVELFLY;

                        ChrList[cnt].zpos += ChrList[cnt].zvel;
                        if ( ChrList[cnt].jumpnumberreset != JUMPINFINITE && ChrList[cnt].jumpnumber != 0 )
                            ChrList[cnt].jumpnumber--;

                        // Play the jump sound
                        ijump = CapList[ChrList[cnt].model].soundindex[SOUND_JUMP];
                        if ( ijump >= 0 && ijump < MAXWAVE )
                        {
                            sound_play_chunk( ChrList[cnt].xpos, ChrList[cnt].ypos, CapList[ChrList[cnt].model].wavelist[ijump] );
                        }

                    }
                    if ( ChrList[cnt].jumptime == 0 && ChrList[cnt].jumpnumber != 0 && ChrList[cnt].flyheight == 0 )
                    {
                        if ( ChrList[cnt].jumpnumberreset != 1 || ChrList[cnt].jumpready )
                        {
                            // Make the character jump
                            ChrList[cnt].hitready = btrue;
                            if ( ChrList[cnt].inwater )
                            {
                                ChrList[cnt].zvel = WATERJUMP;
                            }
                            else
                            {
                                ChrList[cnt].zvel = ChrList[cnt].jump;
                            }

                            ChrList[cnt].jumptime = JUMPDELAY;
                            ChrList[cnt].jumpready = bfalse;
                            if ( ChrList[cnt].jumpnumberreset != JUMPINFINITE ) ChrList[cnt].jumpnumber--;

                            // Set to jump animation if not doing anything better
                            if ( ChrList[cnt].actionready )    chr_play_action( cnt, ACTIONJA, btrue );

                            // Play the jump sound (Boing!)
                            distance = ABS( gCamera.trackx - ChrList[cnt].xpos ) + ABS( gCamera.tracky - ChrList[cnt].ypos );
                            volume = -distance;
                            if ( volume > VOLMIN )
                            {
                                int ijump = CapList[ChrList[cnt].model].soundindex[SOUND_JUMP];
                                if ( ijump >= 0 && ijump < MAXWAVE )
                                {
                                    sound_play_chunk( ChrList[cnt].xpos, ChrList[cnt].ypos, CapList[ChrList[cnt].model].wavelist[ijump] );
                                }
                            }

                        }
                    }
                }
                if ( 0 != ( ChrList[cnt].latchbutton & LATCHBUTTON_ALTLEFT ) && ChrList[cnt].actionready && ChrList[cnt].reloadtime == 0 )
                {
                    ChrList[cnt].reloadtime = GRABDELAY;
                    if ( ChrList[cnt].holdingwhich[0] == MAXCHR )
                    {
                        // Grab left
                        chr_play_action( cnt, ACTIONME, bfalse );
                    }
                    else
                    {
                        // Drop left
                        chr_play_action( cnt, ACTIONMA, bfalse );
                    }
                }
                if ( 0 != ( ChrList[cnt].latchbutton & LATCHBUTTON_ALTRIGHT ) && ChrList[cnt].actionready && ChrList[cnt].reloadtime == 0 )
                {
                    ChrList[cnt].reloadtime = GRABDELAY;
                    if ( ChrList[cnt].holdingwhich[1] == MAXCHR )
                    {
                        // Grab right
                        chr_play_action( cnt, ACTIONMF, bfalse );
                    }
                    else
                    {
                        // Drop right
                        chr_play_action( cnt, ACTIONMB, bfalse );
                    }
                }
                if ( 0 != ( ChrList[cnt].latchbutton & LATCHBUTTON_PACKLEFT ) && ChrList[cnt].actionready && ChrList[cnt].reloadtime == 0 )
                {
                    ChrList[cnt].reloadtime = PACKDELAY;
                    item = ChrList[cnt].holdingwhich[0];
                    if ( item != MAXCHR )
                    {
                        if ( ( ChrList[item].iskursed || CapList[ChrList[item].model].istoobig ) && !CapList[ChrList[item].model].isequipment )
                        {
                            // The item couldn't be put away
                            ChrList[item].ai.alert |= ALERTIF_NOTPUTAWAY;
                        }
                        else
                        {
                            // Put the item into the pack
                            pack_add_item( item, cnt );
                        }
                    }
                    else
                    {
                        // Get a new one out and put it in hand
                        pack_get_item( cnt, GRIP_LEFT, bfalse );
                    }

                    // Make it take a little time
                    chr_play_action( cnt, ACTIONMG, bfalse );
                }
                if ( 0 != ( ChrList[cnt].latchbutton & LATCHBUTTON_PACKRIGHT ) && ChrList[cnt].actionready && ChrList[cnt].reloadtime == 0 )
                {
                    ChrList[cnt].reloadtime = PACKDELAY;
                    item = ChrList[cnt].holdingwhich[1];
                    if ( item != MAXCHR )
                    {
                        if ( ( ChrList[item].iskursed || CapList[ChrList[item].model].istoobig ) && !CapList[ChrList[item].model].isequipment )
                        {
                            // The item couldn't be put away
                            ChrList[item].ai.alert |= ALERTIF_NOTPUTAWAY;
                        }
                        else
                        {
                            // Put the item into the pack
                            pack_add_item( item, cnt );
                        }
                    }
                    else
                    {
                        // Get a new one out and put it in hand
                        pack_get_item( cnt, GRIP_RIGHT, bfalse );
                    }

                    // Make it take a little time
                    chr_play_action( cnt, ACTIONMG, bfalse );
                }
                if ( 0 != ( ChrList[cnt].latchbutton & LATCHBUTTON_LEFT ) && ChrList[cnt].reloadtime == 0 )
                {
                    // Which weapon?
                    weapon = ChrList[cnt].holdingwhich[0];
                    if ( weapon == MAXCHR )
                    {
                        // Unarmed means character itself is the weapon
                        weapon = cnt;
                    }

                    action = CapList[ChrList[weapon].model].weaponaction;

                    // Can it do it?
                    allowedtoattack = btrue;

                    // First check if reload time and action is okay
                    if ( !MadList[ChrList[cnt].model].actionvalid[action] || ChrList[weapon].reloadtime > 0 ) allowedtoattack = bfalse;
                    else
                    {
                        // Then check if a skill is needed
                        if ( CapList[ChrList[weapon].model].needskillidtouse )
                        {
                            if (check_skills( cnt, CapList[ChrList[weapon].model].idsz[IDSZ_SKILL]) == bfalse )
                                allowedtoattack = bfalse;
                        }
                    }
                    if ( !allowedtoattack )
                    {
                        if ( ChrList[weapon].reloadtime == 0 )
                        {
                            // This character can't use this weapon
                            ChrList[weapon].reloadtime = 50;
                            if ( ChrList[cnt].staton )
                            {
                                // Tell the player that they can't use this weapon
                                STRING text;
                                snprintf( text, sizeof(text),  "%s can't use this item...", ChrList[cnt].name );
                                debug_message( text );
                            }
                        }
                    }
                    if ( action == ACTIONDA )
                    {
                        allowedtoattack = bfalse;
                        if ( ChrList[weapon].reloadtime == 0 )
                        {
                            ChrList[weapon].ai.alert |= ALERTIF_USED;
                        }
                    }
                    if ( allowedtoattack )
                    {
                        // Rearing mount
                        mount = ChrList[cnt].attachedto;
                        if ( mount != MAXCHR )
                        {
                            allowedtoattack = CapList[ChrList[mount].model].ridercanattack;
                            if ( ChrList[mount].ismount && ChrList[mount].alive && !ChrList[mount].isplayer && ChrList[mount].actionready )
                            {
                                if ( ( action != ACTIONPA || !allowedtoattack ) && ChrList[cnt].actionready )
                                {
                                    chr_play_action( ChrList[cnt].attachedto, ACTIONUA + ( rand()&1 ), bfalse );
                                    ChrList[ChrList[cnt].attachedto].ai.alert |= ALERTIF_USED;
                                    ChrList[cnt].ai.lastitemused = mount;
                                }
                                else
                                {
                                    allowedtoattack = bfalse;
                                }
                            }
                        }

                        // Attack button
                        if ( allowedtoattack )
                        {
                            if ( ChrList[cnt].actionready && MadList[ChrList[cnt].model].actionvalid[action] )
                            {
                                // Check mana cost
                                if ( ChrList[cnt].mana >= ChrList[weapon].manacost || ChrList[cnt].canchannel )
                                {
                                    cost_mana( cnt, ChrList[weapon].manacost, weapon );
                                    // Check life healing
                                    ChrList[cnt].life += ChrList[weapon].lifeheal;
                                    if ( ChrList[cnt].life > ChrList[cnt].lifemax )  ChrList[cnt].life = ChrList[cnt].lifemax;

                                    actionready = bfalse;
                                    if ( action == ACTIONPA )
                                        actionready = btrue;

                                    action += rand() & 1;
                                    chr_play_action( cnt, action, actionready );
                                    if ( weapon != cnt )
                                    {
                                        // Make the weapon attack too
                                        chr_play_action( weapon, ACTIONMJ, bfalse );
                                        ChrList[weapon].ai.alert |= ALERTIF_USED;
                                        ChrList[cnt].ai.lastitemused = weapon;
                                    }
                                    else
                                    {
                                        // Flag for unarmed attack
                                        ChrList[cnt].ai.alert |= ALERTIF_USED;
                                        ChrList[cnt].ai.lastitemused = cnt;
                                    }
                                }
                            }
                        }
                    }
                }
                else if ( 0 != (ChrList[cnt].latchbutton & LATCHBUTTON_RIGHT) && ChrList[cnt].reloadtime == 0 )
                {
                    // Which weapon?
                    weapon = ChrList[cnt].holdingwhich[1];
                    if ( weapon == MAXCHR )
                    {
                        // Unarmed means character itself is the weapon
                        weapon = cnt;
                    }

                    action = CapList[ChrList[weapon].model].weaponaction + 2;

                    // Can it do it? (other hand)
                    allowedtoattack = btrue;

                    // First check if reload time and action is okay
                    if ( !MadList[ChrList[cnt].model].actionvalid[action] || ChrList[weapon].reloadtime > 0 ) allowedtoattack = bfalse;
                    else
                    {
                        // Then check if a skill is needed
                        if ( CapList[ChrList[weapon].model].needskillidtouse )
                        {
                            if ( check_skills( cnt, CapList[ChrList[weapon].model].idsz[IDSZ_SKILL]) == bfalse   )
                                allowedtoattack = bfalse;
                        }
                    }
                    if ( !allowedtoattack )
                    {
                        if ( ChrList[weapon].reloadtime == 0 )
                        {
                            // This character can't use this weapon
                            ChrList[weapon].reloadtime = 50;
                            if ( ChrList[cnt].staton )
                            {
                                // Tell the player that they can't use this weapon
                                STRING text;
                                snprintf( text, sizeof(text), "%s can't use this item...", ChrList[cnt].name );
                                debug_message( text );
                            }
                        }
                    }
                    if ( action == ACTIONDC )
                    {
                        allowedtoattack = bfalse;
                        if ( ChrList[weapon].reloadtime == 0 )
                        {
                            ChrList[weapon].ai.alert |= ALERTIF_USED;
                            ChrList[cnt].ai.lastitemused = weapon;
                        }
                    }
                    if ( allowedtoattack )
                    {
                        // Rearing mount
                        mount = ChrList[cnt].attachedto;
                        if ( mount != MAXCHR )
                        {
                            allowedtoattack = CapList[ChrList[mount].model].ridercanattack;
                            if ( ChrList[mount].ismount && ChrList[mount].alive && !ChrList[mount].isplayer && ChrList[mount].actionready )
                            {
                                if ( ( action != ACTIONPC || !allowedtoattack ) && ChrList[cnt].actionready )
                                {
                                    chr_play_action( ChrList[cnt].attachedto, ACTIONUC + ( rand()&1 ), bfalse );
                                    ChrList[ChrList[cnt].attachedto].ai.alert |= ALERTIF_USED;
                                    ChrList[cnt].ai.lastitemused = ChrList[cnt].attachedto;
                                }
                                else
                                {
                                    allowedtoattack = bfalse;
                                }
                            }
                        }

                        // Attack button
                        if ( allowedtoattack )
                        {
                            if ( ChrList[cnt].actionready && MadList[ChrList[cnt].model].actionvalid[action] )
                            {
                                // Check mana cost
                                if ( ChrList[cnt].mana >= ChrList[weapon].manacost || ChrList[cnt].canchannel )
                                {
                                    cost_mana( cnt, ChrList[weapon].manacost, weapon );
                                    // Check life healing
                                    ChrList[cnt].life += ChrList[weapon].lifeheal;
                                    if ( ChrList[cnt].life > ChrList[cnt].lifemax )  ChrList[cnt].life = ChrList[cnt].lifemax;

                                    actionready = bfalse;
                                    if ( action == ACTIONPC )
                                        actionready = btrue;

                                    action += rand() & 1;
                                    chr_play_action( cnt, action, actionready );
                                    if ( weapon != cnt )
                                    {
                                        // Make the weapon attack too
                                        chr_play_action( weapon, ACTIONMJ, bfalse );
                                        ChrList[weapon].ai.alert |= ALERTIF_USED;
                                        ChrList[cnt].ai.lastitemused = weapon;
                                    }
                                    else
                                    {
                                        // Flag for unarmed attack
                                        ChrList[cnt].ai.alert |= ALERTIF_USED;
                                        ChrList[cnt].ai.lastitemused = cnt;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Is the character in the air?
        level = ChrList[cnt].level;
        if ( ChrList[cnt].flyheight == 0 )
        {
            ChrList[cnt].zpos += ChrList[cnt].zvel;
            if ( ChrList[cnt].zpos > level || ( ChrList[cnt].zvel > STOPBOUNCING && ChrList[cnt].zpos > level - STOPBOUNCING ) )
            {
                // Character is in the air
                ChrList[cnt].jumpready = bfalse;
                ChrList[cnt].zvel += gravity;

                // Down jump timers for flapping characters
                if ( ChrList[cnt].jumptime != 0 ) ChrList[cnt].jumptime--;

                // Airborne characters still get friction to make control easier
                friction = airfriction;
            }
            else
            {
                // Character is on the ground
                ChrList[cnt].zpos = level;
                grounded = btrue;
                if ( ChrList[cnt].hitready )
                {
                    ChrList[cnt].ai.alert |= ALERTIF_HITGROUND;
                    ChrList[cnt].hitready = bfalse;
                }

                // Make the characters slide
                twist = 119;
                friction = noslipfriction;
                if ( INVALID_TILE != ChrList[cnt].onwhichfan )
                {
                    Uint32 itile = ChrList[cnt].onwhichfan;

                    twist = mesh.mem.tile_list[itile].twist;

                    if ( 0 != ( mesh.mem.tile_list[itile].fx & MESHFX_SLIPPY ) )
                    {
                        if ( wateriswater && 0 != ( mesh.mem.tile_list[itile].fx & MESHFX_WATER ) && ChrList[cnt].level < watersurfacelevel + RAISE + 1 )
                        {
                            // It says it's slippy, but the water is covering it...
                            // Treat exactly as normal
                            ChrList[cnt].jumpready = btrue;
                            ChrList[cnt].jumpnumber = ChrList[cnt].jumpnumberreset;
                            if ( ChrList[cnt].jumptime > 0 ) ChrList[cnt].jumptime--;

                            ChrList[cnt].zvel = -ChrList[cnt].zvel * ChrList[cnt].dampen;
                            ChrList[cnt].zvel += gravity;
                        }
                        else
                        {
                            // It's slippy all right...
                            friction = slippyfriction;
                            ChrList[cnt].jumpready = btrue;
                            if ( ChrList[cnt].jumptime > 0 ) ChrList[cnt].jumptime--;
                            if ( ChrList[cnt].weight != 0xFFFF )
                            {
                                // Slippy hills make characters slide
                                ChrList[cnt].xvel += vellrtwist[twist];
                                ChrList[cnt].yvel += veludtwist[twist];
                                ChrList[cnt].zvel = -SLIDETOLERANCE;
                            }
                            if ( flattwist[twist] )
                            {
                                // Reset jumping on flat areas of slippiness
                                ChrList[cnt].jumpnumber = ChrList[cnt].jumpnumberreset;
                            }
                        }
                    }
                    else
                    {
                        // Reset jumping
                        ChrList[cnt].jumpready = btrue;
                        ChrList[cnt].jumpnumber = ChrList[cnt].jumpnumberreset;
                        if ( ChrList[cnt].jumptime > 0 ) ChrList[cnt].jumptime--;

                        ChrList[cnt].zvel = -ChrList[cnt].zvel * ChrList[cnt].dampen;
                        ChrList[cnt].zvel += gravity;
                    }
                }

                // Characters with sticky butts lie on the surface of the mesh
                if ( ChrList[cnt].stickybutt || !ChrList[cnt].alive )
                {
                    maplr = ChrList[cnt].turnmaplr;
                    maplr = ( maplr << 3 ) - maplr + maplrtwist[twist];
                    mapud = ChrList[cnt].turnmapud;
                    mapud = ( mapud << 3 ) - mapud + mapudtwist[twist];
                    ChrList[cnt].turnmaplr = maplr >> 3;
                    ChrList[cnt].turnmapud = mapud >> 3;
                }
            }
        }
        else
        {
            //  Flying
            ChrList[cnt].jumpready = bfalse;
            ChrList[cnt].zpos += ChrList[cnt].zvel;
            if ( level < 0 ) level = 0;  // Don't fall in pits...

            ChrList[cnt].zvel += ( level + ChrList[cnt].flyheight - ChrList[cnt].zpos ) * FLYDAMPEN;
            if ( ChrList[cnt].zpos < level )
            {
                ChrList[cnt].zpos = level;
                ChrList[cnt].zvel = 0;
            }

            // Airborne characters still get friction to make control easier
            friction = airfriction;
        }

        // Move the character
        ChrList[cnt].xpos += ChrList[cnt].xvel;
        if ( __chrhitawall( cnt ) ) { ChrList[cnt].xpos = ChrList[cnt].oldx; ChrList[cnt].xvel = -ChrList[cnt].xvel; }

        ChrList[cnt].ypos += ChrList[cnt].yvel;
        if ( __chrhitawall( cnt ) ) { ChrList[cnt].ypos = ChrList[cnt].oldy; ChrList[cnt].yvel = -ChrList[cnt].yvel; }

        // Apply friction for next time
        ChrList[cnt].xvel = ChrList[cnt].xvel * friction;
        ChrList[cnt].yvel = ChrList[cnt].yvel * friction;

        // Animate the character
        ChrList[cnt].lip = ( ChrList[cnt].lip + 64 );
        if ( ChrList[cnt].lip == 192 )
        {
            // Check frame effects
            if ( Md2FrameList[ChrList[cnt].frame].framefx&MADFXACTLEFT )
                character_swipe( cnt, 0 );
            if ( Md2FrameList[ChrList[cnt].frame].framefx&MADFXACTRIGHT )
                character_swipe( cnt, 1 );
            if ( Md2FrameList[ChrList[cnt].frame].framefx&MADFXGRABLEFT )
                character_grab_stuff( cnt, GRIP_LEFT, bfalse );
            if ( Md2FrameList[ChrList[cnt].frame].framefx&MADFXGRABRIGHT )
                character_grab_stuff( cnt, GRIP_RIGHT, bfalse );
            if ( Md2FrameList[ChrList[cnt].frame].framefx&MADFXCHARLEFT )
                character_grab_stuff( cnt, GRIP_LEFT, btrue );
            if ( Md2FrameList[ChrList[cnt].frame].framefx&MADFXCHARRIGHT )
                character_grab_stuff( cnt, GRIP_RIGHT, btrue );
            if ( Md2FrameList[ChrList[cnt].frame].framefx&MADFXDROPLEFT )
                detach_character_from_mount( ChrList[cnt].holdingwhich[0], bfalse, btrue );
            if ( Md2FrameList[ChrList[cnt].frame].framefx&MADFXDROPRIGHT )
                detach_character_from_mount( ChrList[cnt].holdingwhich[1], bfalse, btrue );
            if ( Md2FrameList[ChrList[cnt].frame].framefx&MADFXPOOF && !ChrList[cnt].isplayer )
                ChrList[cnt].ai.poof_time = frame_wld;
            if ( Md2FrameList[ChrList[cnt].frame].framefx&MADFXFOOTFALL )
            {
                int ifoot = CapList[ChrList[cnt].model].soundindex[SOUND_FOOTFALL];
                if ( ifoot >= 0 && ifoot < MAXWAVE )
                {
                    sound_play_chunk( ChrList[cnt].xpos, ChrList[cnt].ypos, CapList[ChrList[cnt].model].wavelist[ifoot] );
                }
            }
        }
        if ( ChrList[cnt].lip == 0 )
        {
            // Change frames
            ChrList[cnt].lastframe = ChrList[cnt].frame;
            ChrList[cnt].frame++;
            if ( ChrList[cnt].frame == MadList[ChrList[cnt].model].actionend[ChrList[cnt].action] )
            {
                // Action finished
                if ( ChrList[cnt].keepaction )
                {
                    // Keep the last frame going
                    ChrList[cnt].frame = ChrList[cnt].lastframe;
                }
                else
                {
                    if ( !ChrList[cnt].loopaction )
                    {
                        // Go on to the next action
                        ChrList[cnt].action = ChrList[cnt].nextaction;
                        ChrList[cnt].nextaction = ACTIONDA;
                    }
                    else
                    {
                        // See if the character is mounted...
                        if ( ChrList[cnt].attachedto != MAXCHR )
                        {
                            ChrList[cnt].action = ACTIONMI;
                        }
                    }

                    ChrList[cnt].frame = MadList[ChrList[cnt].model].actionstart[ChrList[cnt].action];
                }

                ChrList[cnt].actionready = btrue;
            }
        }

        // Do "Be careful!" delay
        if ( ChrList[cnt].carefultime > 0 )
        {
            ChrList[cnt].carefultime--;
        }

        // Get running, walking, sneaking, or dancing, from speed
        if ( !( ChrList[cnt].keepaction || ChrList[cnt].loopaction ) )
        {
            framelip = Md2FrameList[ChrList[cnt].frame].framelip;  // 0 - 15...  Way through animation
            if ( ChrList[cnt].actionready && ChrList[cnt].lip == 0 && grounded && ChrList[cnt].flyheight == 0 && ( framelip&7 ) < 2 )
            {
                // Do the motion stuff
                speed = ABS( ( int ) ChrList[cnt].xvel ) + ABS( ( int ) ChrList[cnt].yvel );
                if ( speed < ChrList[cnt].sneakspd )
                {
                    //                        ChrList[cnt].nextaction = ACTIONDA;
                    // Do boredom
                    ChrList[cnt].boretime--;
                    if ( ChrList[cnt].boretime < 0 )
                    {
                        ChrList[cnt].ai.alert |= ALERTIF_BORED;
                        ChrList[cnt].boretime = BORETIME;
                    }
                    else
                    {
                        // Do standstill
                        if ( ChrList[cnt].action > ACTIONDD )
                        {
                            ChrList[cnt].action = ACTIONDA;
                            ChrList[cnt].frame = MadList[ChrList[cnt].model].actionstart[ChrList[cnt].action];
                        }
                    }
                }
                else
                {
                    ChrList[cnt].boretime = BORETIME;
                    if ( speed < ChrList[cnt].walkspd )
                    {
                        ChrList[cnt].nextaction = ACTIONWA;
                        if ( ChrList[cnt].action != ACTIONWA )
                        {
                            ChrList[cnt].frame = MadList[ChrList[cnt].model].frameliptowalkframe[LIPWA][framelip];
                            ChrList[cnt].action = ACTIONWA;
                        }
                    }
                    else
                    {
                        if ( speed < ChrList[cnt].runspd )
                        {
                            ChrList[cnt].nextaction = ACTIONWB;
                            if ( ChrList[cnt].action != ACTIONWB )
                            {
                                ChrList[cnt].frame = MadList[ChrList[cnt].model].frameliptowalkframe[LIPWB][framelip];
                                ChrList[cnt].action = ACTIONWB;
                            }
                        }
                        else
                        {
                            ChrList[cnt].nextaction = ACTIONWC;
                            if ( ChrList[cnt].action != ACTIONWC )
                            {
                                ChrList[cnt].frame = MadList[ChrList[cnt].model].frameliptowalkframe[LIPWC][framelip];
                                ChrList[cnt].action = ACTIONWC;
                            }
                        }
                    }
                }
            }
        }
    }

    // Do poofing
    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( !ChrList[cnt].on || !(ChrList[cnt].ai.poof_time >= 0 && ChrList[cnt].ai.poof_time <= frame_wld)  ) continue;

        if ( ChrList[cnt].attachedto != MAXCHR )
        {
            detach_character_from_mount( cnt, btrue, bfalse );
        }

        if ( ChrList[cnt].holdingwhich[0] != MAXCHR )
        {
            detach_character_from_mount( ChrList[cnt].holdingwhich[0], btrue, bfalse );
        }

        if ( ChrList[cnt].holdingwhich[1] != MAXCHR )
        {
            detach_character_from_mount( ChrList[cnt].holdingwhich[1], btrue, bfalse );
        }

        free_inventory( cnt );
        free_one_character( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void chop_load( Uint16 profile, const char *szLoadname )
{
    // ZZ> This function reads a naming file
    FILE *fileread;
    int section, chopinsection, cnt;
    char mychop[32], cTmp;

    fileread = fopen( szLoadname, "r" );
    if ( fileread )
    {
        section = 0;
        chopinsection = 0;

        while ( goto_colon_yesno( fileread ) && section < MAXSECTION )
        {
            fscanf( fileread, "%s", mychop );
            if ( strcmp(mychop, "STOP") ) // mychop[0] != 'S' || mychop[1] != 'T' || mychop[2] != 'O' || mychop[3] != 'P' )
            {
                if ( chop.carat >= CHOPDATACHUNK )  chop.carat = CHOPDATACHUNK - 1;

                chop.start[chop.count] = chop.carat;
                cnt = 0;
                cTmp = mychop[0];

                while ( cTmp != 0 && cnt < 31 && chop.carat < CHOPDATACHUNK )
                {
                    if ( cTmp == '_' ) cTmp = ' ';

                    chop.buffer[chop.carat] = cTmp;
                    cnt++;
                    chop.carat++;
                    cTmp = mychop[cnt];
                }
                if ( chop.carat >= CHOPDATACHUNK )  chop.carat = CHOPDATACHUNK - 1;

                chop.buffer[chop.carat] = 0;  chop.carat++;
                chopinsection++;
                chop.count++;
            }
            else
            {
                CapList[profile].chop_sectionsize[section] = chopinsection;
                CapList[profile].chop_sectionstart[section] = chop.count - chopinsection;
                section++;
                chopinsection = 0;
            }
        }

        fclose( fileread );
    }
}

