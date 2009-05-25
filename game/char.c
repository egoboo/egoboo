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
#include "graphic.h"
#include "mad.h"

#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo_math.h"
#include "egoboo.h"

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//These are shop orders

#define BUY			0
#define SELL		1
#define NOAFFORD	2
#define THEFT		3


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
chop_data_t chop = {0, 0};

static int            numfreechr = 0;             // For allocation
static Uint16         freechrlist[MAX_CHR];

team_t TeamList[MAXTEAM];

int   importobject;
cap_t CapList[MAX_PROFILE];

chr_t ChrList[MAX_CHR];

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

    frame = ChrList[character].inst.frame;

    for ( cnt = 0; cnt < MadList[ChrList[character].inst.imad].transvertices; cnt++  )
    {
        z = Md2FrameList[frame].vrtz[cnt];
        if ( z < low )
        {
            ChrList[character].inst.vrta[cnt] = valuelow;
        }
        else
        {
            if ( z > high )
            {
                ChrList[character].inst.vrta[cnt] = valuehigh;
            }
            else
            {
                ChrList[character].inst.vrta[cnt] = ( valuehigh * ( z - low ) / ( high - low ) ) +
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
   
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ChrList[cnt].on ) continue;

        character = ChrList[cnt].attachedto;
        if ( INVALID_CHR(character) )
        {
            ChrList[cnt].attachedto = MAX_CHR;

            // Keep inventory with character
            if ( !ChrList[cnt].inpack )
            {
                character = ChrList[cnt].nextinpack;

                while ( character != MAX_CHR )
                {
                    ChrList[character].pos.x = ChrList[cnt].pos.x;
                    ChrList[character].pos.y = ChrList[cnt].pos.y;
                    ChrList[character].pos.z = ChrList[cnt].pos.z;

                    // Copy olds to make SendMessageNear work
                    ChrList[character].pos_old.x = ChrList[cnt].pos_old.x;
                    ChrList[character].pos_old.y = ChrList[cnt].pos_old.y;
                    ChrList[character].pos_old.z = ChrList[cnt].pos_old.z;

                    character = ChrList[character].nextinpack;
                }
            }
        }
        else
        {
            // Keep in hand weapons with character
            if ( ChrList[cnt].inst.matrixvalid )
            {
                ChrList[cnt].pos.x = ChrList[cnt].inst.matrix.CNV( 3, 0 );
                ChrList[cnt].pos.y = ChrList[cnt].inst.matrix.CNV( 3, 1 );
                ChrList[cnt].pos.z = ChrList[cnt].inst.matrix.CNV( 3, 2 );
            }
            else
            {
                ChrList[cnt].pos.x = ChrList[character].pos.x;
                ChrList[cnt].pos.y = ChrList[character].pos.y;
                ChrList[cnt].pos.z = ChrList[character].pos.z;
            }

            ChrList[cnt].turnleftright = ChrList[character].turnleftright;

            // Copy this stuff ONLY if it's a weapon, not for mounts
            if ( ChrList[character].transferblend && ChrList[cnt].isitem )
            {

                // Items become partially invisible in hands of players
                if ( ChrList[character].isplayer && ChrList[character].inst.alpha != 255 )
                {
                    ChrList[cnt].inst.alpha = 128;
                }
                else
                {
                    // Only if not naturally transparent
                    if ( CapList[ChrList[cnt].model].alpha == 255 )
                    {
                        ChrList[cnt].inst.alpha = ChrList[character].inst.alpha;
                    }
                    else 
                    {
                        ChrList[cnt].inst.alpha = CapList[ChrList[cnt].model].alpha;
                    }
                }

                //Do light too
                if ( ChrList[character].isplayer && ChrList[character].inst.light != 255 )
                {
                    ChrList[cnt].inst.light = 128;
                }
                else
                {
                    // Only if not naturally transparent
                    if ( CapList[ChrList[cnt].model].light == 255 )
                    {
                        ChrList[cnt].inst.light = ChrList[character].inst.light;
                    }
                    else 
                    {
                        ChrList[cnt].inst.light = CapList[ChrList[cnt].model].light;
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix( Uint16 cnt )
{
    // ZZ> This function sets one character's matrix
    Uint16 tnc;

    if( INVALID_CHR( cnt ) ) return;

    ChrList[cnt].inst.matrixvalid = bfalse;

    if ( ChrList[cnt].overlay )
    {
        // Overlays are kept with their target...
        tnc = ChrList[cnt].ai.target;

        if( VALID_CHR(tnc) )
        {
            ChrList[cnt].pos.x = ChrList[tnc].pos.x;
            ChrList[cnt].pos.y = ChrList[tnc].pos.y;
            ChrList[cnt].pos.z = ChrList[tnc].pos.z;

            ChrList[cnt].inst.matrixvalid = ChrList[tnc].inst.matrixvalid;
            CopyMatrix( &(ChrList[cnt].inst.matrix), &(ChrList[tnc].inst.matrix) );
        }
    }
    else
    {
        ChrList[cnt].inst.matrix = ScaleXYZRotateXYZTranslate( ChrList[cnt].fat, ChrList[cnt].fat, ChrList[cnt].fat,
                                   ChrList[cnt].turnleftright >> 2,
                                   ( ( Uint16 ) ( ChrList[cnt].turnmapud + 32768 ) ) >> 2,
                                   ( ( Uint16 ) ( ChrList[cnt].turnmaplr + 32768 ) ) >> 2,
                                   ChrList[cnt].pos.x, ChrList[cnt].pos.y, ChrList[cnt].pos.z );
        ChrList[cnt].inst.matrixvalid = btrue;
    }
}

//--------------------------------------------------------------------------------------------
void free_one_character( Uint16 character )
{
    if( VALID_CHR_RANGE( character ) )
    {
        // the character "destructor"
        // sets all boolean values to false, incluting the "on" flag
        memset( ChrList + character, 0, sizeof(chr_t) );

        ChrList[character].nextinpack = MAX_CHR;

        // push it on the stack
        freechrlist[numfreechr] = character;
        numfreechr++;
    }
}

//--------------------------------------------------------------------------------------------
void free_one_character_in_game( Uint16 character )
{
    // ZZ> This function sticks a character back on the free character stack
    int cnt;

    if ( VALID_CHR( character ) )
    {
        // Remove from stat list
        if ( ChrList[character].staton )
        {
            bool_t stat_found;

            ChrList[character].staton = bfalse;
            
            stat_found = bfalse;
            for ( cnt = 0; cnt < numstat; cnt++ )
            {
                if ( statlist[cnt] == character ) 
                {
                    stat_found = btrue;
                    break;
                }
            }

            if( stat_found )
            {
                for ( cnt++; cnt < numstat; cnt++ )
                {
                    statlist[cnt-1] = statlist[cnt];
                }
                numstat--;
            }
        }

        // Make sure everyone knows it died
        for ( cnt = 0; cnt < MAX_CHR; cnt++ )
        {
            if ( !ChrList[cnt].on || cnt == character ) continue;

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


        // Handle the team
        if ( ChrList[character].alive && !CapList[ChrList[character].model].invictus )
        {
            TeamList[ChrList[character].baseteam].morale--;
        }

        if ( TeamList[ChrList[character].team].leader == character )
        {
            TeamList[ChrList[character].team].leader = NOLEADER;
        }
    }

    // actually get rid of the character
    free_one_character( character );
}

//--------------------------------------------------------------------------------------------
void free_inventory( Uint16 character )
{
    // ZZ> This function frees every item in the character's inventory
    int cnt, next;

    cnt = ChrList[character].nextinpack;
    while ( cnt < MAX_CHR )
    {
        next = ChrList[cnt].nextinpack;
        free_one_character_in_game( cnt );
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
    GLvector4 point[1], nupoint[1];

    // Check validity of attachment
    if ( !ChrList[character].on || ChrList[character].inpack )
    {
        PrtList[particle].time = 1;
        return;
    }

    // Do we have a matrix???
    if ( ChrList[character].inst.matrixvalid )// PMesh->mem.inrenderlist[ChrList[character].onwhichfan])
    {
        // Transform the weapon grip from model to world space
        model = ChrList[character].model;
        frame = ChrList[character].inst.frame;
        lastframe = ChrList[character].inst.lastframe;
        lip = ChrList[character].inst.lip >> 6;
        flip = lip / 4.0f;

        if ( grip == GRIP_ORIGIN )
        {
            PrtList[particle].pos.x = ChrList[character].inst.matrix.CNV( 3, 0 );
            PrtList[particle].pos.y = ChrList[character].inst.matrix.CNV( 3, 1 );
            PrtList[particle].pos.z = ChrList[character].inst.matrix.CNV( 3, 2 );
            return;
        }

        vertex = MadList[model].md2.vertices - grip;

        // Calculate grip point locations with linear interpolation and other silly things
        point[0].x = Md2FrameList[lastframe].vrtx[vertex] + (Md2FrameList[frame].vrtx[vertex] - Md2FrameList[lastframe].vrtx[vertex]) * flip;
        point[0].y = Md2FrameList[lastframe].vrty[vertex] + (Md2FrameList[frame].vrty[vertex] - Md2FrameList[lastframe].vrty[vertex]) * flip;
        point[0].z = Md2FrameList[lastframe].vrtz[vertex] + (Md2FrameList[frame].vrtz[vertex] - Md2FrameList[lastframe].vrtz[vertex]) * flip;
        point[0].w = 1.0f;

        // Do the transform
        TransformVertices( &(ChrList[character].inst.matrix), point, nupoint, 1 );

        PrtList[particle].pos.x = nupoint[0].x;
        PrtList[particle].pos.y = nupoint[0].y;
        PrtList[particle].pos.z = nupoint[0].z;
    }
    else
    {
        // No matrix, so just wing it...
        PrtList[particle].pos.x = ChrList[character].pos.x;
        PrtList[particle].pos.y = ChrList[character].pos.y;
        PrtList[particle].pos.z = ChrList[character].pos.z;
    }
}

//--------------------------------------------------------------------------------------------
void make_one_weapon_matrix( Uint16 iweap, Uint16 iholder, bool_t do_physics )
{
    // ZZ> This function sets one weapon's matrix, based on who it's attached to
    int    cnt, vertex;
    Uint16 iholder_model, iholder_frame, iholder_lastframe;
    Uint8  iholder_lip;
    float  iholder_flip;
    GLvector4  point[GRIP_VERTS], nupoint[GRIP_VERTS], ptemp;
    int    iweap_points;

    chr_t * pweap, * pholder;

    // turn this off for now
    do_physics = bfalse;

    if ( INVALID_CHR(iweap) || INVALID_CHR(iholder) ) return;
    pweap = ChrList + iweap;
    pholder = ChrList + iholder;

    // make sure that the matrix is invalid incase of an error
    pweap->inst.matrixvalid = bfalse;

    // Transform the weapon grip from model space to world space
    iholder_model     = pholder->model;
    iholder_frame     = pholder->inst.frame;
    iholder_lastframe = pholder->inst.lastframe;
    iholder_lip       = pholder->inst.lip >> 6;
    iholder_flip      = iholder_lip / 4.0f;

    // count the valid weapon connection points
    iweap_points = 0;
    for (cnt = 0; cnt < GRIP_VERTS; cnt++)
    {
        if (0xFFFF != pweap->weapongrip[cnt])
        {
            iweap_points++;
        }
    }

    // do the best we can
    if (0 == iweap_points)
    {
        // punt! attach to origin
        point[0].x = ChrList[0].pos.x;
        point[0].y = ChrList[0].pos.y;
        point[0].z = ChrList[0].pos.z;
        point[0].w = 1;

        iweap_points = 1;
    }
    else
    {
        // Calculate grip point locations with linear interpolation and other silly things
        for (cnt = 0; cnt < GRIP_VERTS; cnt++ )
        {
            vertex = pweap->weapongrip[cnt];
            if (0xFFFF == vertex) continue;

            // Calculate grip point locations with linear interpolation and other silly things
            point[cnt].x = Md2FrameList[iholder_lastframe].vrtx[vertex] + (Md2FrameList[iholder_frame].vrtx[vertex] - Md2FrameList[iholder_lastframe].vrtx[vertex]) * iholder_flip;
            point[cnt].y = Md2FrameList[iholder_lastframe].vrty[vertex] + (Md2FrameList[iholder_frame].vrty[vertex] - Md2FrameList[iholder_lastframe].vrty[vertex]) * iholder_flip;
            point[cnt].z = Md2FrameList[iholder_lastframe].vrtz[vertex] + (Md2FrameList[iholder_frame].vrtz[vertex] - Md2FrameList[iholder_lastframe].vrtz[vertex]) * iholder_flip;
            point[cnt].w = 1;
        }
    }

    // use the math function instead of rolling out own
    TransformVertices( &(pholder->inst.matrix), point, nupoint, iweap_points );

    if (1 == iweap_points)
    {
        // attach to single point
        pweap->inst.matrix = ScaleXYZRotateXYZTranslate(pweap->fat, pweap->fat, pweap->fat,
                                     pweap->turnleftright >> 2,
                                     ( ( Uint16 ) ( pweap->turnmapud + 32768 ) ) >> 2,
                                     ( ( Uint16 ) ( pweap->turnmaplr + 32768 ) ) >> 2,
                                     nupoint[0].x, nupoint[0].y, nupoint[0].z);

        pweap->inst.matrixvalid = btrue;
    }
    else if (4 == iweap_points)
    {
        // Calculate weapon's matrix based on positions of grip points
        // chrscale is recomputed at time of attachment
        pweap->inst.matrix = FourPoints( 
            nupoint[0].x, nupoint[0].y, nupoint[0].z,
            nupoint[1].x, nupoint[1].y, nupoint[1].z,
            nupoint[2].x, nupoint[2].y, nupoint[2].z,
            nupoint[3].x, nupoint[3].y, nupoint[3].z, pweap->fat );

        pweap->inst.matrixvalid = btrue;
    }

    ptemp.x = pweap->pos.x;
    ptemp.y = pweap->pos.y;
    ptemp.z = pweap->pos.z;

    // update the position of the object
    pweap->pos.x = nupoint[0].x;
    pweap->pos.y = nupoint[0].y;
    pweap->pos.z = nupoint[0].z;

    if( do_physics )
    {
        float dx, dy, dz;
        float wt_weap, wt_holder, damp = 0.5f;
        GLvector3 vcom;

        // calculate the "tweety bird swinging a sledgehammer" effect

        dx = ptemp.x - nupoint[0].x;
        dy = ptemp.y - nupoint[0].y;
        dz = ptemp.z - nupoint[0].z;

        wt_weap   = 0xFFFFFFFF == pweap->weight ? -(float)0xFFFFFFFF : pweap->weight;
        wt_holder = 0xFFFFFFFF == pholder->weight ? -(float)0xFFFFFFFF : pholder->weight;

        if ( wt_weap == 0 && wt_holder == 0 )
        {
            wt_weap = wt_holder = 1;
        }
        else if ( wt_weap == 0 )
        {
            wt_weap = 1;
            wt_holder = -0xFFFF;
        }
        else if ( wt_holder == 0 )
        {
            wt_holder = 1;
            wt_weap = -0xFFFF;
        }

        if ( 0.0f == pweap->bumpdampen && 0.0f == pholder->bumpdampen )
        {
            /* do nothing */
        }
        else if ( 0.0f == pweap->bumpdampen )
        {
            // make the weight infinite
            wt_weap = -0xFFFF;
        }
        else if ( 0.0f == pholder->bumpdampen )
        {
            // make the weight infinite
            wt_holder = -0xFFFF;
        }
        else
        {
            // adjust the weights to respect bumpdampen
            wt_weap /= pweap->bumpdampen;
            wt_holder /= pholder->bumpdampen;
        }

        // this "velocity matching with damping" makes the mounts really sluggish
        // figure out a better way!

        // calculate the center-of-mass velocity
        //vcom.x = (ABS(wt_weap) * dx + ABS(wt_holder) * pholder->vel.x) / ( ABS(wt_weap) + ABS(wt_holder) );
        //vcom.y = (ABS(wt_weap) * dy + ABS(wt_holder) * pholder->vel.y) / ( ABS(wt_weap) + ABS(wt_holder) );
        //vcom.z = (ABS(wt_weap) * dz + ABS(wt_holder) * pholder->vel.z) / ( ABS(wt_weap) + ABS(wt_holder) );

        if ( wt_weap >= 0.0f )
        {
            // the object has already been moved the full distance
            // move it back some

            float ratio = 1.0f - (float)ABS(wt_holder) / ((float)ABS(wt_weap) + (float)ABS(wt_holder));

            pweap->phys_pos_x -= dx * ratio;
            pweap->phys_pos_y -= dy * ratio;
            pweap->phys_pos_z -= dz * ratio;

            //pweap->phys_vel_x += (dx-vcom.x)*damp + vcom.x - pweap->vel.x;
            //pweap->phys_vel_y += (dy-vcom.y)*damp + vcom.y - pweap->vel.y;
            //pweap->phys_vel_z += (dz-vcom.z)*damp + vcom.z - pweap->vel.z;
        }

        if ( wt_holder >= 0.0f )
        {
            float ratio = (float)ABS(wt_weap) / ((float)ABS(wt_weap) + (float)ABS(wt_holder));

            pholder->phys_pos_x -= dx * ratio;
            pholder->phys_pos_y -= dy * ratio;
            pholder->phys_pos_z -= dz * ratio;

            //pholder->phys_vel_x += (pholder->vel.x-vcom.x)*damp + vcom.x - pholder->vel.x;
            //pholder->phys_vel_y += (pholder->vel.y-vcom.y)*damp + vcom.y - pholder->vel.y;
            //pholder->phys_vel_z += (pholder->vel.z-vcom.z)*damp + vcom.z - pholder->vel.z;
        }

    }

}

//--------------------------------------------------------------------------------------------
void make_character_matrices(bool_t do_physics)
{
    // ZZ> This function makes all of the character's matrices
    int cnt, ichr;
    bool_t done;

    // Forget about old matrices
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        ChrList[ichr].inst.matrixvalid = bfalse;
    } 

    // blank the accumulators
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        ChrList[ichr].phys_pos_x = 0.0f;
        ChrList[ichr].phys_pos_y = 0.0f;
        ChrList[ichr].phys_pos_z = 0.0f;
        ChrList[ichr].phys_vel_x = 0.0f;
        ChrList[ichr].phys_vel_y = 0.0f;
        ChrList[ichr].phys_vel_z = 0.0f;
    }

    // Do base characters
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        if( !ChrList[ichr].on ) continue;

        if ( INVALID_CHR( ChrList[ichr].attachedto ) )
        {
            make_one_character_matrix( ichr );
        }
    }

    // do all levels of attachment
    done = bfalse;
    while( !done )
    {
        cnt = 0;
        for ( ichr = 0; ichr < MAX_CHR; ichr++ )
        {
            Uint16 imount;

            if( !ChrList[ichr].on ) continue;
            if( ChrList[cnt].inst.matrixvalid ) continue;

            imount = ChrList[ichr].attachedto;
            if( INVALID_CHR(imount) ) { ChrList[ichr].attachedto = MAX_CHR; continue; }
            if( imount == ichr ) { imount = MAX_CHR; continue; }

            // can't evaluate this link yet
            if( !ChrList[imount].inst.matrixvalid )
            {
                cnt++;
            }
            else
            {
                make_one_weapon_matrix( ichr, imount, do_physics );
            }
        }

        done = (0 == cnt);
    }


    if( do_physics )
    {
        // accumulate the accumulators
        for ( ichr = 0; ichr < MAX_CHR; ichr++ )
        {
            float tmpx, tmpy, tmpz;

            if( !ChrList[ichr].on ) continue;

            // do the "integration" of the accumulated accelerations
            ChrList[ichr].vel.x += ChrList[ichr].phys_vel_x;
            ChrList[ichr].vel.y += ChrList[ichr].phys_vel_y;
            ChrList[ichr].vel.z += ChrList[ichr].phys_vel_z;

            // do the "integration" on the position
            if ( ABS(ChrList[ichr].phys_pos_x) > 0 )
            {
                tmpx = ChrList[ichr].pos.x;
                ChrList[ichr].pos.x += ChrList[ichr].phys_pos_x;
                if ( __chrhitawall(ichr) )
                {
                    // restore the old values
                    ChrList[ichr].pos.x = tmpx;
                }
                else
                {
                    //ChrList[ichr].vel.x += ChrList[ichr].phys_pos_x;
                    ChrList[ichr].pos_old.x = tmpx;
                }
            }

            if ( ABS(ChrList[ichr].phys_pos_y) > 0 )
            {
                tmpy = ChrList[ichr].pos.y;
                ChrList[ichr].pos.y += ChrList[ichr].phys_pos_y;
                if ( __chrhitawall(ichr) )
                {
                    // restore the old values
                    ChrList[ichr].pos.y = tmpy;
                }
                else
                {
                    //ChrList[ichr].vel.y += ChrList[ichr].phys_pos_y;
                    ChrList[ichr].pos_old.y = tmpy;
                }
            }

            if ( ABS(ChrList[ichr].phys_pos_z) > 0 )
            {
                tmpz = ChrList[ichr].pos.z;
                ChrList[ichr].pos.z += ChrList[ichr].phys_pos_z;
                if ( ChrList[ichr].pos.z < ChrList[ichr].phys_level )
                {
                    // restore the old values
                    ChrList[ichr].pos.z = tmpz;
                }
                else
                {
                    //ChrList[ichr].vel.z += ChrList[ichr].phys_pos_z;
                    ChrList[ichr].pos_old.z = tmpz;
                }
            }
        }

        // fix the matrix positions
        for ( ichr = 0; ichr < MAX_CHR; ichr++ )
        {
            if( !ChrList[ichr].inst.matrixvalid ) continue;

            ChrList[ichr].inst.matrix.CNV( 3, 0 ) = ChrList[ichr].pos.x;
            ChrList[ichr].inst.matrix.CNV( 3, 1 ) = ChrList[ichr].pos.y;
            ChrList[ichr].inst.matrix.CNV( 3, 2 ) = ChrList[ichr].pos.z;
        } 
    }
}

//--------------------------------------------------------------------------------------------
int get_free_character()
{
    // ZZ> This function gets an unused character and returns its index
    int character;
    if ( numfreechr == 0 )
    {
        // Return MAX_CHR if we can't find one
        return MAX_CHR;
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

    int cnt;

    numfreechr = 0;
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        free_one_character( cnt );
    }

    // free_all_players
    numpla = 0;
    local_numlpla = 0;
    local_noplayers = btrue;

    // free_all_stats
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

    y = ChrList[character].pos.y;  x = ChrList[character].pos.x;  bs = ChrList[character].bumpsize >> 1;

    fx = x - bs; fy = y - bs;
    passtl = MESHFX_IMPASS;
    itile  = mesh_get_tile( PMesh, fx, fy );
    if ( INVALID_TILE != itile )
    {
        passtl = PMesh->mem.tile_list[itile].fx;
    }

    fx = x + bs; fy = y - bs;
    passtr = MESHFX_IMPASS;
    itile  = mesh_get_tile( PMesh, fx, fy );
    if ( INVALID_TILE != itile )
    {
        passtr = PMesh->mem.tile_list[itile].fx;
    }

    fx = x - bs; fy = y + bs;
    passbl = MESHFX_IMPASS;
    itile  = mesh_get_tile( PMesh, fx, fy );
    if ( INVALID_TILE != itile )
    {
        passbl = PMesh->mem.tile_list[itile].fx;
    }

    fx = x + bs; fy = y + bs;
    passbr = MESHFX_IMPASS;
    itile  = mesh_get_tile( PMesh, fx, fy );
    if ( INVALID_TILE != itile )
    {
        passbr = PMesh->mem.tile_list[itile].fx;
    }

    return ( passtl | passtr | passbr | passbl ) & ChrList[character].stoppedby;
}

//--------------------------------------------------------------------------------------------
void reset_character_accel( Uint16 character )
{
    // ZZ> This function fixes a character's max acceleration
    Uint16 enchant;
    if ( character != MAX_CHR )
    {
        if ( ChrList[character].on )
        {
            // Okay, remove all acceleration enchants
            enchant = ChrList[character].firstenchant;

            while ( enchant < MAX_ENC )
            {
                remove_enchant_value( enchant, ADDACCEL );
                enchant = EncList[enchant].nextenchant;
            }

            // Set the starting value
            ChrList[character].maxaccel = CapList[ChrList[character].model].maxaccel[ChrList[character].skin];
            // Put the acceleration enchants back on
            enchant = ChrList[character].firstenchant;

            while ( enchant < MAX_ENC )
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
    if ( character == MAX_CHR )
        return;

    // Make sure the character is mounted
    mount = ChrList[character].attachedto;
    if ( mount >= MAX_CHR )
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

    // set the dismount timer
    ChrList[character].phys_dismount_timer = PHYS_DISMOUNT_TIME;

    // Figure out which hand it's in
    hand = ChrList[character].inwhichhand;

    // Rip 'em apart
    ChrList[character].attachedto = MAX_CHR;
    if ( ChrList[mount].holdingwhich[SLOT_LEFT] == character )
        ChrList[mount].holdingwhich[SLOT_LEFT] = MAX_CHR;
    if ( ChrList[mount].holdingwhich[SLOT_RIGHT] == character )
        ChrList[mount].holdingwhich[SLOT_RIGHT] = MAX_CHR;

    if ( ChrList[character].alive )
    {
        // play the falling animation...
        chr_play_action( character, ACTION_JB + hand, bfalse );
    }
    else if ( ChrList[character].action < ACTION_KA || ChrList[character].action > ACTION_KD )
    {
        // play the "killed" animation...
        chr_play_action( character, ACTION_KA + hand, bfalse );
        ChrList[character].keepaction = btrue;
    }

    // Set the positions
    if ( ChrList[character].inst.matrixvalid )
    {
        ChrList[character].pos.x = ChrList[character].inst.matrix.CNV( 3, 0 );
        ChrList[character].pos.y = ChrList[character].inst.matrix.CNV( 3, 1 );
        ChrList[character].pos.z = ChrList[character].inst.matrix.CNV( 3, 2 );
    }
    else
    {
        ChrList[character].pos.x = ChrList[mount].pos.x;
        ChrList[character].pos.y = ChrList[mount].pos.y;
        ChrList[character].pos.z = ChrList[mount].pos.z;
    }

    // Make sure it's not dropped in a wall...
    if ( __chrhitawall( character ) )
    {
        ChrList[character].pos.x = ChrList[mount].pos.x;
        ChrList[character].pos.y = ChrList[mount].pos.y;
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
            loc = ChrList[character].pos.x;
            loc = loc >> 7;
            if ( loc >= passtlx[passage] && loc <= passbrx[passage] )
            {
                loc = ChrList[character].pos.y;
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
        ChrList[character].vel.x = 0;
        ChrList[character].vel.y = 0;
    }
    else
    {
        ChrList[character].vel.x = ChrList[mount].vel.x;
        ChrList[character].vel.y = ChrList[mount].vel.y;
    }

    ChrList[character].vel.z = DROPZVEL;

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

        while ( enchant < MAX_ENC )
        {
            unset_enchant_value( enchant, SETALPHABLEND );
            unset_enchant_value( enchant, SETLIGHTBLEND );
            enchant = EncList[enchant].nextenchant;
        }

        ChrList[character].inst.alpha = ChrList[character].basealpha;
        ChrList[character].inst.light = CapList[ChrList[character].model].light;
        enchant = ChrList[character].firstenchant;

        while ( enchant < MAX_ENC )
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
    if ( character != MAX_CHR )
    {
        mount = ChrList[character].attachedto;
        if ( ChrList[character].on && mount != MAX_CHR && ChrList[character].isitem && ChrList[mount].transferblend )
        {
            // Okay, reset transparency
            enchant = ChrList[character].firstenchant;

            while ( enchant < MAX_ENC )
            {
                unset_enchant_value( enchant, SETALPHABLEND );
                unset_enchant_value( enchant, SETLIGHTBLEND );
                enchant = EncList[enchant].nextenchant;
            }

            ChrList[character].inst.alpha = ChrList[character].basealpha;
            ChrList[character].inst.light = CapList[ChrList[character].model].light;
            enchant = ChrList[character].firstenchant;

            while ( enchant < MAX_ENC )
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
	if ( INVALID_CHR(character) || INVALID_CHR(mount) ) return; 
    
    if( ChrList[character].inpack || ChrList[mount].inpack ) return;

#ifdef DISABLE_BODY_GRAB
	if(!ChrList[character].alive) return;
#endif

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
    if ( !CapList[ChrList[mount].model].slotvalid[slot] )
        return;

    // Put 'em together
    ChrList[character].inwhichhand    = slot;
    ChrList[character].attachedto     = mount;
    ChrList[mount].holdingwhich[slot] = character;

    tnc = MadList[ChrList[mount].inst.imad].md2.vertices - grip;

    for (i = 0; i < GRIP_VERTS; i++)
    {
        if (tnc + i < MadList[ChrList[mount].inst.imad].md2.vertices )
        {
            ChrList[character].weapongrip[i] = i + tnc;
        }
        else
        {
            ChrList[character].weapongrip[i] = 0xFFFF;
        }
    }

    // catually make position of the object coincide with its actual held position
    make_one_weapon_matrix( character, mount, bfalse );

    ChrList[character].pos.x = ChrList[character].inst.matrix.CNV( 3, 0 );
    ChrList[character].pos.y = ChrList[character].inst.matrix.CNV( 3, 1 );
    ChrList[character].pos.z = ChrList[character].inst.matrix.CNV( 3, 2 );

    ChrList[character].inwater = bfalse;
    ChrList[character].jumptime = JUMPDELAY * 4;

    // Run the held animation
    if ( ChrList[mount].ismount && grip == GRIP_ONLY )
    {
        // Riding mount
        chr_play_action( character, ACTION_MI, btrue );
        ChrList[character].loopaction = btrue;
    }
    else if ( ChrList[character].alive )
    {
        chr_play_action( character, ACTION_MM + slot, bfalse );
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
    //     index number, otherwise it returns MAX_CHR.
    Uint16 inpack, id;
    bool_t allok;
    if ( CapList[ChrList[item].model].isstackable )
    {
        inpack = ChrList[character].nextinpack;
        allok = bfalse;

        while ( inpack != MAX_CHR && !allok )
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

    return MAX_CHR;
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
    if ( stack != MAX_CHR )
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
            if ( ChrList[item].attachedto != MAX_CHR )
            {
                detach_character_from_mount( item, btrue, bfalse );
            }

            free_one_character_in_game( item );
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
        if ( ChrList[item].attachedto != MAX_CHR )
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
    //     it into the designated hand.  It returns the item number or MAX_CHR.
    Uint16 item, nexttolastitem;

    // Make sure everything is hunkydori
    if ( ( !ChrList[character].on ) || ChrList[character].inpack || ChrList[character].isitem || ChrList[character].nextinpack == MAX_CHR )
        return MAX_CHR;
    if ( ChrList[character].numinpack == 0 )
        return MAX_CHR;

    // Find the last item in the pack
    nexttolastitem = character;
    item = ChrList[character].nextinpack;

    while ( ChrList[item].nextinpack != MAX_CHR )
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
        ChrList[nexttolastitem].nextinpack = MAX_CHR;
        ChrList[character].nextinpack = item;
        if ( character == nexttolastitem )
        {
            ChrList[item].nextinpack = MAX_CHR;
        }

        return MAX_CHR;
    }
    else
    {
        // Remove the last item from the pack
        ChrList[item].inpack = bfalse;
        ChrList[item].isequipped = bfalse;
        ChrList[nexttolastitem].nextinpack = MAX_CHR;
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
    if ( character < MAX_CHR )
    {
        if ( ChrList[character].on )
        {
            if ( ChrList[character].pos.z > -2 ) // Don't lose keys in pits...
            {
                // The IDSZs to find
                testa = Make_IDSZ( "KEYA" );  // [KEYA]
                testz = Make_IDSZ( "KEYZ" );  // [KEYZ]

                lastitem = character;
                item = ChrList[character].nextinpack;

                while ( item != MAX_CHR )
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
                            ChrList[item].nextinpack = MAX_CHR;
                            ChrList[character].numinpack--;
                            ChrList[item].attachedto = MAX_CHR;
                            ChrList[item].ai.alert |= ALERTIF_DROPPED;
                            ChrList[item].hitready = btrue;

                            direction = RANDIE;
                            ChrList[item].turnleftright = direction + 32768;
                            cosdir = direction + 16384;
                            ChrList[item].phys_level = ChrList[character].phys_level;
                            ChrList[item].floor_level = ChrList[character].floor_level;
                            ChrList[item].onwhichplatform = ChrList[character].onwhichplatform;
                            ChrList[item].pos.x = ChrList[character].pos.x;
                            ChrList[item].pos.y = ChrList[character].pos.y;
                            ChrList[item].pos.z = ChrList[character].pos.z;
                            ChrList[item].vel.x = turntocos[direction>>2] * DROPXYVEL;
                            ChrList[item].vel.y = turntosin[direction>>2] * DROPXYVEL;
                            ChrList[item].vel.z = DROPZVEL;
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
    if ( character < MAX_CHR )
    {
        if ( ChrList[character].on )
        {
            detach_character_from_mount( ChrList[character].holdingwhich[SLOT_LEFT], btrue, bfalse );
            detach_character_from_mount( ChrList[character].holdingwhich[SLOT_RIGHT], btrue, bfalse );
            if ( ChrList[character].numinpack > 0 )
            {
                direction = ChrList[character].turnleftright + 32768;
                diradd = 0xFFFF / ChrList[character].numinpack;

                while ( ChrList[character].numinpack > 0 )
                {
                    item = pack_get_item( character, GRIP_LEFT, bfalse );
                    if ( item < MAX_CHR )
                    {
                        detach_character_from_mount( item, btrue, btrue );
                        ChrList[item].hitready = btrue;
                        ChrList[item].ai.alert |= ALERTIF_DROPPED;
                        ChrList[item].pos.x = ChrList[character].pos.x;
                        ChrList[item].pos.y = ChrList[character].pos.y;
                        ChrList[item].pos.z = ChrList[character].pos.z;
                        ChrList[item].phys_level = ChrList[character].phys_level;
                        ChrList[item].floor_level = ChrList[character].floor_level;
                        ChrList[item].onwhichplatform = ChrList[character].onwhichplatform;
                        ChrList[item].turnleftright = direction + 32768;
                        ChrList[item].vel.x = turntocos[direction>>2] * DROPXYVEL;
                        ChrList[item].vel.y = turntosin[direction>>2] * DROPXYVEL;
                        ChrList[item].vel.z = DROPZVEL;
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
    float xb, yb, zb, dist;
    int charb, slot;
    Uint16 vertex, model, frame, passage, cnt, owner = NOOWNER;
    GLvector4 point[1], nupoint[1];
    bool_t inshop;
    int loc;
    float price;

    // Make life easier
    model = ChrList[chara].model;
    slot = ( grip / GRIP_VERTS ) - 1;  // 0 is left, 1 is right

    // Make sure the character doesn't have something already, and that it has hands
    if ( ChrList[chara].holdingwhich[slot] != MAX_CHR || !CapList[model].slotvalid[slot] )
        return bfalse;

    // Do we have a matrix???
    if ( ChrList[chara].inst.matrixvalid )// PMesh->mem.inrenderlist[ChrList[chara].onwhichfan])
    {
        // Transform the weapon grip from model to world space
        frame = ChrList[chara].inst.frame;
        vertex = MadList[model].md2.vertices - grip;

        // Calculate grip point locations
        point[0].x = Md2FrameList[frame].vrtx[vertex];/// ChrList[cnt].scale;
        point[0].y = Md2FrameList[frame].vrty[vertex];/// ChrList[cnt].scale;
        point[0].z = Md2FrameList[frame].vrtz[vertex];/// ChrList[cnt].scale;
        point[0].w = 1.0f;

        // Do the transform
        TransformVertices( &(ChrList[chara].inst.matrix), point, nupoint, 1 );
    }
    else
    {
        // Just wing it
        nupoint[0].x = ChrList[chara].pos.x;
        nupoint[0].y = ChrList[chara].pos.y;
        nupoint[0].z = ChrList[chara].pos.z;
    }

    // Go through all characters to find the best match
    for ( charb = 0; charb < MAX_CHR; charb++ )
    {
        if ( !ChrList[charb].on ) continue;

        if ( ChrList[charb].inpack ) continue;              // pickpocket not allowed yet
        if ( MAX_CHR != ChrList[charb].attachedto) continue; // disarm not allowed yet

        if ( ChrList[charb].weight > ChrList[chara].weight + ChrList[chara].strength ) continue; // reasonable carrying capacity

        // people == btrue allows you to pick up living non-items
        // people == false allows you to pick up living (functioning) items
        if ( ChrList[charb].alive && (people == ChrList[charb].isitem) ) continue;

        // do not pick up your mount
        if ( ChrList[charb].holdingwhich[SLOT_LEFT] == chara || ChrList[charb].holdingwhich[SLOT_RIGHT] == chara ) continue;

        xb = ChrList[charb].pos.x;
        yb = ChrList[charb].pos.y;
        zb = ChrList[charb].pos.z;

        // First check absolute value diamond
        xb = ABS( nupoint[0].x - xb );
        yb = ABS( nupoint[0].y - yb );
        zb = ABS( nupoint[0].z - zb );
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

                    loc = ChrList[charb].pos.x;
                    loc = loc >> 7;
                    if ( loc >= passtlx[passage] && loc <= passbrx[passage] )
                    {
                        loc = ChrList[charb].pos.y;
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
                    if ( ChrList[chara].isitem || ( ChrList[chara].inst.alpha < INVISIBLE) )
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
                            ChrList[owner].ai.rank  = THEFT;
							ChrList[owner].ai.target = chara;
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
                            ChrList[owner].ai.rank = NOAFFORD;  // 2 for "you can't afford that"
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
                    chr_play_action( chara, ACTION_MC + slot, bfalse );
                }
                return btrue;
            }
            else
            {
                // Lift the item a little and quit...
                ChrList[charb].vel.z = DROPZVEL;
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
    if ( weapon == MAX_CHR )
    {
        weapon = cnt;
        spawngrip = (slot + 1) * GRIP_VERTS;  // 0 -> GRIP_LEFT, 1 -> GRIP_RIGHT
    }
    if ( weapon != cnt && ( ( CapList[ChrList[weapon].model].isstackable && ChrList[weapon].ammo > 1 ) || ( action >= ACTION_FA && action <= ACTION_FD ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation
        x = ChrList[cnt].pos.x;
        y = ChrList[cnt].pos.y;
        z = ChrList[cnt].pos.z;
        thrown = spawn_one_character( x, y, z, ChrList[weapon].model, ChrList[cnt].team, 0, ChrList[cnt].turnleftright, ChrList[weapon].name, MAX_CHR );
        if ( thrown < MAX_CHR )
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
            ChrList[thrown].vel.x += turntocos[( tTmp+8192 )&TRIG_TABLE_MASK] * velocity;
            ChrList[thrown].vel.y += turntosin[( tTmp+8192 )&TRIG_TABLE_MASK] * velocity;
            ChrList[thrown].vel.z = DROPZVEL;
            if ( ChrList[weapon].ammo <= 1 )
            {
                // Poof the item
                detach_character_from_mount( weapon, btrue, bfalse );
                free_one_character_in_game( weapon );
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
                particle = spawn_one_particle( ChrList[weapon].pos.x, ChrList[weapon].pos.y, ChrList[weapon].pos.z, ChrList[cnt].turnleftright, ChrList[weapon].model, CapList[ChrList[weapon].model].attackprttype, weapon, spawngrip, ChrList[cnt].team, cnt, 0, MAX_CHR );
                if ( particle != TOTAL_MAX_PRT )
                {
                    if ( !CapList[ChrList[weapon].model].attackattached )
                    {
                        // Detach the particle
                        if ( !PipList[PrtList[particle].pip].startontarget || PrtList[particle].target == MAX_CHR )
                        {
                            attach_particle_to_character( particle, weapon, spawngrip );
                            // Correct Z spacing base, but nothing else...
                            PrtList[particle].pos.z += PipList[PrtList[particle].pip].zspacingbase;
                        }

                        PrtList[particle].attachedtocharacter = MAX_CHR;

                        // Don't spawn in walls
                        if ( __prthitawall( particle ) )
                        {
                            PrtList[particle].pos.x = ChrList[weapon].pos.x;
                            PrtList[particle].pos.y = ChrList[weapon].pos.y;
                            if ( __prthitawall( particle ) )
                            {
                                PrtList[particle].pos.x = ChrList[cnt].pos.x;
                                PrtList[particle].pos.y = ChrList[cnt].pos.y;
                            }
                        }
                    }
                    else
                    {
                        // Attached particles get a strength bonus for reeling...
                        dampen = REELBASE + ( ChrList[cnt].strength / REEL );
                        PrtList[particle].vel.x = PrtList[particle].vel.x * dampen;
                        PrtList[particle].vel.y = PrtList[particle].vel.y * dampen;
                        PrtList[particle].vel.z = PrtList[particle].vel.z * dampen;
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
    if ( money > 0 && ChrList[character].pos.z > -2 )
    {
        ChrList[character].money = ChrList[character].money - money;
        huns = money / 100;  money -= ( huns << 7 ) - ( huns << 5 ) + ( huns << 2 );
        tfives = money / 25;  money -= ( tfives << 5 ) - ( tfives << 3 ) + tfives;
        fives = money / 5;  money -= ( fives << 2 ) + fives;
        ones = money;

        for ( cnt = 0; cnt < ones; cnt++ )
        {
            spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y,  ChrList[character].pos.z, 0, MAX_PROFILE, COIN1, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, cnt, MAX_CHR );
        }

        for ( cnt = 0; cnt < fives; cnt++ )
        {
            spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y,  ChrList[character].pos.z, 0, MAX_PROFILE, COIN5, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, cnt, MAX_CHR );
        }

        for ( cnt = 0; cnt < tfives; cnt++ )
        {
            spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y,  ChrList[character].pos.z, 0, MAX_PROFILE, COIN25, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, cnt, MAX_CHR );
        }

        for ( cnt = 0; cnt < huns; cnt++ )
        {
            spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y,  ChrList[character].pos.z, 0, MAX_PROFILE, COIN100, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, cnt, MAX_CHR );
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

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
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
    //This calculates the xp needed to reach next level
    Uint32 curlevel;
    Uint16 profile;
    Uint32 xpneeded = (Uint32)(~0);
    if ( !ChrList[character].on ) return xpneeded;

    profile  = ChrList[character].model;
    if (profile == MAX_PROFILE) return xpneeded;

    // Calculate xp needed
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
    if (character >= MAX_CHR || !ChrList[character].on) return;

    profile = ChrList[character].model;
    if ( profile >= MAX_PROFILE ) return;

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
            ChrList[character].sizegoto += CapList[profile].sizeperlevel * 0.25f;  // Limit this?
            ChrList[character].sizegototime += SIZETIME;

            // Strength
            number = generate_number( CapList[profile].strengthperlevelbase, CapList[profile].strengthperlevelrand );
            number += ChrList[character].strength;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList[character].strength = number;

            // Wisdom
            number = generate_number( CapList[profile].wisdomperlevelbase, CapList[profile].wisdomperlevelrand );
            number += ChrList[character].wisdom;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList[character].wisdom = number;

            // Intelligence
            number = generate_number( CapList[profile].intelligenceperlevelbase, CapList[profile].intelligenceperlevelrand );
            number += ChrList[character].intelligence;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList[character].intelligence = number;

            // Dexterity
            number = generate_number( CapList[profile].dexterityperlevelbase, CapList[profile].dexterityperlevelrand );
            number += ChrList[character].dexterity;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList[character].dexterity = number;

            // Life
            number = generate_number( CapList[profile].lifeperlevelbase, CapList[profile].lifeperlevelrand );
            number += ChrList[character].lifemax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            ChrList[character].life += ( number - ChrList[character].lifemax );
            ChrList[character].lifemax = number;

            // Mana
            number = generate_number( CapList[profile].manaperlevelbase, CapList[profile].manaperlevelrand );
            number += ChrList[character].manamax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            ChrList[character].mana += ( number - ChrList[character].manamax );
            ChrList[character].manamax = number;

            // Mana Return
            number = generate_number( CapList[profile].manareturnperlevelbase, CapList[profile].manareturnperlevelrand );
            number += ChrList[character].manareturn;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList[character].manareturn = number;

            // Mana Flow
            number = generate_number( CapList[profile].manaflowperlevelbase, CapList[profile].manaflowperlevelrand );
            number += ChrList[character].manaflow;
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
		{
			float intadd = (FP8_TO_INT(ChrList[character].intelligence) - 10.0f) / 200.0f;
			float wisadd = (FP8_TO_INT(ChrList[character].wisdom) - 10.0f)		 / 400.0f;
			newamount *= 1.00f + intadd + wisadd;
		}

        ChrList[character].experience += newamount;
    }
}

//--------------------------------------------------------------------------------------------
void give_team_experience( Uint8 team, int amount, Uint8 xptype )
{
    // ZZ> This function gives a character experience, and pawns off level gains to
    //     another function
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
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

    while ( cnt < MAX_CHR )
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
                    ChrList[cnt].weight = 0xFFFFFFFF;
                }
                else
                {
                    int itmp = CapList[ChrList[cnt].model].weight * ChrList[cnt].fat * ChrList[cnt].fat * ChrList[cnt].fat;
                    ChrList[cnt].weight = MIN( itmp, 0xFFFFFFFE );
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
                if ( CapList[profile].damagemodifier[damagetype][skin]&DAMAGEMANA )
                    codes[skin] = 'M';

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
        ftruthf( filewrite, "Left valid     : ", CapList[profile].slotvalid[SLOT_LEFT] );
        ftruthf( filewrite, "Right valid    : ", CapList[profile].slotvalid[SLOT_RIGHT] );
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
int load_one_character_profile( const char * tmploadname )
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
    IDSZ idsz;
    cap_t * pcap;
    int cnt;
    STRING szLoadName, wavename;

    make_newloadname( tmploadname, SLASH_STR "data.txt", szLoadName );

    // Open the file
    fileread = fopen( szLoadName, "r" );
    if ( fileread == NULL )
    {
        // The data file wasn't found
        log_error( "DATA.TXT was not found! (%s)\n", szLoadName );
        return MAX_PROFILE;
    }

    parse_filename = szLoadName;  //For debugging goto_colon()

    // Read in the object slot
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp ); object = iTmp;
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

    if( !VALID_CAP_RANGE( object ) ) return MAX_PROFILE;
    pcap = CapList + object;

    // Make sure global objects don't load over existing models
    if ( pcap->loaded )
    {
        if ( object == SPELLBOOK ) 
            log_error( "Object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, szLoadName );
        else if ( overrideslots )  
            log_error( "Object slot %i used twice (%s, %s)\n", object, pcap->name, szLoadName );
        else return -1;   //Stop, we don't want to override it
    }

    // clear out all the data
    memset( pcap, 0, sizeof(cap_t) );

    strncpy( pcap->name, szLoadName, SDL_arraysize(pcap->name) );

    // mark it as loaded
    pcap->loaded = btrue;

    // clear out the sounds
    for ( iTmp = 0; iTmp < SOUND_COUNT; iTmp++ )
    {
        pcap->soundindex[iTmp] = -1;
    }

    // Read in the real general data
    goto_colon( NULL, fileread, bfalse );  fget_name( fileread, pcap->classname, sizeof(pcap->classname) );

    // Light cheat
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->uniformlit = bfalse;
    if ( cTmp == 'T' || cTmp == 't' || GL_FLAT == shading )  pcap->uniformlit = btrue;

    // Ammo
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->ammomax = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->ammo = iTmp;
    // Gender
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->gender = GENOTHER;
    if ( cTmp == 'F' || cTmp == 'f' )  pcap->gender = GENFEMALE;
    if ( cTmp == 'M' || cTmp == 'm' )  pcap->gender = GENMALE;
    if ( cTmp == 'R' || cTmp == 'r' )  pcap->gender = GENRANDOM;

    // Read in the object stats
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->lifecolor = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->manacolor = iTmp;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->lifebase = pairbase;  pcap->liferand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->lifeperlevelbase = pairbase;  pcap->lifeperlevelrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->manabase = pairbase;  pcap->manarand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->manaperlevelbase = pairbase;  pcap->manaperlevelrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->manareturnbase = pairbase;  pcap->manareturnrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->manareturnperlevelbase = pairbase;  pcap->manareturnperlevelrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->manaflowbase = pairbase;  pcap->manaflowrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->manaflowperlevelbase = pairbase;  pcap->manaflowperlevelrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->strengthbase = pairbase;  pcap->strengthrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->strengthperlevelbase = pairbase;  pcap->strengthperlevelrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->wisdombase = pairbase;  pcap->wisdomrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->wisdomperlevelbase = pairbase;  pcap->wisdomperlevelrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->intelligencebase = pairbase;  pcap->intelligencerand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->intelligenceperlevelbase = pairbase;  pcap->intelligenceperlevelrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->dexteritybase = pairbase;  pcap->dexterityrand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pcap->dexterityperlevelbase = pairbase;  pcap->dexterityperlevelrand = pairrand;

    // More physical attributes
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->size = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->sizeperlevel = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->shadowsize = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->bumpsize = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->bumpheight = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->bumpdampen = MAX(0.01, fTmp);
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->weight = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->jump = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->jumpnumber = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->sneakspd = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->walkspd = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->runspd = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->flyheight = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->flashand = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->alpha = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->light = iTmp;
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->transferblend = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->transferblend = btrue;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->sheen = iTmp;
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->enviro = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->enviro = btrue;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->uoffvel = fTmp * 0xFFFF;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->voffvel = fTmp * 0xFFFF;
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->stickybutt = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->stickybutt = btrue;

    // Invulnerability data
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->invictus = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->invictus = btrue;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->nframefacing = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->nframeangle = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->iframefacing = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->iframeangle = iTmp;

    // Resist burning and stuck arrows with nframe angle of 1 or more
    if ( pcap->nframeangle > 0 )
    {
        if ( pcap->nframeangle == 1 )
        {
            pcap->nframeangle = 0;
        }
    }

    // Skin defenses ( 4 skins )
    goto_colon( NULL, fileread, bfalse );
    fscanf( fileread, "%d", &iTmp );  pcap->defense[0] = 255 - iTmp;
    fscanf( fileread, "%d", &iTmp );  pcap->defense[1] = 255 - iTmp;
    fscanf( fileread, "%d", &iTmp );  pcap->defense[2] = 255 - iTmp;
    fscanf( fileread, "%d", &iTmp );  pcap->defense[3] = 255 - iTmp;

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        goto_colon( NULL, fileread, bfalse );
        fscanf( fileread, "%d", &iTmp );  pcap->damagemodifier[damagetype][0] = iTmp;
        fscanf( fileread, "%d", &iTmp );  pcap->damagemodifier[damagetype][1] = iTmp;
        fscanf( fileread, "%d", &iTmp );  pcap->damagemodifier[damagetype][2] = iTmp;
        fscanf( fileread, "%d", &iTmp );  pcap->damagemodifier[damagetype][3] = iTmp;
    }

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        goto_colon( NULL, fileread, bfalse );

        cTmp = fget_first_letter( fileread );
        if ( cTmp == 'T' || cTmp == 't' )  pcap->damagemodifier[damagetype][0] |= DAMAGEINVERT;
        else if ( cTmp == 'C' || cTmp == 'c' )  pcap->damagemodifier[damagetype][0] |= DAMAGECHARGE;
        else if ( toupper(cTmp) == 'M' )  pcap->damagemodifier[damagetype][0] |= DAMAGEMANA;

        cTmp = fget_first_letter( fileread );
        if ( cTmp == 'T' || cTmp == 't' )  pcap->damagemodifier[damagetype][1] |= DAMAGEINVERT;
        else if ( cTmp == 'C' || cTmp == 'c' )  pcap->damagemodifier[damagetype][1] |= DAMAGECHARGE;
        else if ( toupper(cTmp) == 'M' )  pcap->damagemodifier[damagetype][1] |= DAMAGEMANA;

        cTmp = fget_first_letter( fileread );
        if ( cTmp == 'T' || cTmp == 't' )  pcap->damagemodifier[damagetype][2] |= DAMAGEINVERT;
        else if ( cTmp == 'C' || cTmp == 'c' )  pcap->damagemodifier[damagetype][2] |= DAMAGECHARGE;
        else if ( toupper(cTmp) == 'M' )  pcap->damagemodifier[damagetype][2] |= DAMAGEMANA;

        cTmp = fget_first_letter( fileread );
        if ( cTmp == 'T' || cTmp == 't' )  pcap->damagemodifier[damagetype][3] |= DAMAGEINVERT;
        else if ( cTmp == 'C' || cTmp == 'c' )  pcap->damagemodifier[damagetype][3] |= DAMAGECHARGE;
        else if ( toupper(cTmp) == 'M' )  pcap->damagemodifier[damagetype][3] |= DAMAGEMANA;
    }

    goto_colon( NULL, fileread, bfalse );
    fscanf( fileread, "%f", &fTmp );  pcap->maxaccel[0] = fTmp / 80.0f;
    fscanf( fileread, "%f", &fTmp );  pcap->maxaccel[1] = fTmp / 80.0f;
    fscanf( fileread, "%f", &fTmp );  pcap->maxaccel[2] = fTmp / 80.0f;
    fscanf( fileread, "%f", &fTmp );  pcap->maxaccel[3] = fTmp / 80.0f;

    // Experience and level data
    pcap->experienceforlevel[0] = 0;

    for ( level = 1; level < MAXLEVEL; level++ )
    {
        goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->experienceforlevel[level] = iTmp;
    }

    goto_colon( NULL, fileread, bfalse );  fget_pair( fileread );
    pairbase = pairbase >> 8;
    pairrand = pairrand >> 8;
    if ( pairrand < 1 )  pairrand = 1;

    pcap->experiencebase = pairbase;
    pcap->experiencerand = pairrand;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->experienceworth = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->experienceexchange = fTmp;

    for ( xptype = 0; xptype < XP_COUNT; xptype++ )
    {
        goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->experiencerate[xptype] = fTmp + 0.001f;
    }

    // IDSZ tags
    for ( idsz_cnt = 0; idsz_cnt < IDSZ_COUNT; idsz_cnt++ )
    {
        goto_colon( NULL, fileread, bfalse );  iTmp = fget_idsz( fileread );  pcap->idsz[idsz_cnt] = iTmp;
    }

    // Item and damage flags
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->isitem = bfalse;  pcap->ripple = btrue;
    if ( cTmp == 'T' || cTmp == 't' )  { pcap->isitem = btrue; pcap->ripple = bfalse; }

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->ismount = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->ismount = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->isstackable = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->isstackable = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->nameknown = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->nameknown = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->usageknown = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->usageknown = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->cancarrytonextmodule = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->cancarrytonextmodule = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->needskillidtouse = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->needskillidtouse = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->platform = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->platform = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->cangrabmoney = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->cangrabmoney = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->canopenstuff = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->canopenstuff = btrue;

    // More item and damage stuff
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    if ( cTmp == 'S' || cTmp == 's' )  pcap->damagetargettype = DAMAGE_SLASH;
    if ( cTmp == 'C' || cTmp == 'c' )  pcap->damagetargettype = DAMAGE_CRUSH;
    if ( cTmp == 'P' || cTmp == 'p' )  pcap->damagetargettype = DAMAGE_POKE;
    if ( cTmp == 'H' || cTmp == 'h' )  pcap->damagetargettype = DAMAGE_HOLY;
    if ( cTmp == 'E' || cTmp == 'e' )  pcap->damagetargettype = DAMAGE_EVIL;
    if ( cTmp == 'F' || cTmp == 'f' )  pcap->damagetargettype = DAMAGE_FIRE;
    if ( cTmp == 'I' || cTmp == 'i' )  pcap->damagetargettype = DAMAGE_ICE;
    if ( cTmp == 'Z' || cTmp == 'z' )  pcap->damagetargettype = DAMAGE_ZAP;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->weaponaction = action_which( cTmp );

    // Particle attachments
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->attachedprtamount = iTmp;
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    if ( cTmp == 'N' || cTmp == 'n' )  pcap->attachedprtreaffirmdamagetype = DAMAGE_NONE;
    if ( cTmp == 'S' || cTmp == 's' )  pcap->attachedprtreaffirmdamagetype = DAMAGE_SLASH;
    if ( cTmp == 'C' || cTmp == 'c' )  pcap->attachedprtreaffirmdamagetype = DAMAGE_CRUSH;
    if ( cTmp == 'P' || cTmp == 'p' )  pcap->attachedprtreaffirmdamagetype = DAMAGE_POKE;
    if ( cTmp == 'H' || cTmp == 'h' )  pcap->attachedprtreaffirmdamagetype = DAMAGE_HOLY;
    if ( cTmp == 'E' || cTmp == 'e' )  pcap->attachedprtreaffirmdamagetype = DAMAGE_EVIL;
    if ( cTmp == 'F' || cTmp == 'f' )  pcap->attachedprtreaffirmdamagetype = DAMAGE_FIRE;
    if ( cTmp == 'I' || cTmp == 'i' )  pcap->attachedprtreaffirmdamagetype = DAMAGE_ICE;
    if ( cTmp == 'Z' || cTmp == 'z' )  pcap->attachedprtreaffirmdamagetype = DAMAGE_ZAP;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->attachedprttype = iTmp;

    // Character hands
    pcap->slotvalid[SLOT_LEFT] = bfalse;
    pcap->slotvalid[SLOT_RIGHT] = bfalse;
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    if ( cTmp == 'T' || cTmp == 't' )  pcap->slotvalid[SLOT_LEFT] = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    if ( cTmp == 'T' || cTmp == 't' )  pcap->slotvalid[SLOT_RIGHT] = btrue;

    // Attack order ( weapon )
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->attackattached = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->attackattached = btrue;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->attackprttype = iTmp;

    // GoPoof
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->gopoofprtamount = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->gopoofprtfacingadd = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->gopoofprttype = iTmp;

    // Blud
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->bludvalid = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->bludvalid = btrue;
    if ( cTmp == 'U' || cTmp == 'u' )  pcap->bludvalid = ULTRABLUDY;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->bludprttype = iTmp;

    // Stuff I forgot
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->waterwalk = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->waterwalk = btrue;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->dampen = fTmp;

    // More stuff I forgot
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->lifeheal = fTmp * 256;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->manacost = fTmp * 256;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->lifereturn = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->stoppedby = iTmp | MESHFX_IMPASS;
    goto_colon( NULL, fileread, bfalse );  fget_name( fileread, pcap->skinname[0], sizeof(pcap->skinname[0]) );
    goto_colon( NULL, fileread, bfalse );  fget_name( fileread, pcap->skinname[1], sizeof(pcap->skinname[1]) );
    goto_colon( NULL, fileread, bfalse );  fget_name( fileread, pcap->skinname[2], sizeof(pcap->skinname[2]) );
    goto_colon( NULL, fileread, bfalse );  fget_name( fileread, pcap->skinname[3], sizeof(pcap->skinname[3]) );
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->skincost[0] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->skincost[1] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->skincost[2] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->skincost[3] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  pcap->strengthdampen = fTmp;

    // Another memory lapse
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pcap->ridercanattack = btrue;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->ridercanattack = bfalse;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );  // Can be dazed
    pcap->canbedazed = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->canbedazed = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );  // Can be grogged
    pcap->canbegrogged = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->canbegrogged = btrue;

    goto_colon( NULL, fileread, bfalse );  // !!!BAD!!! Life add
    goto_colon( NULL, fileread, bfalse );  // !!!BAD!!! Mana add
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );  // Can see invisible
    pcap->canseeinvisible = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pcap->canseeinvisible = btrue;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  // Chance of kursed
    pcap->kursechance = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  // Footfall sound
    pcap->soundindex[SOUND_FOOTFALL] = CLIP(iTmp, -1, MAX_WAVE);
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  // Jump sound
    pcap->soundindex[SOUND_JUMP] = CLIP(iTmp, -1, MAX_WAVE);

    // Clear expansions...
    pcap->skindressy = bfalse;
    pcap->resistbumpspawn = bfalse;
    pcap->istoobig = bfalse;
    pcap->reflect = btrue;
    pcap->alwaysdraw = bfalse;
    pcap->isranged = bfalse;
    pcap->hidestate = NOHIDE;
    pcap->isequipment = bfalse;
    pcap->bumpsizebig = pcap->bumpsize + ( pcap->bumpsize >> 1 );
    pcap->canseekurse = bfalse;
    pcap->money = 0;
    pcap->icon = pcap->usageknown;
    pcap->forceshadow = bfalse;
    pcap->skinoverride = NOSKINOVERRIDE;
    pcap->contentoverride = 0;
    pcap->stateoverride = 0;
    pcap->leveloverride = 0;
    pcap->canuseplatforms = !pcap->platform;
    pcap->isvaluable = 0;

    //Skills
    pcap->canuseadvancedweapons = 0;
    pcap->canjoust = 0;
    pcap->canusetech = 0;
    pcap->canusedivine = 0;
    pcap->canusearcane = 0;
    pcap->shieldproficiency = 0;
    pcap->candisarm = 0;
    pcap->canbackstab = 0;
    pcap->canusepoison = 0;
    pcap->canread = 0;

    // Read expansions
    while ( goto_colon( NULL, fileread, btrue ) )
    {
        idsz = fget_idsz( fileread );

             if ( idsz == Make_IDSZ( "DRES" ) ) pcap->skindressy |= 1 << fget_int( fileread );
        else if ( idsz == Make_IDSZ( "GOLD" ) ) pcap->money = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "STUK" ) ) pcap->resistbumpspawn = 1 - fget_int( fileread );
        else if ( idsz == Make_IDSZ( "PACK" ) ) pcap->istoobig = 1 - fget_int( fileread );
        else if ( idsz == Make_IDSZ( "VAMP" ) ) pcap->reflect = 1 - fget_int( fileread );
        else if ( idsz == Make_IDSZ( "DRAW" ) ) pcap->alwaysdraw = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "RANG" ) ) pcap->isranged = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "HIDE" ) ) pcap->hidestate = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "EQUI" ) ) pcap->isequipment = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "SQUA" ) ) pcap->bumpsizebig = pcap->bumpsize << 1;
        else if ( idsz == Make_IDSZ( "ICON" ) ) pcap->icon = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "SHAD" ) ) pcap->forceshadow = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "CKUR" ) ) pcap->canseekurse = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "SKIN" ) ) pcap->skinoverride = fget_int( fileread ) & 3;
        else if ( idsz == Make_IDSZ( "CONT" ) ) pcap->contentoverride = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "STAT" ) ) pcap->stateoverride = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "LEVL" ) ) pcap->leveloverride = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "PLAT" ) ) pcap->canuseplatforms = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "RIPP" ) ) pcap->ripple = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "VALU" ) ) pcap->isvaluable = fget_int( fileread );

        //Read Skills
        else if ( idsz == Make_IDSZ( "AWEP" ) ) pcap->canuseadvancedweapons = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "SHPR" ) ) pcap->shieldproficiency = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "JOUS" ) ) pcap->canjoust = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "WMAG" ) ) pcap->canusearcane = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "HMAG" ) ) pcap->canusedivine = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "TECH" ) ) pcap->canusetech = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "DISA" ) ) pcap->candisarm = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "STAB" ) ) pcap->canbackstab = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "POIS" ) ) pcap->canusepoison = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "READ" ) ) pcap->canread = fget_int( fileread );
    }

    fclose( fileread );

    // Load the random naming table for this object
    make_newloadname( tmploadname, SLASH_STR "naming.txt", szLoadName );
    chop_load( object, szLoadName );

    // Load the waves for this object
    for ( cnt = 0; cnt < MAX_WAVE; cnt++ )
    {
        sprintf( wavename, SLASH_STR "sound%d", cnt );
        make_newloadname( tmploadname, wavename, szLoadName );
        pcap->wavelist[cnt] = sound_load_chunk( szLoadName );
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

        // Allow damage to be dealt to mana (mana shield spell)
        if ( ChrList[character].damagemodifier[damagetype]&DAMAGEMANA )
        {
            int manadamage;
            manadamage = MAX(damage - ChrList[character].mana, 0);
            ChrList[character].mana = MAX(ChrList[character].mana - damage, 0);
        }

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
            if ( (0 == ChrList[character].damagetime || ignoreinvincible) && !ChrList[character].invictus )
            {
                model = ChrList[character].model;

                if ( 0 == ( effects&DAMFX_BLOC ) )
                {
                    // Only damage if hitting from proper direction
                    if ( Md2FrameList[ChrList[character].inst.frame].framefx&MADFX_INVICTUS )
                    {
                        // I Frame...
                        direction -= CapList[model].iframefacing;
                        left = ( ~CapList[model].iframeangle );
                        right = CapList[model].iframeangle;

                        // Check for shield
                        if ( ChrList[character].action >= ACTION_PA && ChrList[character].action <= ACTION_PD )
                        {
                            // Using a shield?
                            if ( ChrList[character].action < ACTION_PC )
                            {
                                // Check left hand
                                if ( ChrList[character].holdingwhich[SLOT_LEFT] != MAX_CHR )
                                {
                                    left = ( ~CapList[ChrList[ChrList[character].holdingwhich[SLOT_LEFT]].model].iframeangle );
                                    right = CapList[ChrList[ChrList[character].holdingwhich[SLOT_LEFT]].model].iframeangle;
                                }
                            }
                            else
                            {
                                // Check right hand
                                if ( ChrList[character].holdingwhich[SLOT_RIGHT] != MAX_CHR )
                                {
                                    left = ( ~CapList[ChrList[ChrList[character].holdingwhich[SLOT_RIGHT]].model].iframeangle );
                                    right = CapList[ChrList[ChrList[character].holdingwhich[SLOT_RIGHT]].model].iframeangle;
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
                    if ( effects&DAMFX_ARMO )
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
                            spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].pos.z,
                                                ChrList[character].turnleftright + direction, ChrList[character].model, CapList[model].bludprttype,
                                                MAX_CHR, GRIP_LAST, ChrList[character].team, character, 0, MAX_CHR );
                        }

                        // Set attack alert if it wasn't an accident
                        if ( team == DAMAGETEAM )
                        {
                            ChrList[character].ai.attacklast = MAX_CHR;
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
                    action = ACTION_HA;
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
                        action = ACTION_KA;
                        // Give kill experience
                        experience = CapList[model].experienceworth + ( ChrList[character].experience * CapList[model].experienceexchange );
                        if ( attacker < MAX_CHR )
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

                        while ( tnc < MAX_CHR )
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

                            while ( tnc < MAX_CHR )
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
						stop_object_looped_sound( character );			//Stop sound loops

                        let_character_think( character );
                    }
                    else
                    {
                        if ( basedamage > HURTDAMAGE )
                        {
                            action += ( rand() & 3 );
                            chr_play_action( character, action, bfalse );

                            // Make the character invincible for a limited time only
                            if ( !( effects & DAMFX_TIME ) )
                                ChrList[character].damagetime = DAMAGETIME;
                        }
                    }
                }
                else
                {
                    // Spawn a defend particle
                    spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].pos.z, ChrList[character].turnleftright, MAX_PROFILE, DEFEND, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, 0, MAX_CHR );
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
                ChrList[character].ai.attacklast = MAX_CHR;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t stop_object_looped_sound( Uint16 character )
{
	//ZF> This makes a object stop playing it's looping sound
	if( INVALID_CHR( character ) || ChrList[character].loopedsound == INVALID_SOUND ) return bfalse;
	
	sound_stop_channel( ChrList[character].loopedsound );
	ChrList[character].loopedsound = INVALID_SOUND;
	return btrue;
}

//--------------------------------------------------------------------------------------------
void kill_character( Uint16 character, Uint16 killer )
{
    // ZZ> This function kills a character...  MAX_CHR killer for accidental death
    Uint8 modifier;
    chr_t * pchr;

    if( INVALID_CHR( character ) ) return;
    pchr = ChrList + character;

    if ( pchr->alive )
    {
        pchr->damagetime = 0;
        pchr->life = 1;
        modifier = pchr->damagemodifier[DAMAGE_CRUSH];
        pchr->damagemodifier[DAMAGE_CRUSH] = 1;

        if ( VALID_CHR( killer ) )
        {
            damage_character( character, 0, 512, 1, DAMAGE_CRUSH, ChrList[killer].team, killer, DAMFX_ARMO | DAMFX_BLOC, btrue );
        }
        else
        {
            damage_character( character, 0, 512, 1, DAMAGE_CRUSH, DAMAGETEAM, pchr->ai.bumplast, DAMFX_ARMO | DAMFX_BLOC, btrue );
        }

        pchr->damagemodifier[DAMAGE_CRUSH] = modifier;
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
        spawn_one_particle( ChrList[character].pos_old.x, ChrList[character].pos_old.y, ChrList[character].pos_old.z,
                            sTmp, profile, CapList[profile].gopoofprttype,
                            MAX_CHR, GRIP_LAST, ChrList[character].team, origin, iTmp, MAX_CHR );
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
    if ( NULL == pself || index >= MAX_CHR ) return;

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
    pself->attacklast = MAX_CHR;
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
    //     if it worked, MAX_CHR otherwise

    Uint16 ichr;
    int tnc;
    chr_t * pchr;
    cap_t * pcap;

    if ( profile >= MAX_PROFILE )
    {
        log_warning( "spawn_one_character() - profile value too large %d out of %d\n", profile, MAX_PROFILE );
        return MAX_CHR;
    }

    if ( !MadList[profile].used )
    {
        if ( profile > importamount * 9 )
        {
            log_warning( "spawn_one_character() - trying to spawn using invalid profile %d\n", profile );
        }

        return MAX_CHR;
    }

    // allocate a new character
    ichr = MAX_CHR;
    if ( override < MAX_CHR )
    {
        ichr = get_free_character();
        if ( ichr != override )
        {
            // Picked the wrong one, so put this one back and find the right one

            for ( tnc = 0; tnc < MAX_CHR; tnc++ )
            {
                if ( freechrlist[tnc] == override )
                {
                    freechrlist[tnc] = ichr;
                    break;
                }
            }

            ichr = override;
        }

        if ( MAX_CHR == ichr )
        {
            log_warning( "spawn_one_character() - failed to override a character? character %d already spawned? \n", override );
            return ichr;
        }
    }
    else
    {
        ichr = get_free_character();
        if ( MAX_CHR == ichr )
        {
            log_warning( "spawn_one_character() - failed to allocate a new character\n" );
            return ichr;
        }
    }

    if ( MAX_CHR == ichr )
    {
        log_warning( "spawn_one_character() - failed to spawn character\n" );
        return ichr;
    }

    pchr = ChrList + ichr;
    pcap = CapList + profile;

    // Make sure the team is valid
    team = MIN( team, MAXTEAM - 1 );

    // IMPORTANT!!!
    pchr->indolist = bfalse;
    pchr->isequipped = bfalse;
    pchr->sparkle = NOSPARKLE;
    pchr->overlay = bfalse;
    pchr->missilehandler = ichr;
	pchr->loopedsound = INVALID_SOUND;

    // sound stuff...  copy from the cap
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pchr->soundindex[tnc] = pcap->soundindex[tnc];
    }

    // Set up model stuff
    pchr->reloadtime = 0;
    pchr->inwhichhand = SLOT_LEFT;
    pchr->waskilled = bfalse;
    pchr->inpack = bfalse;
    pchr->nextinpack = MAX_CHR;
    pchr->numinpack = 0;
    pchr->model = profile;
    pchr->basemodel = profile;
    pchr->stoppedby = pcap->stoppedby;
    pchr->lifeheal = pcap->lifeheal;
    pchr->manacost = pcap->manacost;
    pchr->inwater = bfalse;
    pchr->nameknown = pcap->nameknown;
    pchr->ammoknown = pcap->nameknown;
    pchr->hitready = btrue;
    pchr->boretime = BORETIME;
    pchr->carefultime = CAREFULTIME;
    pchr->canbecrushed = bfalse;
    pchr->damageboost = 0;
    pchr->icon = pcap->icon;

    // Enchant stuff
    pchr->firstenchant = MAX_ENC;
    pchr->undoenchant = MAX_ENC;
    pchr->canseeinvisible = pcap->canseeinvisible;
    pchr->canchannel = bfalse;
    pchr->missiletreatment = MISNORMAL;
    pchr->missilecost = 0;

    //Skillz
    pchr->canjoust = pcap->canjoust;
    pchr->canuseadvancedweapons = pcap->canuseadvancedweapons;
    pchr->shieldproficiency = pcap->shieldproficiency;
    pchr->canusedivine = pcap->canusedivine;
    pchr->canusearcane = pcap->canusearcane;
    pchr->canusetech = pcap->canusetech;
    pchr->candisarm = pcap->candisarm;
    pchr->canbackstab = pcap->canbackstab;
    pchr->canusepoison = pcap->canusepoison;
    pchr->canread = pcap->canread;
    pchr->canseekurse = pcap->canseekurse;

    // Kurse state
    pchr->iskursed = ( ( rand() % 100 ) < pcap->kursechance );
    if ( !pcap->isitem )  pchr->iskursed = bfalse;

    // Ammo
    pchr->ammomax = pcap->ammomax;
    pchr->ammo = pcap->ammo;

    // Gender
    pchr->gender = pcap->gender;
    if ( pchr->gender == GENRANDOM )  pchr->gender = GENFEMALE + ( rand() & 1 );

    pchr->isplayer = bfalse;
    pchr->islocalplayer = bfalse;

    // AI stuff
    init_ai_state( &(pchr->ai), ichr, profile, pchr->model, TeamList[team].morale );

    // Team stuff
    pchr->team = team;
    pchr->baseteam = team;
    if ( !pcap->invictus )  TeamList[team].morale++;

    // Firstborn becomes the leader
    if ( TeamList[team].leader == NOLEADER )
    {
        TeamList[team].leader = ichr;
    }

    // Skin
    if ( pcap->skinoverride != NOSKINOVERRIDE )
    {
        skin = pcap->skinoverride % MAXSKIN;
    }
    if ( skin >= MadList[profile].skins )
    {
        skin = 0;
        if ( MadList[profile].skins > 1 )
        {
            skin = rand() % MadList[profile].skins;
        }
    }

    pchr->skin    = skin;

    // Life and Mana
    pchr->alive = btrue;
    pchr->lifecolor = pcap->lifecolor;
    pchr->manacolor = pcap->manacolor;
    pchr->lifemax = generate_number( pcap->lifebase, pcap->liferand );
    pchr->life = pchr->lifemax;
    pchr->lifereturn = pcap->lifereturn;
    pchr->manamax = generate_number( pcap->manabase, pcap->manarand );
    pchr->manaflow = generate_number( pcap->manaflowbase, pcap->manaflowrand );
    pchr->manareturn = generate_number( pcap->manareturnbase, pcap->manareturnrand );
    pchr->mana = pchr->manamax;

    // SWID
    pchr->strength = generate_number( pcap->strengthbase, pcap->strengthrand );
    pchr->wisdom = generate_number( pcap->wisdombase, pcap->wisdomrand );
    pchr->intelligence = generate_number( pcap->intelligencebase, pcap->intelligencerand );
    pchr->dexterity = generate_number( pcap->dexteritybase, pcap->dexterityrand );

    // Damage
    pchr->defense = pcap->defense[skin];
    pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;
    pchr->damagetargettype = pcap->damagetargettype;
    tnc = 0;

    while ( tnc < DAMAGE_COUNT )
    {
        pchr->damagemodifier[tnc] = pcap->damagemodifier[tnc][skin];
        tnc++;
    }

    //latches
    pchr->latchx = 0;
    pchr->latchy = 0;
    pchr->latchbutton = 0;

    pchr->turnmode = TURNMODEVELOCITY;

    // Flags
    pchr->stickybutt = pcap->stickybutt;
    pchr->openstuff = pcap->canopenstuff;
    pchr->transferblend = pcap->transferblend;
    pchr->waterwalk = pcap->waterwalk;
    pchr->platform = pcap->platform;
    pchr->isitem = pcap->isitem;
    pchr->invictus = pcap->invictus;
    pchr->ismount = pcap->ismount;
    pchr->cangrabmoney = pcap->cangrabmoney;

    // Jumping
    pchr->jump = pcap->jump;
    pchr->jumpnumber = 0;
    pchr->jumpnumberreset = pcap->jumpnumber;
    pchr->jumptime = JUMPDELAY;

    // Other junk
    pchr->flyheight = pcap->flyheight;
    pchr->maxaccel = pcap->maxaccel[skin];
    pchr->basealpha = pcap->alpha;
    pchr->flashand = pcap->flashand;
    pchr->dampen = pcap->dampen;

    // Character size and bumping
    pchr->fat = pcap->size;
    pchr->sizegoto = pchr->fat;
    pchr->sizegototime = 0;
    pchr->shadowsize = pcap->shadowsize * pchr->fat;
    pchr->bumpsize = pcap->bumpsize * pchr->fat;
    pchr->bumpsizebig = pcap->bumpsizebig * pchr->fat;
    pchr->bumpheight = pcap->bumpheight * pchr->fat;

    pchr->shadowsizesave = pcap->shadowsize;
    pchr->bumpsizesave = pcap->bumpsize;
    pchr->bumpsizebigsave = pcap->bumpsizebig;
    pchr->bumpheightsave = pcap->bumpheight;

    pchr->bumpdampen = pcap->bumpdampen;
    if ( pcap->weight == 0xFF )
    {
        pchr->weight = 0xFFFFFFFF;
    }
    else
    {
        int itmp = pcap->weight * pchr->fat * pchr->fat * pchr->fat;
        pchr->weight = MIN( itmp, 0xFFFFFFFE );
    }

    // Grip info
    pchr->attachedto = MAX_CHR;
    pchr->holdingwhich[SLOT_LEFT] = MAX_CHR;
    pchr->holdingwhich[SLOT_RIGHT] = MAX_CHR;

    // Image rendering
    pchr->uoffvel = pcap->uoffvel;
    pchr->voffvel = pcap->voffvel;

    // Movement
    pchr->sneakspd = pcap->sneakspd;
    pchr->walkspd = pcap->walkspd;
    pchr->runspd = pcap->runspd;

    // Set up position
    pchr->pos.x = x;
    pchr->pos.y = y;
    pchr->pos_old.x = x;
    pchr->pos_old.y = y;
    pchr->turnleftright = facing;
    pchr->lightturnleftright = 0;
    pchr->onwhichfan   = mesh_get_tile( PMesh, x, y );
    pchr->onwhichblock = mesh_get_block( PMesh, x, y );
    pchr->floor_level = get_level( PMesh, pchr->pos.x, pchr->pos.y, pchr->waterwalk ) + RAISE;
    pchr->phys_level = pchr->floor_level;
    if ( z < pchr->phys_level ) z = pchr->phys_level;

    pchr->pos.z = z;
    pchr->pos_old.z = z;
    pchr->pos_stt.x = pchr->pos.x;
    pchr->pos_stt.y = pchr->pos.y;
    pchr->pos_stt.z = pchr->pos.z;

    pchr->vel.x = 0;
    pchr->vel.y = 0;
    pchr->vel.z = 0;
    pchr->vel_old.x = 0;
    pchr->vel_old.y = 0;
    pchr->vel_old.z = 0;

    pchr->turnmaplr = 32768;  // These two mean on level surface
    pchr->turnmapud = 32768;

    // action stuff
    pchr->actionready = btrue;
    pchr->keepaction = bfalse;
    pchr->loopaction = bfalse;
    pchr->action = ACTION_DA;
    pchr->nextaction = ACTION_DA;


    pchr->holdingweight = 0;
    pchr->onwhichplatform = MAX_CHR;

    // Timers set to 0
    pchr->grogtime = 0;
    pchr->dazetime = 0;

    // Money is added later
    pchr->money = pcap->money;

    // Name the character
    if ( name == NULL )
    {
        // Generate a random name
        sprintf( pchr->name, "%s", chop_create( profile ) );
    }
    else
    {
        // A name has been given
        tnc = 0;

        while ( tnc < MAXCAPNAMESIZE - 1 )
        {
            pchr->name[tnc] = name[tnc];
            tnc++;
        }

        pchr->name[tnc] = 0;
    }

    // initalize the character instance
    {
        chr_instance_t * pinst = &(pchr->inst);

        pinst->imad = profile;
        pinst->texture = MadList[pinst->imad].skinstart + skin;
        pinst->enviro = pcap->enviro;
        pinst->alpha = pchr->basealpha;
        pinst->light = pcap->light;
        pinst->sheen = pcap->sheen;
        pinst->uoffset = 0;
        pinst->voffset = 0;
        pinst->redshift = 0;
        pinst->grnshift = 0;
        pinst->blushift = 0;

        pinst->light_turn_z = 0;
        pinst->lip = 0;
        pinst->frame = MadList[pinst->imad].md2.framestart;
        pinst->lastframe = pinst->frame;

        // Set up initial fade in lighting
        for ( tnc = 0; tnc < MadList[pinst->imad].transvertices; tnc++ )
        {
            pinst->vrta[tnc] = 0;
        }
    }



    // Particle attachments
    tnc = 0;
    while ( tnc < pcap->attachedprtamount )
    {
        spawn_one_particle( pchr->pos.x, pchr->pos.y, pchr->pos.z,
                            0, pchr->model, pcap->attachedprttype,
                            ichr, GRIP_LAST + tnc, pchr->team, ichr, tnc, MAX_CHR );
        tnc++;
    }

    pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;

    // Experience
    tnc = generate_number( pcap->experiencebase, pcap->experiencerand );
    if ( tnc > MAXXP ) tnc = MAXXP;

    pchr->experience = tnc;
    pchr->experiencelevel = pcap->leveloverride;

    //Items that are spawned inside shop passages are more expensive than normal
    if (pcap->isvaluable)
    {
        pchr->isshopitem = btrue;
    }
    else
    {
        pchr->isshopitem = bfalse;
        if (pchr->isitem && !pchr->inpack && pchr->attachedto == MAX_CHR)
        {
            float tlx, tly, brx, bry;
            Uint16 passage = 0;
            float bumpsize;

            bumpsize = pchr->bumpsize;

            while (passage < numpassage)
            {
                // Passage area
                tlx = ( passtlx[passage] << 7 ) - CLOSETOLERANCE;
                tly = ( passtly[passage] << 7 ) - CLOSETOLERANCE;
                brx = ( ( passbrx[passage] + 1 ) << 7 ) + CLOSETOLERANCE;
                bry = ( ( passbry[passage] + 1 ) << 7 ) + CLOSETOLERANCE;

                //Check if the character is inside that passage
                if ( pchr->pos.x > tlx - bumpsize && pchr->pos.x < brx + bumpsize )
                {
                    if ( pchr->pos.y > tly - bumpsize && pchr->pos.y < bry + bumpsize )
                    {
                        //Yep, flag as valuable (does not export)
                        pchr->isshopitem = btrue;
                        break;
                    }
                }

                passage++;
            }
        }
    }

    pchr->on = btrue;

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
    ChrList[character].pos.x = ChrList[character].pos_stt.x;
    ChrList[character].pos.y = ChrList[character].pos_stt.y;
    ChrList[character].pos.z = ChrList[character].pos_stt.z;
    ChrList[character].vel.x = 0;
    ChrList[character].vel.y = 0;
    ChrList[character].vel.z = 0;
    ChrList[character].team = ChrList[character].baseteam;
    ChrList[character].canbecrushed = bfalse;
    ChrList[character].turnmaplr = 32768;  // These two mean on level surface
    ChrList[character].turnmapud = 32768;
    if ( TeamList[ChrList[character].team].leader == NOLEADER )  TeamList[ChrList[character].team].leader = character;
    if ( !ChrList[character].invictus )  TeamList[ChrList[character].baseteam].morale++;

    ChrList[character].actionready = btrue;
    ChrList[character].keepaction = bfalse;
    ChrList[character].loopaction = bfalse;
    ChrList[character].action = ACTION_DA;
    ChrList[character].nextaction = ACTION_DA;

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
    for ( item = ChrList[character].nextinpack; item < MAX_CHR; item = ChrList[item].nextinpack )
    {
        if ( ChrList[item].on && ChrList[item].isequipped )
        {
            ChrList[item].isequipped = bfalse;
            ChrList[item].ai.alert |= ALERTIF_ATLASTWAYPOINT;  // doubles as PutAway
        }
    }

    // re-initialize the instance
    {
        ChrList[character].inst.lip       = 0;
        ChrList[character].inst.frame     = MadList[ChrList[character].inst.imad].md2.framestart;
        ChrList[character].inst.lastframe = ChrList[character].inst.frame;
    }

}

//--------------------------------------------------------------------------------------------
int chr_change_skin( Uint16 character, int skin )
{
    Uint16 model, imad;

    if( character >= MAX_CHR || !ChrList[character].on ) return 0;

    model = ChrList[character].model;
    if( model >= MAX_PROFILE || !MadList[model].used ) 
    {
        ChrList[character].skin    = 0;
        ChrList[character].inst.texture = TX_WATER_TOP;
        return 0;
    }

    // make sure that the instance has a valid model
    imad = ChrList[character].inst.imad;
    if( imad >= MAX_PROFILE || !MadList[imad].used )
    {
        imad = model;
        ChrList[character].inst.imad = model;
    }

    // do the best we can to change the skin
    if( MadList[imad].skins == 0 )
    {
        ChrList[character].skin    = 0;
        ChrList[character].inst.texture = TX_WATER_TOP;
    }
    else
    {
        // limit the skin
        if( skin > MadList[imad].skins) skin = 0;

        ChrList[character].skin         = skin;
        ChrList[character].inst.texture = MadList[imad].skinstart + skin;
    }

    return ChrList[character].skin;
};

//--------------------------------------------------------------------------------------------
Uint16 change_armor( Uint16 character, Uint16 skin )
{
    // ZZ> This function changes the armor of the character

    Uint16 enchant, sTmp;
    int iTmp;

    // Remove armor enchantments
    enchant = ChrList[character].firstenchant;

    while ( enchant < MAX_ENC )
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
    skin = chr_change_skin( character, skin );

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

    while ( enchant < MAX_ENC )
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
void change_character( Uint16 ichr, Uint16 profile, Uint8 skin, Uint8 leavewhich )
{
    // ZZ> This function polymorphs a character, changing stats, dropping weapons
    int tnc, enchant;
    Uint16 sTmp, item;

    if ( profile > MAX_PROFILE || !MadList[profile].used ) return;

    // Drop left weapon
    sTmp = ChrList[ichr].holdingwhich[SLOT_LEFT];
    if ( sTmp != MAX_CHR && ( !CapList[profile].slotvalid[SLOT_LEFT] || CapList[profile].ismount ) )
    {
        detach_character_from_mount( sTmp, btrue, btrue );
        if ( ChrList[ichr].ismount )
        {
            ChrList[sTmp].vel.z = DISMOUNTZVEL;
            ChrList[sTmp].pos.z += DISMOUNTZVEL;
            ChrList[sTmp].jumptime = JUMPDELAY;
        }
    }

    // Drop right weapon
    sTmp = ChrList[ichr].holdingwhich[SLOT_RIGHT];
    if ( sTmp != MAX_CHR && !CapList[profile].slotvalid[SLOT_RIGHT] )
    {
        detach_character_from_mount( sTmp, btrue, btrue );
        if ( ChrList[ichr].ismount )
        {
            ChrList[sTmp].vel.z = DISMOUNTZVEL;
            ChrList[sTmp].pos.z += DISMOUNTZVEL;
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
        if ( enchant != MAX_ENC )
        {
            while ( EncList[enchant].nextenchant != MAX_ENC )
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
        ChrList[ichr].weight = 0xFFFFFFFF;
    }
    else
    {
        int itmp = CapList[profile].weight * ChrList[ichr].fat * ChrList[ichr].fat * ChrList[ichr].fat;
        ChrList[ichr].weight = MIN( itmp, 0xFFFFFFFE );
    }

    // Character scales...  Magic numbers
    if ( ChrList[ichr].attachedto != MAX_CHR )
    {
        int i;
        Uint16 iholder = ChrList[ichr].attachedto;
        tnc = MadList[ChrList[iholder].inst.imad].md2.vertices - (ChrList[ichr].inwhichhand + 1) * GRIP_VERTS;

        for (i = 0; i < GRIP_VERTS; i++)
        {
            if (tnc + i < MadList[ChrList[iholder].inst.imad].md2.vertices )
            {
                ChrList[ichr].weapongrip[i] = tnc + i;
            }
            else
            {
                ChrList[ichr].weapongrip[i] = 0xFFFF;
            }
        }
    }

    item = ChrList[ichr].holdingwhich[SLOT_LEFT];
    if ( item != MAX_CHR )
    {
        int i;

        tnc = MadList[ChrList[ichr].inst.imad].md2.vertices - GRIP_LEFT;

        for (i = 0; i < GRIP_VERTS; i++)
        {
            if (tnc + i < MadList[ChrList[ichr].inst.imad].md2.vertices )
            {
                ChrList[item].weapongrip[i] = i + tnc;
            }
            else
            {
                ChrList[item].weapongrip[i] = 0xFFFF;
            }
        }
    }

    item = ChrList[ichr].holdingwhich[SLOT_RIGHT];
    if ( item != MAX_CHR )
    {
        int i;

        tnc = MadList[ChrList[ichr].inst.imad].md2.vertices - GRIP_RIGHT;

        for (i = 0; i < GRIP_VERTS; i++)
        {
            if (tnc + i < MadList[ChrList[ichr].inst.imad].md2.vertices )
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
    ChrList[ichr].action = ACTION_DA;
    ChrList[ichr].nextaction = ACTION_DA;
    ChrList[ichr].holdingweight = 0;
    ChrList[ichr].onwhichplatform = MAX_CHR;

    // initialize the instance
    {
        chr_instance_t * pinst = &(ChrList[ichr].inst);

        pinst->imad    = profile;
        pinst->texture = MadList[pinst->imad].skinstart + ChrList[ichr].skin;
        pinst->enviro  = CapList[pinst->imad].enviro;
        pinst->uoffset = 0;
        pinst->voffset = 0;

        pinst->lip       = 0;
        pinst->frame     = MadList[pinst->imad].md2.framestart;
        pinst->lastframe = pinst->frame;

        // Set up initial fade in lighting
        for ( tnc = 0; tnc < MadList[pinst->imad].transvertices; tnc++ )
        {
            pinst->vrta[tnc] = 0;
        }
    }

    // Set the skin
    change_armor( ichr, skin );

    // Reaffirm them particles...
    ChrList[ichr].reaffirmdamagetype = CapList[profile].attachedprtreaffirmdamagetype;
    reaffirm_attached_particles( ichr );
}

//--------------------------------------------------------------------------------------------
/*Uint16 get_target_in_block( int x, int y, Uint16 character, char items,
                            char friends, char enemies, char dead, char seeinvisible, IDSZ idsz,
                            char excludeid )
{
  // ZZ> This is a good little helper, that returns != MAX_CHR if a suitable target
  //     was found
  int cnt;
  Uint16 charb;
  Uint32 fanblock;
  Uint8 team;
  if ( x >= 0 && x < meshbloksx && y >= 0 && y < meshbloksy )
  {
    team = ChrList[character].team;
    fanblock = mesh_get_block_int(PMesh, x,y);
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
  return MAX_CHR;
}*/

//--------------------------------------------------------------------------------------------
/*Uint16 get_nearby_target( Uint16 character, char items,
                          char friends, char enemies, char dead, IDSZ idsz )
{
  // ZZ> This function finds a nearby target, or it returns MAX_CHR if it can't find one
  int x, y;
  char seeinvisible;
  seeinvisible = ChrList[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )ChrList[character].pos.x ) >> 9;
  y = ( ( int )ChrList[character].pos.y ) >> 9;
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
                kill_character( character, (killer == MAX_CHR) ? character : killer );
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
  // ZZ> This function finds a target, or it returns MAX_CHR if it can't find one...
  //     maxdistance should be the square of the actual distance you want to use
  //     as the cutoff...
  int cnt, distance, xdistance, ydistance;
  Uint8 team;

  team = ChrList[character].team;
  cnt = 0;
  while ( cnt < MAX_CHR )
  {
    if ( ChrList[cnt].on )
    {
      if ( ChrList[cnt].attachedto == MAX_CHR && !ChrList[cnt].inpack )
      {
        if ( TeamList[team].hatesteam[ChrList[cnt].team] && ChrList[cnt].alive && !ChrList[cnt].invictus )
        {
          if ( ChrList[character].canseeinvisible || ( ChrList[cnt].alpha > INVISIBLE && ChrList[cnt].light > INVISIBLE ) )
          {
            xdistance = (int) (ChrList[cnt].pos.x - ChrList[character].pos.x);
            ydistance = (int) (ChrList[cnt].pos.y - ChrList[character].pos.y);
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
  return MAX_CHR;
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
        if ( ( !ChrList[character].ismount || ChrList[character].holdingwhich[SLOT_LEFT] == MAX_CHR ) &&
                ( !ChrList[character].isitem || ChrList[character].attachedto == MAX_CHR ) )
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
    fanblock = mesh_get_block_int(PMesh, x,y);
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
          if ( charb != character && ChrList[character].attachedto != charb && ChrList[charb].attachedto == MAX_CHR && !ChrList[charb].inpack )
          {
            if ( !ChrList[charb].invictus || items )
            {
              if ( idsz != IDSZ_NONE )
              {
                if ( CapList[ChrList[charb].model].idsz[IDSZ_PARENT] == idsz ||
                     CapList[ChrList[charb].model].idsz[IDSZ_TYPE] == idsz )
                {
                  xdis = ChrList[character].pos.x - ChrList[charb].pos.x;
                  ydis = ChrList[character].pos.y - ChrList[charb].pos.y;
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
                xdis = ChrList[character].pos.x - ChrList[charb].pos.x;
                ydis = ChrList[character].pos.y - ChrList[charb].pos.y;
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
  // ZZ> This function finds a target, or it returns MAX_CHR if it can't find one
  int x, y;
  char seeinvisible;
  seeinvisible = ChrList[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )ChrList[character].pos.x ) >> 9;
  y = ( ( int )ChrList[character].pos.y ) >> 9;

  globalnearest = MAX_CHR;
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
  // ZZ> This function finds a target, or it returns MAX_CHR if it can't find one
  int x, y;
  Uint16 enemy;
  char seeinvisible;
  seeinvisible = ChrList[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )ChrList[character].pos.x ) >> 9;
  y = ( ( int )ChrList[character].pos.y ) >> 9;
  enemy = get_target_in_block( x, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAX_CHR )  return enemy;

  enemy = get_target_in_block( x - 1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAX_CHR )  return enemy;
  enemy = get_target_in_block( x + 1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAX_CHR )  return enemy;
  enemy = get_target_in_block( x, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAX_CHR )  return enemy;
  enemy = get_target_in_block( x, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAX_CHR )  return enemy;

  enemy = get_target_in_block( x - 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAX_CHR )  return enemy;
  enemy = get_target_in_block( x + 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAX_CHR )  return enemy;
  enemy = get_target_in_block( x - 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAX_CHR )  return enemy;
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
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ChrList[cnt].on || team != ChrList[cnt].team ) continue;

        if( !ChrList[cnt].alive ) 
        {
            ChrList[cnt].ai.timer  = frame_wld + 2;  // Don't let it think too much...
        }

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
    if ( character < MAX_CHR )
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
    snprintf(newloadname, sizeof(newloadname), "players/%s/quest.txt", str_encode_path(whichplayer) );
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
        snprintf( newloadname, sizeof( newloadname ), "players/%s/quest.txt", str_encode_path(whichplayer));
        snprintf( copybuffer, sizeof( copybuffer ), "players/%s/tmp_quest.txt", str_encode_path(whichplayer));
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
            else if ( goto_colon( NULL, fileread, btrue ) )
            {
                // scan the line for quest info
                newidsz = fget_idsz( fileread );
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

    snprintf( newloadname, sizeof(newloadname), "players/%s/quest.txt", str_encode_path(whichplayer) );
    fileread = fopen( newloadname, "r" );
    if ( NULL == fileread ) return result;

    //Always return "true" for [NONE] IDSZ checks
    if (idsz == IDSZ_NONE) result = QUEST_BEATEN;

    // Check each expansion
    while ( !foundidsz && goto_colon( NULL, fileread, btrue ) )
    {
        newidsz = fget_idsz( fileread );
        if ( newidsz == idsz )
        {
            foundidsz = btrue;
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
    float level, friction_xy, friction_z;
    float dvx, dvy, dvmax;
    Uint16 action, weapon, mount, item;
    bool_t watchtarget;

    // Move every character
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        float zlerp;

        chr_t * pchr;

        if ( !ChrList[cnt].on || ChrList[cnt].inpack ) continue;
        pchr = ChrList + cnt;

        // Down that ol' damage timer
        pchr->damagetime -= ( pchr->damagetime != 0 );

        // Character's old location
        pchr->pos_old.x = pchr->pos.x;
        pchr->pos_old.y = pchr->pos.y;
        pchr->pos_old.z = pchr->pos.z;

        pchr->vel_old.x = pchr->vel.x;
        pchr->vel_old.y = pchr->vel.y;
        pchr->vel_old.z = pchr->vel.z;

        pchr->oldturn = pchr->turnleftright;

        // Texture movement
        pchr->inst.uoffset += pchr->uoffvel;
        pchr->inst.voffset += pchr->voffvel;
        if ( pchr->alive )
        {
            if ( pchr->attachedto == MAX_CHR )
            {
                float new_vx, new_vy;
                float new_ax, new_ay;

                // Character latches for generalized movement
                dvx = pchr->latchx;
                dvy = pchr->latchy;

                // Reverse movements for daze
                if ( pchr->dazetime > 0 )
                {
                    dvx = -dvx;
                    dvy = -dvy;
                }

                // Switch x and y for grog
                if ( pchr->grogtime > 0 )
                {
					float savex;
                    savex = dvx;
                    dvx = dvy;
                    dvy = savex;
                }

                new_vx = dvx * airfriction / (1.0f - airfriction);
                new_vy = dvy * airfriction / (1.0f - airfriction);

                new_ax = new_vx - pchr->vel.x;
                new_ay = new_vy - pchr->vel.y;

                dvmax = pchr->maxaccel;
                if ( new_ax < -dvmax ) new_ax = -dvmax;
                if ( new_ax >  dvmax ) new_ax =  dvmax;
                if ( new_ay < -dvmax ) new_ay = -dvmax;
                if ( new_ay >  dvmax ) new_ay =  dvmax;

                // Get direction from the DESIRED change in velocity
                if ( pchr->turnmode == TURNMODEWATCH )
                {
                    if ( ( ABS( dvx ) > WATCHMIN || ABS( dvy ) > WATCHMIN ) )
                    {
                        pchr->turnleftright = terp_dir( pchr->turnleftright, ( ATAN2( dvx, dvy ) + PI ) * 0xFFFF / ( TWO_PI ) );
                    }
                }

                // Face the target
                watchtarget = ( pchr->turnmode == TURNMODEWATCHTARGET );
                if ( watchtarget )
                {
                    if ( cnt != pchr->ai.target )
                    {
                        pchr->turnleftright = terp_dir( pchr->turnleftright, ( ATAN2( ChrList[pchr->ai.target].pos.y - pchr->pos.y, ChrList[pchr->ai.target].pos.x - pchr->pos.x ) + PI ) * 0xFFFF / ( TWO_PI ) );
                    }
                }

                if ( Md2FrameList[pchr->inst.frame].framefx&MADFX_STOP )
                {
                    new_ax = 0;
                    new_ay = 0;
                }
                else
                {
                    // Limit to max acceleration
                    pchr->vel.x += new_ax;
                    pchr->vel.y += new_ay;
                }

                // Get direction from ACTUAL change in velocity
                if ( pchr->turnmode == TURNMODEVELOCITY )
                {
                    if ( dvx < -TURNSPD || dvx > TURNSPD || dvy < -TURNSPD || dvy > TURNSPD )
                    {
                        if ( pchr->isplayer )
                        {
                            // Players turn quickly
                            pchr->turnleftright = terp_dir_fast( pchr->turnleftright, ( ATAN2( dvy, dvx ) + PI ) * 0xFFFF / ( TWO_PI ) );
                        }
                        else
                        {
                            // AI turn slowly
                            pchr->turnleftright = terp_dir( pchr->turnleftright, ( ATAN2( dvy, dvx ) + PI ) * 0xFFFF / ( TWO_PI ) );
                        }
                    }
                }

                // Otherwise make it spin
                else if ( pchr->turnmode == TURNMODESPIN )
                {
                    pchr->turnleftright += SPINRATE;
                }
            }

            // Character latches for generalized buttons
            if ( pchr->latchbutton != 0 )
            {
                if ( 0 != (pchr->latchbutton & LATCHBUTTON_JUMP) )
                {
                    if ( pchr->attachedto != MAX_CHR && pchr->jumptime == 0 )
                    {
                        int ijump;

                        detach_character_from_mount( cnt, btrue, btrue );
                        pchr->jumptime = JUMPDELAY;
                        pchr->vel.z = DISMOUNTZVEL;
                        if ( pchr->flyheight != 0 )
                            pchr->vel.z = DISMOUNTZVELFLY;

                        pchr->pos.z += pchr->vel.z;
                        if ( pchr->jumpnumberreset != JUMPINFINITE && pchr->jumpnumber != 0 )
                            pchr->jumpnumber--;

                        // Play the jump sound
                        ijump = CapList[pchr->model].soundindex[SOUND_JUMP];
                        if ( ijump >= 0 && ijump < MAX_WAVE )
                        {
                            sound_play_chunk( pchr->pos.x, pchr->pos.y, CapList[pchr->model].wavelist[ijump] );
                        }

                    }
                    if ( pchr->jumptime == 0 && pchr->jumpnumber != 0 && pchr->flyheight == 0 )
                    {
                        if ( pchr->jumpnumberreset != 1 || pchr->jumpready )
                        {
                            // Make the character jump
                            pchr->hitready = btrue;
                            if ( pchr->inwater )
                            {
                                pchr->vel.z = WATERJUMP * 1.2;
                            }
                            else
                            {
                                pchr->vel.z = pchr->jump * 1.2;
                            }

                            pchr->jumptime = JUMPDELAY;
                            pchr->jumpready = bfalse;
                            if ( pchr->jumpnumberreset != JUMPINFINITE ) pchr->jumpnumber--;

                            // Set to jump animation if not doing anything better
                            if ( pchr->actionready )    chr_play_action( cnt, ACTION_JA, btrue );

                            // Play the jump sound (Boing!)
                            {
                                int ijump = CapList[pchr->model].soundindex[SOUND_JUMP];
                                if ( ijump >= 0 && ijump < MAX_WAVE )
                                {
                                    sound_play_chunk( pchr->pos.x, pchr->pos.y, CapList[pchr->model].wavelist[ijump] );
                                }
                            }

                        }
                    }
                }
                if ( 0 != ( pchr->latchbutton & LATCHBUTTON_ALTLEFT ) && pchr->actionready && pchr->reloadtime == 0 )
                {
                    pchr->reloadtime = GRABDELAY;
                    if ( pchr->holdingwhich[SLOT_LEFT] == MAX_CHR )
                    {
                        // Grab left
                        chr_play_action( cnt, ACTION_ME, bfalse );
                    }
                    else
                    {
                        // Drop left
                        chr_play_action( cnt, ACTION_MA, bfalse );
                    }
                }
                if ( 0 != ( pchr->latchbutton & LATCHBUTTON_ALTRIGHT ) && pchr->actionready && pchr->reloadtime == 0 )
                {
                    pchr->reloadtime = GRABDELAY;
                    if ( pchr->holdingwhich[SLOT_RIGHT] == MAX_CHR )
                    {
                        // Grab right
                        chr_play_action( cnt, ACTION_MF, bfalse );
                    }
                    else
                    {
                        // Drop right
                        chr_play_action( cnt, ACTION_MB, bfalse );
                    }
                }
                if ( 0 != ( pchr->latchbutton & LATCHBUTTON_PACKLEFT ) && pchr->actionready && pchr->reloadtime == 0 )
                {
                    pchr->reloadtime = PACKDELAY;
                    item = pchr->holdingwhich[SLOT_LEFT];
                    if ( item != MAX_CHR )
                    {
                        if ( ( ChrList[item].iskursed || CapList[ChrList[item].model].istoobig ) && !CapList[ChrList[item].model].isequipment )
                        {
							// The item couldn't be put away
                            ChrList[item].ai.alert |= ALERTIF_NOTPUTAWAY;
							if( pchr->isplayer && CapList[ChrList[item].model].istoobig )
							{
								STRING text;
                            	snprintf( text, sizeof(text), "The %s is too big to be put away...", CapList[ChrList[item].model].classname);
								debug_message( text );
							}
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
                    chr_play_action( cnt, ACTION_MG, bfalse );
                }
                if ( 0 != ( pchr->latchbutton & LATCHBUTTON_PACKRIGHT ) && pchr->actionready && pchr->reloadtime == 0 )
                {
                    pchr->reloadtime = PACKDELAY;
                    item = pchr->holdingwhich[SLOT_RIGHT];
                    if ( item != MAX_CHR )
                    {
                        if ( ( ChrList[item].iskursed || CapList[ChrList[item].model].istoobig ) && !CapList[ChrList[item].model].isequipment )
                        {
							// The item couldn't be put away
                            ChrList[item].ai.alert |= ALERTIF_NOTPUTAWAY;
							if( pchr->isplayer && CapList[ChrList[item].model].istoobig )
							{
								STRING text;
                            	snprintf( text, sizeof(text), "The %s is too big to be put away...", CapList[ChrList[item].model].classname);
								debug_message( text );
							}
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
                    chr_play_action( cnt, ACTION_MG, bfalse );
                }
                if ( 0 != ( pchr->latchbutton & LATCHBUTTON_LEFT ) && pchr->reloadtime == 0 )
                {
                    // Which weapon?
                    weapon = pchr->holdingwhich[SLOT_LEFT];
                    if ( weapon == MAX_CHR )
                    {
                        // Unarmed means character itself is the weapon
                        weapon = cnt;
                    }

                    action = CapList[ChrList[weapon].model].weaponaction;

                    // Can it do it?
                    allowedtoattack = btrue;

                    // First check if reload time and action is okay
                    if ( !MadList[pchr->inst.imad].actionvalid[action] || ChrList[weapon].reloadtime > 0 ) allowedtoattack = bfalse;
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
                            if ( pchr->staton )
                            {
                                // Tell the player that they can't use this weapon
                                STRING text;
                                snprintf( text, sizeof(text),  "%s can't use this item...", pchr->name );
                                debug_message( text );
                            }
                        }
                    }
                    if ( action == ACTION_DA )
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
                        mount = pchr->attachedto;
                        if ( mount != MAX_CHR )
                        {
                            allowedtoattack = CapList[ChrList[mount].model].ridercanattack;
                            if ( ChrList[mount].ismount && ChrList[mount].alive && !ChrList[mount].isplayer && ChrList[mount].actionready )
                            {
                                if ( ( action != ACTION_PA || !allowedtoattack ) && pchr->actionready )
                                {
                                    chr_play_action( pchr->attachedto, ACTION_UA + ( rand()&1 ), bfalse );
                                    ChrList[pchr->attachedto].ai.alert |= ALERTIF_USED;
                                    pchr->ai.lastitemused = mount;
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
                            if ( pchr->actionready && MadList[pchr->inst.imad].actionvalid[action] )
                            {
                                // Check mana cost
                                if ( pchr->mana >= ChrList[weapon].manacost || pchr->canchannel )
                                {
                                    cost_mana( cnt, ChrList[weapon].manacost, weapon );
                                    // Check life healing
                                    pchr->life += ChrList[weapon].lifeheal;
                                    if ( pchr->life > pchr->lifemax )  pchr->life = pchr->lifemax;

                                    actionready = bfalse;
                                    if ( action == ACTION_PA )
                                        actionready = btrue;

                                    action += rand() & 1;
                                    chr_play_action( cnt, action, actionready );
                                    if ( weapon != cnt )
                                    {
                                        // Make the weapon attack too
                                        chr_play_action( weapon, ACTION_MJ, bfalse );
                                        ChrList[weapon].ai.alert |= ALERTIF_USED;
                                        pchr->ai.lastitemused = weapon;
                                    }
                                    else
                                    {
                                        // Flag for unarmed attack
                                        pchr->ai.alert |= ALERTIF_USED;
                                        pchr->ai.lastitemused = cnt;
                                    }
                                }
                            }
                        }
                    }
                }
                else if ( 0 != (pchr->latchbutton & LATCHBUTTON_RIGHT) && pchr->reloadtime == 0 )
                {
                    // Which weapon?
                    weapon = pchr->holdingwhich[SLOT_RIGHT];
                    if ( weapon == MAX_CHR )
                    {
                        // Unarmed means character itself is the weapon
                        weapon = cnt;
                    }

                    action = CapList[ChrList[weapon].model].weaponaction + 2;

                    // Can it do it? (other hand)
                    allowedtoattack = btrue;

                    // First check if reload time and action is okay
                    if ( !MadList[pchr->inst.imad].actionvalid[action] || ChrList[weapon].reloadtime > 0 ) allowedtoattack = bfalse;
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
                            if ( pchr->staton )
                            {
                                // Tell the player that they can't use this weapon
                                STRING text;
                                snprintf( text, sizeof(text), "%s can't use this item...", pchr->name );
                                debug_message( text );
                            }
                        }
                    }
                    if ( action == ACTION_DC )
                    {
                        allowedtoattack = bfalse;
                        if ( ChrList[weapon].reloadtime == 0 )
                        {
                            ChrList[weapon].ai.alert |= ALERTIF_USED;
                            pchr->ai.lastitemused = weapon;
                        }
                    }
                    if ( allowedtoattack )
                    {
                        // Rearing mount
                        mount = pchr->attachedto;
                        if ( mount != MAX_CHR )
                        {
                            allowedtoattack = CapList[ChrList[mount].model].ridercanattack;
                            if ( ChrList[mount].ismount && ChrList[mount].alive && !ChrList[mount].isplayer && ChrList[mount].actionready )
                            {
                                if ( ( action != ACTION_PC || !allowedtoattack ) && pchr->actionready )
                                {
                                    chr_play_action( pchr->attachedto, ACTION_UC + ( rand()&1 ), bfalse );
                                    ChrList[pchr->attachedto].ai.alert |= ALERTIF_USED;
                                    pchr->ai.lastitemused = pchr->attachedto;
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
                            if ( pchr->actionready && MadList[pchr->inst.imad].actionvalid[action] )
                            {
                                // Check mana cost
                                if ( pchr->mana >= ChrList[weapon].manacost || pchr->canchannel )
                                {
                                    cost_mana( cnt, ChrList[weapon].manacost, weapon );
                                    // Check life healing
                                    pchr->life += ChrList[weapon].lifeheal;
                                    if ( pchr->life > pchr->lifemax )  pchr->life = pchr->lifemax;

                                    actionready = bfalse;
                                    if ( action == ACTION_PC )
                                        actionready = btrue;

                                    action += rand() & 1;
                                    chr_play_action( cnt, action, actionready );
                                    if ( weapon != cnt )
                                    {
                                        // Make the weapon attack too
                                        chr_play_action( weapon, ACTION_MJ, bfalse );
                                        ChrList[weapon].ai.alert |= ALERTIF_USED;
                                        pchr->ai.lastitemused = weapon;
                                    }
                                    else
                                    {
                                        // Flag for unarmed attack
                                        pchr->ai.alert |= ALERTIF_USED;
                                        pchr->ai.lastitemused = cnt;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Is the character in the air?
        level = pchr->phys_level;
        zlerp = (pchr->pos.z - level) / PLATTOLERANCE;
        zlerp = CLIP(zlerp, 0, 1);

        pchr->phys_grounded = (pchr->flyheight == 0) && ((pchr->pos.z - pchr->phys_level) < PLATTOLERANCE / 4);

        friction_z = pchr->inwater ? waterfriction : 0.9868;
        if ( pchr->flyheight == 0 )
        {
            pchr->pos.z += pchr->vel.z;
            if ( pchr->pos.z > level || ( pchr->vel.z > STOPBOUNCING && pchr->pos.z > level - STOPBOUNCING ) )
            {
                // Character is in the air
                pchr->jumpready = bfalse;
                pchr->vel.z += gravity * zlerp;

                // Down jump timers for flapping characters
                if ( pchr->jumptime != 0 ) pchr->jumptime--;

                // Airborne characters still get friction_xy to make control easier
                friction_xy = airfriction;
            }
            else
            {
                // Character is on the ground
                pchr->pos.z = level;
                if ( pchr->hitready )
                {
                    pchr->ai.alert |= ALERTIF_HITGROUND;
                    pchr->hitready = bfalse;
                }

                // Make the characters slide
                twist = 119;
                friction_xy = noslipfriction;
                if ( INVALID_TILE != pchr->onwhichfan )
                {
                    Uint32 itile = pchr->onwhichfan;

                    twist = PMesh->mem.tile_list[itile].twist;

                    if ( 0 != ( PMesh->mem.tile_list[itile].fx & MESHFX_SLIPPY ) )
                    {
                        if ( wateriswater && 0 != ( PMesh->mem.tile_list[itile].fx & MESHFX_WATER ) && pchr->floor_level < watersurfacelevel + RAISE + 1 )
                        {
                            // It says it's slippy, but the water is covering it...
                            // Treat exactly as normal
                            pchr->jumpready = btrue;
                            pchr->jumpnumber = pchr->jumpnumberreset;
                            if ( pchr->jumptime > 0 ) pchr->jumptime--;

                            pchr->vel.z *= -pchr->dampen;
                            pchr->vel.z += gravity * zlerp;
                        }
                        else
                        {
                            // It's slippy all right...
                            friction_xy = slippyfriction;
                            pchr->jumpready = btrue;
                            if ( pchr->jumptime > 0 ) pchr->jumptime--;
                            if ( pchr->weight != 0xFFFFFFFF )
                            {
                                // Slippy hills make characters slide
                                pchr->vel.x += vellrtwist[twist];
                                pchr->vel.y += veludtwist[twist];
                                pchr->vel.z = -SLIDETOLERANCE;
                            }
                            if ( flattwist[twist] )
                            {
                                // Reset jumping on flat areas of slippiness
                                pchr->jumpnumber = pchr->jumpnumberreset;
                            }
                        }
                    }
                    else
                    {
                        // Reset jumping
                        pchr->jumpready = btrue;
                        pchr->jumpnumber = pchr->jumpnumberreset;
                        if ( pchr->jumptime > 0 ) pchr->jumptime--;

                        pchr->vel.z *= -pchr->dampen;
                        pchr->vel.z += gravity * zlerp;
                    }
                }

                // Characters with sticky butts lie on the surface of the mesh
                if ( pchr->stickybutt || !pchr->alive )
                {
                    maplr = pchr->turnmaplr;
                    maplr = ( maplr << 3 ) - maplr + maplrtwist[twist];
                    mapud = pchr->turnmapud;
                    mapud = ( mapud << 3 ) - mapud + mapudtwist[twist];
                    pchr->turnmaplr = maplr >> 3;
                    pchr->turnmapud = mapud >> 3;
                }
            }
        }
        else
        {
            //  Flying
            pchr->jumpready = bfalse;
            pchr->pos.z += pchr->vel.z;
            if ( level < 0 ) level = 0;  // Don't fall in pits...

            pchr->vel.z += ( level + pchr->flyheight - pchr->pos.z ) * FLYDAMPEN;
            if ( pchr->pos.z < level )
            {
                pchr->pos.z = level;
                pchr->vel.z = 0;
            }

            // Airborne characters still get friction_xy to make control easier
            friction_xy = airfriction;
            friction_z = 1.0f;
        }

        // Move the character
        pchr->pos.x += pchr->vel.x;
        if ( __chrhitawall( cnt ) ) { pchr->pos.x = pchr->pos_old.x; pchr->vel.x = -pchr->vel.x; }

        pchr->pos.y += pchr->vel.y;
        if ( __chrhitawall( cnt ) ) { pchr->pos.y = pchr->pos_old.y; pchr->vel.y = -pchr->vel.y; }

        // Apply friction for next time
        pchr->vel.x *= friction_xy;
        pchr->vel.y *= friction_xy;
        pchr->vel.z *= friction_z;

        // Animate the character
        pchr->inst.lip = ( pchr->inst.lip + 64 );
        if ( pchr->inst.lip == 192 )
        {
            // Check frame effects
            if ( Md2FrameList[pchr->inst.frame].framefx&MADFX_ACTLEFT )
                character_swipe( cnt, 0 );
            if ( Md2FrameList[pchr->inst.frame].framefx&MADFX_ACTRIGHT )
                character_swipe( cnt, 1 );
            if ( Md2FrameList[pchr->inst.frame].framefx&MADFX_GRABLEFT )
                character_grab_stuff( cnt, GRIP_LEFT, bfalse );
            if ( Md2FrameList[pchr->inst.frame].framefx&MADFX_GRABRIGHT )
                character_grab_stuff( cnt, GRIP_RIGHT, bfalse );
            if ( Md2FrameList[pchr->inst.frame].framefx&MADFX_CHARLEFT )
                character_grab_stuff( cnt, GRIP_LEFT, btrue );
            if ( Md2FrameList[pchr->inst.frame].framefx&MADFX_CHARRIGHT )
                character_grab_stuff( cnt, GRIP_RIGHT, btrue );
            if ( Md2FrameList[pchr->inst.frame].framefx&MADFX_DROPLEFT )
                detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], bfalse, btrue );
            if ( Md2FrameList[pchr->inst.frame].framefx&MADFX_DROPRIGHT )
                detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], bfalse, btrue );
            if ( Md2FrameList[pchr->inst.frame].framefx&MADFX_POOF && !pchr->isplayer )
                pchr->ai.poof_time = frame_wld;
            if ( Md2FrameList[pchr->inst.frame].framefx&MADFX_FOOTFALL )
            {
                int ifoot = CapList[pchr->model].soundindex[SOUND_FOOTFALL];
                if ( ifoot >= 0 && ifoot < MAX_WAVE )
                {
                    sound_play_chunk( pchr->pos.x, pchr->pos.y, CapList[pchr->model].wavelist[ifoot] );
                }
            }
        }

        if ( pchr->inst.lip == 0 )
        {
            // Change frames
            pchr->inst.lastframe = pchr->inst.frame;
            pchr->inst.frame++;
            if ( pchr->inst.frame == MadList[pchr->inst.imad].actionend[pchr->action] )
            {
                // Action finished
                if ( pchr->keepaction )
                {
                    // Keep the last frame going
                    pchr->inst.frame = pchr->inst.lastframe;
                }
                else
                {
                    if ( !pchr->loopaction )
                    {
                        // Go on to the next action
                        pchr->action = pchr->nextaction;
                        pchr->nextaction = ACTION_DA;
                    }
                    else
                    {
                        // See if the character is mounted...
                        if ( pchr->attachedto != MAX_CHR )
                        {
                            pchr->action = ACTION_MI;
                        }
                    }

                    pchr->inst.frame = MadList[pchr->inst.imad].actionstart[pchr->action];
                }

                pchr->actionready = btrue;
            }
        }

        // Do "Be careful!" delay
        if ( pchr->carefultime > 0 )
        {
            pchr->carefultime--;
        }

        // Get running, walking, sneaking, or dancing, from speed
        if ( !( pchr->keepaction || pchr->loopaction ) )
        {
            framelip = Md2FrameList[pchr->inst.frame].framelip;  // 0 - 15...  Way through animation
            if ( pchr->actionready && pchr->inst.lip == 0 && pchr->phys_grounded && pchr->flyheight == 0 && ( framelip&7 ) < 2 )
            {
                // Do the motion stuff
                speed = ABS( ( int ) pchr->vel.x ) + ABS( ( int ) pchr->vel.y );
                if ( speed < pchr->sneakspd )
                {
                    //                        pchr->nextaction = ACTION_DA;
                    // Do boredom
                    pchr->boretime--;
                    if ( pchr->boretime < 0 )
                    {
                        pchr->ai.alert |= ALERTIF_BORED;
                        pchr->boretime = BORETIME;
                    }
                    else
                    {
                        // Do standstill
                        if ( pchr->action > ACTION_DD )
                        {
                            pchr->action = ACTION_DA;
                            pchr->inst.frame = MadList[pchr->inst.imad].actionstart[pchr->action];
                        }
                    }
                }
                else
                {
                    pchr->boretime = BORETIME;
                    if ( speed < pchr->walkspd )
                    {
                        pchr->nextaction = ACTION_WA;
                        if ( pchr->action != ACTION_WA )
                        {
                            pchr->inst.frame = MadList[pchr->inst.imad].frameliptowalkframe[LIPWA][framelip];
                            pchr->action = ACTION_WA;
                        }
                    }
                    else
                    {
                        if ( speed < pchr->runspd )
                        {
                            pchr->nextaction = ACTION_WB;
                            if ( pchr->action != ACTION_WB )
                            {
                                pchr->inst.frame = MadList[pchr->inst.imad].frameliptowalkframe[LIPWB][framelip];
                                pchr->action = ACTION_WB;
                            }
                        }
                        else
                        {
                            pchr->nextaction = ACTION_WC;
                            if ( pchr->action != ACTION_WC )
                            {
                                pchr->inst.frame = MadList[pchr->inst.imad].frameliptowalkframe[LIPWC][framelip];
                                pchr->action = ACTION_WC;
                            }
                        }
                    }
                }
            }
        }
    }

    // Do poofing
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ChrList[cnt].on || !(ChrList[cnt].ai.poof_time >= 0 && ChrList[cnt].ai.poof_time <= (Sint32)frame_wld)  ) continue;

        if ( ChrList[cnt].attachedto != MAX_CHR )
        {
            detach_character_from_mount( cnt, btrue, bfalse );
        }

        if ( ChrList[cnt].holdingwhich[SLOT_LEFT] != MAX_CHR )
        {
            detach_character_from_mount( ChrList[cnt].holdingwhich[SLOT_LEFT], btrue, bfalse );
        }

        if ( ChrList[cnt].holdingwhich[SLOT_RIGHT] != MAX_CHR )
        {
            detach_character_from_mount( ChrList[cnt].holdingwhich[SLOT_RIGHT], btrue, bfalse );
        }

		stop_object_looped_sound( cnt );
        free_inventory( cnt );
        free_one_character_in_game( cnt );
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

        while ( goto_colon( NULL, fileread, btrue ) && section < MAXSECTION )
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
