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
#include "egoboo.h"
#include "log.h"
#include "script.h"

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void do_level_up( Uint16 character );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void flash_character_height( int character, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high )
{
    // ZZ> This function sets a character's lighting depending on vertex height...
    //     Can make feet dark and head light...
    int cnt;
    Uint16 frame;
    Sint16 z;

    frame = chr[character].frame;

    for ( cnt = 0; cnt < madtransvertices[chr[character].model]; cnt++  )
    {
        z = madvrtz[frame][cnt];

        if ( z < low )
        {
            chr[character].vrta[cnt] = valuelow;
        }
        else
        {
            if ( z > high )
            {
                chr[character].vrta[cnt] = valuehigh;
            }
            else
            {
                chr[character].vrta[cnt] = ( valuehigh * ( z - low ) / ( high - low ) ) +
                                          ( valuelow * ( high - z ) / ( high - low ) );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void flash_character( int character, Uint8 value )
{
    // ZZ> This function sets a character's lighting
    int cnt;

    for ( cnt = 0; cnt < madtransvertices[chr[character].model]; cnt++  )
    {
        chr[character].vrta[cnt] = value;
    }
}

//--------------------------------------------------------------------------------------------
void add_to_dolist( int cnt )
{
    // This function puts a character in the list
    int fan;

    if ( !chr[cnt].indolist )
    {
        fan = chr[cnt].onwhichfan;

        if ( meshinrenderlist[fan] )
        {
            chr[cnt].lightlevel = meshvrtl[meshvrtstart[fan]];
            dolist[numdolist] = cnt;
            chr[cnt].indolist = btrue;
            numdolist++;

            // Do flashing
            if ( 0 == ( allframe & chr[cnt].flashand ) && chr[cnt].flashand != DONTFLASH )
            {
                flash_character( cnt, 255 );
            }

            // Do blacking
            if ( ( allframe&SEEKURSEAND ) == 0 && local_seekurse && chr[cnt].iskursed )
            {
                flash_character( cnt, 0 );
            }
        }
        else
        {
            // Double check for large/special objects
            if ( capalwaysdraw[chr[cnt].model] )
            {
                dolist[numdolist] = cnt;
                chr[cnt].indolist = btrue;
                numdolist++;
            }
        }

        // Add its weapons too
        if ( chr[cnt].holdingwhich[0] != MAXCHR )
            add_to_dolist( chr[cnt].holdingwhich[0] );

        if ( chr[cnt].holdingwhich[1] != MAXCHR )
            add_to_dolist( chr[cnt].holdingwhich[1] );
    }
}

//--------------------------------------------------------------------------------------------
void order_dolist( void )
{
    // ZZ> This function orders the dolist based on distance from camera,
    //     which is needed for reflections to properly clip themselves.
    //     Order from closest to farthest
    int tnc, character, order;
    int dist[MAXCHR];
    Uint16 olddolist[MAXCHR], cnt;

    // Figure the distance of each
    cnt = 0;

    while ( cnt < numdolist )
    {
        character = dolist[cnt];  olddolist[cnt] = character;

        if ( chr[character].light != 255 || chr[character].alpha != 255 )
        {
            // This makes stuff inside an invisible character visible...
            // A key inside a Jellcube, for example
            dist[cnt] = 0x7fffffff;
        }
        else
        {
            dist[cnt] = (int) (ABS( chr[character].xpos - camx ) + ABS( chr[character].ypos - camy ));
        }

        cnt++;
    }

    // Put em in the right order
    cnt = 0;

    while ( cnt < numdolist )
    {
        character = olddolist[cnt];
        order = 0;  // Assume this character is closest
        tnc = 0;

        while ( tnc < numdolist )
        {
            // For each one closer, increment the order
            order += ( dist[cnt] > dist[tnc] );
            order += ( dist[cnt] == dist[tnc] ) && ( cnt < tnc );
            tnc++;
        }

        dolist[order] = character;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_dolist( void )
{
    // ZZ> This function finds the characters that need to be drawn and puts them in the list
    int cnt, character;

    // Remove everyone from the dolist
    cnt = 0;

    while ( cnt < numdolist )
    {
        character = dolist[cnt];
        chr[character].indolist = bfalse;
        cnt++;
    }

    numdolist = 0;

    // Now fill it up again
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].on && ( !chr[cnt].inpack ) )
        {
            // Add the character
            add_to_dolist( cnt );
        }

        cnt++;
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
        if ( chr[cnt].on )
        {
            character = chr[cnt].attachedto;

            if ( character == MAXCHR )
            {
                // Keep inventory with character
                if ( !chr[cnt].inpack )
                {
                    character = chr[cnt].nextinpack;

                    while ( character != MAXCHR )
                    {
                        chr[character].xpos = chr[cnt].xpos;
                        chr[character].ypos = chr[cnt].ypos;
                        chr[character].zpos = chr[cnt].zpos;
                        // Copy olds to make SendMessageNear work
                        chr[character].oldx = chr[cnt].xpos;
                        chr[character].oldy = chr[cnt].ypos;
                        chr[character].oldz = chr[cnt].zpos;
                        character = chr[character].nextinpack;
                    }
                }
            }
            else
            {
                // Keep in hand weapons with character
                if ( chr[character].matrixvalid && chr[cnt].matrixvalid )
                {
                    chr[cnt].xpos = chr[cnt].matrix.CNV( 3, 0 );
                    chr[cnt].ypos = chr[cnt].matrix.CNV( 3, 1 );
                    chr[cnt].zpos = chr[cnt].matrix.CNV( 3, 2 );
                }
                else
                {
                    chr[cnt].xpos = chr[character].xpos;
                    chr[cnt].ypos = chr[character].ypos;
                    chr[cnt].zpos = chr[character].zpos;
                }

                chr[cnt].turnleftright = chr[character].turnleftright;

                // Copy this stuff ONLY if it's a weapon, not for mounts
                if ( chr[character].transferblend && chr[cnt].isitem )
                {

                    // Items become partially invisible in hands of players
                    if ( chr[character].isplayer && chr[character].alpha != 255 )
                        chr[cnt].alpha = 128;
                    else
                    {
                        // Only if not naturally transparent
                        if ( capalpha[chr[cnt].model] == 255 )
                            chr[cnt].alpha = chr[character].alpha;
                        else chr[cnt].alpha = capalpha[chr[cnt].model];
                    }

                    //Do light too
                    if ( chr[character].isplayer && chr[character].light != 255 )
                        chr[cnt].light = 128;
                    else
                    {
                        // Only if not naturally transparent
                        if ( caplight[chr[cnt].model] == 255 )
                            chr[cnt].light = chr[character].light;
                        else chr[cnt].light = caplight[chr[cnt].model];
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
    chr[cnt].matrixvalid = btrue;

    if ( chr[cnt].overlay )
    {
        // Overlays are kept with their target...
        tnc = chr[cnt].aitarget;
        chr[cnt].xpos = chr[tnc].xpos;
        chr[cnt].ypos = chr[tnc].ypos;
        chr[cnt].zpos = chr[tnc].zpos;
        chr[cnt].matrix.CNV( 0, 0 ) = chr[tnc].matrix.CNV( 0, 0 );
        chr[cnt].matrix.CNV( 0, 1 ) = chr[tnc].matrix.CNV( 0, 1 );
        chr[cnt].matrix.CNV( 0, 2 ) = chr[tnc].matrix.CNV( 0, 2 );
        chr[cnt].matrix.CNV( 0, 3 ) = chr[tnc].matrix.CNV( 0, 3 );
        chr[cnt].matrix.CNV( 1, 0 ) = chr[tnc].matrix.CNV( 1, 0 );
        chr[cnt].matrix.CNV( 1, 1 ) = chr[tnc].matrix.CNV( 1, 1 );
        chr[cnt].matrix.CNV( 1, 2 ) = chr[tnc].matrix.CNV( 1, 2 );
        chr[cnt].matrix.CNV( 1, 3 ) = chr[tnc].matrix.CNV( 1, 3 );
        chr[cnt].matrix.CNV( 2, 0 ) = chr[tnc].matrix.CNV( 2, 0 );
        chr[cnt].matrix.CNV( 2, 1 ) = chr[tnc].matrix.CNV( 2, 1 );
        chr[cnt].matrix.CNV( 2, 2 ) = chr[tnc].matrix.CNV( 2, 2 );
        chr[cnt].matrix.CNV( 2, 3 ) = chr[tnc].matrix.CNV( 2, 3 );
        chr[cnt].matrix.CNV( 3, 0 ) = chr[tnc].matrix.CNV( 3, 0 );
        chr[cnt].matrix.CNV( 3, 1 ) = chr[tnc].matrix.CNV( 3, 1 );
        chr[cnt].matrix.CNV( 3, 2 ) = chr[tnc].matrix.CNV( 3, 2 );
        chr[cnt].matrix.CNV( 3, 3 ) = chr[tnc].matrix.CNV( 3, 3 );
    }
    else
    {
        chr[cnt].matrix = ScaleXYZRotateXYZTranslate( chr[cnt].scale, chr[cnt].scale, chr[cnt].scale,
                         chr[cnt].turnleftright >> 2,
                         ( ( Uint16 ) ( chr[cnt].turnmapud + 32768 ) ) >> 2,
                         ( ( Uint16 ) ( chr[cnt].turnmaplr + 32768 ) ) >> 2,
                         chr[cnt].xpos, chr[cnt].ypos, chr[cnt].zpos );
    }
}

//--------------------------------------------------------------------------------------------
void free_one_character( int character )
{
    // ZZ> This function sticks a character back on the free character stack
    int cnt;

    freechrlist[numfreechr] = character;
    numfreechr++;

    // Remove from stat list
    if ( chr[character].staton )
    {
        chr[character].staton = bfalse;
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
    if ( chr[character].alive && !capinvictus[chr[character].model] )
    {
        teammorale[chr[character].baseteam]--;
    }

    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].on )
        {
            if ( chr[cnt].aitarget == character )
            {
                chr[cnt].alert |= ALERTIFTARGETKILLED;
                chr[cnt].aitarget = cnt;
            }

            if ( teamleader[chr[cnt].team] == character )
            {
                chr[cnt].alert |= ALERTIFLEADERKILLED;
            }
        }

        cnt++;
    }

    if ( teamleader[chr[character].team] == character )
    {
        teamleader[chr[character].team] = NOLEADER;
    }

    chr[character].on = bfalse;
    chr[character].alive = bfalse;
    chr[character].inpack = bfalse;
}

//--------------------------------------------------------------------------------------------
void free_inventory( int character )
{
    // ZZ> This function frees every item in the character's inventory
    int cnt, next;

    cnt = chr[character].nextinpack;

    while ( cnt < MAXCHR )
    {
        next = chr[cnt].nextinpack;
        free_one_character( cnt );
        cnt = next;
    }
}

//--------------------------------------------------------------------------------------------
void attach_particle_to_character( int particle, int character, int grip )
{
    // ZZ> This function sets one particle's position to be attached to a character.
    //     It will kill the particle if the character is no longer around
    Uint16 vertex, model, frame, lastframe;
    Uint8 lip;
    float pointx = 0;
    float pointy = 0;
    float pointz = 0;
    int temp;

    // Check validity of attachment
    if ( !chr[character].on || chr[character].inpack )
    {
        prttime[particle] = 1;
        return;
    }

    // Do we have a matrix???
    if ( chr[character].matrixvalid )// meshinrenderlist[chr[character].onwhichfan])
    {
        // Transform the weapon grip from model to world space
        model = chr[character].model;
        frame = chr[character].frame;
        lastframe = chr[character].lastframe;
        lip = chr[character].lip >> 6;

        if ( grip == SPAWNORIGIN )
        {
            prtxpos[particle] = chr[character].matrix.CNV( 3, 0 );
            prtypos[particle] = chr[character].matrix.CNV( 3, 1 );
            prtzpos[particle] = chr[character].matrix.CNV( 3, 2 );
            return;
        }

        vertex = madvertices[model] - grip;

        // Calculate grip point locations with linear interpolation and other silly things
        switch ( lip )
        {
            case 0:  // 25% this frame
                temp = madvrtx[lastframe][vertex];
                temp = ( temp + temp + temp + madvrtx[frame][vertex] ) >> 2;
                pointx = temp;/// chr[cnt].scale;
                temp = madvrty[lastframe][vertex];
                temp = ( temp + temp + temp + madvrty[frame][vertex] ) >> 2;
                pointy = temp;/// chr[cnt].scale;
                temp = madvrtz[lastframe][vertex];
                temp = ( temp + temp + temp + madvrtz[frame][vertex] ) >> 2;
                pointz = temp;/// chr[cnt].scale;
                break;
            case 1:  // 50% this frame
                pointx = ( ( madvrtx[frame][vertex] + madvrtx[lastframe][vertex] ) >> 1 );/// chr[cnt].scale;
                pointy = ( ( madvrty[frame][vertex] + madvrty[lastframe][vertex] ) >> 1 );/// chr[cnt].scale;
                pointz = ( ( madvrtz[frame][vertex] + madvrtz[lastframe][vertex] ) >> 1 );/// chr[cnt].scale;
                break;
            case 2:  // 75% this frame
                temp = madvrtx[frame][vertex];
                temp = ( temp + temp + temp + madvrtx[lastframe][vertex] ) >> 2;
                pointx = temp;/// chr[cnt].scale;
                temp = madvrty[frame][vertex];
                temp = ( temp + temp + temp + madvrty[lastframe][vertex] ) >> 2;
                pointy = temp;/// chr[cnt].scale;
                temp = madvrtz[frame][vertex];
                temp = ( temp + temp + temp + madvrtz[lastframe][vertex] ) >> 2;
                pointz = temp;/// chr[cnt].scale;
                break;
            case 3:  // 100% this frame...  This is the legible one
                pointx = madvrtx[frame][vertex];/// chr[cnt].scale;
                pointy = madvrty[frame][vertex];/// chr[cnt].scale;
                pointz = madvrtz[frame][vertex];/// chr[cnt].scale;
                break;
        }

        // Do the transform
        prtxpos[particle] = ( pointx * chr[character].matrix.CNV( 0, 0 ) +
                              pointy * chr[character].matrix.CNV( 1, 0 ) +
                              pointz * chr[character].matrix.CNV( 2, 0 ) );
        prtypos[particle] = ( pointx * chr[character].matrix.CNV( 0, 1 ) +
                              pointy * chr[character].matrix.CNV( 1, 1 ) +
                              pointz * chr[character].matrix.CNV( 2, 1 ) );
        prtzpos[particle] = ( pointx * chr[character].matrix.CNV( 0, 2 ) +
                              pointy * chr[character].matrix.CNV( 1, 2 ) +
                              pointz * chr[character].matrix.CNV( 2, 2 ) );
        prtxpos[particle] += chr[character].matrix.CNV( 3, 0 );
        prtypos[particle] += chr[character].matrix.CNV( 3, 1 );
        prtzpos[particle] += chr[character].matrix.CNV( 3, 2 );
    }
    else
    {
        // No matrix, so just wing it...
        prtxpos[particle] = chr[character].xpos;
        prtypos[particle] = chr[character].ypos;
        prtzpos[particle] = chr[character].zpos;
    }
}

//--------------------------------------------------------------------------------------------
void make_one_weapon_matrix( Uint16 iweap )
{
    // ZZ> This function sets one weapon's matrix, based on who it's attached to
#define POINTS 4

    int    cnt, tnc, vertex;
    Uint16 ichr, ichr_model, ichr_frame, ichr_lastframe;
    Uint8  ichr_lip;
    float  pointx[POINTS], pointy[POINTS], pointz[POINTS];
    float  nupointx[POINTS], nupointy[POINTS], nupointz[POINTS];
    int    temp, iweappoints;

    // make sure that we are attached to a valid character
    ichr = chr[iweap].attachedto;

    if (ichr >= MAXCHR || !chr[ichr].on) return;

    // make sure that the matrix is invalid incase of an error
    chr[iweap].matrixvalid = bfalse;

    // Transform the weapon grip from model space to world space
    ichr_model = chr[ichr].model;
    ichr_frame = chr[ichr].frame;
    ichr_lastframe = chr[ichr].lastframe;
    ichr_lip = chr[ichr].lip >> 6;

    iweappoints = 0;

    for (cnt = 0; cnt < POINTS; cnt++)
    {
        if (0xFFFF != chr[iweap].weapongrip[cnt])
        {
            iweappoints++;
        }
    }

    if (0 == iweappoints)
    {
        // punt! attach to origin
        pointx[0] = chr[0].xpos;
        pointy[0] = chr[0].ypos;
        pointz[0] = chr[0].zpos;
        iweappoints = 1;
    }
    else
    {
        // Calculate grip point locations with linear interpolation and other silly things
        switch ( ichr_lip )
        {
            case 0:  // 25% this ichr_frame

                for (cnt = 0; cnt < POINTS; cnt++ )
                {
                    vertex = chr[iweap].weapongrip[cnt];

                    if (0xFFFF == vertex) continue;

                    temp = madvrtx[ichr_lastframe][vertex];
                    temp = ( temp + temp + temp + madvrtx[ichr_frame][vertex] ) >> 2;
                    pointx[cnt] = temp;
                    temp = madvrty[ichr_lastframe][vertex];
                    temp = ( temp + temp + temp + madvrty[ichr_frame][vertex] ) >> 2;
                    pointy[cnt] = temp;
                    temp = madvrtz[ichr_lastframe][vertex];
                    temp = ( temp + temp + temp + madvrtz[ichr_frame][vertex] ) >> 2;
                    pointz[cnt] = temp;
                }
                break;

            case 1:  // 50% this ichr_frame

                for (cnt = 0; cnt < POINTS; cnt++ )
                {
                    vertex = chr[iweap].weapongrip[cnt];

                    if (0xFFFF == vertex) continue;

                    pointx[cnt] = ( ( madvrtx[ichr_frame][vertex] + madvrtx[ichr_lastframe][vertex] ) >> 1 );
                    pointy[cnt] = ( ( madvrty[ichr_frame][vertex] + madvrty[ichr_lastframe][vertex] ) >> 1 );
                    pointz[cnt] = ( ( madvrtz[ichr_frame][vertex] + madvrtz[ichr_lastframe][vertex] ) >> 1 );
                }
                break;

            case 2:  // 75% this ichr_frame

                for (cnt = 0; cnt < POINTS; cnt++ )
                {
                    vertex = chr[iweap].weapongrip[cnt];

                    if (0xFFFF == vertex) continue;

                    temp = madvrtx[ichr_frame][vertex];
                    temp = ( temp + temp + temp + madvrtx[ichr_lastframe][vertex] ) >> 2;
                    pointx[cnt] = temp;
                    temp = madvrty[ichr_frame][vertex];
                    temp = ( temp + temp + temp + madvrty[ichr_lastframe][vertex] ) >> 2;
                    pointy[cnt] = temp;
                    temp = madvrtz[ichr_frame][vertex];
                    temp = ( temp + temp + temp + madvrtz[ichr_lastframe][vertex] ) >> 2;
                    pointz[cnt] = temp;
                }
                break;

            case 3:  // 100% this ichr_frame...  This is the legible one

                for (cnt = 0; cnt < POINTS; cnt++ )
                {
                    vertex = chr[iweap].weapongrip[cnt];

                    if (0xFFFF == vertex) continue;

                    pointx[cnt] = madvrtx[ichr_frame][vertex];
                    pointy[cnt] = madvrty[ichr_frame][vertex];
                    pointz[cnt] = madvrtz[ichr_frame][vertex];
                }
                break;
        }
    }

    tnc = 0;

    while ( tnc < iweappoints )
    {
        // Do the transform
        nupointx[tnc] = ( pointx[tnc] * chr[ichr].matrix.CNV( 0, 0 ) +
                          pointy[tnc] * chr[ichr].matrix.CNV( 1, 0 ) +
                          pointz[tnc] * chr[ichr].matrix.CNV( 2, 0 ) );
        nupointy[tnc] = ( pointx[tnc] * chr[ichr].matrix.CNV( 0, 1 ) +
                          pointy[tnc] * chr[ichr].matrix.CNV( 1, 1 ) +
                          pointz[tnc] * chr[ichr].matrix.CNV( 2, 1 ) );
        nupointz[tnc] = ( pointx[tnc] * chr[ichr].matrix.CNV( 0, 2 ) +
                          pointy[tnc] * chr[ichr].matrix.CNV( 1, 2 ) +
                          pointz[tnc] * chr[ichr].matrix.CNV( 2, 2 ) );

        nupointx[tnc] += chr[ichr].matrix.CNV( 3, 0 );
        nupointy[tnc] += chr[ichr].matrix.CNV( 3, 1 );
        nupointz[tnc] += chr[ichr].matrix.CNV( 3, 2 );

        tnc++;
    }

    if (1 == iweappoints)
    {
        // attach to single point
        chr[iweap].matrix = ScaleXYZRotateXYZTranslate(chr[iweap].scale, chr[iweap].scale, chr[iweap].scale,
                           chr[iweap].turnleftright >> 2,
                           ( ( Uint16 ) ( chr[iweap].turnmapud + 32768 ) ) >> 2,
                           ( ( Uint16 ) ( chr[iweap].turnmaplr + 32768 ) ) >> 2,
                           nupointx[0], nupointy[0], nupointz[0]);

        chr[iweap].matrixvalid = btrue;
    }
    else if (4 == iweappoints)
    {
        // Calculate weapon's matrix based on positions of grip points
        // chrscale is recomputed at time of attachment
        chr[iweap].matrix = FourPoints( nupointx[0], nupointy[0], nupointz[0],
                                       nupointx[1], nupointy[1], nupointz[1],
                                       nupointx[2], nupointy[2], nupointz[2],
                                       nupointx[3], nupointy[3], nupointz[3],
                                       chr[iweap].scale );
        chr[iweap].matrixvalid = btrue;
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
        chr[cnt].matrixvalid = bfalse;
        cnt++;
    }

    // Do base characters
    tnc = 0;

    while ( tnc < MAXCHR )
    {
        if ( chr[tnc].attachedto == MAXCHR && chr[tnc].on )  // Skip weapons for now
        {
            make_one_character_matrix( tnc );
        }

        tnc++;
    }

    // Do first level of attachments
    tnc = 0;

    while ( tnc < MAXCHR )
    {
        if ( chr[tnc].attachedto != MAXCHR && chr[tnc].on )
        {
            if ( chr[chr[tnc].attachedto].attachedto == MAXCHR )
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
        if ( chr[tnc].attachedto != MAXCHR && chr[tnc].on )
        {
            if ( chr[chr[tnc].attachedto].attachedto != MAXCHR )
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
/*Uint8 find_target_in_block( int x, int y, float chrx, float chry, Uint16 facing,
                            Uint8 onlyfriends, Uint8 anyone, Uint8 team,
                            Uint16 donttarget, Uint16 oldtarget )
{
  // ZZ> This function helps find a target, returning btrue if it found a decent target
  int cnt;
  Uint16 angle;
  Uint16 charb;
  Uint8 enemies, returncode;
  Uint32 fanblock;
  int distance;

  returncode = bfalse;

  // Current fanblock
  if ( x >= 0 && x < ( meshsizex >> 2 ) && y >= 0 && y < ( meshsizey >> 2 ) )
  {
    fanblock = x + meshblockstart[y];

    enemies = bfalse;
    if ( !onlyfriends ) enemies = btrue;

    charb = meshbumplistchr[fanblock];
    cnt = 0;
    while ( cnt < meshbumplistchrnum[fanblock] )
    {
      if ( chr[charb].alive && !chr[charb].invictus && charb != donttarget && charb != oldtarget )
      {
        if ( anyone || ( chr[charb].team == team && onlyfriends ) || ( teamhatesteam[team][chr[charb].team] && enemies ) )
        {
          distance = ABS( chr[charb].xpos - chrx ) + ABS( chr[charb].ypos - chry );
          if ( distance < globestdistance )
          {
            angle = ( ATAN2( chr[charb].ypos - chry, chr[charb].xpos - chrx ) + PI ) * 65535 / ( TWO_PI );
            angle = facing - angle;
            if ( angle < globestangle || angle > ( 65535 - globestangle ) )
            {
              returncode = btrue;
              globesttarget = charb;
              globestdistance = distance;
              glouseangle = angle;
              if ( angle  > 32767 )
                globestangle = -angle;
              else
                globestangle = angle;
            }
          }
        }
      }
      charb = chr[charb].bumpnext;
      cnt++;
    }
  }
  return returncode;
}*/

//--------------------------------------------------------------------------------------------
Uint16 get_particle_target( float xpos, float ypos, float zpos, Uint16 facing,
                            Uint16 particletype, Uint8 team, Uint16 donttarget,
                            Uint16 oldtarget )
{
    //ZF> This is the new improved targeting system for particles. Also includes distance in the Z direction.
    Uint16 besttarget = MAXCHR, cnt;
    Uint16 longdist = WIDE;

    for (cnt = 0; cnt < MAXCHR; cnt++)
    {
        if (chr[cnt].on && chr[cnt].alive && !chr[cnt].isitem && chr[cnt].attachedto == MAXCHR
                && !chr[cnt].invictus)
        {
            if ((piponlydamagefriendly[particletype] && team == chr[cnt].team) || (!piponlydamagefriendly[particletype] && teamhatesteam[team][chr[cnt].team]) )
            {
                //Don't retarget someone we already had or not supposed to target
                if (cnt != oldtarget && cnt != donttarget)
                {
                    Uint16 angle = (ATAN2( chr[cnt].ypos - ypos, chr[cnt].xpos - xpos ) * 65535 / ( TWO_PI ))
                                   + BEHIND - facing;

                    //Only proceed if we are facing the target
                    if (angle < piptargetangle[particletype] || angle > ( 65535 - piptargetangle[particletype] ) )
                    {
                        Uint32 dist = ( Uint32 ) SQRT(ABS( pow(chr[cnt].xpos - xpos, 2))
                                                      + ABS( pow(chr[cnt].ypos - ypos, 2))
                                                      + ABS( pow(chr[cnt].zpos - zpos, 2)) );

                        if (dist < longdist && dist < WIDE )
                        {
                            besttarget = cnt;
                            longdist = dist;
                        }
                    }
                }
            }
        }

    }

    //All done
    return besttarget;
}
//--------------------------------------------------------------------------------------------
/*Uint16 find_target( float chrx, float chry, Uint16 facing,
                    Uint16 targetangle, Uint8 onlyfriends, Uint8 anyone,
                    Uint8 team, Uint16 donttarget, Uint16 oldtarget )
{
  // This function finds the best target for the given parameters
  Uint8 done;
  int x, y;

  x = chrx;
  y = chry;
  x = x >> 9;
  y = y >> 9;
  globestdistance = 9999;
  globestangle = targetangle;
  done = find_target_in_block( x, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x + 1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x - 1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  if ( done ) return globesttarget;

  done = find_target_in_block( x + 1, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x + 1, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x - 1, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x - 1, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  if ( done ) return globesttarget;

  return MAXCHR;
}*/

//--------------------------------------------------------------------------------------------
void free_all_characters()
{
    // ZZ> This function resets the character allocation list
    nolocalplayers = btrue;
    numfreechr = 0;

    while ( numfreechr < MAXCHR )
    {
        chr[numfreechr].on = bfalse;
        chr[numfreechr].alive = bfalse;
        chr[numfreechr].inpack = bfalse;
        chr[numfreechr].numinpack = 0;
        chr[numfreechr].nextinpack = MAXCHR;
        chr[numfreechr].staton = bfalse;
        chr[numfreechr].matrixvalid = bfalse;
        freechrlist[numfreechr] = numfreechr;
        numfreechr++;
    }

    numpla = 0;
    numlocalpla = 0;
    numstat = 0;
}

//--------------------------------------------------------------------------------------------
Uint8 __chrhitawall( int character )
{
    // ZZ> This function returns nonzero if the character hit a wall that the
    //     character is not allowed to cross
    Uint8 passtl, passtr, passbr, passbl;
    float x, y, bs;
    float fx, fy;
    int ix, iy;

    y = chr[character].ypos;  x = chr[character].xpos;  bs = chr[character].bumpsize >> 1;

    passtl = MESHFXIMPASS;
    fx = x - bs; fy = y - bs;

    if ( fx > 0.0f && fx < meshedgex && fy > 0.0f && fy < meshedgey )
    {
        ix = ( int )fx; iy = ( int )fy;
        passtl = meshfx[meshfanstart[iy>>7] + ( ix >> 7 )];
    }

    passtr = MESHFXIMPASS;
    fx = x + bs; fy = y - bs;

    if ( fx > 0.0f && fx < meshedgex && fy > 0.0f && fy < meshedgey )
    {
        ix = ( int )fx; iy = ( int )fy;
        passtr = meshfx[meshfanstart[iy>>7] + ( ix >> 7 )];
    }

    passbl = MESHFXIMPASS;
    fx = x - bs; fy = y + bs;

    if ( fx > 0.0f && fx < meshedgex && fy > 0.0f && fy < meshedgey )
    {
        ix = ( int )fx; iy = ( int )fy;
        passbl = meshfx[meshfanstart[iy>>7] + ( ix >> 7 )];
    }

    passbr = MESHFXIMPASS;
    fx = x + bs; fy = y + bs;

    if ( fx > 0.0f && fx < meshedgex && fy > 0.0f && fy < meshedgey )
    {
        ix = ( int )fx; iy = ( int )fy;
        passbr = meshfx[meshfanstart[iy>>7] + ( ix >> 7 )];
    }

    return ( passtl | passtr | passbr | passbl ) & chr[character].stoppedby;
}

//--------------------------------------------------------------------------------------------
void reset_character_accel( Uint16 character )
{
    // ZZ> This function fixes a character's max acceleration
    Uint16 enchant;

    if ( character != MAXCHR )
    {
        if ( chr[character].on )
        {
            // Okay, remove all acceleration enchants
            enchant = chr[character].firstenchant;

            while ( enchant < MAXENCHANT )
            {
                remove_enchant_value( enchant, ADDACCEL );
                enchant = encnextenchant[enchant];
            }

            // Set the starting value
            chr[character].maxaccel = capmaxaccel[chr[character].model][chr[character].texture - madskinstart[chr[character].model]];
            // Put the acceleration enchants back on
            enchant = chr[character].firstenchant;

            while ( enchant < MAXENCHANT )
            {
                add_enchant_value( enchant, ADDACCEL, enceve[enchant] );
                enchant = encnextenchant[enchant];
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
    mount = chr[character].attachedto;

    if ( mount >= MAXCHR )
        return;

    // Make sure both are still around
    if ( !chr[character].on || !chr[mount].on )
        return;

    // Don't allow living characters to drop kursed weapons
    if ( !ignorekurse && chr[character].iskursed && chr[mount].alive && chr[character].isitem )
    {
        chr[character].alert = chr[character].alert | ALERTIFNOTDROPPED;
        return;
    }

    // Figure out which hand it's in
    hand = 0;

    if ( chr[character].inwhichhand == GRIPRIGHT )
    {
        hand = 1;
    }

    // Rip 'em apart
    chr[character].attachedto = MAXCHR;

    if ( chr[mount].holdingwhich[0] == character )
        chr[mount].holdingwhich[0] = MAXCHR;

    if ( chr[mount].holdingwhich[1] == character )
        chr[mount].holdingwhich[1] = MAXCHR;

    chr[character].scale = chr[character].fat * madscale[chr[character].model] * 4;

    // Run the falling animation...
    play_action( character, ACTIONJB + hand, bfalse );

    // Set the positions
    if ( chr[character].matrixvalid )
    {
        chr[character].xpos = chr[character].matrix.CNV( 3, 0 );
        chr[character].ypos = chr[character].matrix.CNV( 3, 1 );
        chr[character].zpos = chr[character].matrix.CNV( 3, 2 );
    }
    else
    {
        chr[character].xpos = chr[mount].xpos;
        chr[character].ypos = chr[mount].ypos;
        chr[character].zpos = chr[mount].zpos;
    }

    // Make sure it's not dropped in a wall...
    if ( __chrhitawall( character ) )
    {
        chr[character].xpos = chr[mount].xpos;
        chr[character].ypos = chr[mount].ypos;
    }

    // Check for shop passages
    inshop = bfalse;

    if ( chr[character].isitem && numshoppassage != 0 && doshop )
    {
        //This is a hack that makes spellbooks in shops cost correctly
        if (chr[mount].isshopitem) chr[character].isshopitem = btrue;

        cnt = 0;

        while ( cnt < numshoppassage )
        {
            passage = shoppassage[cnt];
            loc = chr[character].xpos;
            loc = loc >> 7;

            if ( loc >= passtlx[passage] && loc <= passbrx[passage] )
            {
                loc = chr[character].ypos;
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
            price = (float) capskincost[chr[character].model][0];

            if ( capisstackable[chr[character].model] )
            {
                price = price * chr[character].ammo;
            }

            // Reduce value depending on charges left
            else if (capisranged[chr[character].model] && chr[character].ammo < chr[character].ammomax)
            {
                if (chr[character].ammo == 0)
                {
                    price /= 2;
                }
                else price -= ((chr[character].ammomax - chr[character].ammo) * ((float)(price / chr[character].ammomax))) / 2;
            }

            //Items spawned within shops are more valuable
            if (!chr[character].isshopitem) price *= 0.5;

            chr[mount].money += (Sint16) price;
            chr[owner].money -= (Sint16) price;

            if ( chr[owner].money < 0 )  chr[owner].money = 0;

            if ( chr[mount].money > MAXMONEY )  chr[mount].money = MAXMONEY;

            chr[owner].alert |= ALERTIFORDERED;
            chr[owner].order = (Uint32) price;  // Tell owner how much...
            chr[owner].counter = BUY;  // 0 for buying an item
        }
    }

    // Make sure it works right
    chr[character].hitready = btrue;

    if ( inshop )
    {
        // Drop straight down to avoid theft
        chr[character].xvel = 0;
        chr[character].yvel = 0;
    }
    else
    {
        chr[character].xvel = chr[mount].xvel;
        chr[character].yvel = chr[mount].yvel;
    }

    chr[character].zvel = DROPZVEL;

    // Turn looping off
    chr[character].loopaction = bfalse;

    // Reset the team if it is a mount
    if ( chr[mount].ismount )
    {
        chr[mount].team = chr[mount].baseteam;
        chr[mount].alert |= ALERTIFDROPPED;
    }

    chr[character].team = chr[character].baseteam;
    chr[character].alert |= ALERTIFDROPPED;

    // Reset transparency
    if ( chr[character].isitem && chr[mount].transferblend )
    {
        // Okay, reset transparency
        enchant = chr[character].firstenchant;

        while ( enchant < MAXENCHANT )
        {
            unset_enchant_value( enchant, SETALPHABLEND );
            unset_enchant_value( enchant, SETLIGHTBLEND );
            enchant = encnextenchant[enchant];
        }

        chr[character].alpha = chr[character].basealpha;
        chr[character].light = caplight[chr[character].model];
        enchant = chr[character].firstenchant;

        while ( enchant < MAXENCHANT )
        {
            set_enchant_value( enchant, SETALPHABLEND, enceve[enchant] );
            set_enchant_value( enchant, SETLIGHTBLEND, enceve[enchant] );
            enchant = encnextenchant[enchant];
        }
    }

    // Set twist
    chr[character].turnmaplr = 32768;
    chr[character].turnmapud = 32768;
}

//--------------------------------------------------------------------------------------------
void attach_character_to_mount( Uint16 character, Uint16 mount,
                                Uint16 grip )
{
    // ZZ> This function attaches one character to another ( the mount )
    //     at either the left or right grip
    int i, tnc, hand;

    // Make sure both are still around
    if ( !chr[character].on || !chr[mount].on || chr[character].inpack || chr[mount].inpack )
        return;

    // Figure out which hand this grip relates to
    hand = 1;

    if ( grip == GRIPLEFT )
        hand = 0;

    // Make sure the the hand is valid
    if ( !capgripvalid[chr[mount].model][hand] )
        return;

    // Put 'em together
    chr[character].inwhichhand = grip;
    chr[character].attachedto = mount;
    chr[mount].holdingwhich[hand] = character;

    tnc = madvertices[chr[mount].model] - grip;

    for (i = 0; i < 4; i++)
    {
        if (tnc + i < madvertices[chr[mount].model] )
        {
            chr[character].weapongrip[i] = i + tnc;
        }
        else
        {
            chr[character].weapongrip[i] = 0xFFFF;
        }
    }

    // catually make position of the object coincide with its actual held position
    make_one_weapon_matrix( character );

    chr[character].xpos = chr[character].matrix.CNV( 3, 0 );
    chr[character].ypos = chr[character].matrix.CNV( 3, 1 );
    chr[character].zpos = chr[character].matrix.CNV( 3, 2 );

    chr[character].scale = chr[character].fat / ( chr[mount].fat * 1280 );
    chr[character].inwater = bfalse;
    chr[character].jumptime = JUMPDELAY * 4;

    // Run the held animation
    if ( chr[mount].ismount && grip == GRIPONLY )
    {
        // Riding mount
        play_action( character, ACTIONMI, btrue );
        chr[character].loopaction = btrue;
    }
    else
    {
        play_action( character, ACTIONMM + hand, bfalse );

        if ( chr[character].isitem )
        {
            // Item grab
            chr[character].keepaction = btrue;
        }
    }

    // Set the team
    if ( chr[character].isitem )
    {
        chr[character].team = chr[mount].team;
        // Set the alert
        chr[character].alert = chr[character].alert | ALERTIFGRABBED;
    }

    if ( chr[mount].ismount )
    {
        chr[mount].team = chr[character].team;

        // Set the alert
        if ( !chr[mount].isitem )
        {
            chr[mount].alert = chr[mount].alert | ALERTIFGRABBED;
        }
    }

    // It's not gonna hit the floor
    chr[character].hitready = bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 stack_in_pack( Uint16 item, Uint16 character )
{
    // ZZ> This function looks in the character's pack for an item similar
    //     to the one given.  If it finds one, it returns the similar item's
    //     index number, otherwise it returns MAXCHR.
    Uint16 inpack, id;
    bool_t allok;

    if ( capisstackable[chr[item].model] )
    {
        inpack = chr[character].nextinpack;
        allok = bfalse;

        while ( inpack != MAXCHR && !allok )
        {
            allok = btrue;

            if ( chr[inpack].model != chr[item].model )
            {
                if ( !capisstackable[chr[inpack].model] )
                {
                    allok = bfalse;
                }

                if ( chr[inpack].ammomax != chr[item].ammomax )
                {
                    allok = bfalse;
                }

                id = 0;

                while ( id < IDSZ_COUNT && allok )
                {
                    if ( capidsz[chr[inpack].model][id] != capidsz[chr[item].model][id] )
                    {
                        allok = bfalse;
                    }

                    id++;
                }
            }

            if ( !allok )
            {
                inpack = chr[inpack].nextinpack;
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
void add_item_to_character_pack( Uint16 item, Uint16 character )
{
    // ZZ> This function puts one character inside the other's pack
    Uint16 oldfirstitem, newammo, stack;

    // Make sure everything is hunkydori
    if ( ( !chr[item].on ) || ( !chr[character].on ) || chr[item].inpack || chr[character].inpack ||
            chr[character].isitem )
        return;

    stack = stack_in_pack( item, character );

    if ( stack != MAXCHR )
    {
        // We found a similar, stackable item in the pack
        if ( chr[item].nameknown || chr[stack].nameknown )
        {
            chr[item].nameknown = btrue;
            chr[stack].nameknown = btrue;
        }

        if ( capusageknown[chr[item].model] || capusageknown[chr[stack].model] )
        {
            capusageknown[chr[item].model] = btrue;
            capusageknown[chr[stack].model] = btrue;
        }

        newammo = chr[item].ammo + chr[stack].ammo;

        if ( newammo <= chr[stack].ammomax )
        {
            // All transfered, so kill the in hand item
            chr[stack].ammo = newammo;

            if ( chr[item].attachedto != MAXCHR )
            {
                detach_character_from_mount( item, btrue, bfalse );
            }

            free_one_character( item );
        }
        else
        {
            // Only some were transfered,
            chr[item].ammo = chr[item].ammo + chr[stack].ammo - chr[stack].ammomax;
            chr[stack].ammo = chr[stack].ammomax;
            chr[character].alert |= ALERTIFTOOMUCHBAGGAGE;
        }
    }
    else
    {
        // Make sure we have room for another item
        if ( chr[character].numinpack >= MAXNUMINPACK )
        {
            chr[character].alert |= ALERTIFTOOMUCHBAGGAGE;
            return;
        }

        // Take the item out of hand
        if ( chr[item].attachedto != MAXCHR )
        {
            detach_character_from_mount( item, btrue, bfalse );
            chr[item].alert &= ( ~ALERTIFDROPPED );
        }

        // Remove the item from play
        chr[item].hitready = bfalse;
        chr[item].inpack = btrue;

        // Insert the item into the pack as the first one
        oldfirstitem = chr[character].nextinpack;
        chr[character].nextinpack = item;
        chr[item].nextinpack = oldfirstitem;
        chr[character].numinpack++;

        if ( capisequipment[chr[item].model] )
        {
            // AtLastWaypoint doubles as PutAway
            chr[item].alert |= ALERTIFATLASTWAYPOINT;
        }
    }

    return;
}

//--------------------------------------------------------------------------------------------
Uint16 get_item_from_character_pack( Uint16 character, Uint16 grip, Uint8 ignorekurse )
{
    // ZZ> This function takes the last item in the character's pack and puts
    //     it into the designated hand.  It returns the item number or MAXCHR.
    Uint16 item, nexttolastitem;

    // Make sure everything is hunkydori
    if ( ( !chr[character].on ) || chr[character].inpack || chr[character].isitem || chr[character].nextinpack == MAXCHR )
        return MAXCHR;

    if ( chr[character].numinpack == 0 )
        return MAXCHR;

    // Find the last item in the pack
    nexttolastitem = character;
    item = chr[character].nextinpack;

    while ( chr[item].nextinpack != MAXCHR )
    {
        nexttolastitem = item;
        item = chr[item].nextinpack;
    }

    // Figure out what to do with it
    if ( chr[item].iskursed && chr[item].isequipped && !ignorekurse )
    {
        // Flag the last item as not removed
        chr[item].alert |= ALERTIFNOTPUTAWAY;  // Doubles as IfNotTakenOut
        // Cycle it to the front
        chr[item].nextinpack = chr[character].nextinpack;
        chr[nexttolastitem].nextinpack = MAXCHR;
        chr[character].nextinpack = item;

        if ( character == nexttolastitem )
        {
            chr[item].nextinpack = MAXCHR;
        }

        return MAXCHR;
    }
    else
    {
        // Remove the last item from the pack
        chr[item].inpack = bfalse;
        chr[item].isequipped = bfalse;
        chr[nexttolastitem].nextinpack = MAXCHR;
        chr[character].numinpack--;
        chr[item].team = chr[character].team;

        // Attach the item to the character's hand
        attach_character_to_mount( item, character, grip );
        chr[item].alert &= ( ~ALERTIFGRABBED );
        chr[item].alert |= ( ALERTIFTAKENOUT );
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
        if ( chr[character].on )
        {
            if ( chr[character].zpos > -2 ) // Don't lose keys in pits...
            {
                // The IDSZs to find
                testa = Make_IDSZ( "KEYA" );  // [KEYA]
                testz = Make_IDSZ( "KEYZ" );  // [KEYZ]

                lastitem = character;
                item = chr[character].nextinpack;

                while ( item != MAXCHR )
                {
                    nextitem = chr[item].nextinpack;

                    if ( item != character )  // Should never happen...
                    {
                        if ( ( capidsz[chr[item].model][IDSZ_PARENT] >= testa &&
                                capidsz[chr[item].model][IDSZ_PARENT] <= testz ) ||
                                ( capidsz[chr[item].model][IDSZ_TYPE] >= testa &&
                                  capidsz[chr[item].model][IDSZ_TYPE] <= testz ) )
                        {
                            // We found a key...
                            chr[item].inpack = bfalse;
                            chr[item].isequipped = bfalse;
                            chr[lastitem].nextinpack = nextitem;
                            chr[item].nextinpack = MAXCHR;
                            chr[character].numinpack--;
                            chr[item].attachedto = MAXCHR;
                            chr[item].alert |= ALERTIFDROPPED;
                            chr[item].hitready = btrue;

                            direction = RANDIE;
                            chr[item].turnleftright = direction + 32768;
                            cosdir = direction + 16384;
                            chr[item].level = chr[character].level;
                            chr[item].xpos = chr[character].xpos;
                            chr[item].ypos = chr[character].ypos;
                            chr[item].zpos = chr[character].zpos;
                            chr[item].xvel = turntocos[direction>>2] * DROPXYVEL;
                            chr[item].yvel = turntosin[direction>>2] * DROPXYVEL;
                            chr[item].zvel = DROPZVEL;
                            chr[item].team = chr[item].baseteam;
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
        if ( chr[character].on )
        {
            detach_character_from_mount( chr[character].holdingwhich[0], btrue, bfalse );
            detach_character_from_mount( chr[character].holdingwhich[1], btrue, bfalse );

            if ( chr[character].numinpack > 0 )
            {
                direction = chr[character].turnleftright + 32768;
                diradd = 65535 / chr[character].numinpack;

                while ( chr[character].numinpack > 0 )
                {
                    item = get_item_from_character_pack( character, GRIPLEFT, bfalse );

                    if ( item < MAXCHR )
                    {
                        detach_character_from_mount( item, btrue, btrue );
                        chr[item].hitready = btrue;
                        chr[item].alert |= ALERTIFDROPPED;
                        chr[item].xpos = chr[character].xpos;
                        chr[item].ypos = chr[character].ypos;
                        chr[item].zpos = chr[character].zpos;
                        chr[item].level = chr[character].level;
                        chr[item].turnleftright = direction + 32768;
                        chr[item].xvel = turntocos[direction>>2] * DROPXYVEL;
                        chr[item].yvel = turntosin[direction>>2] * DROPXYVEL;
                        chr[item].zvel = DROPZVEL;
                        chr[item].team = chr[item].baseteam;
                    }

                    direction += diradd;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void character_grab_stuff( int chara, int grip, Uint8 people )
{
    // ZZ> This function makes the character pick up an item if there's one around
    float xa, ya, za, xb, yb, zb, dist;
    int charb, hand;
    Uint16 vertex, model, frame, passage, cnt, owner = NOOWNER;
    float pointx, pointy, pointz;
    bool_t inshop;
    int loc;
    float price;

    // Make life easier
    model = chr[chara].model;
    hand = ( grip - 4 ) >> 2;  // 0 is left, 1 is right

    // Make sure the character doesn't have something already, and that it has hands
    if ( chr[chara].holdingwhich[hand] != MAXCHR || !capgripvalid[model][hand] )
        return;

    // Do we have a matrix???
    if ( chr[chara].matrixvalid )// meshinrenderlist[chr[chara].onwhichfan])
    {
        // Transform the weapon grip from model to world space
        frame = chr[chara].frame;
        vertex = madvertices[model] - grip;

        // Calculate grip point locations
        pointx = madvrtx[frame][vertex];/// chr[cnt].scale;
        pointy = madvrty[frame][vertex];/// chr[cnt].scale;
        pointz = madvrtz[frame][vertex];/// chr[cnt].scale;

        // Do the transform
        xa = ( pointx * chr[chara].matrix.CNV( 0, 0 ) +
               pointy * chr[chara].matrix.CNV( 1, 0 ) +
               pointz * chr[chara].matrix.CNV( 2, 0 ) );
        ya = ( pointx * chr[chara].matrix.CNV( 0, 1 ) +
               pointy * chr[chara].matrix.CNV( 1, 1 ) +
               pointz * chr[chara].matrix.CNV( 2, 1 ) );
        za = ( pointx * chr[chara].matrix.CNV( 0, 2 ) +
               pointy * chr[chara].matrix.CNV( 1, 2 ) +
               pointz * chr[chara].matrix.CNV( 2, 2 ) );
        xa += chr[chara].matrix.CNV( 3, 0 );
        ya += chr[chara].matrix.CNV( 3, 1 );
        za += chr[chara].matrix.CNV( 3, 2 );
    }
    else
    {
        // Just wing it
        xa = chr[chara].xpos;
        ya = chr[chara].ypos;
        za = chr[chara].zpos;
    }

    // Go through all characters to find the best match
    charb = 0;

    while ( charb < MAXCHR )
    {
        if ( chr[charb].on && ( !chr[charb].inpack ) && chr[charb].weight < chr[chara].weight && chr[charb].alive && chr[charb].attachedto == MAXCHR && ( ( !people && chr[charb].isitem ) || ( people && !chr[charb].isitem ) ) )
        {
            xb = chr[charb].xpos;
            yb = chr[charb].ypos;
            zb = chr[charb].zpos;
            // First check absolute value diamond
            xb = ABS( xa - xb );
            yb = ABS( ya - yb );
            zb = ABS( za - zb );
            dist = xb + yb;

            if ( dist < GRABSIZE && zb < GRABSIZE )
            {
                // Don't grab your mount
                if ( chr[charb].holdingwhich[0] != chara && chr[charb].holdingwhich[1] != chara )
                {
                    // Check for shop
                    inshop = bfalse;

                    if ( chr[charb].isitem && numshoppassage != 0 )
                    {
                        cnt = 0;

                        while ( cnt < numshoppassage )
                        {
                            passage = shoppassage[cnt];
                            loc = chr[charb].xpos;
                            loc = loc >> 7;

                            if ( loc >= passtlx[passage] && loc <= passbrx[passage] )
                            {
                                loc = chr[charb].ypos;
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
                            // Pay the shop owner, or don't allow grab...
                            if ( chr[chara].isitem || ( chr[chara].alpha < INVISIBLE) )
                            {
                                // Pets can try to steal in addition to invisible characters
                                STRING text;
                                inshop = bfalse;
                                snprintf( text, sizeof(text), "%s stole something! (%s)", chr[chara].name, capclassname[chr[charb].model] );
                                debug_message( text );

                                // Check if it was detected. 50% chance +2% per pet DEX and -2% per shopkeeper wisdom
                                if (chr[owner].canseeinvisible || generate_number( 1, 100 ) - ( chr[chara].dexterity >> 7 ) + ( chr[owner].wisdom >> 7 ) > 50 )
                                {
                                    snprintf( text, sizeof(text), "%s was detected!!", chr[chara].name );
                                    debug_message( text );
                                    chr[owner].alert |= ALERTIFORDERED;
                                    chr[owner].order = STOLEN;
                                    chr[owner].counter = 3;
                                }
                            }
                            else
                            {
                                chr[owner].alert |= ALERTIFORDERED;
                                price = (float) capskincost[chr[charb].model][0];

                                if ( capisstackable[chr[charb].model] )
                                {
                                    price = price * chr[charb].ammo;
                                }

                                // Reduce value depending on charges left
                                else if (capisranged[chr[charb].model] && chr[charb].ammo < chr[charb].ammomax)
                                {
                                    if (chr[charb].ammo == 0) price /= 2;
                                    else price -= ((chr[charb].ammomax - chr[charb].ammo) * ((float)(price / chr[charb].ammomax))) / 2;
                                }

                                //Items spawned in shops are more valuable
                                if (!chr[charb].isshopitem) price *= 0.5;

                                chr[owner].order = (Uint32) price;  // Tell owner how much...

                                if ( chr[chara].money >= price )
                                {
                                    // Okay to buy
                                    chr[owner].counter = SELL;  // 1 for selling an item
                                    chr[chara].money -= (Sint16) price;  // Skin 0 cost is price
                                    chr[owner].money += (Sint16) price;

                                    if ( chr[owner].money > MAXMONEY )  chr[owner].money = MAXMONEY;

                                    inshop = bfalse;
                                }
                                else
                                {
                                    // Don't allow purchase
                                    chr[owner].counter = 2;  // 2 for "you can't afford that"
                                    inshop = btrue;
                                }
                            }
                        }
                    }

                    if ( !inshop )
                    {
                        // Stick 'em together and quit
                        attach_character_to_mount( charb, chara, grip );
                        charb = MAXCHR;

                        if ( people )
                        {
                            // Do a slam animation...  ( Be sure to drop!!! )
                            play_action( chara, ACTIONMC + hand, bfalse );
                        }
                    }
                    else
                    {
                        // Lift the item a little and quit...
                        chr[charb].zvel = DROPZVEL;
                        chr[charb].hitready = btrue;
                        chr[charb].alert |= ALERTIFDROPPED;
                        charb = MAXCHR;
                    }
                }
            }
        }

        charb++;
    }
}

//--------------------------------------------------------------------------------------------
void character_swipe( Uint16 cnt, Uint8 grip )
{
    // ZZ> This function spawns an attack particle
    int weapon, particle, spawngrip, thrown;
    Uint8 action;
    Uint16 tTmp;
    float dampen;
    float x, y, z, velocity;

    weapon = chr[cnt].holdingwhich[grip];
    spawngrip = SPAWNLAST;
    action = chr[cnt].action;

    // See if it's an unarmed attack...
    if ( weapon == MAXCHR )
    {
        weapon = cnt;
        spawngrip = 4 + ( grip << 2 );  // 0 = GRIPLEFT, 1 = GRIPRIGHT
    }

    if ( weapon != cnt && ( ( capisstackable[chr[weapon].model] && chr[weapon].ammo > 1 ) || ( action >= ACTIONFA && action <= ACTIONFD ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation
        x = chr[cnt].xpos;
        y = chr[cnt].ypos;
        z = chr[cnt].zpos;
        thrown = spawn_one_character( x, y, z, chr[weapon].model, chr[cnt].team, 0, chr[cnt].turnleftright, chr[weapon].name, MAXCHR );

        if ( thrown < MAXCHR )
        {
            chr[thrown].iskursed = bfalse;
            chr[thrown].ammo = 1;
            chr[thrown].alert |= ALERTIFTHROWN;
            velocity = chr[cnt].strength / ( chr[thrown].weight * THROWFIX );
            velocity += MINTHROWVELOCITY;

            if ( velocity > MAXTHROWVELOCITY )
            {
                velocity = MAXTHROWVELOCITY;
            }

            tTmp = chr[cnt].turnleftright >> 2;
            chr[thrown].xvel += turntocos[( tTmp+8192 )&TRIG_TABLE_MASK] * velocity;
            chr[thrown].yvel += turntosin[( tTmp+8192 )&TRIG_TABLE_MASK] * velocity;
            chr[thrown].zvel = DROPZVEL;

            if ( chr[weapon].ammo <= 1 )
            {
                // Poof the item
                detach_character_from_mount( weapon, btrue, bfalse );
                free_one_character( weapon );
            }
            else
            {
                chr[weapon].ammo--;
            }
        }
    }
    else
    {
        // Spawn an attack particle
        if ( chr[weapon].ammomax == 0 || chr[weapon].ammo != 0 )
        {
            if ( chr[weapon].ammo > 0 && !capisstackable[chr[weapon].model] )
            {
                chr[weapon].ammo--;  // Ammo usage
            }

            // HERE
            if ( capattackprttype[chr[weapon].model] != -1 )
            {
                particle = spawn_one_particle( chr[weapon].xpos, chr[weapon].ypos, chr[weapon].zpos, chr[cnt].turnleftright, chr[weapon].model, capattackprttype[chr[weapon].model], weapon, spawngrip, chr[cnt].team, cnt, 0, MAXCHR );

                if ( particle != maxparticles )
                {
                    if ( !capattackattached[chr[weapon].model] )
                    {
                        // Detach the particle
                        if ( !pipstartontarget[prtpip[particle]] || prttarget[particle] == MAXCHR )
                        {
                            attach_particle_to_character( particle, weapon, spawngrip );
                            // Correct Z spacing base, but nothing else...
                            prtzpos[particle] += pipzspacingbase[prtpip[particle]];
                        }

                        prtattachedtocharacter[particle] = MAXCHR;

                        // Don't spawn in walls
                        if ( __prthitawall( particle ) )
                        {
                            prtxpos[particle] = chr[weapon].xpos;
                            prtypos[particle] = chr[weapon].ypos;

                            if ( __prthitawall( particle ) )
                            {
                                prtxpos[particle] = chr[cnt].xpos;
                                prtypos[particle] = chr[cnt].ypos;
                            }
                        }
                    }
                    else
                    {
                        // Attached particles get a strength bonus for reeling...
                        dampen = REELBASE + ( chr[cnt].strength / REEL );
                        prtxvel[particle] = prtxvel[particle] * dampen;
                        prtyvel[particle] = prtyvel[particle] * dampen;
                        prtzvel[particle] = prtzvel[particle] * dampen;
                    }

                    // Initial particles get a strength bonus, which may be 0.00f
                    prtdamagebase[particle] += ( chr[cnt].strength * capstrengthdampen[chr[weapon].model] );
                    // Initial particles get an enchantment bonus
                    prtdamagebase[particle] += chr[weapon].damageboost;
                    // Initial particles inherit damage type of weapon
                    prtdamagetype[particle] = chr[weapon].damagetargettype;
                }
            }
        }
        else
        {
            chr[weapon].ammoknown = btrue;
        }
    }
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
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].on && ( !chr[cnt].inpack ) )
        {
            grounded = bfalse;
            valuegopoof = bfalse;
            // Down that ol' damage timer
            chr[cnt].damagetime -= ( chr[cnt].damagetime != 0 );

            // Character's old location
            chr[cnt].oldx = chr[cnt].xpos;
            chr[cnt].oldy = chr[cnt].ypos;
            chr[cnt].oldz = chr[cnt].zpos;
            chr[cnt].oldturn = chr[cnt].turnleftright;
//            if(chr[cnt].attachedto!=MAXCHR)
//            {
//                chr[cnt].turnleftright = chr[chr[cnt].attachedto].turnleftright;
//                if(chr[cnt].indolist==bfalse)
//                {
//                    chr[cnt].xpos = chr[chr[cnt].attachedto].xpos;
//                    chr[cnt].ypos = chr[chr[cnt].attachedto].ypos;
//                    chr[cnt].zpos = chr[chr[cnt].attachedto].zpos;
//                }
//            }

            // Texture movement
            chr[cnt].uoffset += chr[cnt].uoffvel;
            chr[cnt].voffset += chr[cnt].voffvel;

            if ( chr[cnt].alive )
            {
                if ( chr[cnt].attachedto == MAXCHR )
                {
                    // Character latches for generalized movement
                    dvx = chr[cnt].latchx;
                    dvy = chr[cnt].latchy;

                    // Reverse movements for daze
                    if ( chr[cnt].dazetime > 0 )
                    {
                        dvx = -dvx;
                        dvy = -dvy;
                    }

                    // Switch x and y for daze
                    if ( chr[cnt].grogtime > 0 )
                    {
                        dvmax = dvx;
                        dvx = dvy;
                        dvy = dvmax;
                    }

                    // Get direction from the DESIRED change in velocity
                    if ( chr[cnt].turnmode == TURNMODEWATCH )
                    {
                        if ( ( ABS( dvx ) > WATCHMIN || ABS( dvy ) > WATCHMIN ) )
                        {
                            chr[cnt].turnleftright = terp_dir( chr[cnt].turnleftright, ( ATAN2( dvy, dvx ) + PI ) * 65535 / ( TWO_PI ) );
                        }
                    }

                    // Face the target
                    watchtarget = ( chr[cnt].turnmode == TURNMODEWATCHTARGET );

                    if ( watchtarget )
                    {
                        if ( cnt != chr[cnt].aitarget )
                            chr[cnt].turnleftright = terp_dir( chr[cnt].turnleftright, ( ATAN2( chr[chr[cnt].aitarget].ypos - chr[cnt].ypos, chr[chr[cnt].aitarget].xpos - chr[cnt].xpos ) + PI ) * 65535 / ( TWO_PI ) );
                    }

                    if ( madframefx[chr[cnt].frame]&MADFXSTOP )
                    {
                        dvx = 0;
                        dvy = 0;
                    }
                    else
                    {
                        // Limit to max acceleration
                        dvmax = chr[cnt].maxaccel;

                        if ( dvx < -dvmax ) dvx = -dvmax;

                        if ( dvx > dvmax ) dvx = dvmax;

                        if ( dvy < -dvmax ) dvy = -dvmax;

                        if ( dvy > dvmax ) dvy = dvmax;

                        chr[cnt].xvel += dvx;
                        chr[cnt].yvel += dvy;
                    }

                    // Get direction from ACTUAL change in velocity
                    if ( chr[cnt].turnmode == TURNMODEVELOCITY )
                    {
                        if ( dvx < -TURNSPD || dvx > TURNSPD || dvy < -TURNSPD || dvy > TURNSPD )
                        {
                            if ( chr[cnt].isplayer )
                            {
                                // Players turn quickly
                                chr[cnt].turnleftright = terp_dir_fast( chr[cnt].turnleftright, ( ATAN2( dvy, dvx ) + PI ) * 65535 / ( TWO_PI ) );
                            }
                            else
                            {
                                // AI turn slowly
                                chr[cnt].turnleftright = terp_dir( chr[cnt].turnleftright, ( ATAN2( dvy, dvx ) + PI ) * 65535 / ( TWO_PI ) );
                            }
                        }
                    }

                    // Otherwise make it spin
                    else if ( chr[cnt].turnmode == TURNMODESPIN )
                    {
                        chr[cnt].turnleftright += SPINRATE;
                    }
                }

                // Character latches for generalized buttons
                if ( chr[cnt].latchbutton != 0 )
                {
                    if ( chr[cnt].latchbutton&LATCHBUTTONJUMP )
                    {
                        if ( chr[cnt].attachedto != MAXCHR && chr[cnt].jumptime == 0 )
                        {
                            detach_character_from_mount( cnt, btrue, btrue );
                            chr[cnt].jumptime = JUMPDELAY;
                            chr[cnt].zvel = DISMOUNTZVEL;

                            if ( chr[cnt].flyheight != 0 )
                                chr[cnt].zvel = DISMOUNTZVELFLY;

                            chr[cnt].zpos += chr[cnt].zvel;

                            if ( chr[cnt].jumpnumberreset != JUMPINFINITE && chr[cnt].jumpnumber != 0 )
                                chr[cnt].jumpnumber--;

                            // Play the jump sound
                            if ( capwavejump[chr[cnt].model] >= 0 && capwavejump[chr[cnt].model] < MAXWAVE )
                            {
                                play_mix( chr[cnt].xpos, chr[cnt].ypos, capwaveindex[chr[cnt].model] + capwavejump[chr[cnt].model] );
                            }

                        }

                        if ( chr[cnt].jumptime == 0 && chr[cnt].jumpnumber != 0 && chr[cnt].flyheight == 0 )
                        {
                            if ( chr[cnt].jumpnumberreset != 1 || chr[cnt].jumpready )
                            {
                                // Make the character jump
                                chr[cnt].hitready = btrue;

                                if ( chr[cnt].inwater )
                                {
                                    chr[cnt].zvel = WATERJUMP;
                                }
                                else
                                {
                                    chr[cnt].zvel = chr[cnt].jump;
                                }

                                chr[cnt].jumptime = JUMPDELAY;
                                chr[cnt].jumpready = bfalse;

                                if ( chr[cnt].jumpnumberreset != JUMPINFINITE ) chr[cnt].jumpnumber--;

                                // Set to jump animation if not doing anything better
                                if ( chr[cnt].actionready )    play_action( cnt, ACTIONJA, btrue );

                                // Play the jump sound (Boing!)
                                distance = ABS( camtrackx - chr[cnt].xpos ) + ABS( camtracky - chr[cnt].ypos );
                                volume = -distance;

                                if ( volume > VOLMIN )
                                {
                                    if ( capwavejump[chr[cnt].model] >= 0 && capwavejump[chr[cnt].model] < MAXWAVE )
                                    {
                                        play_mix( chr[cnt].xpos, chr[cnt].ypos, capwaveindex[chr[cnt].model] + capwavejump[chr[cnt].model] );
                                    }
                                }

                            }
                        }
                    }

                    if ( ( chr[cnt].latchbutton&LATCHBUTTONALTLEFT ) && chr[cnt].actionready && chr[cnt].reloadtime == 0 )
                    {
                        chr[cnt].reloadtime = GRABDELAY;

                        if ( chr[cnt].holdingwhich[0] == MAXCHR )
                        {
                            // Grab left
                            play_action( cnt, ACTIONME, bfalse );
                        }
                        else
                        {
                            // Drop left
                            play_action( cnt, ACTIONMA, bfalse );
                        }
                    }

                    if ( ( chr[cnt].latchbutton&LATCHBUTTONALTRIGHT ) && chr[cnt].actionready && chr[cnt].reloadtime == 0 )
                    {
                        chr[cnt].reloadtime = GRABDELAY;

                        if ( chr[cnt].holdingwhich[1] == MAXCHR )
                        {
                            // Grab right
                            play_action( cnt, ACTIONMF, bfalse );
                        }
                        else
                        {
                            // Drop right
                            play_action( cnt, ACTIONMB, bfalse );
                        }
                    }

                    if ( ( chr[cnt].latchbutton&LATCHBUTTONPACKLEFT ) && chr[cnt].actionready && chr[cnt].reloadtime == 0 )
                    {
                        chr[cnt].reloadtime = PACKDELAY;
                        item = chr[cnt].holdingwhich[0];

                        if ( item != MAXCHR )
                        {
                            if ( ( chr[item].iskursed || capistoobig[chr[item].model] ) && !capisequipment[chr[item].model] )
                            {
                                // The item couldn't be put away
                                chr[item].alert |= ALERTIFNOTPUTAWAY;
                            }
                            else
                            {
                                // Put the item into the pack
                                add_item_to_character_pack( item, cnt );
                            }
                        }
                        else
                        {
                            // Get a new one out and put it in hand
                            get_item_from_character_pack( cnt, GRIPLEFT, bfalse );
                        }

                        // Make it take a little time
                        play_action( cnt, ACTIONMG, bfalse );
                    }

                    if ( ( chr[cnt].latchbutton&LATCHBUTTONPACKRIGHT ) && chr[cnt].actionready && chr[cnt].reloadtime == 0 )
                    {
                        chr[cnt].reloadtime = PACKDELAY;
                        item = chr[cnt].holdingwhich[1];

                        if ( item != MAXCHR )
                        {
                            if ( ( chr[item].iskursed || capistoobig[chr[item].model] ) && !capisequipment[chr[item].model] )
                            {
                                // The item couldn't be put away
                                chr[item].alert |= ALERTIFNOTPUTAWAY;
                            }
                            else
                            {
                                // Put the item into the pack
                                add_item_to_character_pack( item, cnt );
                            }
                        }
                        else
                        {
                            // Get a new one out and put it in hand
                            get_item_from_character_pack( cnt, GRIPRIGHT, bfalse );
                        }

                        // Make it take a little time
                        play_action( cnt, ACTIONMG, bfalse );
                    }

                    if ( chr[cnt].latchbutton&LATCHBUTTONLEFT && chr[cnt].reloadtime == 0 )
                    {
                        // Which weapon?
                        weapon = chr[cnt].holdingwhich[0];

                        if ( weapon == MAXCHR )
                        {
                            // Unarmed means character itself is the weapon
                            weapon = cnt;
                        }

                        action = capweaponaction[chr[weapon].model];

                        // Can it do it?
                        allowedtoattack = btrue;

                        // First check if reload time and action is okay
                        if ( !madactionvalid[chr[cnt].model][action] || chr[weapon].reloadtime > 0 ) allowedtoattack = bfalse;
                        else
                        {
                            // Then check if a skill is needed
                            if ( capneedskillidtouse[chr[weapon].model] )
                            {
                                if (check_skills( cnt, capidsz[chr[weapon].model][IDSZ_SKILL]) == bfalse )
                                    allowedtoattack = bfalse;
                            }
                        }

                        if ( !allowedtoattack )
                        {
                            if ( chr[weapon].reloadtime == 0 )
                            {
                                // This character can't use this weapon
                                chr[weapon].reloadtime = 50;

                                if ( chr[cnt].staton )
                                {
                                    // Tell the player that they can't use this weapon
                                    STRING text;
                                    snprintf( text, sizeof(text),  "%s can't use this item...", chr[cnt].name );
                                    debug_message( text );
                                }
                            }
                        }

                        if ( action == ACTIONDA )
                        {
                            allowedtoattack = bfalse;

                            if ( chr[weapon].reloadtime == 0 )
                            {
                                chr[weapon].alert = chr[weapon].alert | ALERTIFUSED;
                            }
                        }

                        if ( allowedtoattack )
                        {
                            // Rearing mount
                            mount = chr[cnt].attachedto;

                            if ( mount != MAXCHR )
                            {
                                allowedtoattack = capridercanattack[chr[mount].model];

                                if ( chr[mount].ismount && chr[mount].alive && !chr[mount].isplayer && chr[mount].actionready )
                                {
                                    if ( ( action != ACTIONPA || !allowedtoattack ) && chr[cnt].actionready )
                                    {
                                        play_action( chr[cnt].attachedto, ACTIONUA + ( rand()&1 ), bfalse );
                                        chr[chr[cnt].attachedto].alert |= ALERTIFUSED;
                                        chr[cnt].lastitemused = mount;
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
                                if ( chr[cnt].actionready && madactionvalid[chr[cnt].model][action] )
                                {
                                    // Check mana cost
                                    if ( chr[cnt].mana >= chr[weapon].manacost || chr[cnt].canchannel )
                                    {
                                        cost_mana( cnt, chr[weapon].manacost, weapon );
                                        // Check life healing
                                        chr[cnt].life += chr[weapon].lifeheal;

                                        if ( chr[cnt].life > chr[cnt].lifemax )  chr[cnt].life = chr[cnt].lifemax;

                                        actionready = bfalse;

                                        if ( action == ACTIONPA )
                                            actionready = btrue;

                                        action += rand() & 1;
                                        play_action( cnt, action, actionready );

                                        if ( weapon != cnt )
                                        {
                                            // Make the weapon attack too
                                            play_action( weapon, ACTIONMJ, bfalse );
                                            chr[weapon].alert = chr[weapon].alert | ALERTIFUSED;
                                            chr[cnt].lastitemused = weapon;
                                        }
                                        else
                                        {
                                            // Flag for unarmed attack
                                            chr[cnt].alert |= ALERTIFUSED;
                                            chr[cnt].lastitemused = cnt;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if ( chr[cnt].latchbutton&LATCHBUTTONRIGHT && chr[cnt].reloadtime == 0 )
                    {
                        // Which weapon?
                        weapon = chr[cnt].holdingwhich[1];

                        if ( weapon == MAXCHR )
                        {
                            // Unarmed means character itself is the weapon
                            weapon = cnt;
                        }

                        action = capweaponaction[chr[weapon].model] + 2;

                        // Can it do it? (other hand)
                        allowedtoattack = btrue;

                        // First check if reload time and action is okay
                        if ( !madactionvalid[chr[cnt].model][action] || chr[weapon].reloadtime > 0 ) allowedtoattack = bfalse;
                        else
                        {
                            // Then check if a skill is needed
                            if ( capneedskillidtouse[chr[weapon].model] )
                            {
                                if ( check_skills( cnt, capidsz[chr[weapon].model][IDSZ_SKILL]) == bfalse   )
                                    allowedtoattack = bfalse;
                            }
                        }

                        if ( !allowedtoattack )
                        {
                            if ( chr[weapon].reloadtime == 0 )
                            {
                                // This character can't use this weapon
                                chr[weapon].reloadtime = 50;

                                if ( chr[cnt].staton )
                                {
                                    // Tell the player that they can't use this weapon
                                    STRING text;
                                    snprintf( text, sizeof(text), "%s can't use this item...", chr[cnt].name );
                                    debug_message( text );
                                }
                            }
                        }

                        if ( action == ACTIONDC )
                        {
                            allowedtoattack = bfalse;

                            if ( chr[weapon].reloadtime == 0 )
                            {
                                chr[weapon].alert = chr[weapon].alert | ALERTIFUSED;
                                chr[cnt].lastitemused = weapon;
                            }
                        }

                        if ( allowedtoattack )
                        {
                            // Rearing mount
                            mount = chr[cnt].attachedto;

                            if ( mount != MAXCHR )
                            {
                                allowedtoattack = capridercanattack[chr[mount].model];

                                if ( chr[mount].ismount && chr[mount].alive && !chr[mount].isplayer && chr[mount].actionready )
                                {
                                    if ( ( action != ACTIONPC || !allowedtoattack ) && chr[cnt].actionready )
                                    {
                                        play_action( chr[cnt].attachedto, ACTIONUC + ( rand()&1 ), bfalse );
                                        chr[chr[cnt].attachedto].alert |= ALERTIFUSED;
                                        chr[cnt].lastitemused = chr[cnt].attachedto;
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
                                if ( chr[cnt].actionready && madactionvalid[chr[cnt].model][action] )
                                {
                                    // Check mana cost
                                    if ( chr[cnt].mana >= chr[weapon].manacost || chr[cnt].canchannel )
                                    {
                                        cost_mana( cnt, chr[weapon].manacost, weapon );
                                        // Check life healing
                                        chr[cnt].life += chr[weapon].lifeheal;

                                        if ( chr[cnt].life > chr[cnt].lifemax )  chr[cnt].life = chr[cnt].lifemax;

                                        actionready = bfalse;

                                        if ( action == ACTIONPC )
                                            actionready = btrue;

                                        action += rand() & 1;
                                        play_action( cnt, action, actionready );

                                        if ( weapon != cnt )
                                        {
                                            // Make the weapon attack too
                                            play_action( weapon, ACTIONMJ, bfalse );
                                            chr[weapon].alert = chr[weapon].alert | ALERTIFUSED;
                                            chr[cnt].lastitemused = weapon;
                                        }
                                        else
                                        {
                                            // Flag for unarmed attack
                                            chr[cnt].alert |= ALERTIFUSED;
                                            chr[cnt].lastitemused = cnt;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Is the character in the air?
            level = chr[cnt].level;

            if ( chr[cnt].flyheight == 0 )
            {
                chr[cnt].zpos += chr[cnt].zvel;

                if ( chr[cnt].zpos > level || ( chr[cnt].zvel > STOPBOUNCING && chr[cnt].zpos > level - STOPBOUNCING ) )
                {
                    // Character is in the air
                    chr[cnt].jumpready = bfalse;
                    chr[cnt].zvel += gravity;

                    // Down jump timers for flapping characters
                    if ( chr[cnt].jumptime != 0 ) chr[cnt].jumptime--;

                    // Airborne characters still get friction to make control easier
                    friction = airfriction;
                }
                else
                {
                    // Character is on the ground
                    chr[cnt].zpos = level;
                    grounded = btrue;

                    if ( chr[cnt].hitready )
                    {
                        chr[cnt].alert |= ALERTIFHITGROUND;
                        chr[cnt].hitready = bfalse;
                    }

                    // Make the characters slide
                    twist = meshtwist[chr[cnt].onwhichfan];
                    friction = noslipfriction;

                    if ( meshfx[chr[cnt].onwhichfan]&MESHFXSLIPPY )
                    {
                        if ( wateriswater && ( meshfx[chr[cnt].onwhichfan]&MESHFXWATER ) && chr[cnt].level < watersurfacelevel + RAISE + 1 )
                        {
                            // It says it's slippy, but the water is covering it...
                            // Treat exactly as normal
                            chr[cnt].jumpready = btrue;
                            chr[cnt].jumpnumber = chr[cnt].jumpnumberreset;

                            if ( chr[cnt].jumptime > 0 ) chr[cnt].jumptime--;

                            chr[cnt].zvel = -chr[cnt].zvel * chr[cnt].dampen;
                            chr[cnt].zvel += gravity;
                        }
                        else
                        {
                            // It's slippy all right...
                            friction = slippyfriction;
                            chr[cnt].jumpready = btrue;

                            if ( chr[cnt].jumptime > 0 ) chr[cnt].jumptime--;

                            if ( chr[cnt].weight != 65535 )
                            {
                                // Slippy hills make characters slide
                                chr[cnt].xvel += vellrtwist[twist];
                                chr[cnt].yvel += veludtwist[twist];
                                chr[cnt].zvel = -SLIDETOLERANCE;
                            }

                            if ( flattwist[twist] )
                            {
                                // Reset jumping on flat areas of slippiness
                                chr[cnt].jumpnumber = chr[cnt].jumpnumberreset;
                            }
                        }
                    }
                    else
                    {
                        // Reset jumping
                        chr[cnt].jumpready = btrue;
                        chr[cnt].jumpnumber = chr[cnt].jumpnumberreset;

                        if ( chr[cnt].jumptime > 0 ) chr[cnt].jumptime--;

                        chr[cnt].zvel = -chr[cnt].zvel * chr[cnt].dampen;
                        chr[cnt].zvel += gravity;
                    }

                    // Characters with sticky butts lie on the surface of the mesh
                    if ( chr[cnt].stickybutt || !chr[cnt].alive )
                    {
                        maplr = chr[cnt].turnmaplr;
                        maplr = ( maplr << 6 ) - maplr + maplrtwist[twist];
                        mapud = chr[cnt].turnmapud;
                        mapud = ( mapud << 6 ) - mapud + mapudtwist[twist];
                        chr[cnt].turnmaplr = maplr >> 6;
                        chr[cnt].turnmapud = mapud >> 6;
                    }
                }
            }
            else
            {
                //  Flying
                chr[cnt].jumpready = bfalse;
                chr[cnt].zpos += chr[cnt].zvel;

                if ( level < 0 ) level = 0;  // Don't fall in pits...

                chr[cnt].zvel += ( level + chr[cnt].flyheight - chr[cnt].zpos ) * FLYDAMPEN;

                if ( chr[cnt].zpos < level )
                {
                    chr[cnt].zpos = level;
                    chr[cnt].zvel = 0;
                }

                // Airborne characters still get friction to make control easier
                friction = airfriction;
            }

            // Move the character
            chr[cnt].xpos += chr[cnt].xvel;

            if ( __chrhitawall( cnt ) ) { chr[cnt].xpos = chr[cnt].oldx; chr[cnt].xvel = -chr[cnt].xvel; }

            chr[cnt].ypos += chr[cnt].yvel;

            if ( __chrhitawall( cnt ) ) { chr[cnt].ypos = chr[cnt].oldy; chr[cnt].yvel = -chr[cnt].yvel; }

            // Apply friction for next time
            chr[cnt].xvel = chr[cnt].xvel * friction;
            chr[cnt].yvel = chr[cnt].yvel * friction;

            // Animate the character
            chr[cnt].lip = ( chr[cnt].lip + 64 );

            if ( chr[cnt].lip == 192 )
            {
                // Check frame effects
                if ( madframefx[chr[cnt].frame]&MADFXACTLEFT )
                    character_swipe( cnt, 0 );

                if ( madframefx[chr[cnt].frame]&MADFXACTRIGHT )
                    character_swipe( cnt, 1 );

                if ( madframefx[chr[cnt].frame]&MADFXGRABLEFT )
                    character_grab_stuff( cnt, GRIPLEFT, bfalse );

                if ( madframefx[chr[cnt].frame]&MADFXGRABRIGHT )
                    character_grab_stuff( cnt, GRIPRIGHT, bfalse );

                if ( madframefx[chr[cnt].frame]&MADFXCHARLEFT )
                    character_grab_stuff( cnt, GRIPLEFT, btrue );

                if ( madframefx[chr[cnt].frame]&MADFXCHARRIGHT )
                    character_grab_stuff( cnt, GRIPRIGHT, btrue );

                if ( madframefx[chr[cnt].frame]&MADFXDROPLEFT )
                    detach_character_from_mount( chr[cnt].holdingwhich[0], bfalse, btrue );

                if ( madframefx[chr[cnt].frame]&MADFXDROPRIGHT )
                    detach_character_from_mount( chr[cnt].holdingwhich[1], bfalse, btrue );

                if ( madframefx[chr[cnt].frame]&MADFXPOOF && !chr[cnt].isplayer )
                    valuegopoof = btrue;

                if ( madframefx[chr[cnt].frame]&MADFXFOOTFALL )
                {
                    if ( capwavefootfall[chr[cnt].model] >= 0 && capwavefootfall[chr[cnt].model] < MAXWAVE )
                    {
                        play_mix( chr[cnt].xpos, chr[cnt].ypos, capwaveindex[chr[cnt].model] + capwavefootfall[chr[cnt].model] );
                    }
                }
            }

            if ( chr[cnt].lip == 0 )
            {
                // Change frames
                chr[cnt].lastframe = chr[cnt].frame;
                chr[cnt].frame++;

                if ( chr[cnt].frame == madactionend[chr[cnt].model][chr[cnt].action] )
                {
                    // Action finished
                    if ( chr[cnt].keepaction )
                    {
                        // Keep the last frame going
                        chr[cnt].frame = chr[cnt].lastframe;
                    }
                    else
                    {
                        if ( !chr[cnt].loopaction )
                        {
                            // Go on to the next action
                            chr[cnt].action = chr[cnt].nextaction;
                            chr[cnt].nextaction = ACTIONDA;
                        }
                        else
                        {
                            // See if the character is mounted...
                            if ( chr[cnt].attachedto != MAXCHR )
                            {
                                chr[cnt].action = ACTIONMI;
                            }
                        }

                        chr[cnt].frame = madactionstart[chr[cnt].model][chr[cnt].action];
                    }

                    chr[cnt].actionready = btrue;
                }
            }

            // Do "Be careful!" delay
            if ( chr[cnt].carefultime != 0 )
            {
                chr[cnt].carefultime--;
            }

            // Get running, walking, sneaking, or dancing, from speed
            if ( !( chr[cnt].keepaction || chr[cnt].loopaction ) )
            {
                framelip = madframelip[chr[cnt].frame];  // 0 - 15...  Way through animation

                if ( chr[cnt].actionready && chr[cnt].lip == 0 && grounded && chr[cnt].flyheight == 0 && ( framelip&7 ) < 2 )
                {
                    // Do the motion stuff
                    speed = ABS( ( int ) chr[cnt].xvel ) + ABS( ( int ) chr[cnt].yvel );

                    if ( speed < chr[cnt].sneakspd )
                    {
//                        chr[cnt].nextaction = ACTIONDA;
                        // Do boredom
                        chr[cnt].boretime--;

                        if ( chr[cnt].boretime < 0 )
                        {
                            chr[cnt].alert |= ALERTIFBORED;
                            chr[cnt].boretime = BORETIME;
                        }
                        else
                        {
                            // Do standstill
                            if ( chr[cnt].action > ACTIONDD )
                            {
                                chr[cnt].action = ACTIONDA;
                                chr[cnt].frame = madactionstart[chr[cnt].model][chr[cnt].action];
                            }
                        }
                    }
                    else
                    {
                        chr[cnt].boretime = BORETIME;

                        if ( speed < chr[cnt].walkspd )
                        {
                            chr[cnt].nextaction = ACTIONWA;

                            if ( chr[cnt].action != ACTIONWA )
                            {
                                chr[cnt].frame = madframeliptowalkframe[chr[cnt].model][LIPWA][framelip];
                                chr[cnt].action = ACTIONWA;
                            }
                        }
                        else
                        {
                            if ( speed < chr[cnt].runspd )
                            {
                                chr[cnt].nextaction = ACTIONWB;

                                if ( chr[cnt].action != ACTIONWB )
                                {
                                    chr[cnt].frame = madframeliptowalkframe[chr[cnt].model][LIPWB][framelip];
                                    chr[cnt].action = ACTIONWB;
                                }
                            }
                            else
                            {
                                chr[cnt].nextaction = ACTIONWC;

                                if ( chr[cnt].action != ACTIONWC )
                                {
                                    chr[cnt].frame = madframeliptowalkframe[chr[cnt].model][LIPWC][framelip];
                                    chr[cnt].action = ACTIONWC;
                                }
                            }
                        }
                    }
                }
            }

            // Do poofing
            if ( valuegopoof )
            {
                if ( chr[cnt].attachedto != MAXCHR )
                    detach_character_from_mount( cnt, btrue, bfalse );

                if ( chr[cnt].holdingwhich[0] != MAXCHR )
                    detach_character_from_mount( chr[cnt].holdingwhich[0], btrue, bfalse );

                if ( chr[cnt].holdingwhich[1] != MAXCHR )
                    detach_character_from_mount( chr[cnt].holdingwhich[1], btrue, bfalse );

                free_inventory( cnt );
                free_one_character( cnt );
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void setup_characters( char *modname )
{
    // ZZ> This function sets up character data, loaded from "SPAWN.TXT"
    int currentcharacter = 0, lastcharacter, passage, content, money, level, skin, cnt, tnc, localnumber = 0;
    bool_t ghost;
    char cTmp;
    Uint8 team, stat;
    char *name;
    char itislocal;
    char myname[256], newloadname[256];
    Uint16 facing, attach, grip = NORTH;
    Sint32 slot;
    float x, y, z;
    FILE *fileread;

    // Turn all characters off
    free_all_characters();

    // Turn some back on
    make_newloadname( modname, "gamedat" SLASH_STR "spawn.txt", newloadname );
    fileread = fopen( newloadname, "r" );
    currentcharacter = MAXCHR;

    if ( fileread )
    {
        while ( goto_colon_yesno( fileread ) )
        {
            fscanf( fileread, "%s", myname );
            name = myname;

            if ( myname[0] == 'N' && myname[1] == 'O' && myname[2] == 'N' &&
                    myname[3] == 'E' && myname[4] == 0 )
            {
                // Random name
                name = NULL;
            }

            cnt = 0;

            while ( cnt < 256 )
            {
                if ( myname[cnt] == '_' )  myname[cnt] = ' ';

                cnt++;
            }

            fscanf( fileread, "%d", &slot );
            fscanf( fileread, "%f%f%f", &x, &y, &z ); x = x * 128;  y = y * 128;  z = z * 128;
            cTmp = get_first_letter( fileread );
            attach = MAXCHR;
            facing = NORTH;

            if ( cTmp == 'S' || cTmp == 's' )  facing = SOUTH;

            if ( cTmp == 'E' || cTmp == 'e' )  facing = EAST;

            if ( cTmp == 'W' || cTmp == 'w' )  facing = WEST;

            if ( cTmp == 'L' || cTmp == 'l' )  { attach = currentcharacter; grip = GRIPLEFT;   }

            if ( cTmp == 'R' || cTmp == 'r' )  { attach = currentcharacter; grip = GRIPRIGHT;  }

            if ( cTmp == 'I' || cTmp == 'i' )  { attach = currentcharacter; grip = INVENTORY;  }

            fscanf( fileread, "%d%d%d%d%d", &money, &skin, &passage, &content, &level );
            cTmp = get_first_letter( fileread );
            stat = bfalse;

            if ( cTmp == 'T' || cTmp == 't' ) stat = btrue;

            cTmp = get_first_letter( fileread );
            ghost = bfalse;

            if ( cTmp == 'T' || cTmp == 't' ) ghost = btrue;

            team = ( get_first_letter( fileread ) - 'A' ) % MAXTEAM;

            // Spawn the character
            if ( team < numplayer || !rtscontrol || team >= MAXPLAYER )
            {
                lastcharacter = spawn_one_character( x, y, z, slot, team, skin, facing, name, MAXCHR );

                if ( lastcharacter < MAXCHR )
                {
                    chr[lastcharacter].money += money;

                    if ( chr[lastcharacter].money > MAXMONEY )  chr[lastcharacter].money = MAXMONEY;

                    if ( chr[lastcharacter].money < 0 )  chr[lastcharacter].money = 0;

                    chr[lastcharacter].aicontent = content;
                    chr[lastcharacter].passage = passage;

                    if ( attach == MAXCHR )
                    {
                        // Free character
                        currentcharacter = lastcharacter;
                        make_one_character_matrix( currentcharacter );
                    }
                    else
                    {
                        // Attached character
                        if ( grip != INVENTORY )
                        {
                            // Wielded character
                            attach_character_to_mount( lastcharacter, currentcharacter, grip );
                            let_character_think( lastcharacter );  // Empty the grabbed messages
                        }
                        else
                        {
                            // Inventory character
                            add_item_to_character_pack( lastcharacter, currentcharacter );
                            chr[lastcharacter].alert |= ALERTIFGRABBED;  // Make spellbooks change
                            chr[lastcharacter].attachedto = currentcharacter;  // Make grab work
                            let_character_think( lastcharacter );  // Empty the grabbed messages
                            chr[lastcharacter].attachedto = MAXCHR;  // Fix grab
                        }
                    }

                    // Turn on player input devices
                    if ( stat )
                    {
                        if ( importamount == 0 )
                        {
                            if ( playeramount < 2 )
                            {
                                if ( numstat == 0 )
                                {
                                    // Single player module
                                    add_player( lastcharacter, numstat, INPUT_BITS_MOUSE | INPUT_BITS_KEYBOARD | INPUT_BITS_JOYA | INPUT_BITS_JOYB );
                                }
                            }
                            else
                            {
                                if ( !networkon )
                                {
                                    if ( playeramount == 2 )
                                    {
                                        // Two player hack
                                        if ( numstat == 0 )
                                        {
                                            // First player
                                            add_player( lastcharacter, numstat, INPUT_BITS_MOUSE | INPUT_BITS_KEYBOARD | INPUT_BITS_JOYB );
                                        }

                                        if ( numstat == 1 )
                                        {
                                            // Second player
                                            add_player( lastcharacter, numstat, INPUT_BITS_JOYA );
                                        }
                                    }
                                    else
                                    {
                                        // Three player hack
                                        if ( numstat == 0 )
                                        {
                                            // First player
                                            add_player( lastcharacter, numstat, INPUT_BITS_KEYBOARD );
                                        }

                                        if ( numstat == 1 )
                                        {
                                            // Second player
                                            add_player( lastcharacter, numstat, INPUT_BITS_JOYA );
                                        }

                                        if ( numstat == 2 )
                                        {
                                            // Third player
                                            add_player( lastcharacter, numstat, INPUT_BITS_JOYB | INPUT_BITS_MOUSE );
                                        }
                                    }
                                }
                                else
                                {
                                    // One player per machine hack
                                    if ( localmachine == numstat )
                                    {
                                        add_player( lastcharacter, numstat, INPUT_BITS_MOUSE | INPUT_BITS_KEYBOARD | INPUT_BITS_JOYA | INPUT_BITS_JOYB );
                                    }
                                }
                            }
                        }

                        if ( numstat < importamount )
                        {
                            // Multiplayer import module
                            itislocal = bfalse;
                            tnc = 0;

                            while ( tnc < numimport )
                            {
                                if ( capimportslot[chr[lastcharacter].model] == localslot[tnc] )
                                {
                                    itislocal = btrue;
                                    localnumber = tnc;
                                    tnc = numimport;
                                }

                                tnc++;
                            }

                            if ( itislocal )
                            {
                                // It's a local player
                                add_player( lastcharacter, numstat, localcontrol[localnumber] );
                            }
                            else
                            {
                                // It's a remote player
                                add_player( lastcharacter, numstat, INPUT_BITS_NONE );
                            }
                        }

                        // Turn on the stat display
                        add_stat( lastcharacter );
                    }

                    // Set the starting level
                    if ( !chr[lastcharacter].isplayer )
                    {
                        // Let the character gain levels
                        level = level - 1;

                        while ( chr[lastcharacter].experiencelevel < level && chr[lastcharacter].experience < MAXXP )
                        {
                            give_experience( lastcharacter, 25, XPDIRECT );
                        }
                    }

                    if ( ghost )
                    {
                        // Make the character a ghost !!!BAD!!!  Can do with enchants
                        chr[lastcharacter].alpha = 128;
                        chr[lastcharacter].light = 255;
                    }
                }
            }
        }

        fclose( fileread );
    }
    else
    {
        log_error( "Cannot read file: %s", newloadname );
    }

    clear_messages();

    // Make sure local players are displayed first
    sort_stat();

    // Fix tilting trees problem
    tilt_characters_to_terrain();
}

//--------------------------------------------------------------------------------------------
void set_one_player_latch( Uint16 player )
{
    // ZZ> This function converts input readings to latch settings, so players can
    //     move around
    float newx, newy;
    Uint16 turnsin, turncos, character;
    Uint8 device;
    float dist, scale;
    float inputx, inputy;

    // Check to see if we need to bother
    if ( plavalid[player] && pladevice[player] != INPUT_BITS_NONE )
    {
        // Make life easier
        character = plaindex[player];
        device = pladevice[player];

        // Clear the player's latch buffers
        plalatchbutton[player] = 0;
        plalatchx[player] = 0;
        plalatchy[player] = 0;

        // Mouse routines
        if ( ( device & INPUT_BITS_MOUSE ) && mous.on )
        {
            // Movement
            newx = 0;
            newy = 0;

            if ( ( autoturncamera == 255 && numlocalpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) )  // Don't allow movement in camera control mode
            {
                dist = SQRT( mous.x * mous.x + mous.y * mous.y );

                if ( dist > 0 )
                {
                    scale = mous.sense / dist;

                    if ( dist < mous.sense )
                    {
                        scale = dist / mous.sense;
                    }

                    scale = scale / mous.sense;

                    if ( chr[character].attachedto != MAXCHR )
                    {
                        // Mounted
                        inputx = mous.x * chr[chr[character].attachedto].maxaccel * scale;
                        inputy = mous.y * chr[chr[character].attachedto].maxaccel * scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = mous.x * chr[character].maxaccel * scale;
                        inputy = mous.y * chr[character].maxaccel * scale;
                    }

                    turnsin = ( camturnleftrightone * 16383 );
                    turnsin = turnsin & 16383;
                    turncos = ( turnsin + 4096 ) & 16383;

                    if ( autoturncamera == 255 &&
                            numlocalpla == 1 &&
                            control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) == 0 )  inputx = 0;

                    newx = ( inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
                    newy = (-inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
//                    plalatchx[player]+=newx;
//                    plalatchy[player]+=newy;
                }
            }

            plalatchx[player] += newx * mous.cover + mous.latcholdx * mous.sustain;
            plalatchy[player] += newy * mous.cover + mous.latcholdy * mous.sustain;
            mous.latcholdx = plalatchx[player];
            mous.latcholdy = plalatchy[player];

            // Sustain old movements to ease mouse play
//            plalatchx[player]+=mous.latcholdx*mous.sustain;
//            plalatchy[player]+=mous.latcholdy*mous.sustain;
//            mous.latcholdx = plalatchx[player];
//            mous.latcholdy = plalatchy[player];
            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_JUMP ) )
                plalatchbutton[player] |= LATCHBUTTONJUMP;

            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_USE ) )
                plalatchbutton[player] |= LATCHBUTTONLEFT;

            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_GET ) )
                plalatchbutton[player] |= LATCHBUTTONALTLEFT;

            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTONPACKLEFT;

            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_USE ) )
                plalatchbutton[player] |= LATCHBUTTONRIGHT;

            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_GET ) )
                plalatchbutton[player] |= LATCHBUTTONALTRIGHT;

            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTONPACKRIGHT;
        }

        // Joystick A routines
        if ( ( device & INPUT_BITS_JOYA ) && joy[0].on )
        {
            // Movement
            if ( ( autoturncamera == 255 && numlocalpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )
            {
                newx = 0;
                newy = 0;
                inputx = 0;
                inputy = 0;
                dist = SQRT( joy[0].x * joy[0].x + joy[0].y * joy[0].y );

                if ( dist > 0 )
                {
                    scale = 1.0f / dist;

                    if ( chr[character].attachedto != MAXCHR )
                    {
                        // Mounted
                        inputx = joy[0].x * chr[chr[character].attachedto].maxaccel * scale;
                        inputy = joy[0].y * chr[chr[character].attachedto].maxaccel * scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = joy[0].x * chr[character].maxaccel * scale;
                        inputy = joy[0].y * chr[character].maxaccel * scale;
                    }
                }

                turnsin = ( camturnleftrightone * 16383 );
                turnsin = turnsin & 16383;
                turncos = ( turnsin + 4096 ) & 16383;

                if ( autoturncamera == 255 &&
                        numlocalpla == 1 &&
                        !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )  inputx = 0;

                newx = (  inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
                newy = ( -inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
                plalatchx[player] += newx;
                plalatchy[player] += newy;
            }

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_JUMP ) )
                plalatchbutton[player] |= LATCHBUTTONJUMP;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_USE ) )
                plalatchbutton[player] |= LATCHBUTTONLEFT;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_GET ) )
                plalatchbutton[player] |= LATCHBUTTONALTLEFT;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTONPACKLEFT;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_USE ) )
                plalatchbutton[player] |= LATCHBUTTONRIGHT;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_GET ) )
                plalatchbutton[player] |= LATCHBUTTONALTRIGHT;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTONPACKRIGHT;
        }

        // Joystick B routines
        if ( ( device & INPUT_BITS_JOYB ) && joy[1].on )
        {
            // Movement
            if ( ( autoturncamera == 255 && numlocalpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )
            {
                newx = 0;
                newy = 0;
                inputx = 0;
                inputy = 0;
                dist = SQRT( joy[1].x * joy[1].x + joy[1].y * joy[1].y );

                if ( dist > 0 )
                {
                    scale = 1.0f / dist;

                    if ( chr[character].attachedto != MAXCHR )
                    {
                        // Mounted
                        inputx = joy[1].x * chr[chr[character].attachedto].maxaccel * scale;
                        inputy = joy[1].y * chr[chr[character].attachedto].maxaccel * scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = joy[1].x * chr[character].maxaccel * scale;
                        inputy = joy[1].y * chr[character].maxaccel * scale;
                    }
                }

                turnsin = ( camturnleftrightone * 16383 );
                turnsin = turnsin & 16383;
                turncos = ( turnsin + 4096 ) & 16383;

                if ( autoturncamera == 255 &&
                        numlocalpla == 1 &&
                        !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )  inputx = 0;

                newx = (  inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
                newy = ( -inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
                plalatchx[player] += newx;
                plalatchy[player] += newy;
            }

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_JUMP ) )
                plalatchbutton[player] |= LATCHBUTTONJUMP;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_USE ) )
                plalatchbutton[player] |= LATCHBUTTONLEFT;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_GET ) )
                plalatchbutton[player] |= LATCHBUTTONALTLEFT;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTONPACKLEFT;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_USE ) )
                plalatchbutton[player] |= LATCHBUTTONRIGHT;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_GET ) )
                plalatchbutton[player] |= LATCHBUTTONALTRIGHT;

            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTONPACKRIGHT;
        }

        // Keyboard routines
        if ( ( device & INPUT_BITS_KEYBOARD ) && keyb.on )
        {
            // Movement
            if ( chr[character].attachedto != MAXCHR )
            {
                // Mounted
                inputx = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) ) * chr[chr[character].attachedto].maxaccel;
                inputy = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_DOWN ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_UP ) ) * chr[chr[character].attachedto].maxaccel;
            }
            else
            {
                // Unmounted
                inputx = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) ) * chr[character].maxaccel;
                inputy = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_DOWN ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_UP ) ) * chr[character].maxaccel;
            }

            turnsin = ( camturnleftrightone * 16383 );
            turnsin = turnsin & 16383;
            turncos = ( turnsin + 4096 ) & 16383;

            if ( autoturncamera == 255 && numlocalpla == 1 )  inputx = 0;

            newx = (  inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
            newy = ( -inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
            plalatchx[player] += newx;
            plalatchy[player] += newy;

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_JUMP ) )
                plalatchbutton[player] |= LATCHBUTTONJUMP;

            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_USE ) )
                plalatchbutton[player] |= LATCHBUTTONLEFT;

            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_GET ) )
                plalatchbutton[player] |= LATCHBUTTONALTLEFT;

            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTONPACKLEFT;

            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_USE ) )
                plalatchbutton[player] |= LATCHBUTTONRIGHT;

            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_GET ) )
                plalatchbutton[player] |= LATCHBUTTONALTRIGHT;

            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTONPACKRIGHT;
        }
    }
}

//--------------------------------------------------------------------------------------------
void set_local_latches( void )
{
    // ZZ> This function emulates AI thinkin' by setting latches from input devices
    int cnt;

    cnt = 0;

    while ( cnt < MAXPLAYER )
    {
        set_one_player_latch( cnt );
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_onwhichfan( void )
{
    // ZZ> This function figures out which fan characters are on and sets their level
    Uint16 character, distance;
    int x, y, ripand;
    // int volume;
    float level;

    // First figure out which fan each character is in
    character = 0;

    while ( character < MAXCHR )
    {
        if ( chr[character].on && ( !chr[character].inpack ) )
        {
            x = chr[character].xpos;
            y = chr[character].ypos;
            x = x >> 7;
            y = y >> 7;
            chr[character].onwhichfan = meshfanstart[y] + x;
        }

        character++;
    }

    // Get levels every update
    character = 0;

    while ( character < MAXCHR )
    {
        if ( chr[character].on && ( !chr[character].inpack ) )
        {
            level = get_level( chr[character].xpos, chr[character].ypos, chr[character].onwhichfan, chr[character].waterwalk ) + RAISE;

            if ( chr[character].alive )
            {
                if ( meshfx[chr[character].onwhichfan]&MESHFXDAMAGE && chr[character].zpos <= chr[character].level + DAMAGERAISE && chr[character].attachedto == MAXCHR )
                {
                    if ( ( chr[character].damagemodifier[damagetiletype]&DAMAGESHIFT ) != 3 && !chr[character].invictus ) // 3 means they're pretty well immune
                    {
                        distance = ABS( camtrackx - chr[character].xpos ) + ABS( camtracky - chr[character].ypos );

                        if ( distance < damagetilemindistance )
                        {
                            damagetilemindistance = distance;
                        }

                        if ( distance < damagetilemindistance + 256 )
                        {
                            damagetilesoundtime = 0;
                        }

                        if ( chr[character].damagetime == 0 )
                        {
                            damage_character( character, 32768, damagetileamount, 1, damagetiletype, DAMAGETEAM, chr[character].bumplast, DAMFXBLOC | DAMFXARMO );
                            chr[character].damagetime = DAMAGETILETIME;
                        }

                        if ( (damagetileparttype != ((Sint16)~0)) && ( wldframe&damagetilepartand ) == 0 )
                        {
                            spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos,
                                                0, MAXMODEL, damagetileparttype, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                        }
                    }

                    if ( chr[character].reaffirmdamagetype == damagetiletype )
                    {
                        if ( ( wldframe&TILEREAFFIRMAND ) == 0 )
                            reaffirm_attached_particles( character );
                    }
                }
            }

            if ( chr[character].zpos < watersurfacelevel && ( meshfx[chr[character].onwhichfan]&MESHFXWATER ) )
            {
                if ( !chr[character].inwater )
                {
                    // Splash
                    if ( chr[character].attachedto == MAXCHR )
                    {
                        spawn_one_particle( chr[character].xpos, chr[character].ypos, watersurfacelevel + RAISE,
                                            0, MAXMODEL, SPLASH, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                    }

                    chr[character].inwater = btrue;

                    if ( wateriswater )
                    {
                        chr[character].alert |= ALERTIFINWATER;
                    }
                }
                else
                {
                    if ( chr[character].zpos > watersurfacelevel - RIPPLETOLERANCE && capripple[chr[character].model] )
                    {
                        // Ripples
                        ripand = ( ( int )chr[character].xvel != 0 ) | ( ( int )chr[character].yvel != 0 );
                        ripand = RIPPLEAND >> ripand;

                        if ( ( wldframe&ripand ) == 0 && chr[character].zpos < watersurfacelevel && chr[character].alive )
                        {
                            spawn_one_particle( chr[character].xpos, chr[character].ypos, watersurfacelevel,
                                                0, MAXMODEL, RIPPLE, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                        }
                    }

                    if ( wateriswater && ( wldframe&7 ) == 0 )
                    {
                        chr[character].jumpready = btrue;
                        chr[character].jumpnumber = 1; // chr[character].jumpnumberreset;
                    }
                }

                chr[character].xvel = chr[character].xvel * waterfriction;
                chr[character].yvel = chr[character].yvel * waterfriction;
                chr[character].zvel = chr[character].zvel * waterfriction;
            }
            else
            {
                chr[character].inwater = bfalse;
            }

            chr[character].level = level;
        }

        character++;
    }

    // Play the damage tile sound
    if ( damagetilesound >= 0 )
    {
        if ( ( wldframe & 3 ) == 0 )
        {
            // Change the volume...
            /*PORT
                        volume = -(damagetilemindistance + (damagetilesoundtime<<8));
                        volume = volume<<VOLSHIFT;
                        if(volume > VOLMIN)
                        {
                            lpDSBuffer[damagetilesound]->SetVolume(volume);
                        }
                        if(damagetilesoundtime < TILESOUNDTIME)  damagetilesoundtime++;
                        else damagetilemindistance = 9999;
            */
        }
    }
}

//--------------------------------------------------------------------------------------------
void bump_characters( void )
{
    // ZZ> This function sets handles characters hitting other characters or particles
    Uint16 character, particle, entry, pip, direction;
    Uint32 chara, charb, fanblock, prtidparent, prtidtype, eveidremove;
    IDSZ chridvulnerability;
    Sint8 hide;
    int tnc, dist, chrinblock, prtinblock, enchant, temp;
    float xa, ya, za, xb, yb, zb;
    float ax, ay, nx, ny, scale;  // For deflection
    Uint16 facing;

    // Clear the lists


    for ( fanblock = 0; fanblock < numfanblock; fanblock++ )
    {
        meshbumplistchrnum[fanblock] = 0;
        meshbumplistprtnum[fanblock] = 0;
    }

    // Fill 'em back up
    for ( character = 0; character < MAXCHR; character++ )
    {
        if ( chr[character].on && !chr[character].inpack && ( chr[character].attachedto == MAXCHR || chr[character].reaffirmdamagetype != DAMAGENULL ) )
        {
            hide = caphidestate[chr[character].model];
            if ( hide == NOHIDE || hide != chr[character].aistate )
            {
                chr[character].holdingweight = 0;

                if (chr[character].xpos > 0 && chr[character].xpos < meshedgex &&
                        chr[character].ypos > 0 && chr[character].ypos < meshedgey)
                {
                    fanblock = ( ( ( int )chr[character].xpos ) >> 9 ) + meshblockstart[( ( int )chr[character].ypos ) >> 9];

                    // Insert before any other characters on the block
                    entry = meshbumplistchr[fanblock];
                    chr[character].bumpnext = entry;
                    meshbumplistchr[fanblock] = character;
                    meshbumplistchrnum[fanblock]++;
                }
            }
        }
    }



    for ( particle = 0; particle < maxparticles; particle++ )
    {
        if ( prton[particle] && prtbumpsize[particle] )
        {
            if (prtxpos[particle] > 0 && prtxpos[particle] < meshedgex &&
                    prtypos[particle] > 0 && prtypos[particle] < meshedgey)
            {
                fanblock = ( ( ( int )prtxpos[particle] ) >> 9 ) + meshblockstart[( ( int )prtypos[particle] ) >> 9];
                // Insert before any other particles on the block
                entry = meshbumplistprt[fanblock];
                prtbumpnext[particle] = entry;
                meshbumplistprt[fanblock] = particle;
                meshbumplistprtnum[fanblock]++;
            }
        }

    }

    // Check collisions with other characters and bump particles
    // Only check each pair once

    for (chara = 0; chara < MAXCHR; chara++)
    {
        int ixmax, ixmin;
        int iymax, iymin;

        int ix_block, ixmax_block, ixmin_block;
        int iy_block, iymax_block, iymin_block;

        // make sure that it is on
        if ( !chr[chara].on ) continue;

        // Don't bump held objects
        if ( MAXCHR != chr[chara].attachedto ) continue;

        // Don't bump items
        if ( 0 == chr[chara].bumpheight /* || chr[chara].isitem */ ) continue;

        xa = chr[chara].xpos;
        ya = chr[chara].ypos;
        za = chr[chara].zpos;
        chridvulnerability = capidsz[chr[chara].model][IDSZ_VULNERABILITY];

        // determine the size of this object in blocks
        ixmin = chr[chara].xpos - chr[chara].bumpsize; ixmin = CLIP(ixmin, 0, meshedgex);
        ixmax = chr[chara].xpos + chr[chara].bumpsize; ixmax = CLIP(ixmax, 0, meshedgex);

        iymin = chr[chara].ypos - chr[chara].bumpsize; iymin = CLIP(iymin, 0, meshedgey);
        iymax = chr[chara].ypos + chr[chara].bumpsize; iymax = CLIP(iymax, 0, meshedgey);

        ixmax_block = ixmax >> 9;
        ixmin_block = ixmin >> 9;

        iymax_block = iymax >> 9;
        iymin_block = iymin >> 9;

        for (ix_block = ixmin_block; ix_block <= ixmax_block; ix_block++)
        {
            for (iy_block = iymin_block; iy_block <= iymax_block; iy_block++)
            {
                fanblock = ix_block + meshblockstart[iy_block];

                chrinblock = meshbumplistchrnum[fanblock];
                prtinblock = meshbumplistprtnum[fanblock];

                for ( tnc = 0, charb = meshbumplistchr[fanblock];
                        tnc < chrinblock && charb != MAXCHR;
                        tnc++, charb = chr[charb].bumpnext)
                {
                    float dx, dy;

                    // Don't collide with self
                    if (charb == chara) continue;

                    // don't bump items
                    if ( 0 == chr[charb].bumpheight /* || chr[charb].isitem */ ) continue;

                    xb = chr[charb].xpos;
                    yb = chr[charb].ypos;
                    zb = chr[charb].zpos;

                    // First check absolute value diamond
                    dx = ABS( xa - xb );
                    dy = ABS( ya - yb );
                    dist = dx + dy;

                    if ( dist < chr[chara].bumpsizebig || dist < chr[charb].bumpsizebig )
                    {
                        // Then check bounding box square...  Square+Diamond=Octagon
                        if ( ( dx < chr[chara].bumpsize || dx < chr[charb].bumpsize ) &&
                                ( dy < chr[chara].bumpsize || dy < chr[charb].bumpsize ) )
                        {
                            // Pretend that they collided
                            chr[chara].bumplast = charb;
                            chr[charb].bumplast = chara;

                            // Now see if either is on top the other like a platform
                            if ( za > zb + chr[charb].bumpheight - PLATTOLERANCE + chr[chara].zvel - chr[charb].zvel && ( capcanuseplatforms[chr[chara].model] || za > zb + chr[charb].bumpheight ) )
                            {
                                // Is A falling on B?
                                if ( za < zb + chr[charb].bumpheight && chr[charb].platform )//&&chr[chara].flyheight==0)
                                {
                                    // A is inside, coming from above
                                    chr[chara].zpos = ( chr[chara].zpos ) * PLATKEEP + ( chr[charb].zpos + chr[charb].bumpheight + PLATADD ) * PLATASCEND;
                                    chr[chara].xvel += ( chr[charb].xvel ) * platstick;
                                    chr[chara].yvel += ( chr[charb].yvel ) * platstick;

                                    if ( chr[chara].zvel < chr[charb].zvel )
                                        chr[chara].zvel = chr[charb].zvel;

                                    chr[chara].turnleftright += ( chr[charb].turnleftright - chr[charb].oldturn );
                                    chr[chara].jumpready = btrue;
                                    chr[chara].jumpnumber = chr[chara].jumpnumberreset;
                                    chr[charb].holdingweight = chr[chara].weight;
                                }
                            }
                            else
                            {
                                if ( zb > za + chr[chara].bumpheight - PLATTOLERANCE + chr[charb].zvel - chr[chara].zvel && ( capcanuseplatforms[chr[charb].model] || zb > za + chr[chara].bumpheight ) )
                                {
                                    // Is B falling on A?
                                    if ( zb < za + chr[chara].bumpheight && chr[chara].platform )//&&chr[charb].flyheight==0)
                                    {
                                        // B is inside, coming from above
                                        chr[charb].zpos = ( chr[charb].zpos ) * PLATKEEP + ( chr[chara].zpos + chr[chara].bumpheight + PLATADD ) * PLATASCEND;
                                        chr[charb].xvel += ( chr[chara].xvel ) * platstick;
                                        chr[charb].yvel += ( chr[chara].yvel ) * platstick;

                                        if ( chr[charb].zvel < chr[chara].zvel )
                                            chr[charb].zvel = chr[chara].zvel;

                                        chr[charb].turnleftright += ( chr[chara].turnleftright - chr[chara].oldturn );
                                        chr[charb].jumpready = btrue;
                                        chr[charb].jumpnumber = chr[charb].jumpnumberreset;
                                        chr[chara].holdingweight = chr[charb].weight;
                                    }
                                }
                                else
                                {
                                    // They are inside each other, which ain't good
                                    // Only collide if moving toward the other
                                    if ( chr[chara].xvel > 0 )
                                    {
                                        if ( chr[chara].xpos < chr[charb].xpos ) { chr[charb].xvel += chr[chara].xvel * chr[charb].bumpdampen;  chr[chara].xvel = -chr[chara].xvel * chr[chara].bumpdampen;  chr[chara].xpos = chr[chara].oldx; }
                                    }
                                    else
                                    {
                                        if ( chr[chara].xpos > chr[charb].xpos ) { chr[charb].xvel += chr[chara].xvel * chr[charb].bumpdampen;  chr[chara].xvel = -chr[chara].xvel * chr[chara].bumpdampen;  chr[chara].xpos = chr[chara].oldx; }
                                    }

                                    if ( chr[chara].yvel > 0 )
                                    {
                                        if ( chr[chara].ypos < chr[charb].ypos ) { chr[charb].yvel += chr[chara].yvel * chr[charb].bumpdampen;  chr[chara].yvel = -chr[chara].yvel * chr[chara].bumpdampen;  chr[chara].ypos = chr[chara].oldy; }
                                    }
                                    else
                                    {
                                        if ( chr[chara].ypos > chr[charb].ypos ) { chr[charb].yvel += chr[chara].yvel * chr[charb].bumpdampen;  chr[chara].yvel = -chr[chara].yvel * chr[chara].bumpdampen;  chr[chara].ypos = chr[chara].oldy; }
                                    }

                                    if ( chr[charb].xvel > 0 )
                                    {
                                        if ( chr[charb].xpos < chr[chara].xpos ) { chr[chara].xvel += chr[charb].xvel * chr[chara].bumpdampen;  chr[charb].xvel = -chr[charb].xvel * chr[charb].bumpdampen;  chr[charb].xpos = chr[charb].oldx; }
                                    }
                                    else
                                    {
                                        if ( chr[charb].xpos > chr[chara].xpos ) { chr[chara].xvel += chr[charb].xvel * chr[chara].bumpdampen;  chr[charb].xvel = -chr[charb].xvel * chr[charb].bumpdampen;  chr[charb].xpos = chr[charb].oldx; }
                                    }

                                    if ( chr[charb].yvel > 0 )
                                    {
                                        if ( chr[charb].ypos < chr[chara].ypos ) { chr[chara].yvel += chr[charb].yvel * chr[chara].bumpdampen;  chr[charb].yvel = -chr[charb].yvel * chr[charb].bumpdampen;  chr[charb].ypos = chr[charb].oldy; }
                                    }
                                    else
                                    {
                                        if ( chr[charb].ypos > chr[chara].ypos ) { chr[chara].yvel += chr[charb].yvel * chr[chara].bumpdampen;  chr[charb].yvel = -chr[charb].yvel * chr[charb].bumpdampen;  chr[charb].ypos = chr[charb].oldy; }
                                    }

                                    xa = chr[chara].xpos;
                                    ya = chr[chara].ypos;
                                    chr[chara].alert = chr[chara].alert | ALERTIFBUMPED;
                                    chr[charb].alert = chr[charb].alert | ALERTIFBUMPED;
                                }
                            }
                        }
                    }
                }



                // Now double check the last character we bumped into, in case it's a platform
                charb = chr[chara].bumplast;
                if ( charb != chara && chr[charb].on && !chr[charb].inpack && chr[charb].attachedto == MAXCHR && 0 != chr[chara].bumpheight && 0 != chr[charb].bumpheight )
                {
                    float dx, dy;

                    xb = chr[charb].xpos;
                    yb = chr[charb].ypos;
                    zb = chr[charb].zpos;

                    // First check absolute value diamond
                    dx = ABS( xa - xb );
                    dy = ABS( ya - yb );
                    dist = dx + dy;

                    if ( dist < chr[chara].bumpsizebig || dist < chr[charb].bumpsizebig )
                    {
                        // Then check bounding box square...  Square+Diamond=Octagon
                        if ( ( dx < chr[chara].bumpsize || dx < chr[charb].bumpsize ) &&
                                ( dy < chr[chara].bumpsize || dy < chr[charb].bumpsize ) )
                        {
                            // Now see if either is on top the other like a platform
                            if ( za > zb + chr[charb].bumpheight - PLATTOLERANCE + chr[chara].zvel - chr[charb].zvel && ( capcanuseplatforms[chr[chara].model] || za > zb + chr[charb].bumpheight ) )
                            {
                                // Is A falling on B?
                                if ( za < zb + chr[charb].bumpheight && chr[charb].platform && chr[chara].alive )//&&chr[chara].flyheight==0)
                                {
                                    // A is inside, coming from above
                                    chr[chara].zpos = ( chr[chara].zpos ) * PLATKEEP + ( chr[charb].zpos + chr[charb].bumpheight + PLATADD ) * PLATASCEND;
                                    chr[chara].xvel += ( chr[charb].xvel ) * platstick;
                                    chr[chara].yvel += ( chr[charb].yvel ) * platstick;

                                    if ( chr[chara].zvel < chr[charb].zvel )
                                        chr[chara].zvel = chr[charb].zvel;

                                    chr[chara].turnleftright += ( chr[charb].turnleftright - chr[charb].oldturn );
                                    chr[chara].jumpready = btrue;
                                    chr[chara].jumpnumber = chr[chara].jumpnumberreset;

                                    if ( madactionvalid[chr[chara].model][ACTIONMI] && chr[chara].alive && chr[charb].alive && chr[charb].ismount && !chr[chara].isitem && chr[charb].holdingwhich[0] == MAXCHR && chr[chara].attachedto == MAXCHR && chr[chara].jumptime == 0 && chr[chara].flyheight == 0 )
                                    {
                                        attach_character_to_mount( chara, charb, GRIPONLY );
                                        chr[chara].bumplast = chara;
                                        chr[charb].bumplast = charb;
                                    }

                                    chr[charb].holdingweight = chr[chara].weight;
                                }
                            }
                            else
                            {
                                if ( zb > za + chr[chara].bumpheight - PLATTOLERANCE + chr[charb].zvel - chr[chara].zvel && ( capcanuseplatforms[chr[charb].model] || zb > za + chr[chara].bumpheight ) )
                                {
                                    // Is B falling on A?
                                    if ( zb < za + chr[chara].bumpheight && chr[chara].platform && chr[charb].alive )//&&chr[charb].flyheight==0)
                                    {
                                        // B is inside, coming from above
                                        chr[charb].zpos = ( chr[charb].zpos ) * PLATKEEP + ( chr[chara].zpos + chr[chara].bumpheight + PLATADD ) * PLATASCEND;
                                        chr[charb].xvel += ( chr[chara].xvel ) * platstick;
                                        chr[charb].yvel += ( chr[chara].yvel ) * platstick;

                                        if ( chr[charb].zvel < chr[chara].zvel )
                                            chr[charb].zvel = chr[chara].zvel;

                                        chr[charb].turnleftright += ( chr[chara].turnleftright - chr[chara].oldturn );
                                        chr[charb].jumpready = btrue;
                                        chr[charb].jumpnumber = chr[charb].jumpnumberreset;

                                        if ( madactionvalid[chr[charb].model][ACTIONMI] && chr[chara].alive && chr[charb].alive && chr[chara].ismount && !chr[charb].isitem && chr[chara].holdingwhich[0] == MAXCHR && chr[charb].attachedto == MAXCHR && chr[charb].jumptime == 0 && chr[charb].flyheight == 0 )
                                        {
                                            attach_character_to_mount( charb, chara, GRIPONLY );
                                            chr[chara].bumplast = chara;
                                            chr[charb].bumplast = charb;
                                        }

                                        chr[chara].holdingweight = chr[charb].weight;
                                    }
                                }
                                else
                                {
                                    // They are inside each other, which ain't good
                                    // Only collide if moving toward the other
                                    if ( chr[chara].xvel > 0 )
                                    {
                                        if ( chr[chara].xpos < chr[charb].xpos ) { chr[charb].xvel += chr[chara].xvel * chr[charb].bumpdampen;  chr[chara].xvel = -chr[chara].xvel * chr[chara].bumpdampen;  chr[chara].xpos = chr[chara].oldx; }
                                    }
                                    else
                                    {
                                        if ( chr[chara].xpos > chr[charb].xpos ) { chr[charb].xvel += chr[chara].xvel * chr[charb].bumpdampen;  chr[chara].xvel = -chr[chara].xvel * chr[chara].bumpdampen;  chr[chara].xpos = chr[chara].oldx; }
                                    }

                                    if ( chr[chara].yvel > 0 )
                                    {
                                        if ( chr[chara].ypos < chr[charb].ypos ) { chr[charb].yvel += chr[chara].yvel * chr[charb].bumpdampen;  chr[chara].yvel = -chr[chara].yvel * chr[chara].bumpdampen;  chr[chara].ypos = chr[chara].oldy; }
                                    }
                                    else
                                    {
                                        if ( chr[chara].ypos > chr[charb].ypos ) { chr[charb].yvel += chr[chara].yvel * chr[charb].bumpdampen;  chr[chara].yvel = -chr[chara].yvel * chr[chara].bumpdampen;  chr[chara].ypos = chr[chara].oldy; }
                                    }

                                    if ( chr[charb].xvel > 0 )
                                    {
                                        if ( chr[charb].xpos < chr[chara].xpos ) { chr[chara].xvel += chr[charb].xvel * chr[chara].bumpdampen;  chr[charb].xvel = -chr[charb].xvel * chr[charb].bumpdampen;  chr[charb].xpos = chr[charb].oldx; }
                                    }
                                    else
                                    {
                                        if ( chr[charb].xpos > chr[chara].xpos ) { chr[chara].xvel += chr[charb].xvel * chr[chara].bumpdampen;  chr[charb].xvel = -chr[charb].xvel * chr[charb].bumpdampen;  chr[charb].xpos = chr[charb].oldx; }
                                    }

                                    if ( chr[charb].yvel > 0 )
                                    {
                                        if ( chr[charb].ypos < chr[chara].ypos ) { chr[chara].yvel += chr[charb].yvel * chr[chara].bumpdampen;  chr[charb].yvel = -chr[charb].yvel * chr[charb].bumpdampen;  chr[charb].ypos = chr[charb].oldy; }
                                    }
                                    else
                                    {
                                        if ( chr[charb].ypos > chr[chara].ypos ) { chr[chara].yvel += chr[charb].yvel * chr[chara].bumpdampen;  chr[charb].yvel = -chr[charb].yvel * chr[charb].bumpdampen;  chr[charb].ypos = chr[charb].oldy; }
                                    }

                                    xa = chr[chara].xpos;
                                    ya = chr[chara].ypos;
                                    chr[chara].alert = chr[chara].alert | ALERTIFBUMPED;
                                    chr[charb].alert = chr[charb].alert | ALERTIFBUMPED;
                                }
                            }
                        }
                    }
                }


                // Now check collisions with every bump particle in same area
                if ( chr[chara].alive )
                {
                    for ( tnc = 0, particle = meshbumplistprt[fanblock];
                            tnc < prtinblock;
                            tnc++, particle = prtbumpnext[particle] )
                    {
                        float dx, dy;

                        xb = prtxpos[particle];
                        yb = prtypos[particle];
                        zb = prtzpos[particle];

                        // First check absolute value diamond
                        dx = ABS( xa - xb );
                        dy = ABS( ya - yb );
                        dist = dx + dy;

                        if ( dist < chr[chara].bumpsizebig || dist < prtbumpsizebig[particle] )
                        {
                            // Then check bounding box square...  Square+Diamond=Octagon
                            if ( ( dx < chr[chara].bumpsize  || dx < prtbumpsize[particle] ) &&
                                    ( dy < chr[chara].bumpsize  || dy < prtbumpsize[particle] ) &&
                                    ( zb > za - prtbumpheight[particle] && zb < za + chr[chara].bumpheight + prtbumpheight[particle] ) )
                            {
                                pip = prtpip[particle];

                                if ( zb > za + chr[chara].bumpheight + prtzvel[particle] && prtzvel[particle] < 0 && chr[chara].platform && prtattachedtocharacter[particle] == MAXCHR )
                                {
                                    // Particle is falling on A
                                    prtzpos[particle] = za + chr[chara].bumpheight;
                                    prtzvel[particle] = -prtzvel[particle] * pipdampen[pip];
                                    prtxvel[particle] += ( chr[chara].xvel ) * platstick;
                                    prtyvel[particle] += ( chr[chara].yvel ) * platstick;
                                }

                                // Check reaffirmation of particles
                                if ( prtattachedtocharacter[particle] != chara )
                                {
                                    if ( chr[chara].reloadtime == 0 )
                                    {
                                        if ( chr[chara].reaffirmdamagetype == prtdamagetype[particle] && chr[chara].damagetime == 0 )
                                        {
                                            reaffirm_attached_particles( chara );
                                        }
                                    }
                                }

                                // Check for missile treatment
                                if ( ( chr[chara].damagemodifier[prtdamagetype[particle]]&3 ) < 2 ||
                                        chr[chara].missiletreatment == MISNORMAL ||
                                        prtattachedtocharacter[particle] != MAXCHR ||
                                        ( prtchr[particle] == chara && !pipfriendlyfire[pip] ) ||
                                        ( chr[chr[chara].missilehandler].mana < ( chr[chara].missilecost << 4 ) && !chr[chr[chara].missilehandler].canchannel ) )
                                {
                                    if ( ( teamhatesteam[prtteam[particle]][chr[chara].team] || ( pipfriendlyfire[pip] && ( ( chara != prtchr[particle] && chara != chr[prtchr[particle]].attachedto ) || piponlydamagefriendly[pip] ) ) ) && !chr[chara].invictus )
                                    {
                                        spawn_bump_particles( chara, particle ); // Catch on fire

                                        if ( ( prtdamagebase[particle] | prtdamagerand[particle] ) > 1 )
                                        {
                                            prtidparent = capidsz[prtmodel[particle]][IDSZ_PARENT];
                                            prtidtype = capidsz[prtmodel[particle]][IDSZ_TYPE];

                                            if ( chr[chara].damagetime == 0 && prtattachedtocharacter[particle] != chara && ( pipdamfx[pip]&DAMFXARRO ) == 0 )
                                            {
                                                // Normal particle damage
                                                if ( pipallowpush[pip] )
                                                {
                                                    chr[chara].xvel = prtxvel[particle] * chr[chara].bumpdampen;
                                                    chr[chara].yvel = prtyvel[particle] * chr[chara].bumpdampen;
                                                    chr[chara].zvel = prtzvel[particle] * chr[chara].bumpdampen;
                                                }

                                                direction = ( ATAN2( prtyvel[particle], prtxvel[particle] ) + PI ) * 65535 / ( TWO_PI );
                                                direction = chr[chara].turnleftright - direction + 32768;
                                                // Check all enchants to see if they are removed
                                                enchant = chr[chara].firstenchant;

                                                while ( enchant != MAXENCHANT )
                                                {
                                                    eveidremove = everemovedbyidsz[enceve[enchant]];
                                                    temp = encnextenchant[enchant];

                                                    if ( eveidremove != IDSZ_NONE && ( eveidremove == prtidtype || eveidremove == prtidparent ) )
                                                    {
                                                        remove_enchant( enchant );
                                                    }

                                                    enchant = temp;
                                                }

                                                // Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
                                                //+2% bonus for every point of intelligence and/or wisdom above 14. Below 14 gives -2% instead!
                                                if ( pipintdamagebonus[pip] )
                                                {
                                                    int percent;
                                                    percent = ( chr[prtchr[particle]].intelligence - 3584 ) >> 7;
                                                    percent /= 100;
                                                    prtdamagebase[particle] *= 1 + percent;
                                                }

                                                if ( pipwisdamagebonus[pip] )
                                                {
                                                    int percent;
                                                    percent = ( chr[prtchr[particle]].wisdom - 3584 ) >> 7;
                                                    percent /= 100;
                                                    prtdamagebase[particle] *= 1 + percent;
                                                }

                                                // Damage the character
                                                if ( chridvulnerability != IDSZ_NONE && ( chridvulnerability == prtidtype || chridvulnerability == prtidparent ) )
                                                {
                                                    damage_character( chara, direction, prtdamagebase[particle] << 1, prtdamagerand[particle] << 1, prtdamagetype[particle], prtteam[particle], prtchr[particle], pipdamfx[pip] );
                                                    chr[chara].alert |= ALERTIFHITVULNERABLE;
                                                }
                                                else
                                                {
                                                    damage_character( chara, direction, prtdamagebase[particle], prtdamagerand[particle], prtdamagetype[particle], prtteam[particle], prtchr[particle], pipdamfx[pip] );
                                                }

                                                // Do confuse effects
                                                if ( 0 == ( madframefx[chr[chara].frame]&MADFXINVICTUS ) || pipdamfx[pip]&DAMFXBLOC )
                                                {
                                                    if ( pipgrogtime[pip] != 0 && capcanbegrogged[chr[chara].model] )
                                                    {
                                                        chr[chara].grogtime += pipgrogtime[pip];

                                                        if ( chr[chara].grogtime < 0 )  chr[chara].grogtime = 32767;

                                                        chr[chara].alert = chr[chara].alert | ALERTIFGROGGED;
                                                    }

                                                    if ( pipdazetime[pip] != 0 && capcanbedazed[chr[chara].model] )
                                                    {
                                                        chr[chara].dazetime += pipdazetime[pip];

                                                        if ( chr[chara].dazetime < 0 )  chr[chara].dazetime = 32767;

                                                        chr[chara].alert = chr[chara].alert | ALERTIFDAZED;
                                                    }
                                                }

                                                // Notify the attacker of a scored hit
                                                if ( prtchr[particle] != MAXCHR )
                                                {
                                                    chr[prtchr[particle]].alert = chr[prtchr[particle]].alert | ALERTIFSCOREDAHIT;
                                                    chr[prtchr[particle]].hitlast = chara;
                                                }
                                            }

                                            if ( ( wldframe&31 ) == 0 && prtattachedtocharacter[particle] == chara )
                                            {
                                                // Attached particle damage ( Burning )
                                                if ( pipxyvelbase[pip] == 0 )
                                                {
                                                    // Make character limp
                                                    chr[chara].xvel = 0;
                                                    chr[chara].yvel = 0;
                                                }

                                                damage_character( chara, 32768, prtdamagebase[particle], prtdamagerand[particle], prtdamagetype[particle], prtteam[particle], prtchr[particle], pipdamfx[pip] );
                                            }
                                        }

                                        if ( pipendbump[pip] )
                                        {
                                            if ( pipbumpmoney[pip] )
                                            {
                                                if ( chr[chara].cangrabmoney && chr[chara].alive && chr[chara].damagetime == 0 && chr[chara].money != MAXMONEY )
                                                {
                                                    if ( chr[chara].ismount )
                                                    {
                                                        // Let mounts collect money for their riders
                                                        if ( chr[chara].holdingwhich[0] != MAXCHR )
                                                        {
                                                            chr[chr[chara].holdingwhich[0]].money += pipbumpmoney[pip];

                                                            if ( chr[chr[chara].holdingwhich[0]].money > MAXMONEY ) chr[chr[chara].holdingwhich[0]].money = MAXMONEY;

                                                            if ( chr[chr[chara].holdingwhich[0]].money < 0 ) chr[chr[chara].holdingwhich[0]].money = 0;

                                                            prttime[particle] = 1;
                                                        }
                                                    }
                                                    else
                                                    {
                                                        // Normal money collection
                                                        chr[chara].money += pipbumpmoney[pip];

                                                        if ( chr[chara].money > MAXMONEY ) chr[chara].money = MAXMONEY;

                                                        if ( chr[chara].money < 0 ) chr[chara].money = 0;

                                                        prttime[particle] = 1;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                prttime[particle] = 1;
                                                // Only hit one character, not several
                                                prtdamagebase[particle] = 0;
                                                prtdamagerand[particle] = 1;
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    if ( prtchr[particle] != chara )
                                    {
                                        cost_mana( chr[chara].missilehandler, ( chr[chara].missilecost << 4 ), prtchr[particle] );

                                        // Treat the missile
                                        if ( chr[chara].missiletreatment == MISDEFLECT )
                                        {
                                            // Use old position to find normal
                                            ax = prtxpos[particle] - prtxvel[particle];
                                            ay = prtypos[particle] - prtyvel[particle];
                                            ax = chr[chara].xpos - ax;
                                            ay = chr[chara].ypos - ay;
                                            // Find size of normal
                                            scale = ax * ax + ay * ay;

                                            if ( scale > 0 )
                                            {
                                                // Make the normal a unit normal
                                                scale = SQRT( scale );
                                                nx = ax / scale;
                                                ny = ay / scale;
                                                // Deflect the incoming ray off the normal
                                                scale = ( prtxvel[particle] * nx + prtyvel[particle] * ny ) * 2;
                                                ax = scale * nx;
                                                ay = scale * ny;
                                                prtxvel[particle] = prtxvel[particle] - ax;
                                                prtyvel[particle] = prtyvel[particle] - ay;
                                            }
                                        }
                                        else
                                        {
                                            // Reflect it back in the direction it came
                                            prtxvel[particle] = -prtxvel[particle];
                                            prtyvel[particle] = -prtyvel[particle];
                                        }

                                        // Change the owner of the missile
                                        if ( !piphoming[pip] )
                                        {
                                            prtteam[particle] = chr[chara].team;
                                            prtchr[particle] = chara;
                                        }

                                        // Change the direction of the particle
                                        if ( piprotatetoface[pip] )
                                        {
                                            // Turn to face new direction
                                            facing = ATAN2( prtyvel[particle], prtxvel[particle] ) * 65535 / ( TWO_PI );
                                            facing += 32768;
                                            prtfacing[particle] = facing;
                                        }
                                    }
                                }
                            }
                        }

                    }
                }

            }
        }

    }
}

//--------------------------------------------------------------------------------------------
void stat_return()
{
    // ZZ> This function brings mana and life back
    int cnt, owner, target, eve;

    // Do reload time
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].reloadtime > 0 )
        {
            chr[cnt].reloadtime--;
        }

        cnt++;
    }

    // Do stats
    if ( statclock >= ONESECOND )
    {
        // Reset the clock
        statclock -= ONESECOND;

        // Do all the characters
        for ( cnt = 0; cnt < MAXCHR; cnt++ )
        {
            if ( !chr[cnt].on ) continue;

            // check for a level up
            do_level_up( cnt );

            // do the mana and life regen for "living" characters
            if ( chr[cnt].alive )
            {
                chr[cnt].mana += ( chr[cnt].manareturn / MANARETURNSHIFT );
                chr[cnt].mana = MAX(0, MIN(chr[cnt].mana, chr[cnt].manamax));

                chr[cnt].life += chr[cnt].lifereturn;
                chr[cnt].life = MAX(1, MIN(chr[cnt].life, chr[cnt].life));
            }

            if ( chr[cnt].grogtime > 0 )
            {
                chr[cnt].grogtime--;
            }

            if ( chr[cnt].dazetime > 0 )
            {
                chr[cnt].dazetime--;
            }
        }

        // Run through all the enchants as well
        cnt = 0;

        while ( cnt < MAXENCHANT )
        {
            if ( encon[cnt] )
            {
                if ( enctime[cnt] != 0 )
                {
                    if ( enctime[cnt] > 0 )
                    {
                        enctime[cnt]--;
                    }

                    owner = encowner[cnt];
                    target = enctarget[cnt];
                    eve = enceve[cnt];

                    // Do drains
                    if ( chr[owner].alive )
                    {
                        // Change life
                        chr[owner].life += encownerlife[cnt];

                        if ( chr[owner].life < 1 )
                        {
                            chr[owner].life = 1;
                            kill_character( owner, target );
                        }

                        if ( chr[owner].life > chr[owner].lifemax )
                        {
                            chr[owner].life = chr[owner].lifemax;
                        }

                        // Change mana
                        if ( !cost_mana( owner, -encownermana[cnt], target ) && eveendifcantpay[eve] )
                        {
                            remove_enchant( cnt );
                        }
                    }
                    else
                    {
                        if ( !evestayifnoowner[eve] )
                        {
                            remove_enchant( cnt );
                        }
                    }

                    if ( encon[cnt] )
                    {
                        if ( chr[target].alive )
                        {
                            // Change life
                            chr[target].life += enctargetlife[cnt];

                            if ( chr[target].life < 1 )
                            {
                                chr[target].life = 1;
                                kill_character( target, owner );
                            }

                            if ( chr[target].life > chr[target].lifemax )
                            {
                                chr[target].life = chr[target].lifemax;
                            }

                            // Change mana
                            if ( !cost_mana( target, -enctargetmana[cnt], owner ) && eveendifcantpay[eve] )
                            {
                                remove_enchant( cnt );
                            }
                        }
                        else
                        {
                            remove_enchant( cnt );
                        }
                    }
                }
                else
                {
                    remove_enchant( cnt );
                }
            }

            cnt++;
        }
    }

}

//--------------------------------------------------------------------------------------------
void update_pits()
{
    // ZZ> This function kills any character in a deep pit...
    int cnt;

    if ( pitskill || pitsfall )
    {
        if ( pitclock > 19 )
        {
            pitclock = 0;

            // Kill any particles that fell in a pit, if they die in water...
            cnt = 0;

            while ( cnt < maxparticles )
            {
                if ( prton[cnt] )
                {
                    if ( prtzpos[cnt] < PITDEPTH && pipendwater[prtpip[cnt]] )
                    {
                        prttime[cnt] = 1;
                    }
                }

                cnt++;
            }

            // Kill or teleport any characters that fell in a pit...
            cnt = 0;

            while ( cnt < MAXCHR )
            {
                if ( chr[cnt].on && chr[cnt].alive && !chr[cnt].inpack )
                {
                    if ( !chr[cnt].invictus && chr[cnt].zpos < PITDEPTH && chr[cnt].attachedto == MAXCHR )
                    {
                        //Do we kill it?
                        if (pitskill)
                        {
                            // Got one!
                            kill_character( cnt, MAXCHR );
                            chr[cnt].xvel = 0;
                            chr[cnt].yvel = 0;

                            //Play sound effect
                            play_mix( chr[cnt].xpos, chr[cnt].ypos, globalwave + SND_PITFALL );
                        }

                        //Do we teleport it?
                        if (pitsfall && chr[cnt].zpos < PITDEPTH*8)
                        {
                            // Yeah!  It worked!
                            detach_character_from_mount( cnt, btrue, bfalse );
                            chr[cnt].oldx = chr[cnt].xpos;
                            chr[cnt].oldy = chr[cnt].ypos;
                            chr[cnt].xpos = pitx;
                            chr[cnt].ypos = pity;
                            chr[cnt].zpos = pitz;

                            if ( __chrhitawall( cnt ) )
                            {
                                // No it didn't...
                                chr[cnt].xpos = chr[cnt].oldx;
                                chr[cnt].ypos = chr[cnt].oldy;
                                chr[cnt].zpos = chr[cnt].oldz;

                                // Kill it instead
                                kill_character( cnt, MAXCHR );
                                chr[cnt].xvel = 0;
                                chr[cnt].yvel = 0;
                            }
                            else
                            {
                                chr[cnt].oldx = chr[cnt].xpos;
                                chr[cnt].oldy = chr[cnt].ypos;
                                chr[cnt].oldz = chr[cnt].zpos;

                                //Stop movement
                                chr[cnt].zvel = 0;
                                chr[cnt].xvel = 0;
                                chr[cnt].yvel = 0;

                                //Play sound effect
                                if (chr[cnt].isplayer)
                                {
                                    play_mix( camtrackx, camtracky, globalwave + SND_PITFALL );
                                }
                                else
                                {
                                    play_mix( chr[cnt].xpos, chr[cnt].ypos, globalwave + SND_PITFALL );
                                }

                                //Do some damage (same as damage tile)
                                damage_character( cnt, 32768, damagetileamount, 1, damagetiletype, DAMAGETEAM, chr[cnt].bumplast, DAMFXBLOC | DAMFXARMO );
                            }
                        }
                    }

                }

                cnt++;
            }
        }
        else
        {
            pitclock++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void reset_players()
{
    // ZZ> This function clears the player list data
    int cnt, tnc;

    // Reset the local data stuff
    local_seekurse = bfalse;
    local_senseenemies = MAXCHR;
    local_seeinvisible = bfalse;
    alllocalpladead = bfalse;

    // Reset the initial player data and latches
    cnt = 0;

    while ( cnt < MAXPLAYER )
    {
        plavalid[cnt] = bfalse;
        plaindex[cnt] = 0;
        plalatchx[cnt] = 0;
        plalatchy[cnt] = 0;
        plalatchbutton[cnt] = 0;
        tnc = 0;

        while ( tnc < MAXLAG )
        {
            platimelatchx[cnt][tnc] = 0;
            platimelatchy[cnt][tnc] = 0;
            platimelatchbutton[cnt][tnc] = 0;
            tnc++;
        }

        pladevice[cnt] = INPUT_BITS_NONE;
        cnt++;
    }

    numpla = 0;
    nexttimestamp = ((Uint32)~0);
    numplatimes = STARTTALK + 1;

    if ( hostactive ) numplatimes++;
}
//--------------------------------------------------------------------------------------------
void drop_money( Uint16 character, Uint16 money )
{
    // ZZ> This function drops some of a character's money
    Uint16 huns, tfives, fives, ones, cnt;

    if ( money > chr[character].money )  money = chr[character].money;

    if ( money > 0 && chr[character].zpos > -2 )
    {
        chr[character].money = chr[character].money - money;
        huns = money / 100;  money -= ( huns << 7 ) - ( huns << 5 ) + ( huns << 2 );
        tfives = money / 25;  money -= ( tfives << 5 ) - ( tfives << 3 ) + tfives;
        fives = money / 5;  money -= ( fives << 2 ) + fives;
        ones = money;

        for ( cnt = 0; cnt < ones; cnt++ )
        {
            spawn_one_particle( chr[character].xpos, chr[character].ypos,  chr[character].zpos, 0, MAXMODEL, COIN1, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        for ( cnt = 0; cnt < fives; cnt++ )
        {
            spawn_one_particle( chr[character].xpos, chr[character].ypos,  chr[character].zpos, 0, MAXMODEL, COIN5, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        for ( cnt = 0; cnt < tfives; cnt++ )
        {
            spawn_one_particle( chr[character].xpos, chr[character].ypos,  chr[character].zpos, 0, MAXMODEL, COIN25, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        for ( cnt = 0; cnt < huns; cnt++ )
        {
            spawn_one_particle( chr[character].xpos, chr[character].ypos,  chr[character].zpos, 0, MAXMODEL, COIN100, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        chr[character].damagetime = DAMAGETIME;  // So it doesn't grab it again
    }
}

//--------------------------------------------------------------------------------------------
void call_for_help( Uint16 character )
{
    // ZZ> This function issues a call for help to all allies
    Uint8 team;
    Uint16 cnt;

    team = chr[character].team;
    teamsissy[team] = character;

    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( chr[cnt].on && cnt != character && !teamhatesteam[chr[cnt].team][team] )
        {
            chr[cnt].alert = chr[cnt].alert | ALERTIFCALLEDFORHELP;
        }
    }
}

//--------------------------------------------------------------------------------------------
Uint32 xp_for_next_level(Uint16 character)
{
    Uint32 curlevel;
    Uint16 profile;
    Uint32 xpneeded = (Uint32)(~0);

    if ( !chr[character].on ) return xpneeded;

    profile  = chr[character].model;
    if (profile == MAXMODEL) return xpneeded;

    // Do level ups and stat changes
    curlevel = chr[character].experiencelevel;

    if ( curlevel + 1 < MAXLEVEL )
    {
        xpneeded = capexperienceforlevel[profile][curlevel+1];
    }
    else
    {
        xpneeded = capexperienceforlevel[profile][MAXLEVEL - 1];
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

    if (character >= MAXCHR || !chr[character].on) return;

    profile = chr[character].model;
    if ( profile >= MAXMODEL ) return;

    // Do level ups and stat changes
    curlevel = chr[character].experiencelevel;

    if ( curlevel + 1 < 20 )
    {
        Uint32 xpcurrent, xpneeded;

        xpcurrent = chr[character].experience;
        xpneeded  = xp_for_next_level(character);

        if ( xpcurrent >= xpneeded )
        {
            // do the level up
            chr[character].experiencelevel++;
            xpneeded  = xp_for_next_level(character);

            // The character is ready to advance...
            if ( chr[character].isplayer )
            {
                snprintf( text, sizeof(text), "%s gained a level!!!", chr[character].name );
                debug_message( text );
                play_mix( camtrackx, camtracky, globalwave + SND_LEVELUP );
            }

            // Size
            chr[character].sizegoto += capsizeperlevel[profile] * 0.5f;  // Limit this?
            chr[character].sizegototime = SIZETIME;

            // Strength
            number = generate_number( capstrengthperlevelbase[profile], capstrengthperlevelrand[profile] );
            number = number + chr[character].strength;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].strength = number;

            // Wisdom
            number = generate_number( capwisdomperlevelbase[profile], capwisdomperlevelrand[profile] );
            number = number + chr[character].wisdom;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].wisdom = number;

            // Intelligence
            number = generate_number( capintelligenceperlevelbase[profile], capintelligenceperlevelrand[profile] );
            number = number + chr[character].intelligence;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].intelligence = number;

            // Dexterity
            number = generate_number( capdexterityperlevelbase[profile], capdexterityperlevelrand[profile] );
            number = number + chr[character].dexterity;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].dexterity = number;

            // Life
            number = generate_number( caplifeperlevelbase[profile], caplifeperlevelrand[profile] );
            number = number + chr[character].lifemax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            chr[character].life += ( number - chr[character].lifemax );
            chr[character].lifemax = number;

            // Mana
            number = generate_number( capmanaperlevelbase[profile], capmanaperlevelrand[profile] );
            number = number + chr[character].manamax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            chr[character].mana += ( number - chr[character].manamax );
            chr[character].manamax = number;

            // Mana Return
            number = generate_number( capmanareturnperlevelbase[profile], capmanareturnperlevelrand[profile] );
            number = number + chr[character].manareturn;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].manareturn = number;

            // Mana Flow
            number = generate_number( capmanaflowperlevelbase[profile], capmanaflowperlevelrand[profile] );
            number = number + chr[character].manaflow;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].manaflow = number;
        }
    }
}

//--------------------------------------------------------------------------------------------
void give_experience( int character, int amount, Uint8 xptype )
{
    // ZZ> This function gives a character experience

    int newamount;
    int profile;

    if (amount == 0) return;

    if ( !chr[character].invictus )
    {
        // Figure out how much experience to give
        profile = chr[character].model;
        newamount = amount;

        if ( xptype < XP_COUNT )
        {
            newamount = amount * capexperiencerate[profile][xptype];
        }

        //Intelligence and slightly wisdom increases xp gained (0,5% per int and 0,25% per wisdom above 10)
        newamount = newamount * (1 + ((float)((chr[character].intelligence - 2560) >> 8) / 200))
                    + (1 + ((float)((chr[character].wisdom - 2560) >> 8) / 400));

        chr[character].experience += newamount;
    }
}

//--------------------------------------------------------------------------------------------
void give_team_experience( Uint8 team, int amount, Uint8 xptype )
{
    // ZZ> This function gives a character experience, and pawns off level gains to
    //     another function
    int cnt;

    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( chr[cnt].team == team && chr[cnt].on )
        {
            give_experience( cnt, amount, xptype );
        }
    }
}

//--------------------------------------------------------------------------------------------
void resize_characters()
{
    // ZZ> This function makes the characters get bigger or smaller, depending
    //     on their sizegoto and sizegototime
    int cnt, item, mount;
    bool_t willgetcaught;
    float newsize;

    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].on && chr[cnt].sizegototime )
        {
            // Make sure it won't get caught in a wall
            willgetcaught = bfalse;

            if ( chr[cnt].sizegoto > chr[cnt].fat )
            {
                chr[cnt].bumpsize += 10;

                if ( __chrhitawall( cnt ) )
                {
                    willgetcaught = btrue;
                }

                chr[cnt].bumpsize -= 10;
            }

            // If it is getting caught, simply halt growth until later
            if ( !willgetcaught )
            {
                // Figure out how big it is
                chr[cnt].sizegototime--;
                newsize = chr[cnt].sizegoto;

                if ( chr[cnt].sizegototime != 0 )
                {
                    newsize = ( chr[cnt].fat * 0.90f ) + ( newsize * 0.10f );
                }

                // Make it that big...
                chr[cnt].fat = newsize;
                chr[cnt].shadowsize = chr[cnt].shadowsizesave * newsize;
                chr[cnt].bumpsize = chr[cnt].bumpsizesave * newsize;
                chr[cnt].bumpsizebig = chr[cnt].bumpsizebigsave * newsize;
                chr[cnt].bumpheight = chr[cnt].bumpheightsave * newsize;
                chr[cnt].weight = capweight[chr[cnt].model] * newsize;

                if ( capweight[chr[cnt].model] == 255 ) chr[cnt].weight = 65535;

                // Now come up with the magic number
                mount = chr[cnt].attachedto;

                if ( mount == MAXCHR )
                {
                    chr[cnt].scale = newsize * madscale[chr[cnt].model] * 4;
                }
                else
                {
                    chr[cnt].scale = newsize / ( chr[mount].fat * 1280 );
                }

                // Make in hand items stay the same size...
                newsize = newsize * 1280;
                item = chr[cnt].holdingwhich[0];

                if ( item != MAXCHR )
                    chr[item].scale = chr[item].fat / newsize;

                item = chr[cnt].holdingwhich[1];

                if ( item != MAXCHR )
                    chr[item].scale = chr[item].fat / newsize;
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void export_one_character_name( char *szSaveName, Uint16 character )
{
    // ZZ> This function makes the naming.txt file for the character
    FILE* filewrite;
    int profile;
    char cTmp;
    int cnt, tnc;

    // Can it export?
    profile = chr[character].model;
    filewrite = fopen( szSaveName, "w" );

    if ( filewrite )
    {
        cnt = 0;
        cTmp = chr[character].name[0];
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

                cTmp = chr[character].name[cnt];
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
void export_one_character_profile( char *szSaveName, Uint16 character )
{
    // ZZ> This function creates a data.txt file for the given character.
    //     it is assumed that all enchantments have been done away with
    FILE* filewrite;
    int profile;
    int damagetype, skin;
    char types[10] = "SCPHEFIZ";
    char codes[4];

    // General stuff
    profile = chr[character].model;

    // Open the file
    filewrite = fopen( szSaveName, "w" );

    if ( filewrite )
    {
        // Real general data
        fprintf( filewrite, "Slot number    : -1\n" );  // -1 signals a flexible load thing
        funderf( filewrite, "Class name     : ", capclassname[profile] );
        ftruthf( filewrite, "Uniform light  : ", capuniformlit[profile] );
        fprintf( filewrite, "Maximum ammo   : %d\n", capammomax[profile] );
        fprintf( filewrite, "Current ammo   : %d\n", chr[character].ammo );
        fgendef( filewrite, "Gender         : ", chr[character].gender );
        fprintf( filewrite, "\n" );

        // Object stats
        fprintf( filewrite, "Life color     : %d\n", chr[character].lifecolor );
        fprintf( filewrite, "Mana color     : %d\n", chr[character].manacolor );
        fprintf( filewrite, "Life           : %4.2f\n", chr[character].lifemax / 256.0f );
        fpairof( filewrite, "Life up        : ", caplifeperlevelbase[profile], caplifeperlevelrand[profile] );
        fprintf( filewrite, "Mana           : %4.2f\n", chr[character].manamax / 256.0f );
        fpairof( filewrite, "Mana up        : ", capmanaperlevelbase[profile], capmanaperlevelrand[profile] );
        fprintf( filewrite, "Mana return    : %4.2f\n", chr[character].manareturn / 256.0f );
        fpairof( filewrite, "Mana return up : ", capmanareturnperlevelbase[profile], capmanareturnperlevelrand[profile] );
        fprintf( filewrite, "Mana flow      : %4.2f\n", chr[character].manaflow / 256.0f );
        fpairof( filewrite, "Mana flow up   : ", capmanaflowperlevelbase[profile], capmanaflowperlevelrand[profile] );
        fprintf( filewrite, "STR            : %4.2f\n", chr[character].strength / 256.0f );
        fpairof( filewrite, "STR up         : ", capstrengthperlevelbase[profile], capstrengthperlevelrand[profile] );
        fprintf( filewrite, "WIS            : %4.2f\n", chr[character].wisdom / 256.0f );
        fpairof( filewrite, "WIS up         : ", capwisdomperlevelbase[profile], capwisdomperlevelrand[profile] );
        fprintf( filewrite, "INT            : %4.2f\n", chr[character].intelligence / 256.0f );
        fpairof( filewrite, "INT up         : ", capintelligenceperlevelbase[profile], capintelligenceperlevelrand[profile] );
        fprintf( filewrite, "DEX            : %4.2f\n", chr[character].dexterity / 256.0f );
        fpairof( filewrite, "DEX up         : ", capdexterityperlevelbase[profile], capdexterityperlevelrand[profile] );
        fprintf( filewrite, "\n" );

        // More physical attributes
        fprintf( filewrite, "Size           : %4.2f\n", chr[character].sizegoto );
        fprintf( filewrite, "Size up        : %4.2f\n", capsizeperlevel[profile] );
        fprintf( filewrite, "Shadow size    : %d\n", capshadowsize[profile] );
        fprintf( filewrite, "Bump size      : %d\n", capbumpsize[profile] );
        fprintf( filewrite, "Bump height    : %d\n", capbumpheight[profile] );
        fprintf( filewrite, "Bump dampen    : %4.2f\n", capbumpdampen[profile] );
        fprintf( filewrite, "Weight         : %d\n", capweight[profile] );
        fprintf( filewrite, "Jump power     : %4.2f\n", capjump[profile] );
        fprintf( filewrite, "Jump number    : %d\n", capjumpnumber[profile] );
        fprintf( filewrite, "Sneak speed    : %d\n", capsneakspd[profile] );
        fprintf( filewrite, "Walk speed     : %d\n", capwalkspd[profile] );
        fprintf( filewrite, "Run speed      : %d\n", caprunspd[profile] );
        fprintf( filewrite, "Fly to height  : %d\n", capflyheight[profile] );
        fprintf( filewrite, "Flashing AND   : %d\n", capflashand[profile] );
        fprintf( filewrite, "Alpha blending : %d\n", capalpha[profile] );
        fprintf( filewrite, "Light blending : %d\n", caplight[profile] );
        ftruthf( filewrite, "Transfer blend : ", captransferblend[profile] );
        fprintf( filewrite, "Sheen          : %d\n", capsheen[profile] );
        ftruthf( filewrite, "Phong mapping  : ", capenviro[profile] );
        fprintf( filewrite, "Texture X add  : %4.2f\n", capuoffvel[profile] / 65535.0f );
        fprintf( filewrite, "Texture Y add  : %4.2f\n", capvoffvel[profile] / 65535.0f );
        ftruthf( filewrite, "Sticky butt    : ", capstickybutt[profile] );
        fprintf( filewrite, "\n" );

        // Invulnerability data
        ftruthf( filewrite, "Invictus       : ", capinvictus[profile] );
        fprintf( filewrite, "NonI facing    : %d\n", capnframefacing[profile] );
        fprintf( filewrite, "NonI angle     : %d\n", capnframeangle[profile] );
        fprintf( filewrite, "I facing       : %d\n", capiframefacing[profile] );
        fprintf( filewrite, "I angle        : %d\n", capiframeangle[profile] );
        fprintf( filewrite, "\n" );

        // Skin defenses
        fprintf( filewrite, "Base defense   : %3d %3d %3d %3d\n", 255 - capdefense[profile][0], 255 - capdefense[profile][1],
                 255 - capdefense[profile][2], 255 - capdefense[profile][3] );
        damagetype = 0;

        while ( damagetype < DAMAGE_COUNT )
        {
            fprintf( filewrite, "%c damage shift : %3d %3d %3d %3d\n", types[damagetype],
                     capdamagemodifier[profile][damagetype][0]&DAMAGESHIFT,
                     capdamagemodifier[profile][damagetype][1]&DAMAGESHIFT,
                     capdamagemodifier[profile][damagetype][2]&DAMAGESHIFT,
                     capdamagemodifier[profile][damagetype][3]&DAMAGESHIFT );
            damagetype++;
        }

        damagetype = 0;

        while ( damagetype < DAMAGE_COUNT )
        {
            skin = 0;

            while ( skin < 4 )
            {
                codes[skin] = 'F';

                if ( capdamagemodifier[profile][damagetype][skin]&DAMAGEINVERT )
                    codes[skin] = 'T';

                if ( capdamagemodifier[profile][damagetype][skin]&DAMAGECHARGE )
                    codes[skin] = 'C';

                skin++;
            }

            fprintf( filewrite, "%c damage code  : %3c %3c %3c %3c\n", types[damagetype], codes[0], codes[1], codes[2], codes[3] );
            damagetype++;
        }

        fprintf( filewrite, "Acceleration   : %3.0f %3.0f %3.0f %3.0f\n", capmaxaccel[profile][0]*80,
                 capmaxaccel[profile][1]*80,
                 capmaxaccel[profile][2]*80,
                 capmaxaccel[profile][3]*80 );
        fprintf( filewrite, "\n" );

        // Experience and level data
        fprintf( filewrite, "EXP for 2nd    : %d\n", capexperienceforlevel[profile][1] );
        fprintf( filewrite, "EXP for 3rd    : %d\n", capexperienceforlevel[profile][2] );
        fprintf( filewrite, "EXP for 4th    : %d\n", capexperienceforlevel[profile][3] );
        fprintf( filewrite, "EXP for 5th    : %d\n", capexperienceforlevel[profile][4] );
        fprintf( filewrite, "EXP for 6th    : %d\n", capexperienceforlevel[profile][5] );
        fprintf( filewrite, "Starting EXP   : %d\n", chr[character].experience );
        fprintf( filewrite, "EXP worth      : %d\n", capexperienceworth[profile] );
        fprintf( filewrite, "EXP exchange   : %5.3f\n", capexperienceexchange[profile] );
        fprintf( filewrite, "EXPSECRET      : %4.2f\n", capexperiencerate[profile][0] );
        fprintf( filewrite, "EXPQUEST       : %4.2f\n", capexperiencerate[profile][1] );
        fprintf( filewrite, "EXPDARE        : %4.2f\n", capexperiencerate[profile][2] );
        fprintf( filewrite, "EXPKILL        : %4.2f\n", capexperiencerate[profile][3] );
        fprintf( filewrite, "EXPMURDER      : %4.2f\n", capexperiencerate[profile][4] );
        fprintf( filewrite, "EXPREVENGE     : %4.2f\n", capexperiencerate[profile][5] );
        fprintf( filewrite, "EXPTEAMWORK    : %4.2f\n", capexperiencerate[profile][6] );
        fprintf( filewrite, "EXPROLEPLAY    : %4.2f\n", capexperiencerate[profile][7] );
        fprintf( filewrite, "\n" );

        // IDSZ identification tags
        undo_idsz( capidsz[profile][IDSZ_PARENT] );
        fprintf( filewrite, "IDSZ Parent    : [%s]\n", idsz_string );
        undo_idsz( capidsz[profile][IDSZ_TYPE] );
        fprintf( filewrite, "IDSZ Type      : [%s]\n", idsz_string );
        undo_idsz( capidsz[profile][IDSZ_SKILL] );
        fprintf( filewrite, "IDSZ Skill     : [%s]\n", idsz_string );
        undo_idsz( capidsz[profile][IDSZ_SPECIAL] );
        fprintf( filewrite, "IDSZ Special   : [%s]\n", idsz_string );
        undo_idsz( capidsz[profile][IDSZ_HATE] );
        fprintf( filewrite, "IDSZ Hate      : [%s]\n", idsz_string );
        undo_idsz( capidsz[profile][IDSZ_VULNERABILITY] );
        fprintf( filewrite, "IDSZ Vulnie    : [%s]\n", idsz_string );
        fprintf( filewrite, "\n" );

        // Item and damage flags
        ftruthf( filewrite, "Is an item     : ", capisitem[profile] );
        ftruthf( filewrite, "Is a mount     : ", capismount[profile] );
        ftruthf( filewrite, "Is stackable   : ", capisstackable[profile] );
        ftruthf( filewrite, "Name known     : ", chr[character].nameknown );
        ftruthf( filewrite, "Usage known    : ", capusageknown[profile] );
        ftruthf( filewrite, "Is exportable  : ", capcancarrytonextmodule[profile] );
        ftruthf( filewrite, "Requires skill : ", capneedskillidtouse[profile] );
        ftruthf( filewrite, "Is platform    : ", capplatform[profile] );
        ftruthf( filewrite, "Collects money : ", capcangrabmoney[profile] );
        ftruthf( filewrite, "Can open stuff : ", capcanopenstuff[profile] );
        fprintf( filewrite, "\n" );

        // Other item and damage stuff
        fdamagf( filewrite, "Damage type    : ", capdamagetargettype[profile] );
        factiof( filewrite, "Attack type    : ", capweaponaction[profile] );
        fprintf( filewrite, "\n" );

        // Particle attachments
        fprintf( filewrite, "Attached parts : %d\n", capattachedprtamount[profile] );
        fdamagf( filewrite, "Reaffirm type  : ", capattachedprtreaffirmdamagetype[profile] );
        fprintf( filewrite, "Particle type  : %d\n", capattachedprttype[profile] );
        fprintf( filewrite, "\n" );

        // Character hands
        ftruthf( filewrite, "Left valid     : ", capgripvalid[profile][0] );
        ftruthf( filewrite, "Right valid    : ", capgripvalid[profile][1] );
        fprintf( filewrite, "\n" );

        // Particle spawning on attack
        ftruthf( filewrite, "Part on weapon : ", capattackattached[profile] );
        fprintf( filewrite, "Part type      : %d\n", capattackprttype[profile] );
        fprintf( filewrite, "\n" );

        // Particle spawning for GoPoof
        fprintf( filewrite, "Poof amount    : %d\n", capgopoofprtamount[profile] );
        fprintf( filewrite, "Facing add     : %d\n", capgopoofprtfacingadd[profile] );
        fprintf( filewrite, "Part type      : %d\n", capgopoofprttype[profile] );
        fprintf( filewrite, "\n" );

        // Particle spawning for blud
        ftruthf( filewrite, "Blud valid    : ", capbludvalid[profile] );
        fprintf( filewrite, "Part type      : %d\n", capbludprttype[profile] );
        fprintf( filewrite, "\n" );

        // Extra stuff
        ftruthf( filewrite, "Waterwalking   : ", capwaterwalk[profile] );
        fprintf( filewrite, "Bounce dampen  : %5.3f\n", capdampen[profile] );
        fprintf( filewrite, "\n" );

        // More stuff
        fprintf( filewrite, "Life healing   : %5.3f\n", caplifeheal[profile] / 256.0f );
        fprintf( filewrite, "Mana cost      : %5.3f\n", capmanacost[profile] / 256.0f );
        fprintf( filewrite, "Life return    : %d\n", caplifereturn[profile] );
        fprintf( filewrite, "Stopped by     : %d\n", capstoppedby[profile] );
        funderf( filewrite, "Skin 0 name    : ", capskinname[profile][0] );
        funderf( filewrite, "Skin 1 name    : ", capskinname[profile][1] );
        funderf( filewrite, "Skin 2 name    : ", capskinname[profile][2] );
        funderf( filewrite, "Skin 3 name    : ", capskinname[profile][3] );
        fprintf( filewrite, "Skin 0 cost    : %d\n", capskincost[profile][0] );
        fprintf( filewrite, "Skin 1 cost    : %d\n", capskincost[profile][1] );
        fprintf( filewrite, "Skin 2 cost    : %d\n", capskincost[profile][2] );
        fprintf( filewrite, "Skin 3 cost    : %d\n", capskincost[profile][3] );
        fprintf( filewrite, "STR dampen     : %5.3f\n", capstrengthdampen[profile] );
        fprintf( filewrite, "\n" );

        // Another memory lapse
        ftruthf( filewrite, "No rider attak : ", btrue - capridercanattack[profile] );
        ftruthf( filewrite, "Can be dazed   : ", capcanbedazed[profile] );
        ftruthf( filewrite, "Can be grogged : ", capcanbegrogged[profile] );
        fprintf( filewrite, "NOT USED       : 0\n" );
        fprintf( filewrite, "NOT USED       : 0\n" );
        ftruthf( filewrite, "Can see invisi : ", capcanseeinvisible[profile] );
        fprintf( filewrite, "Kursed chance  : %d\n", chr[character].iskursed*100 );
        fprintf( filewrite, "Footfall sound : %d\n", capwavefootfall[profile] );
        fprintf( filewrite, "Jump sound     : %d\n", capwavejump[profile] );
        fprintf( filewrite, "\n" );

        // Expansions
        if ( capskindressy[profile]&1 )
            fprintf( filewrite, ":[DRES] 0\n" );

        if ( capskindressy[profile]&2 )
            fprintf( filewrite, ":[DRES] 1\n" );

        if ( capskindressy[profile]&4 )
            fprintf( filewrite, ":[DRES] 2\n" );

        if ( capskindressy[profile]&8 )
            fprintf( filewrite, ":[DRES] 3\n" );

        if ( capresistbumpspawn[profile] )
            fprintf( filewrite, ":[STUK] 0\n" );

        if ( capistoobig[profile] )
            fprintf( filewrite, ":[PACK] 0\n" );

        if ( !capreflect[profile] )
            fprintf( filewrite, ":[VAMP] 1\n" );

        if ( capalwaysdraw[profile] )
            fprintf( filewrite, ":[DRAW] 1\n" );

        if ( capisranged[profile] )
            fprintf( filewrite, ":[RANG] 1\n" );

        if ( caphidestate[profile] != NOHIDE )
            fprintf( filewrite, ":[HIDE] %d\n", caphidestate[profile] );

        if ( capisequipment[profile] )
            fprintf( filewrite, ":[EQUI] 1\n" );

        if ( capbumpsizebig[profile] == ( capbumpsize[profile] << 1 ) )
            fprintf( filewrite, ":[SQUA] 1\n" );

        if ( capicon[profile] != capusageknown[profile] )
            fprintf( filewrite, ":[ICON] %d\n", capicon[profile] );

        if ( capforceshadow[profile] )
            fprintf( filewrite, ":[SHAD] 1\n" );

        if ( capripple[profile] == capisitem[profile] )
            fprintf( filewrite, ":[RIPP] %d\n", capripple[profile] );

        if ( capisvaluable[profile] != -1 )
            fprintf( filewrite, ":[VALU] %d\n", capisvaluable[profile] );

        //Basic stuff that is always written
        fprintf( filewrite, ":[GOLD] %d\n", chr[character].money );
        fprintf( filewrite, ":[PLAT] %d\n", capcanuseplatforms[profile] );
        fprintf( filewrite, ":[SKIN] %d\n", chr[character].texture - madskinstart[profile] );
        fprintf( filewrite, ":[CONT] %d\n", chr[character].aicontent );
        fprintf( filewrite, ":[STAT] %d\n", chr[character].aistate );
        fprintf( filewrite, ":[LEVL] %d\n", chr[character].experiencelevel );

        //Copy all skill expansions
        fprintf( filewrite, ":[SHPR] %d\n", chr[character].shieldproficiency );

        if ( chr[character].canuseadvancedweapons )
            fprintf( filewrite, ":[AWEP] 1\n" );

        if ( chr[character].canjoust )
            fprintf( filewrite, ":[JOUS] 1\n" );

        if ( chr[character].candisarm )
            fprintf( filewrite, ":[DISA] 1\n" );

        if ( capcanseekurse[profile] )
            fprintf( filewrite, ":[CKUR] 1\n" );

        if ( chr[character].canusepoison )
            fprintf( filewrite, ":[POIS] 1\n" );

        if ( chr[character].canread )
            fprintf( filewrite, ":[READ] 1\n" );

        if ( chr[character].canbackstab )
            fprintf( filewrite, ":[STAB] 1\n" );

        if ( chr[character].canusedivine )
            fprintf( filewrite, ":[HMAG] 1\n" );

        if ( chr[character].canusearcane )
            fprintf( filewrite, ":[WMAG] 1\n" );

        if ( chr[character].canusetech )
            fprintf( filewrite, ":[TECH] 1\n" );

        //The end
        fclose( filewrite );
    }
}

//--------------------------------------------------------------------------------------------
void export_one_character_skin( char *szSaveName, Uint16 character )
{
    // ZZ> This function creates a skin.txt file for the given character.
    FILE* filewrite;
    int profile;

    // General stuff
    profile = chr[character].model;

    // Open the file
    filewrite = fopen( szSaveName, "w" );

    if ( filewrite )
    {
        fprintf( filewrite, "//This file is used only by the import menu\n" );
        fprintf( filewrite, ": %d\n", ( chr[character].texture - madskinstart[profile] )&3 );
        fclose( filewrite );
    }
}

//--------------------------------------------------------------------------------------------
int load_one_character_profile( char *szLoadName )
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
        if ( madused[object] )
        {
            if ( object == SPELLBOOK ) log_error( "Object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, szLoadName );
            else if (overrideslots)  log_error( "Object slot %i used twice (%s)\n", object, szLoadName );
            else return -1;   //Stop, we don't want to override it
        }

        madused[object] = btrue;

        // Read in the real general data
        goto_colon( fileread );  get_name( fileread, capclassname[object] );

        // Light cheat
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capuniformlit[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' || GL_FLAT == shading )  capuniformlit[object] = btrue;

        // Ammo
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capammomax[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capammo[object] = iTmp;
        // Gender
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capgender[object] = GENOTHER;

        if ( cTmp == 'F' || cTmp == 'f' )  capgender[object] = GENFEMALE;

        if ( cTmp == 'M' || cTmp == 'm' )  capgender[object] = GENMALE;

        if ( cTmp == 'R' || cTmp == 'r' )  capgender[object] = GENRANDOM;

        // Read in the object stats
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  caplifecolor[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capmanacolor[object] = iTmp;
        goto_colon( fileread );  read_pair( fileread );
        caplifebase[object] = pairbase;  capliferand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        caplifeperlevelbase[object] = pairbase;  caplifeperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanabase[object] = pairbase;  capmanarand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanaperlevelbase[object] = pairbase;  capmanaperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanareturnbase[object] = pairbase;  capmanareturnrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanareturnperlevelbase[object] = pairbase;  capmanareturnperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanaflowbase[object] = pairbase;  capmanaflowrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanaflowperlevelbase[object] = pairbase;  capmanaflowperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capstrengthbase[object] = pairbase;  capstrengthrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capstrengthperlevelbase[object] = pairbase;  capstrengthperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capwisdombase[object] = pairbase;  capwisdomrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capwisdomperlevelbase[object] = pairbase;  capwisdomperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capintelligencebase[object] = pairbase;  capintelligencerand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capintelligenceperlevelbase[object] = pairbase;  capintelligenceperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capdexteritybase[object] = pairbase;  capdexterityrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capdexterityperlevelbase[object] = pairbase;  capdexterityperlevelrand[object] = pairrand;

        // More physical attributes
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capsize[object] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capsizeperlevel[object] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capshadowsize[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capbumpsize[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capbumpheight[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capbumpdampen[object] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capweight[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capjump[object] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capjumpnumber[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capsneakspd[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capwalkspd[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  caprunspd[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capflyheight[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capflashand[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capalpha[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  caplight[object] = iTmp;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        captransferblend[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  captransferblend[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capsheen[object] = iTmp;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capenviro[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capenviro[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capuoffvel[object] = fTmp * 65535;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capvoffvel[object] = fTmp * 65535;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capstickybutt[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capstickybutt[object] = btrue;

        // Invulnerability data
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capinvictus[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capinvictus[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capnframefacing[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capnframeangle[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capiframefacing[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capiframeangle[object] = iTmp;

        // Resist burning and stuck arrows with nframe angle of 1 or more
        if ( capnframeangle[object] > 0 )
        {
            if ( capnframeangle[object] == 1 )
            {
                capnframeangle[object] = 0;
            }
        }

        // Skin defenses ( 4 skins )
        goto_colon( fileread );
        fscanf( fileread, "%d", &iTmp );  capdefense[object][0] = 255 - iTmp;
        fscanf( fileread, "%d", &iTmp );  capdefense[object][1] = 255 - iTmp;
        fscanf( fileread, "%d", &iTmp );  capdefense[object][2] = 255 - iTmp;
        fscanf( fileread, "%d", &iTmp );  capdefense[object][3] = 255 - iTmp;

        for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
        {
            goto_colon( fileread );
            fscanf( fileread, "%d", &iTmp );  capdamagemodifier[object][damagetype][0] = iTmp;
            fscanf( fileread, "%d", &iTmp );  capdamagemodifier[object][damagetype][1] = iTmp;
            fscanf( fileread, "%d", &iTmp );  capdamagemodifier[object][damagetype][2] = iTmp;
            fscanf( fileread, "%d", &iTmp );  capdamagemodifier[object][damagetype][3] = iTmp;
        }

        for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
        {
            goto_colon( fileread );

            cTmp = get_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  capdamagemodifier[object][damagetype][0] |= DAMAGEINVERT;

            if ( cTmp == 'C' || cTmp == 'c' )  capdamagemodifier[object][damagetype][0] |= DAMAGECHARGE;

            cTmp = get_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  capdamagemodifier[object][damagetype][1] |= DAMAGEINVERT;

            if ( cTmp == 'C' || cTmp == 'c' )  capdamagemodifier[object][damagetype][1] |= DAMAGECHARGE;

            cTmp = get_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  capdamagemodifier[object][damagetype][2] |= DAMAGEINVERT;

            if ( cTmp == 'C' || cTmp == 'c' )  capdamagemodifier[object][damagetype][2] |= DAMAGECHARGE;

            cTmp = get_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  capdamagemodifier[object][damagetype][3] |= DAMAGEINVERT;

            if ( cTmp == 'C' || cTmp == 'c' )  capdamagemodifier[object][damagetype][3] |= DAMAGECHARGE;
        }

        goto_colon( fileread );
        fscanf( fileread, "%f", &fTmp );  capmaxaccel[object][0] = fTmp / 80.0f;
        fscanf( fileread, "%f", &fTmp );  capmaxaccel[object][1] = fTmp / 80.0f;
        fscanf( fileread, "%f", &fTmp );  capmaxaccel[object][2] = fTmp / 80.0f;
        fscanf( fileread, "%f", &fTmp );  capmaxaccel[object][3] = fTmp / 80.0f;

        // Experience and level data
        capexperienceforlevel[object][0] = 0;

        for ( level = 1; level < MAXLEVEL; level++ )
        {
            goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capexperienceforlevel[object][level] = iTmp;
        }

        goto_colon( fileread );  read_pair( fileread );
        pairbase = pairbase >> 8;
        pairrand = pairrand >> 8;

        if ( pairrand < 1 )  pairrand = 1;

        capexperiencebase[object] = pairbase;
        capexperiencerand[object] = pairrand;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capexperienceworth[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capexperienceexchange[object] = fTmp;

        for ( xptype = 0; xptype < XP_COUNT; xptype++ )
        {
            goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capexperiencerate[object][xptype] = fTmp + 0.001f;
        }

        // IDSZ tags
        for ( idsz_cnt = 0; idsz_cnt < IDSZ_COUNT; idsz_cnt++ )
        {
            goto_colon( fileread );  iTmp = get_idsz( fileread );  capidsz[object][idsz_cnt] = iTmp;
        }

        // Item and damage flags
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capisitem[object] = bfalse;  capripple[object] = btrue;

        if ( cTmp == 'T' || cTmp == 't' )  { capisitem[object] = btrue; capripple[object] = bfalse; }

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capismount[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capismount[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capisstackable[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capisstackable[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capnameknown[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capnameknown[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capusageknown[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capusageknown[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capcancarrytonextmodule[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capcancarrytonextmodule[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capneedskillidtouse[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capneedskillidtouse[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capplatform[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capplatform[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capcangrabmoney[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capcangrabmoney[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capcanopenstuff[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capcanopenstuff[object] = btrue;

        // More item and damage stuff
        goto_colon( fileread );  cTmp = get_first_letter( fileread );

        if ( cTmp == 'S' || cTmp == 's' )  capdamagetargettype[object] = DAMAGE_SLASH;

        if ( cTmp == 'C' || cTmp == 'c' )  capdamagetargettype[object] = DAMAGE_CRUSH;

        if ( cTmp == 'P' || cTmp == 'p' )  capdamagetargettype[object] = DAMAGE_POKE;

        if ( cTmp == 'H' || cTmp == 'h' )  capdamagetargettype[object] = DAMAGE_HOLY;

        if ( cTmp == 'E' || cTmp == 'e' )  capdamagetargettype[object] = DAMAGE_EVIL;

        if ( cTmp == 'F' || cTmp == 'f' )  capdamagetargettype[object] = DAMAGE_FIRE;

        if ( cTmp == 'I' || cTmp == 'i' )  capdamagetargettype[object] = DAMAGE_ICE;

        if ( cTmp == 'Z' || cTmp == 'z' )  capdamagetargettype[object] = DAMAGE_ZAP;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capweaponaction[object] = what_action( cTmp );

        // Particle attachments
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capattachedprtamount[object] = iTmp;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );

        if ( cTmp == 'N' || cTmp == 'n' )  capattachedprtreaffirmdamagetype[object] = DAMAGENULL;

        if ( cTmp == 'S' || cTmp == 's' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_SLASH;

        if ( cTmp == 'C' || cTmp == 'c' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_CRUSH;

        if ( cTmp == 'P' || cTmp == 'p' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_POKE;

        if ( cTmp == 'H' || cTmp == 'h' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_HOLY;

        if ( cTmp == 'E' || cTmp == 'e' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_EVIL;

        if ( cTmp == 'F' || cTmp == 'f' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_FIRE;

        if ( cTmp == 'I' || cTmp == 'i' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_ICE;

        if ( cTmp == 'Z' || cTmp == 'z' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_ZAP;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capattachedprttype[object] = iTmp;

        // Character hands
        capgripvalid[object][0] = bfalse;
        capgripvalid[object][1] = bfalse;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );

        if ( cTmp == 'T' || cTmp == 't' )  capgripvalid[object][0] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );

        if ( cTmp == 'T' || cTmp == 't' )  capgripvalid[object][1] = btrue;

        // Attack order ( weapon )
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capattackattached[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capattackattached[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capattackprttype[object] = iTmp;

        // GoPoof
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capgopoofprtamount[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capgopoofprtfacingadd[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capgopoofprttype[object] = iTmp;

        // Blud
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capbludvalid[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capbludvalid[object] = btrue;

        if ( cTmp == 'U' || cTmp == 'u' )  capbludvalid[object] = ULTRABLUDY;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capbludprttype[object] = iTmp;

        // Stuff I forgot
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capwaterwalk[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capwaterwalk[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capdampen[object] = fTmp;

        // More stuff I forgot
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  caplifeheal[object] = fTmp * 256;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capmanacost[object] = fTmp * 256;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  caplifereturn[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capstoppedby[object] = iTmp | MESHFXIMPASS;
        goto_colon( fileread );  get_name( fileread, capskinname[object][0] );
        goto_colon( fileread );  get_name( fileread, capskinname[object][1] );
        goto_colon( fileread );  get_name( fileread, capskinname[object][2] );
        goto_colon( fileread );  get_name( fileread, capskinname[object][3] );
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capskincost[object][0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capskincost[object][1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capskincost[object][2] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capskincost[object][3] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capstrengthdampen[object] = fTmp;

        // Another memory lapse
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capridercanattack[object] = btrue;

        if ( cTmp == 'T' || cTmp == 't' )  capridercanattack[object] = bfalse;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );  // Can be dazed
        capcanbedazed[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capcanbedazed[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );  // Can be grogged
        capcanbegrogged[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capcanbegrogged[object] = btrue;

        goto_colon( fileread );  // !!!BAD!!! Life add
        goto_colon( fileread );  // !!!BAD!!! Mana add
        goto_colon( fileread );  cTmp = get_first_letter( fileread );  // Can see invisible
        capcanseeinvisible[object] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  capcanseeinvisible[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Chance of kursed
        capkursechance[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Footfall sound
        capwavefootfall[object] = CLIP(iTmp, -1, MAXWAVE);
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Jump sound
        capwavejump[object] = CLIP(iTmp, -1, MAXWAVE);

        // Clear expansions...
        capskindressy[object] = bfalse;
        capresistbumpspawn[object] = bfalse;
        capistoobig[object] = bfalse;
        capreflect[object] = btrue;
        capalwaysdraw[object] = bfalse;
        capisranged[object] = bfalse;
        caphidestate[object] = NOHIDE;
        capisequipment[object] = bfalse;
        capbumpsizebig[object] = capbumpsize[object] + ( capbumpsize[object] >> 1 );
        capcanseekurse[object] = bfalse;
        capmoney[object] = 0;
        capicon[object] = capusageknown[object];
        capforceshadow[object] = bfalse;
        capskinoverride[object] = NOSKINOVERRIDE;
        capcontentoverride[object] = 0;
        capstateoverride[object] = 0;
        capleveloverride[object] = 0;
        capcanuseplatforms[object] = !capplatform[object];
        capisvaluable[object] = 0;

        //Skills
        capcanuseadvancedweapons[object] = 0;
        capcanjoust[object] = 0;
        capcanusetech[object] = 0;
        capcanusedivine[object] = 0;
        capcanusearcane[object] = 0;
        capshieldproficiency[object] = 0;
        capcandisarm[object] = 0;
        capcanbackstab[object] = 0;
        capcanusepoison[object] = 0;
        capcanread[object] = 0;

        // Read expansions
        while ( goto_colon_yesno( fileread ) )
        {
            idsz = get_idsz( fileread );
            fscanf( fileread, "%c%d", &cTmp, &iTmp );

            test = Make_IDSZ( "DRES" );  // [DRES]

            if ( idsz == test )  capskindressy[object] |= 1 << iTmp;

            test = Make_IDSZ( "GOLD" );  // [GOLD]

            if ( idsz == test )  capmoney[object] = iTmp;

            test = Make_IDSZ( "STUK" );  // [STUK]

            if ( idsz == test )  capresistbumpspawn[object] = 1 - iTmp;

            test = Make_IDSZ( "PACK" );  // [PACK]

            if ( idsz == test )  capistoobig[object] = 1 - iTmp;

            test = Make_IDSZ( "VAMP" );  // [VAMP]

            if ( idsz == test )  capreflect[object] = 1 - iTmp;

            test = Make_IDSZ( "DRAW" );  // [DRAW]

            if ( idsz == test )  capalwaysdraw[object] = iTmp;

            test = Make_IDSZ( "RANG" );  // [RANG]

            if ( idsz == test )  capisranged[object] = iTmp;

            test = Make_IDSZ( "HIDE" );  // [HIDE]

            if ( idsz == test )  caphidestate[object] = iTmp;

            test = Make_IDSZ( "EQUI");  // [EQUI]

            if ( idsz == test )  capisequipment[object] = iTmp;

            test = Make_IDSZ( "SQUA");  // [SQUA]

            if ( idsz == test )  capbumpsizebig[object] = capbumpsize[object] << 1;

            test = Make_IDSZ( "ICON" );  // [ICON]

            if ( idsz == test )  capicon[object] = iTmp;

            test = Make_IDSZ( "SHAD" );  // [SHAD]

            if ( idsz == test )  capforceshadow[object] = iTmp;

            test = Make_IDSZ( "CKUR" );  // [CKUR]

            if ( idsz == test )  capcanseekurse[object] = iTmp;

            test = Make_IDSZ( "SKIN" );  // [SKIN]

            if ( idsz == test )  capskinoverride[object] = iTmp & 3;

            test = Make_IDSZ( "CONT" );  // [CONT]

            if ( idsz == test )  capcontentoverride[object] = iTmp;

            test = Make_IDSZ( "STAT" );  // [STAT]

            if ( idsz == test )  capstateoverride[object] = iTmp;

            test = Make_IDSZ( "LEVL" );  // [LEVL]

            if ( idsz == test )  capleveloverride[object] = iTmp;

            test = Make_IDSZ( "PLAT" );  // [PLAT]

            if ( idsz == test )  capcanuseplatforms[object] = iTmp;

            test = Make_IDSZ( "RIPP" );  // [RIPP]

            if ( idsz == test )  capripple[object] = iTmp;

            test = Make_IDSZ( "VALU" );  // [VALU]

            if ( idsz == test ) capisvaluable[object] = iTmp;

            //Read Skills
            test = Make_IDSZ( "AWEP" );  // [AWEP]

            if ( idsz == test )  capcanuseadvancedweapons[object] = iTmp;

            test = Make_IDSZ( "SHPR" );  // [SHPR]

            if ( idsz == test )  capshieldproficiency[object] = iTmp;

            test = Make_IDSZ( "JOUS" );  // [JOUS]

            if ( idsz == test )  capcanjoust[object] = iTmp;

            test = Make_IDSZ( "WMAG" );  // [WMAG]

            if ( idsz == test )  capcanusearcane[object] = iTmp;

            test = Make_IDSZ( "HMAG" );  // [HMAG]

            if ( idsz == test )  capcanusedivine[object] = iTmp;

            test = Make_IDSZ( "TECH" );  // [TECH]

            if ( idsz == test )  capcanusetech[object] = iTmp;

            test = Make_IDSZ( "DISA" );  // [DISA]

            if ( idsz == test )  capcandisarm[object] = iTmp;

            test = Make_IDSZ( "STAB" );  // [STAB]

            if ( idsz == test )  capcanbackstab[object] = iTmp;

            test = Make_IDSZ( "POIS" );  // [POIS]

            if ( idsz == test )  capcanusepoison[object] = iTmp;

            test = Make_IDSZ( "READ" );  // [READ]

            if ( idsz == test )  capcanread[object] = iTmp;
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
int get_skin( char *filename )
{
    // ZZ> This function reads the skin.txt file...
    FILE*   fileread;
    int skin;

    skin = 0;
    fileread = fopen( filename, "r" );

    if ( fileread )
    {
        goto_colon_yesno( fileread );
        fscanf( fileread, "%d", &skin );
        skin = skin & 3;
        fclose( fileread );
    }

    return skin;
}

//--------------------------------------------------------------------------------------------
void check_player_import( char *dirname )
{
    // ZZ> This function figures out which players may be imported, and loads basic
    //     data for each
    char searchname[128];
    STRING filename;
    int skin;
    bool_t keeplooking;
    const char *foundfile;

    // Set up...
    numloadplayer = 0;
    globalnumicon = 0;

    // Search for all objects
    sprintf( searchname, "%s" SLASH_STR "*.obj", dirname );
    foundfile = fs_findFirstFile( dirname, "obj" );
    keeplooking = 1;

    if ( foundfile != NULL )
    {
        while ( keeplooking && numloadplayer < MAXLOADPLAYER )
        {
            prime_names();
            sprintf( loadplayerdir[numloadplayer], "%s", foundfile );

            sprintf( filename, "%s" SLASH_STR "%s" SLASH_STR "skin.txt", dirname, foundfile );
            skin = get_skin( filename );

            snprintf( filename, sizeof(filename), "%s" SLASH_STR "%s" SLASH_STR "tris.md2", dirname, foundfile );
            load_one_md2( filename, numloadplayer );

            sprintf( filename, "%s" SLASH_STR "%s" SLASH_STR "icon%d", dirname, foundfile, skin );
            load_one_icon( filename );

            sprintf( filename, "%s" SLASH_STR "%s" SLASH_STR "naming.txt", dirname, foundfile );
            read_naming( 0, filename );
            naming_names( 0 );
            sprintf( loadplayername[numloadplayer], "%s", namingnames );

            numloadplayer++;

            foundfile = fs_findNextFile();
            if ( foundfile == NULL ) keeplooking = 0; else keeplooking = 1;
        }
    }

    fs_findClose();

    keybplayer = 0;
    mousplayer = 0;
    joyaplayer = 0;
    joybplayer = 0;
}

//--------------------------------------------------------------------------------------------
void damage_character( Uint16 character, Uint16 direction,
                       int damagebase, int damagerand, Uint8 damagetype, Uint8 team,
                       Uint16 attacker, Uint16 effects )
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

    if ( chr[character].alive && damagebase >= 0 && damagerand >= 1 )
    {
        // Lessen damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
        // This can also be used to lessen effectiveness of healing
        damage = damagebase + ( rand() % damagerand );
        basedamage = damage;
        damage = damage >> ( chr[character].damagemodifier[damagetype] & DAMAGESHIFT );

        // Allow charging (Invert damage to mana)
        if ( chr[character].damagemodifier[damagetype]&DAMAGECHARGE )
        {
            chr[character].mana += damage;

            if ( chr[character].mana > chr[character].manamax )
            {
                chr[character].mana = chr[character].manamax;
            }

            return;
        }

        // Invert damage to heal
        if ( chr[character].damagemodifier[damagetype]&DAMAGEINVERT )
            damage = -damage;

        // Remember the damage type
        chr[character].damagetypelast = damagetype;
        chr[character].directionlast = direction;

        // Do it already
        if ( damage > 0 )
        {
            // Only damage if not invincible
            if ( chr[character].damagetime == 0 && !chr[character].invictus )
            {
                model = chr[character].model;

                if ( 0 == ( effects&DAMFXBLOC ) )
                {
                    // Only damage if hitting from proper direction
                    if ( madframefx[chr[character].frame]&MADFXINVICTUS )
                    {
                        // I Frame...
                        direction -= capiframefacing[model];
                        left = ( ~capiframeangle[model] );
                        right = capiframeangle[model];

                        // Check for shield
                        if ( chr[character].action >= ACTIONPA && chr[character].action <= ACTIONPD )
                        {
                            // Using a shield?
                            if ( chr[character].action < ACTIONPC )
                            {
                                // Check left hand
                                if ( chr[character].holdingwhich[0] != MAXCHR )
                                {
                                    left = ( ~capiframeangle[chr[chr[character].holdingwhich[0]].model] );
                                    right = capiframeangle[chr[chr[character].holdingwhich[0]].model];
                                }
                            }
                            else
                            {
                                // Check right hand
                                if ( chr[character].holdingwhich[1] != MAXCHR )
                                {
                                    left = ( ~capiframeangle[chr[chr[character].holdingwhich[1]].model] );
                                    right = capiframeangle[chr[chr[character].holdingwhich[1]].model];
                                }
                            }
                        }
                    }
                    else
                    {
                        // N Frame
                        direction -= capnframefacing[model];
                        left = ( ~capnframeangle[model] );
                        right = capnframeangle[model];
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
                        chr[character].life -= damage;
                    }
                    else
                    {
                        chr[character].life -= ( ( damage * chr[character].defense ) >> 8 );
                    }

                    if ( basedamage > HURTDAMAGE )
                    {
                        // Call for help if below 1/2 life
                        /*if(chr[character].life < (chr[character].lifemax>>1))
                            call_for_help(character);*/

                        // Spawn blud particles
                        if ( capbludvalid[model] && ( damagetype < DAMAGE_HOLY || capbludvalid[model] == ULTRABLUDY ) )
                        {
                            spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos,
                                                chr[character].turnleftright + direction, chr[character].model, capbludprttype[model],
                                                MAXCHR, SPAWNLAST, chr[character].team, character, 0, MAXCHR );
                        }

                        // Set attack alert if it wasn't an accident
                        if ( team == DAMAGETEAM )
                        {
                            chr[character].attacklast = MAXCHR;
                        }
                        else
                        {
                            // Don't alert the character too much if under constant fire
                            if ( chr[character].carefultime == 0 )
                            {
                                // Don't let characters chase themselves...  That would be silly
                                if ( attacker != character )
                                {
                                    chr[character].alert = chr[character].alert | ALERTIFATTACKED;
                                    chr[character].attacklast = attacker;
                                    chr[character].carefultime = CAREFULTIME;
                                }
                            }
                        }
                    }

                    // Taking damage action
                    action = ACTIONHA;

                    if ( chr[character].life < 0 )
                    {
                        // Character has died
                        chr[character].alive = bfalse;
                        disenchant_character( character );
                        chr[character].waskilled = btrue;
                        chr[character].keepaction = btrue;
                        chr[character].life = -1;
                        chr[character].platform = btrue;
                        chr[character].bumpdampen = chr[character].bumpdampen / 2.0f;
                        action = ACTIONKA;
                        // Give kill experience
                        experience = capexperienceworth[model] + ( chr[character].experience * capexperienceexchange[model] );

                        if ( attacker < MAXCHR )
                        {
                            // Set target
                            chr[character].aitarget = attacker;

                            if ( team == DAMAGETEAM )  chr[character].aitarget = character;

                            if ( team == NULLTEAM )  chr[character].aitarget = character;

                            // Award direct kill experience
                            if ( teamhatesteam[chr[attacker].team][chr[character].team] )
                            {
                                give_experience( attacker, experience, XP_KILLENEMY );
                            }

                            // Check for hated
                            if ( capidsz[chr[attacker].model][IDSZ_HATE] == capidsz[model][IDSZ_PARENT] ||
                                    capidsz[chr[attacker].model][IDSZ_HATE] == capidsz[model][IDSZ_TYPE] )
                            {
                                give_experience( attacker, experience, XP_KILLHATED );
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
                            if ( chr[tnc].on && chr[tnc].alive )
                            {
                                if ( chr[tnc].aitarget == character )
                                {
                                    chr[tnc].alert = chr[tnc].alert | ALERTIFTARGETKILLED;
                                }

                                if ( !teamhatesteam[chr[tnc].team][team] && ( teamhatesteam[chr[tnc].team][chr[character].team] ) )
                                {
                                    // All allies get team experience, but only if they also hate the dead guy's team
                                    give_experience( tnc, experience, XP_TEAMKILL );
                                }
                            }

                            tnc++;
                        }

                        // Check if it was a leader
                        if ( teamleader[chr[character].team] == character )
                        {
                            // It was a leader, so set more alerts
                            tnc = 0;

                            while ( tnc < MAXCHR )
                            {
                                if ( chr[tnc].on && chr[tnc].team == chr[character].team )
                                {
                                    // All folks on the leaders team get the alert
                                    chr[tnc].alert = chr[tnc].alert | ALERTIFLEADERKILLED;
                                }

                                tnc++;
                            }

                            // The team now has no leader
                            teamleader[chr[character].team] = NOLEADER;
                        }

                        detach_character_from_mount( character, btrue, bfalse );

                        //Play the death animation
                        action += ( rand() & 3 );
                        play_action( character, action, bfalse );

                        //If it's a player, let it die properly before enabling respawn
                        if (chr[character].isplayer) revivetimer = ONESECOND; //1 second

                        // Afford it one last thought if it's an AI
                        teammorale[chr[character].baseteam]--;
                        chr[character].team = chr[character].baseteam;
                        chr[character].alert = ALERTIFKILLED;
                        chr[character].sparkle = NOSPARKLE;
                        chr[character].aitime = 1;  // No timeout...
                        let_character_think( character );
                    }
                    else
                    {
                        if ( basedamage > HURTDAMAGE )
                        {
                            action += ( rand() & 3 );
                            play_action( character, action, bfalse );

                            // Make the character invincible for a limited time only
                            if ( !( effects & DAMFXTIME ) )
                                chr[character].damagetime = DAMAGETIME;
                        }
                    }
                }
                else
                {
                    // Spawn a defend particle
                    spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos, chr[character].turnleftright, MAXMODEL, DEFEND, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                    chr[character].damagetime = DEFENDTIME;
                    chr[character].alert = chr[character].alert | ALERTIFBLOCKED;
                    chr[character].attacklast = attacker;     // For the ones attacking a shield
                }
            }
        }
        else if ( damage < 0 )
        {
            chr[character].life -= damage;

            if ( chr[character].life > chr[character].lifemax )  chr[character].life = chr[character].lifemax;

            // Isssue an alert
            chr[character].alert = chr[character].alert | ALERTIFHEALED;
            chr[character].attacklast = attacker;

            if ( team != DAMAGETEAM )
            {
                chr[character].attacklast = MAXCHR;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void kill_character( Uint16 character, Uint16 killer )
{
    // ZZ> This function kills a character...  MAXCHR killer for accidental death
    Uint8 modifier;

    if ( chr[character].alive )
    {
        chr[character].damagetime = 0;
        chr[character].life = 1;
        modifier = chr[character].damagemodifier[DAMAGE_CRUSH];
        chr[character].damagemodifier[DAMAGE_CRUSH] = 1;

        if ( killer != MAXCHR )
        {
            damage_character( character, 0, 512, 1, DAMAGE_CRUSH, chr[killer].team, killer, DAMFXARMO | DAMFXBLOC );
        }
        else
        {
            damage_character( character, 0, 512, 1, DAMAGE_CRUSH, DAMAGETEAM, chr[character].bumplast, DAMFXARMO | DAMFXBLOC );
        }

        chr[character].damagemodifier[DAMAGE_CRUSH] = modifier;
    }
}

//--------------------------------------------------------------------------------------------
void spawn_poof( Uint16 character, Uint16 profile )
{
    // ZZ> This function spawns a character poof
    Uint16 sTmp;
    Uint16 origin;
    int iTmp;

    sTmp = chr[character].turnleftright;
    iTmp = 0;
    origin = chr[character].aiowner;

    while ( iTmp < capgopoofprtamount[profile] )
    {
        spawn_one_particle( chr[character].oldx, chr[character].oldy, chr[character].oldz,
                            sTmp, profile, capgopoofprttype[profile],
                            MAXCHR, SPAWNLAST, chr[character].team, origin, iTmp, MAXCHR );
        sTmp += capgopoofprtfacingadd[profile];
        iTmp++;
    }
}

//--------------------------------------------------------------------------------------------
void naming_names( int profile )
{
    // ZZ> This function generates a random name
    int read, write, section, mychop;
    char cTmp;

    if ( capsectionsize[profile][0] == 0 )
    {
        namingnames[0] = 'B';
        namingnames[1] = 'l';
        namingnames[2] = 'a';
        namingnames[3] = 'h';
        namingnames[4] = 0;
    }
    else
    {
        write = 0;
        section = 0;

        while ( section < MAXSECTION )
        {
            if ( capsectionsize[profile][section] != 0 )
            {
                mychop = capsectionstart[profile][section] + ( rand() % capsectionsize[profile][section] );
                read = chopstart[mychop];
                cTmp = chopdata[read];

                while ( cTmp != 0 && write < MAXCAPNAMESIZE - 1 )
                {
                    namingnames[write] = cTmp;
                    write++;
                    read++;
                    cTmp = chopdata[read];
                }
            }

            section++;
        }

        if ( write >= MAXCAPNAMESIZE ) write = MAXCAPNAMESIZE - 1;

        namingnames[write] = 0;
    }
}

//--------------------------------------------------------------------------------------------
void read_naming( int profile, char *szLoadname )
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

            if ( mychop[0] != 'S' || mychop[1] != 'T' || mychop[2] != 'O' || mychop[3] != 'P' )
            {
                if ( chopwrite >= CHOPDATACHUNK )  chopwrite = CHOPDATACHUNK - 1;

                chopstart[numchop] = chopwrite;
                cnt = 0;
                cTmp = mychop[0];

                while ( cTmp != 0 && cnt < 31 && chopwrite < CHOPDATACHUNK )
                {
                    if ( cTmp == '_' ) cTmp = ' ';

                    chopdata[chopwrite] = cTmp;
                    cnt++;
                    chopwrite++;
                    cTmp = mychop[cnt];
                }

                if ( chopwrite >= CHOPDATACHUNK )  chopwrite = CHOPDATACHUNK - 1;

                chopdata[chopwrite] = 0;  chopwrite++;
                chopinsection++;
                numchop++;
            }
            else
            {
                capsectionsize[profile][section] = chopinsection;
                capsectionstart[profile][section] = numchop - chopinsection;
                section++;
                chopinsection = 0;
            }
        }

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void prime_names( void )
{
    // ZZ> This function prepares the name chopper for use
    int cnt, tnc;

    numchop = 0;
    chopwrite = 0;
    cnt = 0;

    while ( cnt < MAXMODEL )
    {
        tnc = 0;

        while ( tnc < MAXSECTION )
        {
            capsectionstart[cnt][tnc] = MAXCHOP;
            capsectionsize[cnt][tnc] = 0;
            tnc++;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain()
{
    // ZZ> This function sets all of the character's starting tilt values
    int cnt;
    Uint8 twist;

    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].stickybutt && chr[cnt].on && chr[cnt].onwhichfan != OFFEDGE )
        {
            twist = meshtwist[chr[cnt].onwhichfan];
            chr[cnt].turnmaplr = maplrtwist[twist];
            chr[cnt].turnmapud = mapudtwist[twist];
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
int spawn_one_character( float x, float y, float z, int profile, Uint8 team,
                         Uint8 skin, Uint16 facing, char *name, int override )
{
    // ZZ> This function spawns a character and returns the character's index number
    //     if it worked, MAXCHR otherwise
    int cnt, tnc, ix, iy;

    // Make sure the team is valid
    if ( team > MAXTEAM - 1 )
        team = MAXTEAM - 1;

    // Get a new character
    cnt = MAXCHR;

    if ( madused[profile] )
    {
        if ( override < MAXCHR )
        {
            cnt = get_free_character();

            if ( cnt != override )
            {
                // Picked the wrong one, so put this one back and find the right one
                tnc = 0;

                while ( tnc < MAXCHR )
                {
                    if ( freechrlist[tnc] == override )
                    {
                        freechrlist[tnc] = cnt;
                        tnc = MAXCHR;
                    }

                    tnc++;
                }

                cnt = override;
            }
        }
        else
        {
            cnt = get_free_character();
        }
        assert(cnt <= MAXCHR);
        if ( cnt != MAXCHR )
        {
            // IMPORTANT!!!
            chr[cnt].indolist = bfalse;
            chr[cnt].isequipped = bfalse;
            chr[cnt].sparkle = NOSPARKLE;
            chr[cnt].overlay = bfalse;
            chr[cnt].missilehandler = cnt;

            // SetXY stuff...  Just in case
            tnc = 0;

            while ( tnc < MAXSTOR )
            {
                chr[cnt].aix[tnc] = 0;
                chr[cnt].aiy[tnc] = 0;
                tnc++;
            }

            // Set up model stuff
            chr[cnt].reloadtime = 0;
            chr[cnt].inwhichhand = GRIPLEFT;
            chr[cnt].waskilled = bfalse;
            chr[cnt].inpack = bfalse;
            chr[cnt].nextinpack = MAXCHR;
            chr[cnt].numinpack = 0;
            chr[cnt].model = profile;
            chr[cnt].basemodel = profile;
            chr[cnt].stoppedby = capstoppedby[profile];
            chr[cnt].lifeheal = caplifeheal[profile];
            chr[cnt].manacost = capmanacost[profile];
            chr[cnt].inwater = bfalse;
            chr[cnt].nameknown = capnameknown[profile];
            chr[cnt].ammoknown = capnameknown[profile];
            chr[cnt].hitready = btrue;
            chr[cnt].boretime = BORETIME;
            chr[cnt].carefultime = CAREFULTIME;
            chr[cnt].canbecrushed = bfalse;
            chr[cnt].damageboost = 0;
            chr[cnt].icon = capicon[profile];

            // Enchant stuff
            chr[cnt].firstenchant = MAXENCHANT;
            chr[cnt].undoenchant = MAXENCHANT;
            chr[cnt].canseeinvisible = capcanseeinvisible[profile];
            chr[cnt].canchannel = bfalse;
            chr[cnt].missiletreatment = MISNORMAL;
            chr[cnt].missilecost = 0;

            //Skillz
            chr[cnt].canjoust = capcanjoust[profile];
            chr[cnt].canuseadvancedweapons = capcanuseadvancedweapons[profile];
            chr[cnt].shieldproficiency = capshieldproficiency[profile];
            chr[cnt].canusedivine = capcanusedivine[profile];
            chr[cnt].canusearcane = capcanusearcane[profile];
            chr[cnt].canusetech = capcanusetech[profile];
            chr[cnt].candisarm = capcandisarm[profile];
            chr[cnt].canbackstab = capcanbackstab[profile];
            chr[cnt].canusepoison = capcanusepoison[profile];
            chr[cnt].canread = capcanread[profile];
            chr[cnt].canseekurse = capcanseekurse[profile];

            // Kurse state
            chr[cnt].iskursed = ( ( rand() % 100 ) < capkursechance[profile] );

            if ( !capisitem[profile] )  chr[cnt].iskursed = bfalse;

            // Ammo
            chr[cnt].ammomax = capammomax[profile];
            chr[cnt].ammo = capammo[profile];

            // Gender
            chr[cnt].gender = capgender[profile];

            if ( chr[cnt].gender == GENRANDOM )  chr[cnt].gender = GENFEMALE + ( rand() & 1 );

            // Team stuff
            chr[cnt].team = team;
            chr[cnt].baseteam = team;
            chr[cnt].counter = teammorale[team];

            if ( !capinvictus[profile] )  teammorale[team]++;

            chr[cnt].order = 0;

            // Firstborn becomes the leader
            if ( teamleader[team] == NOLEADER )
            {
                teamleader[team] = cnt;
            }

            // Skin
            if ( capskinoverride[profile] != NOSKINOVERRIDE )
            {
                skin = capskinoverride[profile] & 3;
            }

            if ( skin >= madskins[profile] )
            {
                skin = 0;

                if ( madskins[profile] > 1 )
                {
                    skin = rand() % madskins[profile];
                }
            }

            chr[cnt].texture = madskinstart[profile] + skin;
            // Life and Mana
            chr[cnt].alive = btrue;
            chr[cnt].lifecolor = caplifecolor[profile];
            chr[cnt].manacolor = capmanacolor[profile];
            chr[cnt].lifemax = generate_number( caplifebase[profile], capliferand[profile] );
            chr[cnt].life = chr[cnt].lifemax;
            chr[cnt].lifereturn = caplifereturn[profile];
            chr[cnt].manamax = generate_number( capmanabase[profile], capmanarand[profile] );
            chr[cnt].manaflow = generate_number( capmanaflowbase[profile], capmanaflowrand[profile] );
            chr[cnt].manareturn = generate_number( capmanareturnbase[profile], capmanareturnrand[profile] );
            chr[cnt].mana = chr[cnt].manamax;
            // SWID
            chr[cnt].strength = generate_number( capstrengthbase[profile], capstrengthrand[profile] );
            chr[cnt].wisdom = generate_number( capwisdombase[profile], capwisdomrand[profile] );
            chr[cnt].intelligence = generate_number( capintelligencebase[profile], capintelligencerand[profile] );
            chr[cnt].dexterity = generate_number( capdexteritybase[profile], capdexterityrand[profile] );
            // Damage
            chr[cnt].defense = capdefense[profile][skin];
            chr[cnt].reaffirmdamagetype = capattachedprtreaffirmdamagetype[profile];
            chr[cnt].damagetargettype = capdamagetargettype[profile];
            tnc = 0;

            while ( tnc < DAMAGE_COUNT )
            {
                chr[cnt].damagemodifier[tnc] = capdamagemodifier[profile][tnc][skin];
                tnc++;
            }

            // AI stuff
            chr[cnt].aitype = madai[chr[cnt].model];
            chr[cnt].isplayer = bfalse;
            chr[cnt].islocalplayer = bfalse;
            chr[cnt].alert = ALERTIFSPAWNED;
            chr[cnt].aistate = capstateoverride[profile];
            chr[cnt].aicontent = capcontentoverride[profile];
            chr[cnt].aitarget = cnt;
            chr[cnt].aiowner = cnt;
            chr[cnt].aichild = cnt;
            chr[cnt].aitime = 0;
            chr[cnt].latchx = 0;
            chr[cnt].latchy = 0;
            chr[cnt].latchbutton = 0;
            chr[cnt].turnmode = TURNMODEVELOCITY;
            // Flags
            chr[cnt].stickybutt = capstickybutt[profile];
            chr[cnt].openstuff = capcanopenstuff[profile];
            chr[cnt].transferblend = captransferblend[profile];
            chr[cnt].enviro = capenviro[profile];
            chr[cnt].waterwalk = capwaterwalk[profile];
            chr[cnt].platform = capplatform[profile];
            chr[cnt].isitem = capisitem[profile];
            chr[cnt].invictus = capinvictus[profile];
            chr[cnt].ismount = capismount[profile];
            chr[cnt].cangrabmoney = capcangrabmoney[profile];
            // Jumping
            chr[cnt].jump = capjump[profile];
            chr[cnt].jumpnumber = 0;
            chr[cnt].jumpnumberreset = capjumpnumber[profile];
            chr[cnt].jumptime = JUMPDELAY;
            // Other junk
            chr[cnt].flyheight = capflyheight[profile];
            chr[cnt].maxaccel = capmaxaccel[profile][skin];
            chr[cnt].alpha = chr[cnt].basealpha = capalpha[profile];
            chr[cnt].light = caplight[profile];
            chr[cnt].flashand = capflashand[profile];
            chr[cnt].sheen = capsheen[profile];
            chr[cnt].dampen = capdampen[profile];
            // Character size and bumping
            chr[cnt].fat = capsize[profile];
            chr[cnt].sizegoto = chr[cnt].fat;
            chr[cnt].sizegototime = 0;
            chr[cnt].shadowsize = capshadowsize[profile] * chr[cnt].fat;
            chr[cnt].bumpsize = capbumpsize[profile] * chr[cnt].fat;
            chr[cnt].bumpsizebig = capbumpsizebig[profile] * chr[cnt].fat;
            chr[cnt].bumpheight = capbumpheight[profile] * chr[cnt].fat;

            chr[cnt].shadowsizesave = capshadowsize[profile];
            chr[cnt].bumpsizesave = capbumpsize[profile];
            chr[cnt].bumpsizebigsave = capbumpsizebig[profile];
            chr[cnt].bumpheightsave = capbumpheight[profile];

            chr[cnt].bumpdampen = capbumpdampen[profile];
            chr[cnt].weight = capweight[profile] * chr[cnt].fat;

            if ( capweight[profile] == 255 ) chr[cnt].weight = 65535;

            chr[cnt].bumplast = cnt;
            chr[cnt].attacklast = MAXCHR;
            chr[cnt].hitlast = cnt;
            // Grip info
            chr[cnt].attachedto = MAXCHR;
            chr[cnt].holdingwhich[0] = MAXCHR;
            chr[cnt].holdingwhich[1] = MAXCHR;
            // Image rendering
            chr[cnt].uoffset = 0;
            chr[cnt].voffset = 0;
            chr[cnt].uoffvel = capuoffvel[profile];
            chr[cnt].voffvel = capvoffvel[profile];
            chr[cnt].redshift = 0;
            chr[cnt].grnshift = 0;
            chr[cnt].blushift = 0;
            // Movement
            chr[cnt].sneakspd = capsneakspd[profile];
            chr[cnt].walkspd = capwalkspd[profile];
            chr[cnt].runspd = caprunspd[profile];

            // Set up position
            chr[cnt].xpos = x;
            chr[cnt].ypos = y;
            chr[cnt].oldx = x;
            chr[cnt].oldy = y;
            chr[cnt].turnleftright = facing;
            chr[cnt].lightturnleftright = 0;
            ix = x;
            iy = y;
            chr[cnt].onwhichfan = ( ix >> 7 ) + meshfanstart[iy>>7];
            chr[cnt].level = get_level( chr[cnt].xpos, chr[cnt].ypos, chr[cnt].onwhichfan, chr[cnt].waterwalk ) + RAISE;

            if ( z < chr[cnt].level ) z = chr[cnt].level;

            chr[cnt].zpos = z;
            chr[cnt].oldz = z;
            chr[cnt].xstt = chr[cnt].xpos;
            chr[cnt].ystt = chr[cnt].ypos;
            chr[cnt].zstt = chr[cnt].zpos;
            chr[cnt].xvel = 0;
            chr[cnt].yvel = 0;
            chr[cnt].zvel = 0;
            chr[cnt].turnmaplr = 32768;  // These two mean on level surface
            chr[cnt].turnmapud = 32768;
            chr[cnt].scale = chr[cnt].fat * madscale[chr[cnt].model] * 4;

            // AI and action stuff
            chr[cnt].aigoto = 0;
            chr[cnt].aigotoadd = 0;
            chr[cnt].actionready = btrue;
            chr[cnt].keepaction = bfalse;
            chr[cnt].loopaction = bfalse;
            chr[cnt].action = ACTIONDA;
            chr[cnt].nextaction = ACTIONDA;
            chr[cnt].lip = 0;
            chr[cnt].frame = madframestart[chr[cnt].model];
            chr[cnt].lastframe = chr[cnt].frame;
            chr[cnt].passage = 0;
            chr[cnt].holdingweight = 0;

            // Timers set to 0
            chr[cnt].grogtime = 0;
            chr[cnt].dazetime = 0;

            // Money is added later
            chr[cnt].money = capmoney[profile];

            // Name the character
            if ( name == NULL )
            {
                // Generate a random name
                naming_names( profile );
                sprintf( chr[cnt].name, "%s", namingnames );
            }
            else
            {
                // A name has been given
                tnc = 0;

                while ( tnc < MAXCAPNAMESIZE - 1 )
                {
                    chr[cnt].name[tnc] = name[tnc];
                    tnc++;
                }

                chr[cnt].name[tnc] = 0;
            }

            // Set up initial fade in lighting
            for ( tnc = 0; tnc < madtransvertices[chr[cnt].model]; tnc++ )
            {
                chr[cnt].vrta[tnc] = 0;
            }

            // Particle attachments
            tnc = 0;

            while ( tnc < capattachedprtamount[profile] )
            {
                spawn_one_particle( chr[cnt].xpos, chr[cnt].ypos, chr[cnt].zpos,
                                    0, chr[cnt].model, capattachedprttype[profile],
                                    cnt, SPAWNLAST + tnc, chr[cnt].team, cnt, tnc, MAXCHR );
                tnc++;
            }

            chr[cnt].reaffirmdamagetype = capattachedprtreaffirmdamagetype[profile];

            // Experience
            tnc = generate_number( capexperiencebase[profile], capexperiencerand[profile] );

            if ( tnc > MAXXP ) tnc = MAXXP;

            chr[cnt].experience = tnc;
            chr[cnt].experiencelevel = capleveloverride[profile];

            //Items that are spawned inside shop passages are more expensive than normal
            if (capisvaluable[profile])
            {
                chr[cnt].isshopitem = btrue;
            }
            else
            {
                chr[cnt].isshopitem = bfalse;

                if (chr[cnt].isitem && !chr[cnt].inpack && chr[cnt].attachedto == MAXCHR)
                {
                    float tlx, tly, brx, bry;
                    Uint16 passage = 0;
                    float bumpsize;

                    bumpsize = chr[cnt].bumpsize;

                    while (passage < numpassage)
                    {
                        // Passage area
                        tlx = ( passtlx[passage] << 7 ) - CLOSETOLERANCE;
                        tly = ( passtly[passage] << 7 ) - CLOSETOLERANCE;
                        brx = ( ( passbrx[passage] + 1 ) << 7 ) + CLOSETOLERANCE;
                        bry = ( ( passbry[passage] + 1 ) << 7 ) + CLOSETOLERANCE;

                        //Check if the character is inside that passage
                        if ( chr[cnt].xpos > tlx - bumpsize && chr[cnt].xpos < brx + bumpsize )
                        {
                            if ( chr[cnt].ypos > tly - bumpsize && chr[cnt].ypos < bry + bumpsize )
                            {
                                //Yep, flag as valuable (does not export)
                                chr[cnt].isshopitem = btrue;
                                break;
                            }
                        }

                        passage++;
                    }
                }
            }

            chr[cnt].on = btrue;

        }
    }

    return cnt;
}

//--------------------------------------------------------------------------------------------
void respawn_character( Uint16 character )
{
    // ZZ> This function respawns a character
    Uint16 item;

    if ( !chr[character].alive )
    {
        spawn_poof( character, chr[character].model );
        disaffirm_attached_particles( character );
        chr[character].alive = btrue;
        chr[character].boretime = BORETIME;
        chr[character].carefultime = CAREFULTIME;
        chr[character].life = chr[character].lifemax;
        chr[character].mana = chr[character].manamax;
        chr[character].xpos = chr[character].xstt;
        chr[character].ypos = chr[character].ystt;
        chr[character].zpos = chr[character].zstt;
        chr[character].xvel = 0;
        chr[character].yvel = 0;
        chr[character].zvel = 0;
        chr[character].team = chr[character].baseteam;
        chr[character].canbecrushed = bfalse;
        chr[character].turnmaplr = 32768;  // These two mean on level surface
        chr[character].turnmapud = 32768;

        if ( teamleader[chr[character].team] == NOLEADER )  teamleader[chr[character].team] = character;

        if ( !chr[character].invictus )  teammorale[chr[character].baseteam]++;

        chr[character].actionready = btrue;
        chr[character].keepaction = bfalse;
        chr[character].loopaction = bfalse;
        chr[character].action = ACTIONDA;
        chr[character].nextaction = ACTIONDA;
        chr[character].lip = 0;
        chr[character].frame = madframestart[chr[character].model];
        chr[character].lastframe = chr[character].frame;
        chr[character].platform = capplatform[chr[character].model];
        chr[character].flyheight = capflyheight[chr[character].model];
        chr[character].bumpdampen = capbumpdampen[chr[character].model];
        chr[character].bumpsize = capbumpsize[chr[character].model] * chr[character].fat;
        chr[character].bumpsizebig = capbumpsizebig[chr[character].model] * chr[character].fat;
        chr[character].bumpheight = capbumpheight[chr[character].model] * chr[character].fat;

        chr[character].bumpsizesave = capbumpsize[chr[character].model];
        chr[character].bumpsizebigsave = capbumpsizebig[chr[character].model];
        chr[character].bumpheightsave = capbumpheight[chr[character].model];

//        chr[character].alert = ALERTIFSPAWNED;
        chr[character].alert = 0;
//        chr[character].aistate = 0;
        chr[character].aitarget = character;
        chr[character].aitime = 0;
        chr[character].grogtime = 0;
        chr[character].dazetime = 0;
        reaffirm_attached_particles( character );

        // Let worn items come back
        item = chr[character].nextinpack;

        while ( item != MAXCHR )
        {
            if ( chr[item].isequipped )
            {
                chr[item].isequipped = bfalse;
                chr[item].alert |= ALERTIFATLASTWAYPOINT;  // doubles as PutAway
            }

            item = chr[item].nextinpack;
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
    enchant = chr[character].firstenchant;

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
        enchant = encnextenchant[enchant];
    }

    // Change the skin
    sTmp = chr[character].model;

    if ( skin > madskins[sTmp] )  skin = 0;

    chr[character].texture = madskinstart[sTmp] + skin;

    // Change stats associated with skin
    chr[character].defense = capdefense[sTmp][skin];
    iTmp = 0;

    while ( iTmp < DAMAGE_COUNT )
    {
        chr[character].damagemodifier[iTmp] = capdamagemodifier[sTmp][iTmp][skin];
        iTmp++;
    }

    chr[character].maxaccel = capmaxaccel[sTmp][skin];

    // Reset armor enchantments
    // These should really be done in reverse order ( Start with last enchant ), but
    // I don't care at this point !!!BAD!!!
    enchant = chr[character].firstenchant;

    while ( enchant < MAXENCHANT )
    {
        set_enchant_value( enchant, SETSLASHMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETCRUSHMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETPOKEMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETHOLYMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETEVILMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETFIREMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETICEMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETZAPMODIFIER, enceve[enchant] );
        add_enchant_value( enchant, ADDACCEL, enceve[enchant] );
        add_enchant_value( enchant, ADDDEFENSE, enceve[enchant] );
        enchant = encnextenchant[enchant];
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

    if ( madused[profile] )
    {
        // Drop left weapon
        sTmp = chr[ichr].holdingwhich[0];

        if ( sTmp != MAXCHR && ( !capgripvalid[profile][0] || capismount[profile] ) )
        {
            detach_character_from_mount( sTmp, btrue, btrue );

            if ( chr[ichr].ismount )
            {
                chr[sTmp].zvel = DISMOUNTZVEL;
                chr[sTmp].zpos += DISMOUNTZVEL;
                chr[sTmp].jumptime = JUMPDELAY;
            }
        }

        // Drop right weapon
        sTmp = chr[ichr].holdingwhich[1];

        if ( sTmp != MAXCHR && !capgripvalid[profile][1] )
        {
            detach_character_from_mount( sTmp, btrue, btrue );

            if ( chr[ichr].ismount )
            {
                chr[sTmp].zvel = DISMOUNTZVEL;
                chr[sTmp].zpos += DISMOUNTZVEL;
                chr[sTmp].jumptime = JUMPDELAY;
            }
        }

        // Remove particles
        disaffirm_attached_particles( ichr );

        // Remove enchantments
        if ( leavewhich == LEAVEFIRST )
        {
            // Remove all enchantments except top one
            enchant = chr[ichr].firstenchant;

            if ( enchant != MAXENCHANT )
            {
                while ( encnextenchant[enchant] != MAXENCHANT )
                {
                    remove_enchant( encnextenchant[enchant] );
                }
            }
        }

        if ( leavewhich == LEAVENONE )
        {
            // Remove all enchantments
            disenchant_character( ichr );
        }

        // Stuff that must be set
        chr[ichr].model = profile;
        chr[ichr].stoppedby = capstoppedby[profile];
        chr[ichr].lifeheal = caplifeheal[profile];
        chr[ichr].manacost = capmanacost[profile];
        // Ammo
        chr[ichr].ammomax = capammomax[profile];
        chr[ichr].ammo = capammo[profile];

        // Gender
        if ( capgender[profile] != GENRANDOM )  // GENRANDOM means keep old gender
        {
            chr[ichr].gender = capgender[profile];
        }

        // AI stuff
        chr[ichr].aitype = madai[profile];
        chr[ichr].aistate = 0;
        chr[ichr].aitime = 0;
        chr[ichr].latchx = 0;
        chr[ichr].latchy = 0;
        chr[ichr].latchbutton = 0;
        chr[ichr].turnmode = TURNMODEVELOCITY;
        // Flags
        chr[ichr].stickybutt = capstickybutt[profile];
        chr[ichr].openstuff = capcanopenstuff[profile];
        chr[ichr].transferblend = captransferblend[profile];
        chr[ichr].enviro = capenviro[profile];
        chr[ichr].platform = capplatform[profile];
        chr[ichr].isitem = capisitem[profile];
        chr[ichr].invictus = capinvictus[profile];
        chr[ichr].ismount = capismount[profile];
        chr[ichr].cangrabmoney = capcangrabmoney[profile];
        chr[ichr].jumptime = JUMPDELAY;
        // Character size and bumping
        chr[ichr].shadowsize = (Uint8)(capshadowsize[profile] * chr[ichr].fat);
        chr[ichr].bumpsize = (Uint8) (capbumpsize[profile] * chr[ichr].fat);
        chr[ichr].bumpsizebig = capbumpsizebig[profile] * chr[ichr].fat;
        chr[ichr].bumpheight = capbumpheight[profile] * chr[ichr].fat;

        chr[ichr].shadowsizesave = capshadowsize[profile];
        chr[ichr].bumpsizesave = capbumpsize[profile];
        chr[ichr].bumpsizebigsave = capbumpsizebig[profile];
        chr[ichr].bumpheightsave = capbumpheight[profile];

        chr[ichr].bumpdampen = capbumpdampen[profile];
        chr[ichr].weight = capweight[profile] * chr[ichr].fat;

        if ( capweight[profile] == 255 ) chr[ichr].weight = 65535;

        // Character scales...  Magic numbers
        if ( chr[ichr].attachedto == MAXCHR )
        {
            chr[ichr].scale = chr[ichr].fat * madscale[profile] * 4;
        }
        else
        {
            int i;
            Uint16 iholder = chr[ichr].attachedto;
            chr[ichr].scale = chr[ichr].fat / ( chr[iholder].fat * 1280 );
            tnc = madvertices[chr[iholder].model] - chr[ichr].inwhichhand;

            for (i = 0; i < 4; i++)
            {
                if (tnc + i < madvertices[chr[iholder].model] )
                {
                    chr[ichr].weapongrip[i] = tnc + i;
                }
                else
                {
                    chr[ichr].weapongrip[i] = 0xFFFF;
                }
            }
        }

        item = chr[ichr].holdingwhich[0];

        if ( item != MAXCHR )
        {
            int i;

            chr[item].scale = chr[item].fat / ( chr[ichr].fat * 1280 );
            tnc = madvertices[chr[ichr].model] - GRIPLEFT;

            for (i = 0; i < 4; i++)
            {
                if (tnc + i < madvertices[chr[ichr].model] )
                {
                    chr[item].weapongrip[i] = i + tnc;
                }
                else
                {
                    chr[item].weapongrip[i] = 0xFFFF;
                }
            }
        }

        item = chr[ichr].holdingwhich[1];

        if ( item != MAXCHR )
        {
            int i;

            chr[item].scale = chr[item].fat / ( chr[ichr].fat * 1280 );
            tnc = madvertices[chr[ichr].model] - GRIPRIGHT;

            for (i = 0; i < 4; i++)
            {
                if (tnc + i < madvertices[chr[ichr].model] )
                {
                    chr[item].weapongrip[i] = i + tnc;
                }
                else
                {
                    chr[item].weapongrip[i] = 0xFFFF;
                }
            }
        }

        // Image rendering
        chr[ichr].uoffset = 0;
        chr[ichr].voffset = 0;
        chr[ichr].uoffvel = capuoffvel[profile];
        chr[ichr].voffvel = capvoffvel[profile];

        // Movement
        chr[ichr].sneakspd = capsneakspd[profile];
        chr[ichr].walkspd = capwalkspd[profile];
        chr[ichr].runspd = caprunspd[profile];

        // AI and action stuff
        chr[ichr].actionready = btrue;
        chr[ichr].keepaction = bfalse;
        chr[ichr].loopaction = bfalse;
        chr[ichr].action = ACTIONDA;
        chr[ichr].nextaction = ACTIONDA;
        chr[ichr].lip = 0;
        chr[ichr].frame = madframestart[profile];
        chr[ichr].lastframe = chr[ichr].frame;
        chr[ichr].holdingweight = 0;

        // Set the skin
        change_armor( ichr, skin );

        // Reaffirm them particles...
        chr[ichr].reaffirmdamagetype = capattachedprtreaffirmdamagetype[profile];
        reaffirm_attached_particles( ichr );

        // Set up initial fade in lighting

        for ( tnc = 0; tnc < madtransvertices[chr[ichr].model]; tnc++ )
        {
            chr[ichr].vrta[tnc] = 0;
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

  if ( x >= 0 && x < ( meshsizex >> 2 ) && y >= 0 && y < ( meshsizey >> 2 ) )
  {
    team = chr[character].team;
    fanblock = x + meshblockstart[y];
    charb = meshbumplistchr[fanblock];
    cnt = 0;
    while ( cnt < meshbumplistchrnum[fanblock] )
    {
      if ( dead != chr[charb].alive && ( seeinvisible || ( chr[charb].alpha > INVISIBLE && chr[charb].light > INVISIBLE ) ) )
      {
        if ( ( enemies && teamhatesteam[team][chr[charb].team] && !chr[charb].invictus ) ||
             ( items && chr[charb].isitem ) ||
             ( friends && chr[charb].baseteam == team ) )
        {
          if ( charb != character && chr[character].attachedto != charb )
          {
            if ( !chr[charb].isitem || items )
            {
              if ( idsz != IDSZ_NONE )
              {
                if ( capidsz[chr[charb].model][IDSZ_PARENT] == idsz ||
                     capidsz[chr[charb].model][IDSZ_TYPE] == idsz )
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
      charb = chr[charb].bumpnext;
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
  seeinvisible = chr[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )chr[character].xpos ) >> 9;
  y = ( ( int )chr[character].ypos ) >> 9;
  return get_target_in_block( x, y, character, items, friends, enemies, dead, seeinvisible, idsz, 0 );
}*/

//--------------------------------------------------------------------------------------------
Uint8 cost_mana( Uint16 character, int amount, Uint16 killer )
{
    // ZZ> This function takes mana from a character ( or gives mana ),
    //     and returns btrue if the character had enough to pay, or bfalse
    //     otherwise
    int iTmp;

    iTmp = chr[character].mana - amount;

    if ( iTmp < 0 )
    {
        chr[character].mana = 0;

        if ( chr[character].canchannel )
        {
            chr[character].life += iTmp;

            if ( chr[character].life <= 0 )
            {
                kill_character( character, (killer == MAXCHR) ? character : killer );
            }

            return btrue;
        }

        return bfalse;
    }
    else
    {
        chr[character].mana = iTmp;

        if ( iTmp > chr[character].manamax )
        {
            chr[character].mana = chr[character].manamax;
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

  team = chr[character].team;
  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( chr[cnt].on )
    {
      if ( chr[cnt].attachedto == MAXCHR && !chr[cnt].inpack )
      {
        if ( teamhatesteam[team][chr[cnt].team] && chr[cnt].alive && !chr[cnt].invictus )
        {
          if ( chr[character].canseeinvisible || ( chr[cnt].alpha > INVISIBLE && chr[cnt].light > INVISIBLE ) )
          {
            xdistance = (int) (chr[cnt].xpos - chr[character].xpos);
            ydistance = (int) (chr[cnt].ypos - chr[character].ypos);
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
void switch_team( int character, Uint8 team )
{
    // ZZ> This function makes a character join another team...
    if ( team < MAXTEAM )
    {
        if ( !chr[character].invictus )
        {
            teammorale[chr[character].baseteam]--;
            teammorale[team]++;
        }

        if ( ( !chr[character].ismount || chr[character].holdingwhich[0] == MAXCHR ) &&
                ( !chr[character].isitem || chr[character].attachedto == MAXCHR ) )
        {
            chr[character].team = team;
        }

        chr[character].baseteam = team;

        if ( teamleader[team] == NOLEADER )
        {
            teamleader[team] = character;
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

  if ( x >= 0 && x < ( meshsizex >> 2 ) && y >= 0 && y < ( meshsizey >> 2 ) )
  {
    team = chr[character].team;
    fanblock = x + meshblockstart[y];
    charb = meshbumplistchr[fanblock];
    cnt = 0;
    while ( cnt < meshbumplistchrnum[fanblock] )
    {
      if ( dead != chr[charb].alive && ( seeinvisible || ( chr[charb].alpha > INVISIBLE && chr[charb].light > INVISIBLE ) ) )
      {
        if ( ( enemies && teamhatesteam[team][chr[charb].team] ) ||
             ( items && chr[charb].isitem ) ||
             ( friends && chr[charb].team == team ) ||
             ( friends && enemies ) )
        {
          if ( charb != character && chr[character].attachedto != charb && chr[charb].attachedto == MAXCHR && !chr[charb].inpack )
          {
            if ( !chr[charb].invictus || items )
            {
              if ( idsz != IDSZ_NONE )
              {
                if ( capidsz[chr[charb].model][IDSZ_PARENT] == idsz ||
                     capidsz[chr[charb].model][IDSZ_TYPE] == idsz )
                {
                  xdis = chr[character].xpos - chr[charb].xpos;
                  ydis = chr[character].ypos - chr[charb].ypos;
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
                xdis = chr[character].xpos - chr[charb].xpos;
                ydis = chr[character].ypos - chr[charb].ypos;
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
      charb = chr[charb].bumpnext;
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
  seeinvisible = chr[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )chr[character].xpos ) >> 9;
  y = ( ( int )chr[character].ypos ) >> 9;

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
  seeinvisible = chr[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )chr[character].xpos ) >> 9;
  y = ( ( int )chr[character].ypos ) >> 9;
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

    team = chr[character].team;
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].team == team && !chr[cnt].alive )
        {
            chr[cnt].aitime = 2;  // Don't let it think too much...
            chr[cnt].alert = ALERTIFCLEANEDUP;
        }

        cnt++;
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
        if ( chr[character].on )
        {
            model = chr[character].model;

            if ( capidsz[model][IDSZ_PARENT] == idsz || capidsz[model][IDSZ_TYPE] == idsz )
            {
                if ( chr[character].ammo < chr[character].ammomax )
                {
                    amount = chr[character].ammomax - chr[character].ammo;
                    chr[character].ammo = chr[character].ammomax;
                }
            }
        }
    }

    return amount;
}

//--------------------------------------------------------------------------------------------
void issue_order( Uint16 character, Uint32 order )
{
    // ZZ> This function issues an order for help to all teammates
    Uint8 team;
    Uint16 counter;
    Uint16 cnt;

    team = chr[character].team;
    counter = 0;
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].team == team )
        {
            chr[cnt].order = order;
            chr[cnt].counter = counter;
            chr[cnt].alert = chr[cnt].alert | ALERTIFORDERED;
            counter++;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void issue_special_order( Uint32 order, IDSZ idsz )
{
    // ZZ> This function issues an order to all characters with the a matching special IDSZ
    Uint16 counter;
    Uint16 cnt;

    counter = 0;
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].on )
        {
            if ( capidsz[chr[cnt].model][IDSZ_SPECIAL] == idsz )
            {
                chr[cnt].order = order;
                chr[cnt].counter = counter;
                chr[cnt].alert = chr[cnt].alert | ALERTIFORDERED;
                counter++;
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
Uint16 get_target( Uint16 character, Uint32 maxdistance, TARGET_TYPE team, bool_t targetitems, bool_t targetdead, IDSZ idsz, bool_t excludeidsz )
{
    //ZF> This is the new improved AI targeting system. Also includes distance in the Z direction.
    //If maxdistance is 0 then it searches without a max limit.
    Uint16 besttarget = MAXCHR, cnt;
    Uint32 longdist = pow(2, 31);

    if (team == NONE) return MAXCHR;

    for (cnt = 0; cnt < MAXCHR; cnt++)
    {
        if (chr[cnt].on && cnt != character && (targetdead || chr[cnt].alive) && (targetitems || (!chr[cnt].isitem && !chr[cnt].invictus)) && chr[cnt].attachedto == MAXCHR
                && (team != ENEMY || chr[character].canseeinvisible || ( chr[cnt].alpha > INVISIBLE && chr[cnt].light > INVISIBLE ) )
                && (team == ALL || team != teamhatesteam[chr[character].team][chr[cnt].team]) )
        {
            //Check for IDSZ too
            if (idsz == IDSZ_NONE
                    || (excludeidsz != (capidsz[chr[cnt].model][IDSZ_PARENT] == idsz))
                    || (excludeidsz != (capidsz[chr[cnt].model][IDSZ_TYPE]   == idsz)) )
            {
                Uint32 dist = ( Uint32 ) SQRT(ABS( pow(chr[cnt].xpos - chr[character].xpos, 2))
                                              + ABS( pow(chr[cnt].ypos - chr[character].ypos, 2))
                                              + ABS( pow(chr[cnt].zpos - chr[character].zpos, 2)) );

                if (dist < longdist && (maxdistance == 0 || dist < maxdistance) )
                {
                    besttarget = cnt;
                    longdist = dist;
                }
            }
        }
    }

    //Now set the target
    if (besttarget != MAXCHR)
    {
        chr[character].aitarget = besttarget;
    }

    return besttarget;
}

//--------------------------------------------------------------------------------------------
void set_alerts( int character )
{
    // ZZ> This function polls some alert conditions

    if ( !chr[character].on && chr[character].attachedto == MAXCHR ) return;

    if ( chr[character].aitime != 0 )
    {
        chr[character].aitime--;
    }

    if ( chr[character].aigoto != chr[character].aigotoadd )
    {
        if ( chr[character].xpos < chr[character].aigotox[chr[character].aigoto] + WAYTHRESH &&
                chr[character].xpos > chr[character].aigotox[chr[character].aigoto] - WAYTHRESH &&
                chr[character].ypos < chr[character].aigotoy[chr[character].aigoto] + WAYTHRESH &&
                chr[character].ypos > chr[character].aigotoy[chr[character].aigoto] - WAYTHRESH )
        {
            chr[character].alert = chr[character].alert | ALERTIFATWAYPOINT;
            chr[character].aigoto++;

            if ( chr[character].aigoto > MAXWAY - 1 ) chr[character].aigoto = MAXWAY - 1;
        }

        if ( chr[character].aigoto == chr[character].aigotoadd )
        {
            // !!!!restart the waypoint list, do not clear them!!!!
            chr[character].aigoto    = 0;

            // if the object can be alerted to last waypoint, do it
            if ( !capisequipment[chr[character].model] )
            {
                chr[character].alert = chr[character].alert | ALERTIFATLASTWAYPOINT;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t add_quest_idsz( char *whichplayer, IDSZ idsz )
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
Sint16 modify_quest_idsz( char *whichplayer, IDSZ idsz, Sint16 adjustment )
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
                newidsz = get_idsz( fileread );
                get_first_letter( fileread );      //Skip the ] bracket
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
Sint16 check_player_quest( char *whichplayer, IDSZ idsz )
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
        newidsz = get_idsz( fileread );

        if ( newidsz == idsz )
        {
            foundidsz = btrue;
            get_first_letter( fileread );   //Skip the ] bracket
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
    if ( capidsz[chr[who].model][IDSZ_SKILL]  == whichskill ) result = btrue;
    else if ( Make_IDSZ( "AWEP" ) == whichskill ) result = chr[who].canuseadvancedweapons;
    else if ( Make_IDSZ( "CKUR" ) == whichskill ) result = chr[who].canseekurse;
    else if ( Make_IDSZ( "JOUS" ) == whichskill ) result = chr[who].canjoust;
    else if ( Make_IDSZ( "SHPR" ) == whichskill ) result = chr[who].shieldproficiency;
    else if ( Make_IDSZ( "TECH" ) == whichskill ) result = chr[who].canusetech;
    else if ( Make_IDSZ( "WMAG" ) == whichskill ) result = chr[who].canusearcane;
    else if ( Make_IDSZ( "HMAG" ) == whichskill ) result = chr[who].canusedivine;
    else if ( Make_IDSZ( "DISA" ) == whichskill ) result = chr[who].candisarm;
    else if ( Make_IDSZ( "STAB" ) == whichskill ) result = chr[who].canbackstab;
    else if ( Make_IDSZ( "POIS" ) == whichskill ) result = chr[who].canusepoison;
    else if ( Make_IDSZ( "READ" ) == whichskill ) result = chr[who].canread;

    return result;
}
