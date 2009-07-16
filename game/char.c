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
#include "game.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo_math.h"
#include "egoboo.h"

#include <assert.h>
#include <float.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int     numfreechr = 0;             // For allocation
static Uint16  freechrlist[MAX_CHR];

static IDSZ    inventory_idsz[INVEN_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int         importobject;

team_t      TeamList[MAXTEAM];
cap_t       CapList[MAX_PROFILE];
chr_t       ChrList[MAX_CHR];

chop_data_t chop = {0, 0};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t chr_instance_init( chr_instance_t * pinst, Uint16 profile, Uint8 skin );
static Uint16 pack_has_a_stack( Uint16 item, Uint16 character );
static bool_t pack_add_item( Uint16 item, Uint16 character );
static Uint16 pack_get_item( Uint16 character, grip_offset_t grip_off, bool_t ignorekurse );
static void set_weapongrip( Uint16 iitem, Uint16 iholder, Uint16 vrt_off );

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
    Uint16 frame_nxt;
    Sint16 z;

    if ( INVALID_CHR( character ) ) return;

    frame_nxt = ChrList[character].inst.frame_nxt;

    for ( cnt = 0; cnt < MadList[ChrList[character].inst.imad].transvertices; cnt++  )
    {
        z = Md2FrameList[frame_nxt].vrtz[cnt];
        if ( z < low )
        {
            ChrList[character].inst.color_amb = valuelow;
        }
        else
        {
            if ( z > high )
            {
                ChrList[character].inst.color_amb = valuehigh;
            }
            else
            {
                ChrList[character].inst.color_amb = ( valuehigh * ( z - low ) / ( high - low ) ) +
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
            if ( !ChrList[cnt].pack_ispacked )
            {
                character = ChrList[cnt].pack_next;

                while ( character != MAX_CHR )
                {
                    ChrList[character].pos = ChrList[cnt].pos;

                    // Copy olds to make SendMessageNear work
                    ChrList[character].pos_old = ChrList[cnt].pos_old;

                    character = ChrList[character].pack_next;
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
                ChrList[cnt].pos = ChrList[character].pos;
            }

            ChrList[cnt].turn_z = ChrList[character].turn_z;

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

    if ( INVALID_CHR( cnt ) ) return;

    ChrList[cnt].inst.matrixvalid = bfalse;

    if ( ChrList[cnt].overlay )
    {
        // Overlays are kept with their target...
        tnc = ChrList[cnt].ai.target;

        if ( VALID_CHR(tnc) )
        {
            ChrList[cnt].pos = ChrList[tnc].pos;
            ChrList[cnt].inst.matrixvalid = ChrList[tnc].inst.matrixvalid;
            CopyMatrix( &(ChrList[cnt].inst.matrix), &(ChrList[tnc].inst.matrix) );
        }
    }
    else
    {
        ChrList[cnt].inst.matrix = ScaleXYZRotateXYZTranslate( ChrList[cnt].fat, ChrList[cnt].fat, ChrList[cnt].fat,
                                   ChrList[cnt].turn_z >> 2,
                                   ( ( Uint16 ) ( ChrList[cnt].map_turn_x + 32768 ) ) >> 2,
                                   ( ( Uint16 ) ( ChrList[cnt].map_turn_y + 32768 ) ) >> 2,
                                   ChrList[cnt].pos.x, ChrList[cnt].pos.y, ChrList[cnt].pos.z );
        ChrList[cnt].inst.matrixvalid = btrue;
    }
}

//--------------------------------------------------------------------------------------------
void free_one_character( Uint16 character )
{
    int cnt;
    chr_t * pchr;

    if ( !VALID_CHR_RANGE( character ) ) return;
    pchr = ChrList + character;

    // the character "destructor"
    // sets all boolean values to false, incluting the "on" flag
    memset( pchr, 0, sizeof(chr_t) );

    // set all of the integer references to invalid values
    pchr->firstenchant = MAX_ENC;
    pchr->undoenchant  = MAX_ENC;
    pchr->attachedto   = MAX_CHR;
    for (cnt = 0; cnt < SLOT_COUNT; cnt++)
    {
        pchr->holdingwhich[cnt] = MAX_CHR;
    }

    pchr->pack_next = MAX_CHR;
    for (cnt = 0; cnt < INVEN_COUNT; cnt++)
    {
        pchr->inventory[cnt] = MAX_CHR;
    }

    pchr->onwhichplatform = MAX_CHR;

    // push it on the stack
    if ( numfreechr < MAX_CHR )
    {
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

            if ( stat_found )
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

    if ( INVALID_CHR( character ) ) return;

    cnt = ChrList[character].pack_next;
    while ( cnt < MAX_CHR )
    {
        next = ChrList[cnt].pack_next;
        free_one_character_in_game( cnt );
        cnt = next;
    }
}

//--------------------------------------------------------------------------------------------
void attach_particle_to_character( Uint16 particle, Uint16 character, int vertex_offset )
{
    // ZZ> This function sets one particle's position to be attached to a character.
    //     It will kill the particle if the character is no longer around

    Uint16 vertex, model;
    GLvector4 point[1], nupoint[1];

    // Check validity of attachment
    if ( INVALID_CHR(character) || ChrList[character].pack_ispacked )
    {
        PrtList[particle].time   = frame_all + 1;
        PrtList[particle].poofme = btrue;
        return;
    }

    // Do we have a matrix???
    if ( ChrList[character].inst.matrixvalid )// PMesh->mmem.inrenderlist[ChrList[character].onwhichfan])
    {
        // Transform the weapon vertex_offset from model to world space
        model = ChrList[character].inst.imad;

        if ( vertex_offset == GRIP_ORIGIN )
        {
            PrtList[particle].pos.x = ChrList[character].inst.matrix.CNV( 3, 0 );
            PrtList[particle].pos.y = ChrList[character].inst.matrix.CNV( 3, 1 );
            PrtList[particle].pos.z = ChrList[character].inst.matrix.CNV( 3, 2 );
            return;
        }

        vertex = MadList[model].md2.vertices - vertex_offset;

        // do the automatic update
        chr_instance_update_vertices( &(ChrList[character].inst), vertex, vertex );

        // Calculate vertex_offset point locations with linear interpolation and other silly things
        point[0].x = ChrList[character].inst.vlst[vertex].pos[XX];
        point[0].y = ChrList[character].inst.vlst[vertex].pos[YY];
        point[0].z = ChrList[character].inst.vlst[vertex].pos[ZZ];
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
        PrtList[particle].pos = ChrList[character].pos;
    }
}

//--------------------------------------------------------------------------------------------
void make_one_weapon_matrix( Uint16 iweap, Uint16 iholder, bool_t do_physics )
{
    // ZZ> This function sets one weapon's matrix, based on who it's attached to
    int       cnt, vertex;
    Uint16    iholder_model, iholder_frame, iholder_lastframe;
    Uint8     iholder_lip;
    float     iholder_flip;
    GLvector4 point[GRIP_VERTS], nupoint[GRIP_VERTS];
    GLvector3 ptemp;
    int       iweap_points;

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
    iholder_frame     = pholder->inst.frame_nxt;
    iholder_lastframe = pholder->inst.frame_lst;
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
    else if (GRIP_VERTS == iweap_points)
    {
        vertex = pweap->weapongrip[0];

        // do the automatic update
        chr_instance_update_vertices( &(pholder->inst), vertex, vertex + GRIP_VERTS - 1 );

        for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
        {
            point[cnt].x = pholder->inst.vlst[vertex + cnt].pos[XX];
            point[cnt].y = pholder->inst.vlst[vertex + cnt].pos[YY];
            point[cnt].z = pholder->inst.vlst[vertex + cnt].pos[ZZ];
            point[cnt].w = 1.0f;
        }
    }
    else
    {
        // Calculate grip point locations with linear interpolation and other silly things
        for (iweap_points = 0, cnt = 0; cnt < GRIP_VERTS; cnt++, iweap_points++ )
        {
            vertex = pweap->weapongrip[cnt];
            if (0xFFFF == vertex) continue;

            // do the automatic update
            chr_instance_update_vertices( &(pholder->inst), vertex, vertex );

            point[iweap_points].x = pholder->inst.vlst[vertex].pos[XX];
            point[iweap_points].y = pholder->inst.vlst[vertex].pos[YY];
            point[iweap_points].z = pholder->inst.vlst[vertex].pos[ZZ];
            point[iweap_points].w = 1.0f;
        }
    }

    // use the math function instead of rolling out own
    TransformVertices( &(pholder->inst.matrix), point, nupoint, iweap_points );

    if (1 == iweap_points)
    {
        // attach to single point
        pweap->inst.matrix = ScaleXYZRotateXYZTranslate(pweap->fat, pweap->fat, pweap->fat,
                             pweap->turn_z >> 2,
                             ( ( Uint16 ) ( pweap->map_turn_x + 32768 ) ) >> 2,
                             ( ( Uint16 ) ( pweap->map_turn_y + 32768 ) ) >> 2,
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

    ptemp = pweap->pos;

    // update the position of the object
    pweap->pos.x = nupoint[0].x;
    pweap->pos.y = nupoint[0].y;
    pweap->pos.z = nupoint[0].z;

    if ( do_physics )
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

            pweap->phys.apos_1.x -= dx * ratio;
            pweap->phys.apos_1.y -= dy * ratio;
            pweap->phys.apos_1.z -= dz * ratio;

            //pweap->phys.avel.x += (dx-vcom.x)*damp + vcom.x - pweap->vel.x;
            //pweap->phys.avel.y += (dy-vcom.y)*damp + vcom.y - pweap->vel.y;
            //pweap->phys.avel.z += (dz-vcom.z)*damp + vcom.z - pweap->vel.z;
        }

        if ( wt_holder >= 0.0f )
        {
            float ratio = (float)ABS(wt_weap) / ((float)ABS(wt_weap) + (float)ABS(wt_holder));

            pholder->phys.apos_1.x -= dx * ratio;
            pholder->phys.apos_1.y -= dy * ratio;
            pholder->phys.apos_1.z -= dz * ratio;

            //pholder->phys.avel.x += (pholder->vel.x-vcom.x)*damp + vcom.x - pholder->vel.x;
            //pholder->phys.avel.y += (pholder->vel.y-vcom.y)*damp + vcom.y - pholder->vel.y;
            //pholder->phys.avel.z += (pholder->vel.z-vcom.z)*damp + vcom.z - pholder->vel.z;
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
        ChrList[ichr].phys.apos_0.x = 0.0f;
        ChrList[ichr].phys.apos_0.y = 0.0f;
        ChrList[ichr].phys.apos_0.z = 0.0f;

        ChrList[ichr].phys.apos_1.x = 0.0f;
        ChrList[ichr].phys.apos_1.y = 0.0f;
        ChrList[ichr].phys.apos_1.z = 0.0f;

        ChrList[ichr].phys.avel.x = 0.0f;
        ChrList[ichr].phys.avel.y = 0.0f;
        ChrList[ichr].phys.avel.z = 0.0f;
    }

    // Do base characters
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        if ( !ChrList[ichr].on ) continue;

        if ( INVALID_CHR( ChrList[ichr].attachedto ) )
        {
            make_one_character_matrix( ichr );
        }
    }

    // do all levels of attachment
    done = bfalse;
    while ( !done )
    {
        cnt = 0;
        for ( ichr = 0; ichr < MAX_CHR; ichr++ )
        {
            Uint16 imount;

            if ( !ChrList[ichr].on ) continue;
            if ( ChrList[cnt].inst.matrixvalid ) continue;

            imount = ChrList[ichr].attachedto;
            if ( INVALID_CHR(imount) ) { ChrList[ichr].attachedto = MAX_CHR; continue; }
            if ( imount == ichr ) { imount = MAX_CHR; continue; }

            // can't evaluate this link yet
            if ( !ChrList[imount].inst.matrixvalid )
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

    if ( do_physics )
    {
        // accumulate the accumulators
        for ( ichr = 0; ichr < MAX_CHR; ichr++ )
        {
            float nrm[2];
            float tmpx, tmpy, tmpz;
            chr_t * pchr;

            if ( !ChrList[ichr].on ) continue;
            pchr = ChrList + ichr;

            // do the "integration" of the accumulated accelerations
            pchr->vel.x += pchr->phys.avel.x;
            pchr->vel.y += pchr->phys.avel.y;
            pchr->vel.z += pchr->phys.avel.z;

            // do the "integration" on the position
            if ( ABS(pchr->phys.apos_1.x) > 0 )
            {
                tmpx = pchr->pos.x;
                pchr->pos.x += pchr->phys.apos_1.x;
                if ( __chrhitawall(ichr, nrm) )
                {
                    // restore the old values
                    pchr->pos.x = tmpx;
                }
                else
                {
                    //pchr->vel.x += pchr->phys.apos_1.x;
                    pchr->pos_safe.x = tmpx;
                }
            }

            if ( ABS(pchr->phys.apos_1.y) > 0 )
            {
                tmpy = pchr->pos.y;
                pchr->pos.y += pchr->phys.apos_1.y;
                if ( __chrhitawall(ichr, nrm) )
                {
                    // restore the old values
                    pchr->pos.y = tmpy;
                }
                else
                {
                    //pchr->vel.y += pchr->phys.apos_1.y;
                    pchr->pos_safe.y = tmpy;
                }
            }

            if ( ABS(pchr->phys.apos_1.z) > 0 )
            {
                tmpz = pchr->pos.z;
                pchr->pos.z += pchr->phys.apos_1.z;
                if ( pchr->pos.z < pchr->phys.level )
                {
                    // restore the old values
                    pchr->pos.z = tmpz;
                }
                else
                {
                    //pchr->vel.z += pchr->phys.apos_1.z;
                    pchr->pos_safe.z = tmpz;
                }
            }

            if ( 0 == __chrhitawall(ichr, nrm) )
            {
                pchr->safe_valid = btrue;
            }
        }

        // fix the matrix positions
        for ( ichr = 0; ichr < MAX_CHR; ichr++ )
        {
            if ( !ChrList[ichr].on || !ChrList[ichr].inst.matrixvalid ) continue;

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
Uint32 __chrhitawall( Uint16 character, float nrm[] )
{
    // ZZ> This function returns nonzero if the character hit a wall that the
    //     character is not allowed to cross

    Uint32 pass;
    float x, y, bs;
    Uint32 itile;
    int tx_min, tx_max, ty_min, ty_max;
    int ix, iy, tx0, ty0;
    bool_t invalid;

    if ( INVALID_CHR(character) ) return 0;

    if ( 0 == ChrList[character].bumpsize || 0xFFFFFFFF == ChrList[character].weight ) return 0;

    y = ChrList[character].pos.y;
    x = ChrList[character].pos.x;
    bs = ChrList[character].bumpsize >> 1;

    tx_min = floor( (x - bs) / 128.0f );
    tx_max = (int) ( (x + bs) / 128.0f );
    ty_min = floor( (y - bs) / 128.0f );
    ty_max = (int) ( (y + bs) / 128.0f );

    tx0 = (int)x / 128;
    ty0 = (int)y / 128;

    pass = 0;
    nrm[XX] = nrm[YY] = 0.0f;
    for ( iy = ty_min; iy <= ty_max; iy++ )
    {
        invalid = bfalse;

        if ( iy < 0 || iy >= PMesh->info.tiles_y )
        {
            pass  |=  MPDFX_IMPASS | MPDFX_WALL;
            nrm[YY]  += (iy + 0.5f) * 128.0f - y;
            invalid = btrue;
        }

        for ( ix = tx_min; ix <= tx_max; ix++ )
        {
            if ( ix < 0 || ix >= PMesh->info.tiles_x )
            {
                pass  |=  MPDFX_IMPASS | MPDFX_WALL;
                nrm[XX]  += (ix + 0.5f) * 128.0f - x;
                invalid = btrue;
            }

            if ( !invalid )
            {
                itile = mesh_get_tile_int( PMesh, ix, iy );
                if ( VALID_TILE(PMesh, itile) )
                {
                    if ( PMesh->mmem.tile_list[itile].fx & ChrList[character].stoppedby )
                    {
                        nrm[XX] += (ix + 0.5f) * 128.0f - x;
                        nrm[YY] += (iy + 0.5f) * 128.0f - y;
                    }
                    else
                    {
                        nrm[XX] -= (ix + 0.5f) * 128.0f - x;
                        nrm[YY] -= (iy + 0.5f) * 128.0f - y;
                    }

                    pass |= PMesh->mmem.tile_list[itile].fx;
                }
            }
        }
    }

    if ( pass & ChrList[character].stoppedby )
    {
        float dist2 = nrm[XX] * nrm[XX] + nrm[YY] * nrm[YY];
        if ( dist2 > 0 )
        {
            float dist = SQRT( dist2 );
            nrm[XX] /= -dist;
            nrm[YY] /= -dist;
        }
    }

    return pass & ChrList[character].stoppedby;

}

//--------------------------------------------------------------------------------------------
void reset_character_accel( Uint16 character )
{
    // ZZ> This function fixes a character's max acceleration
    Uint16 enchant;

    if ( INVALID_CHR( character ) ) return;

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

//--------------------------------------------------------------------------------------------
void detach_character_from_mount( Uint16 character, Uint8 ignorekurse,
                                  Uint8 doshop )
{
    // ZZ> This function drops an item
    Uint16 mount, hand, enchant, cnt, passage, owner = NOOWNER;
    bool_t inshop;
    int loc;
    float price;
    float nrm[2];

    // Make sure the character is valid
    if ( INVALID_CHR(character) ) return;

    // Make sure the character is mounted
    mount = ChrList[character].attachedto;
    if ( INVALID_CHR(mount) ) return;

    // Don't allow living characters to drop kursed weapons
    if ( !ignorekurse && ChrList[character].iskursed && ChrList[mount].alive && ChrList[character].isitem )
    {
        ChrList[character].ai.alert |= ALERTIF_NOTDROPPED;
        return;
    }

    // set the dismount timer
    ChrList[character].phys.dismount_timer = PHYS_DISMOUNT_TIME;

    // Figure out which hand it's in
    hand = ChrList[character].inwhich_slot;

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
        ChrList[character].pos = ChrList[mount].pos;
    }

    // Make sure it's not dropped in a wall...
    if ( __chrhitawall( character, nrm ) )
    {
        ChrList[character].pos.x = ChrList[mount].pos.x;
        ChrList[character].pos.y = ChrList[mount].pos.y;
    }
    else
    {
        ChrList[character].safe_valid = btrue;
        ChrList[character].pos_safe = ChrList[character].pos;
    }

    // Check for shop passages
    inshop = bfalse;
    if ( ChrList[character].isitem && numshoppassage != 0 && doshop )
    {
        //This is a hack that makes spellbooks in shops cost correctly
        if (ChrList[mount].isshopitem) ChrList[character].isshopitem = btrue;

        for ( cnt = 0; cnt < numshoppassage; cnt++ )
        {
            passage = shoppassage[cnt];
            loc = ChrList[character].pos.x;
            loc = loc >> TILE_BITS;
            if ( loc >= passtlx[passage] && loc <= passbrx[passage] )
            {
                loc = ChrList[character].pos.y;
                loc = loc >> TILE_BITS;
                if ( loc >= passtly[passage] && loc <= passbry[passage] )
                {
                    inshop = btrue;
                    owner  = shopowner[cnt];
                    if ( owner == NOOWNER )
                    {
                        // The owner has died!!!
                        inshop = bfalse;
                    }

                    if ( inshop ) break;
                }
            }
        }

        if ( inshop )
        {
            // Give the mount its money back, alert the shop owner
            Uint16 skin, icap;

			//Make sure spell books are priced according to their spell and not the book itself 
			if( ChrList[character].model == SPELLBOOK ) 
			{
				icap = ChrList[character].basemodel;
				skin = 0;
			}
			else
			{
				icap  = ChrList[character].model;
				skin = ChrList[character].skin;
			}
            price = CapList[icap].skincost[skin];

            //Items spawned within shops are more valuable
            if (!ChrList[character].isshopitem) price *= 0.5;

            // cost it based on the number/charges of the item
            if ( CapList[icap].isstackable )
            {
                price *= ChrList[character].ammo;
            }
            else if (CapList[icap].isranged && ChrList[character].ammo < ChrList[character].ammomax)
            {
                if ( 0 != ChrList[character].ammo )
                {
                    price *= (float)ChrList[character].ammo / ChrList[character].ammomax;
                }
            }

            ChrList[mount].money += (Sint16) price;
            ChrList[mount].money  = CLIP(ChrList[mount].money, 0, MAXMONEY);

            ChrList[owner].money -= (Sint16) price;
            ChrList[owner].money  = CLIP(ChrList[owner].money, 0, MAXMONEY);

            ai_add_order( &(ChrList[owner].ai), (Uint32) price, SHOP_BUY );
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
    ChrList[character].map_turn_y = 32768;
    ChrList[character].map_turn_x = 32768;
}
//--------------------------------------------------------------------------------------------
void reset_character_alpha( Uint16 character )
{
    // ZZ> This function fixes an item's transparency
    Uint16 enchant, mount;

    if ( INVALID_CHR( character ) ) return;

    mount = ChrList[character].attachedto;
    if ( INVALID_CHR( mount ) ) return;

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
}

//--------------------------------------------------------------------------------------------
void attach_character_to_mount( Uint16 iitem, Uint16 iholder, grip_offset_t grip_off )
{
    // ZZ> This function attaches one character/item to another ( the holder/mount )
    //     at a certain vertex offset ( grip_off )

    slot_t slot;

    chr_t * pitem, * pholder;

    // Make sure the character/item is valid
    if ( INVALID_CHR(iitem) || ChrList[iitem].pack_ispacked ) return;
    pitem = ChrList + iitem;
    
    // Make sure the holder/mount is valid
    if( INVALID_CHR(iholder) || ChrList[iholder].pack_ispacked ) return;
    pholder = ChrList + iholder;

#if !defined(ENABLE_BODY_GRAB)
    if (!pitem->alive) return;
#endif

    // Figure out which slot this grip_off relates to
    slot = grip_offset_to_slot( grip_off );

    // Make sure the the slot is valid
    if ( !CapList[pholder->model].slotvalid[slot] ) return;

    // Put 'em together
    pitem->inwhich_slot   = slot;
    pitem->attachedto     = iholder;
    pholder->holdingwhich[slot] = iitem;

    // set the grip vertices for the iitem
    set_weapongrip( iitem, iholder, grip_off );

    // actually make position of the object coincide with its actual held position
    make_one_weapon_matrix( iitem, iholder, bfalse );

    pitem->pos      = mat_getTranslate( pitem->inst.matrix );
    pitem->inwater  = bfalse;
    pitem->jumptime = JUMPDELAY * 4;

    // Run the held animation
    if ( pholder->ismount && grip_off == GRIP_ONLY )
    {
        // Riding iholder
        chr_play_action( iitem, ACTION_MI, btrue );
        pitem->loopaction = btrue;
    }
    else if ( pitem->alive )
    {
        chr_play_action( iitem, ACTION_MM + slot, bfalse );
        if ( pitem->isitem )
        {
            // Item grab
            pitem->keepaction = btrue;
        }
    }

    // Set the team
    if ( pitem->isitem )
    {
        pitem->team = pholder->team;

        // Set the alert
        if ( pitem->alive )
        {
            pitem->ai.alert |= ALERTIF_GRABBED;
        }
    }

    if ( pholder->ismount )
    {
        pholder->team = pitem->team;

        // Set the alert
        if ( !pholder->isitem && pholder->alive )
        {
            pholder->ai.alert |= ALERTIF_GRABBED;
        }
    }

    // It's not gonna hit the floor
    pitem->hitready = bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t inventory_add_item( Uint16 item, Uint16 character )
{
    chr_t * pchr, * pitem;
    cap_t * pcap_item;
    bool_t  slot_found, pack_added;
    int     slot_count;
    int     cnt;

    if ( INVALID_CHR(item) ) return bfalse;
    pitem = ChrList + item;

    // don't allow sub-inventories
    if ( pitem->pack_ispacked || pitem->isequipped ) return bfalse;

    if ( INVALID_CAP(pitem->model) ) return bfalse;
    pcap_item = CapList + pitem->model;

    if ( INVALID_CHR(character) ) return bfalse;
    pchr = ChrList + character;

    // don't allow sub-inventories
    if ( pchr->pack_ispacked || pchr->isequipped ) return bfalse;

    slot_found = bfalse;
    slot_count = 0;
    for ( cnt = 0; cnt < INVEN_COUNT; cnt++)
    {
        if ( IDSZ_NONE == inventory_idsz[cnt] ) continue;

        if ( inventory_idsz[cnt] == pcap_item->idsz[IDSZ_PARENT] )
        {
            slot_count = cnt;
            slot_found = btrue;
        }
    }

    if ( slot_found )
    {
        if ( VALID_CHR(pchr->holdingwhich[slot_count]) )
        {
            pchr->inventory[slot_count] = MAX_CHR;
        }
    }

    pack_added = pack_add_item( item, character );

    if ( slot_found && pack_added )
    {
        pchr->inventory[slot_count] = item;
    }

    return pack_added;
}

//--------------------------------------------------------------------------------------------
Uint16 inventory_get_item( Uint16 ichr, grip_offset_t grip_off, bool_t ignorekurse )
{
    chr_t * pchr;
    Uint16  iitem;
    int     cnt;

    if ( INVALID_CHR(ichr) ) return bfalse;
    pchr = ChrList + ichr;

    if (  pchr->pack_ispacked || pchr->isitem || MAX_CHR == pchr->pack_next )
        return MAX_CHR;

    if ( pchr->pack_count == 0 )
        return MAX_CHR;

    iitem = pack_get_item( ichr, grip_off, ignorekurse );

    // remove it from the "equipped" slots
    if ( VALID_CHR(iitem) )
    {
        for ( cnt = 0; cnt < INVEN_COUNT; cnt++)
        {
            if ( pchr->inventory[cnt] == iitem )
            {
                pchr->inventory[cnt] = iitem;
                ChrList[iitem].isequipped = bfalse;
                break;
            }
        }
    }

    return iitem;
}

//--------------------------------------------------------------------------------------------
Uint16 pack_has_a_stack( Uint16 item, Uint16 character )
{
    // ZZ> This function looks in the character's pack for an item similar
    //     to the one given.  If it finds one, it returns the similar item's
    //     index number, otherwise it returns MAX_CHR.

    Uint16 pack_ispacked, id;
    bool_t allok;

    if ( INVALID_CHR( item ) ) return MAX_CHR;

    if ( CapList[ChrList[item].model].isstackable )
    {
        pack_ispacked = ChrList[character].pack_next;

        allok = bfalse;
        while ( VALID_CHR(pack_ispacked) && !allok )
        {
            allok = btrue;
            if ( ChrList[pack_ispacked].model != ChrList[item].model )
            {
                if ( !CapList[ChrList[pack_ispacked].model].isstackable )
                {
                    allok = bfalse;
                }
                if ( ChrList[pack_ispacked].ammomax != ChrList[item].ammomax )
                {
                    allok = bfalse;
                }

                for ( id = 0; id < IDSZ_COUNT && allok; id++ )
                {
                    if ( CapList[ChrList[pack_ispacked].model].idsz[id] != CapList[ChrList[item].model].idsz[id] )
                    {
                        allok = bfalse;
                    }
                }
            }
            if ( !allok )
            {
                pack_ispacked = ChrList[pack_ispacked].pack_next;
            }
        }

        if ( allok )
        {
            return pack_ispacked;
        }
    }

    return MAX_CHR;
}

//--------------------------------------------------------------------------------------------
bool_t pack_add_item( Uint16 item, Uint16 character )
{
    // ZZ> This function puts one character inside the other's pack
    Uint16 oldfirstitem, newammo, stack;

    if ( INVALID_CHR( item ) || INVALID_CHR( character ) ) return bfalse;

    // Make sure everything is hunkydori
    if ( ChrList[item].pack_ispacked || ChrList[character].pack_ispacked || ChrList[character].isitem )
        return bfalse;

    stack = pack_has_a_stack( item, character );
    if ( VALID_CHR(stack) )
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
        if ( ChrList[character].pack_count >= MAXNUMINPACK )
        {
            ChrList[character].ai.alert |= ALERTIF_TOOMUCHBAGGAGE;
            return bfalse;
        }

        // Take the item out of hand
        if ( ChrList[item].attachedto != MAX_CHR )
        {
            detach_character_from_mount( item, btrue, bfalse );
            ChrList[item].ai.alert &= ( ~ALERTIF_DROPPED );
        }

        // Remove the item from play
        ChrList[item].hitready = bfalse;
        ChrList[item].pack_ispacked = btrue;

        // Insert the item into the pack as the first one
        oldfirstitem = ChrList[character].pack_next;
        ChrList[character].pack_next = item;
        ChrList[item].pack_next = oldfirstitem;
        ChrList[character].pack_count++;
        if ( CapList[ChrList[item].model].isequipment )
        {
            // AtLastWaypoint doubles as PutAway
            ChrList[item].ai.alert |= ALERTIF_ATLASTWAYPOINT;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 pack_get_item( Uint16 character, grip_offset_t grip_off, bool_t ignorekurse )
{
    // ZZ> This function takes the last item in the character's pack and puts
    //     it into the designated hand.  It returns the item number or MAX_CHR.
    Uint16 item, nexttolastitem;

    // does the character exist?
    if ( INVALID_CHR( character ) )
        return MAX_CHR;

    // Can the character have a pack?
    if ( ChrList[character].pack_ispacked || ChrList[character].isitem )
        return MAX_CHR;

    // is the pack empty?
    if ( MAX_CHR == ChrList[character].pack_next || 0 == ChrList[character].pack_count )
        return MAX_CHR;

    // Find the last item in the pack
    nexttolastitem = character;
    item = ChrList[character].pack_next;
    while ( VALID_CHR(ChrList[item].pack_next) )
    {
        nexttolastitem = item;
        item = ChrList[item].pack_next;
    }

    // Figure out what to do with it
    if ( ChrList[item].iskursed && ChrList[item].isequipped && !ignorekurse )
    {
        // Flag the last item as not removed
        ChrList[item].ai.alert |= ALERTIF_NOTPUTAWAY;  // Doubles as IfNotTakenOut

        // Cycle it to the front
        ChrList[item].pack_next = ChrList[character].pack_next;
        ChrList[nexttolastitem].pack_next = MAX_CHR;
        ChrList[character].pack_next = item;
        if ( character == nexttolastitem )
        {
            ChrList[item].pack_next = MAX_CHR;
        }

        item = MAX_CHR;
    }
    else
    {
        // Remove the last item from the pack
        ChrList[item].pack_ispacked = bfalse;
        ChrList[item].isequipped = bfalse;
        ChrList[nexttolastitem].pack_next = MAX_CHR;
        ChrList[character].pack_count--;
        ChrList[item].team = ChrList[character].team;

        // Attach the item to the character's hand
        attach_character_to_mount( item, character, grip_off );
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

    Uint16 item, lastitem, nextitem, direction;
    IDSZ testa, testz;

    if ( INVALID_CHR( character ) ) return;

    if ( ChrList[character].pos.z > -2 ) // Don't lose keys in pits...
    {
        // The IDSZs to find
        testa = Make_IDSZ( "KEYA" );  // [KEYA]
        testz = Make_IDSZ( "KEYZ" );  // [KEYZ]

        lastitem = character;
        item = ChrList[character].pack_next;

        while ( item != MAX_CHR )
        {
            nextitem = ChrList[item].pack_next;
            if ( item != character )  // Should never happen...
            {
                if ( ( CapList[ChrList[item].model].idsz[IDSZ_PARENT] >= testa &&
                        CapList[ChrList[item].model].idsz[IDSZ_PARENT] <= testz ) ||
                        ( CapList[ChrList[item].model].idsz[IDSZ_TYPE] >= testa &&
                          CapList[ChrList[item].model].idsz[IDSZ_TYPE] <= testz ) )
                {
                    // We found a key...
                    ChrList[item].pack_ispacked = bfalse;
                    ChrList[item].isequipped = bfalse;
                    ChrList[lastitem].pack_next = nextitem;
                    ChrList[item].pack_next = MAX_CHR;
                    ChrList[character].pack_count--;
                    ChrList[item].attachedto = MAX_CHR;
                    ChrList[item].ai.alert |= ALERTIF_DROPPED;
                    ChrList[item].hitready = btrue;

                    direction = RANDIE;
                    ChrList[item].turn_z = direction + 32768;
                    ChrList[item].phys.level = ChrList[character].phys.level;
                    ChrList[item].floor_level = ChrList[character].floor_level;
                    ChrList[item].onwhichplatform = ChrList[character].onwhichplatform;
                    ChrList[item].pos   = ChrList[character].pos;
                    ChrList[item].vel.x = turntocos[ (direction >> 2) & TRIG_TABLE_MASK ] * DROPXYVEL;
                    ChrList[item].vel.y = turntosin[ (direction >> 2) & TRIG_TABLE_MASK ] * DROPXYVEL;
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

//--------------------------------------------------------------------------------------------
bool_t drop_all_items( Uint16 character )
{
    // ZZ> This function drops all of a character's items
    Uint16 item, direction, diradd;

    if ( INVALID_CHR(character) ) return bfalse;

    detach_character_from_mount( ChrList[character].holdingwhich[SLOT_LEFT], btrue, bfalse );
    detach_character_from_mount( ChrList[character].holdingwhich[SLOT_RIGHT], btrue, bfalse );
    if ( ChrList[character].pack_count > 0 )
    {
        direction = ChrList[character].turn_z + 32768;
        diradd = 0xFFFF / ChrList[character].pack_count;

        while ( ChrList[character].pack_count > 0 )
        {
            item = inventory_get_item( character, GRIP_LEFT, bfalse );

            if ( VALID_CHR(item) )
            {
                detach_character_from_mount( item, btrue, btrue );
                ChrList[item].hitready = btrue;
                ChrList[item].ai.alert |= ALERTIF_DROPPED;
                ChrList[item].pos         = ChrList[character].pos;
                ChrList[item].phys.level  = ChrList[character].phys.level;
                ChrList[item].floor_level = ChrList[character].floor_level;
                ChrList[item].onwhichplatform = ChrList[character].onwhichplatform;
                ChrList[item].turn_z = direction + 32768;
                ChrList[item].vel.x = turntocos[ (direction>>2) & TRIG_TABLE_MASK ] * DROPXYVEL;
                ChrList[item].vel.y = turntosin[ (direction>>2) & TRIG_TABLE_MASK ] * DROPXYVEL;
                ChrList[item].vel.z = DROPZVEL;
                ChrList[item].team = ChrList[item].baseteam;
            }

            direction += diradd;
        }
    }

    return btrue;

}

//--------------------------------------------------------------------------------------------
Uint16 shop_get_owner( int ix, int iy )
{
    int cnt;
    Uint16 owner = NOOWNER;

    for ( cnt = 0; cnt < numshoppassage; cnt++ )
    {
        Uint16 passage;

        passage = shoppassage[cnt];
        if ( passage > numpassage ) continue;

        if ( ix >= passtlx[passage] && ix <= passbrx[passage] )
        {
            if ( iy >= passtly[passage] && iy <= passbry[passage] )
            {
                // if there is NOOWNER, someone has been murdered!
                owner  = shopowner[cnt];
                break;
            }
        }
    }

    return owner;
}

//--------------------------------------------------------------------------------------------
bool_t character_grab_stuff( Uint16 ichr_a, grip_offset_t grip_off, bool_t grab_people )
{
    // ZZ> This function makes the character pick up an item if there's one around
    Uint16 ichr_b;
    Uint16 model, vertex, frame_nxt;
    slot_t slot;
    GLvector4 point[1], nupoint[1];

    // Make life easier
    model = ChrList[ichr_a].model;
    slot = grip_offset_to_slot( grip_off );  // 0 is left, 1 is right

    // Make sure the character doesn't have something already, and that it has hands
    if ( ChrList[ichr_a].holdingwhich[slot] != MAX_CHR || !CapList[model].slotvalid[slot] )
        return bfalse;

    // Do we have a matrix???
    if ( ChrList[ichr_a].inst.matrixvalid )
    {
        // Transform the weapon grip_off from model to world space
        frame_nxt = ChrList[ichr_a].inst.frame_nxt;
        vertex = MadList[model].md2.vertices - grip_off;

        // do the automatic update
        chr_instance_update_vertices( &(ChrList[ichr_a].inst), vertex, vertex );

        // Calculate grip_off point locations with linear interpolation and other silly things
        point[0].x = ChrList[ichr_a].inst.vlst[vertex].pos[XX];
        point[0].y = ChrList[ichr_a].inst.vlst[vertex].pos[YY];
        point[0].z = ChrList[ichr_a].inst.vlst[vertex].pos[ZZ];
        point[0].w = 1.0f;

        // Do the transform
        TransformVertices( &(ChrList[ichr_a].inst.matrix), point, nupoint, 1 );
    }
    else
    {
        // Just wing it
        nupoint[0].x = ChrList[ichr_a].pos.x;
        nupoint[0].y = ChrList[ichr_a].pos.y;
        nupoint[0].z = ChrList[ichr_a].pos.z;
        nupoint[0].w = 1.0f;
    }

    // Go through all characters to find the best match
    for ( ichr_b = 0; ichr_b < MAX_CHR; ichr_b++ )
    {
        GLvector3 pos_b;
        bool_t can_grab;
        float price;

        float dx, dy, dz, dxy;

        if ( !ChrList[ichr_b].on ) continue;

        if ( ChrList[ichr_b].pack_ispacked ) continue;              // pickpocket not allowed yet
        if ( MAX_CHR != ChrList[ichr_b].attachedto) continue; // disarm not allowed yet

        if ( ChrList[ichr_b].weight > ChrList[ichr_a].weight + ChrList[ichr_a].strength ) continue; // reasonable carrying capacity

        // grab_people == btrue allows you to pick up living non-items
        // grab_people == false allows you to pick up living (functioning) items
        if ( ChrList[ichr_b].alive && (grab_people == ChrList[ichr_b].isitem) ) continue;

        // do not pick up your mount
        if ( ChrList[ichr_b].holdingwhich[SLOT_LEFT] == ichr_a || ChrList[ichr_b].holdingwhich[SLOT_RIGHT] == ichr_a ) continue;

        pos_b = ChrList[ichr_b].pos;

        // First check absolute value diamond
        dx = ABS( nupoint[0].x - pos_b.x );
        dy = ABS( nupoint[0].y - pos_b.y );
        dz = ABS( nupoint[0].z - pos_b.z );
        dxy = dx + dy;

        if ( dxy > GRABSIZE || dz > GRABSIZE ) continue;

        // Check for shop

        can_grab = btrue;
        if ( ChrList[ichr_b].isitem && numshoppassage > 0 )
        {
            int    ix, iy;
            bool_t inshop;
            Uint16 owner;

            ix = ChrList[ichr_b].pos.x / TILE_SIZE;
            iy = ChrList[ichr_b].pos.y / TILE_SIZE;

            owner  = shop_get_owner( ix, iy );
            inshop = VALID_CHR(owner);

            if ( inshop )
            {
                // Pay the shop owner, or don't allow grab...
                bool_t is_invis, can_steal;

                is_invis  = FF_MUL(ChrList[ichr_a].inst.alpha, ChrList[ichr_a].inst.max_light) < INVISIBLE;
                can_steal = is_invis || ChrList[ichr_a].isitem;

                if ( can_steal )
                {
                    // Pets can try to steal in addition to invisible characters
                    STRING text;

                    // Check if it was detected. 50% chance +2% per pet DEX and -2% per shopkeeper wisdom
                    if ( ChrList[owner].canseeinvisible || generate_number( 1, 100 ) - ( ChrList[ichr_a].dexterity >> 7 ) + ( ChrList[owner].wisdom >> 7 ) > 50 )
                    {
                        snprintf( text, sizeof(text), "%s was detected!!", ChrList[ichr_a].name );
                        debug_message( text );

                        ai_add_order( &(ChrList[owner].ai), STOLEN, SHOP_THEFT );
                        ChrList[owner].ai.target = ichr_a;
                    }
                    else
                    {
                        snprintf( text, sizeof(text), "%s stole something! (%s)", ChrList[ichr_a].name, CapList[ChrList[ichr_b].model].classname );
                        debug_message( text );
                    }
                }
                else
                {
                    Uint16 icap, iskin;

					//Make sure spell books are priced according to their spell and not the book itself 
					if( ChrList[ichr_b].model == SPELLBOOK ) 
					{
						icap = ChrList[ichr_b].basemodel;
						iskin = 0;
					}
					else
					{
						icap  = ChrList[ichr_b].model;
						iskin = ChrList[ichr_b].skin;
					}
                    price = (float) CapList[icap].skincost[iskin];

                    //Items spawned in shops are more valuable
                    if (!ChrList[ichr_b].isshopitem) price *= 0.5f;

                    // base the cost on the number of items/charges
                    if ( CapList[icap].isstackable )
                    {
                        price *= ChrList[ichr_b].ammo;
                    }
                    else if (CapList[icap].isranged && ChrList[ichr_b].ammo < ChrList[ichr_b].ammomax)
                    {
                        if ( 0 != ChrList[ichr_b].ammo )
                        {
                            price *= (float) ChrList[ichr_b].ammo / (float) ChrList[ichr_b].ammomax;
                        }
                    }

                    // round to int value
                    price = (Sint32) price;

                    if ( ChrList[ichr_a].money >= price )
                    {
                        // Okay to sell
                        ai_add_order( &(ChrList[owner].ai), (Uint32) price, SHOP_SELL );

                        ChrList[ichr_a].money -= (Sint16) price;
                        ChrList[ichr_a].money  = CLIP(ChrList[ichr_a].money, 0, MAXMONEY);

                        ChrList[owner].money += (Sint16) price;
                        ChrList[owner].money  = CLIP(ChrList[owner].money, 0, MAXMONEY);
                    }
                    else
                    {
                        // Don't allow purchase
                        ai_add_order( &(ChrList[owner].ai), (Uint32) price, SHOP_NOAFFORD );
                        can_grab = bfalse;
                    }
                }
            }
        }

        if ( can_grab )
        {
            // Stick 'em together and quit
            attach_character_to_mount( ichr_b, ichr_a, grip_off );
            if ( grab_people )
            {
                // Do a slam animation...  ( Be sure to drop!!! )
                chr_play_action( ichr_a, ACTION_MC + slot, bfalse );
            }
            return btrue;
        }
        else
        {
            // Lift the item a little and quit...
            ChrList[ichr_b].vel.z = DROPZVEL;
            ChrList[ichr_b].hitready = btrue;
            ChrList[ichr_b].ai.alert |= ALERTIF_DROPPED;
            break;
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
    float velocity;

    weapon = ChrList[cnt].holdingwhich[slot];
    spawngrip = GRIP_LAST;
    action = ChrList[cnt].action;

    // See if it's an unarmed attack...
    if ( weapon == MAX_CHR )
    {
        weapon = cnt;
        spawngrip = slot_to_grip_offset( slot );  // 0 -> GRIP_LEFT, 1 -> GRIP_RIGHT
    }
    if ( weapon != cnt && ( ( CapList[ChrList[weapon].model].isstackable && ChrList[weapon].ammo > 1 ) || ( action >= ACTION_FA && action <= ACTION_FD ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation

        thrown = spawn_one_character( ChrList[cnt].pos, ChrList[weapon].model, ChrList[cnt].team, 0, ChrList[cnt].turn_z, ChrList[weapon].name, MAX_CHR );
        if ( VALID_CHR(thrown) )
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

            tTmp = ChrList[cnt].turn_z >> 2;
            ChrList[thrown].vel.x += turntocos[( tTmp+8192 ) & TRIG_TABLE_MASK] * velocity;
            ChrList[thrown].vel.y += turntosin[( tTmp+8192 ) & TRIG_TABLE_MASK] * velocity;
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

            // Spawn an attack particle
            if ( CapList[ChrList[weapon].model].attackprttype != -1 )
            {
                particle = spawn_one_particle( ChrList[weapon].pos.x, ChrList[weapon].pos.y, ChrList[weapon].pos.z, ChrList[cnt].turn_z, ChrList[weapon].model, CapList[ChrList[weapon].model].attackprttype, weapon, spawngrip, ChrList[cnt].team, cnt, 0, MAX_CHR );
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
                    //PrtList[particle].damagetype = ChrList[weapon].damagetargettype;  //Zefz: not sure if we want this. we can have weapons with different damage types
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

    if ( INVALID_CHR( character ) ) return;

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

    if ( INVALID_CHR( character ) ) return;

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

    if ( INVALID_CHR( character ) ) return xpneeded;

    profile  = ChrList[character].model;
    if ( INVALID_CAP(profile) ) return xpneeded;

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

    if ( INVALID_CHR(character) ) return;

    profile = ChrList[character].model;
    if ( INVALID_CAP(profile) ) return;

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
                sound_play_chunk( PCamera->track_pos, g_wavelist[GSND_LEVELUP] );
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

    if ( INVALID_CHR( character ) ) return;
    if ( 0 == amount ) return;

    if ( !ChrList[character].invictus || override_invictus )
    {
        float intadd = (FP8_TO_INT(ChrList[character].intelligence) - 10.0f) / 200.0f;
        float wisadd = (FP8_TO_INT(ChrList[character].wisdom) - 10.0f)       / 400.0f;

        // Figure out how much experience to give
        profile = ChrList[character].model;
        newamount = amount;
        if ( xptype < XP_COUNT )
        {
            newamount = amount * CapList[profile].experiencerate[xptype];
        }

        //Intelligence and slightly wisdom increases xp gained (0,5% per int and 0,25% per wisdom above 10)
        newamount *= 1.00f + intadd + wisadd;

        //Apply XP bonus/penality depending on game difficulty
        if ( cfg.difficulty >= GAME_HARD ) newamount *= 1.10f;          //10% extra on hard

        ChrList[character].experience += newamount;
    }
}

//--------------------------------------------------------------------------------------------
void give_team_experience( Uint8 team, int amount, Uint8 xptype )
{
    // ZZ> This function gives every character on a team experience
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
            float nrm[2];

            bump_increase = ( ChrList[cnt].sizegoto - ChrList[cnt].fat ) * 0.10f * ChrList[cnt].bumpsize;

            // Make sure it won't get caught in a wall
            willgetcaught = bfalse;
            if ( ChrList[cnt].sizegoto > ChrList[cnt].fat )
            {
                ChrList[cnt].bumpsize += bump_increase;

                if ( __chrhitawall( cnt, nrm ) )
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
bool_t export_one_character_name( const char *szSaveName, Uint16 character )
{
    // ZZ> This function makes the naming.txt file for the character
    FILE* filewrite;
    char cTmp;
    int cnt, tnc;

    if ( INVALID_CHR(character) ) return bfalse;

    // Can it export?
    filewrite = fopen( szSaveName, "w" );
    if ( NULL == filewrite ) return bfalse;

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

    return btrue;

}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_profile( const char *szSaveName, Uint16 character )
{
    // ZZ> This function creates a data.txt file for the given character.
    //     it is assumed that all enchantments have been done away with

    FILE* filewrite;
    int icap;
    int damagetype, skin;
    char types[10] = "SCPHEFIZ";
    char codes[4];
    chr_t * pchr;
    cap_t * pcap;

    if ( INVALID_CHR(character) ) return bfalse;
    pchr = ChrList + character;

    // General stuff
    icap = ChrList[character].model;
    if ( INVALID_CAP(icap) ) return bfalse;
    pcap = CapList + icap;

    // Open the file
    filewrite = fopen( szSaveName, "w" );
    if ( NULL == filewrite ) return bfalse;


    // Real general data
    fprintf( filewrite, "Slot number    : -1\n" );  // -1 signals a flexible load thing
    funderf( filewrite, "Class name     : ", pcap->classname );
    ftruthf( filewrite, "Uniform light  : ", pcap->uniformlit );
    fprintf( filewrite, "Maximum ammo   : %d\n", pcap->ammomax );
    fprintf( filewrite, "Current ammo   : %d\n", pchr->ammo );
    fgendef( filewrite, "Gender         : ", pchr->gender );
    fprintf( filewrite, "\n" );

    // Object stats
    fprintf( filewrite, "Life color     : %d\n", pchr->lifecolor );
    fprintf( filewrite, "Mana color     : %d\n", pchr->manacolor );
    fprintf( filewrite, "Life           : %4.2f\n", pchr->lifemax / 256.0f );
    fpairof( filewrite, "Life up        : ", pcap->lifeperlevelbase, pcap->lifeperlevelrand );
    fprintf( filewrite, "Mana           : %4.2f\n", pchr->manamax / 256.0f );
    fpairof( filewrite, "Mana up        : ", pcap->manaperlevelbase, pcap->manaperlevelrand );
    fprintf( filewrite, "Mana return    : %4.2f\n", pchr->manareturn / 256.0f );
    fpairof( filewrite, "Mana return up : ", pcap->manareturnperlevelbase, pcap->manareturnperlevelrand );
    fprintf( filewrite, "Mana flow      : %4.2f\n", pchr->manaflow / 256.0f );
    fpairof( filewrite, "Mana flow up   : ", pcap->manaflowperlevelbase, pcap->manaflowperlevelrand );
    fprintf( filewrite, "STR            : %4.2f\n", pchr->strength / 256.0f );
    fpairof( filewrite, "STR up         : ", pcap->strengthperlevelbase, pcap->strengthperlevelrand );
    fprintf( filewrite, "WIS            : %4.2f\n", pchr->wisdom / 256.0f );
    fpairof( filewrite, "WIS up         : ", pcap->wisdomperlevelbase, pcap->wisdomperlevelrand );
    fprintf( filewrite, "INT            : %4.2f\n", pchr->intelligence / 256.0f );
    fpairof( filewrite, "INT up         : ", pcap->intelligenceperlevelbase, pcap->intelligenceperlevelrand );
    fprintf( filewrite, "DEX            : %4.2f\n", pchr->dexterity / 256.0f );
    fpairof( filewrite, "DEX up         : ", pcap->dexterityperlevelbase, pcap->dexterityperlevelrand );
    fprintf( filewrite, "\n" );

    // More physical attributes
    fprintf( filewrite, "Size           : %4.2f\n", pchr->sizegoto );
    fprintf( filewrite, "Size up        : %4.2f\n", pcap->sizeperlevel );
    fprintf( filewrite, "Shadow size    : %d\n", pcap->shadowsize );
    fprintf( filewrite, "Bump size      : %d\n", pcap->bumpsize );
    fprintf( filewrite, "Bump height    : %d\n", pcap->bumpheight );
    fprintf( filewrite, "Bump dampen    : %4.2f\n", pcap->bumpdampen );
    fprintf( filewrite, "Weight         : %d\n", pcap->weight );
    fprintf( filewrite, "Jump power     : %4.2f\n", pcap->jump );
    fprintf( filewrite, "Jump number    : %d\n", pcap->jumpnumber );
    fprintf( filewrite, "Sneak speed    : %d\n", pcap->sneakspd );
    fprintf( filewrite, "Walk speed     : %d\n", pcap->walkspd );
    fprintf( filewrite, "Run speed      : %d\n", pcap->runspd );
    fprintf( filewrite, "Fly to height  : %d\n", pcap->flyheight );
    fprintf( filewrite, "Flashing AND   : %d\n", pcap->flashand );
    fprintf( filewrite, "Alpha blending : %d\n", pcap->alpha );
    fprintf( filewrite, "Light blending : %d\n", pcap->light );
    ftruthf( filewrite, "Transfer blend : ", pcap->transferblend );
    fprintf( filewrite, "Sheen          : %d\n", pcap->sheen );
    ftruthf( filewrite, "Phong mapping  : ", pcap->enviro );
    fprintf( filewrite, "Texture X add  : %4.2f\n", pcap->uoffvel / (float)0xFFFF );
    fprintf( filewrite, "Texture Y add  : %4.2f\n", pcap->voffvel / (float)0xFFFF );
    ftruthf( filewrite, "Sticky butt    : ", pcap->stickybutt );
    fprintf( filewrite, "\n" );

    // Invulnerability data
    ftruthf( filewrite, "Invictus       : ", pcap->invictus );
    fprintf( filewrite, "NonI facing    : %d\n", pcap->nframefacing );
    fprintf( filewrite, "NonI angle     : %d\n", pcap->nframeangle );
    fprintf( filewrite, "I facing       : %d\n", pcap->iframefacing );
    fprintf( filewrite, "I angle        : %d\n", pcap->iframeangle );
    fprintf( filewrite, "\n" );

    // Skin defenses
    fprintf( filewrite, "Base defense   : %3d %3d %3d %3d\n", 255 - pcap->defense[0], 255 - pcap->defense[1],
             255 - pcap->defense[2], 255 - pcap->defense[3] );
    damagetype = 0;

    while ( damagetype < DAMAGE_COUNT )
    {
        fprintf( filewrite, "%c damage shift : %3d %3d %3d %3d\n", types[damagetype],
                 pcap->damagemodifier[damagetype][0]&DAMAGESHIFT,
                 pcap->damagemodifier[damagetype][1]&DAMAGESHIFT,
                 pcap->damagemodifier[damagetype][2]&DAMAGESHIFT,
                 pcap->damagemodifier[damagetype][3]&DAMAGESHIFT );
        damagetype++;
    }

    damagetype = 0;

    while ( damagetype < DAMAGE_COUNT )
    {
        skin = 0;

        while ( skin < MAXSKIN )
        {
            codes[skin] = 'F';
            if ( pcap->damagemodifier[damagetype][skin]&DAMAGEINVERT )
                codes[skin] = 'T';
            if ( pcap->damagemodifier[damagetype][skin]&DAMAGECHARGE )
                codes[skin] = 'C';
            if ( pcap->damagemodifier[damagetype][skin]&DAMAGEMANA )
                codes[skin] = 'M';

            skin++;
        }

        fprintf( filewrite, "%c damage code  : %3c %3c %3c %3c\n", types[damagetype], codes[0], codes[1], codes[2], codes[3] );
        damagetype++;
    }

    fprintf( filewrite, "Acceleration   : %3.0f %3.0f %3.0f %3.0f\n", pcap->maxaccel[0]*80,
             pcap->maxaccel[1]*80,
             pcap->maxaccel[2]*80,
             pcap->maxaccel[3]*80 );
    fprintf( filewrite, "\n" );

    // Experience and level data
    fprintf( filewrite, "EXP for 2nd    : %d\n", pcap->experienceforlevel[1] );
    fprintf( filewrite, "EXP for 3rd    : %d\n", pcap->experienceforlevel[2] );
    fprintf( filewrite, "EXP for 4th    : %d\n", pcap->experienceforlevel[3] );
    fprintf( filewrite, "EXP for 5th    : %d\n", pcap->experienceforlevel[4] );
    fprintf( filewrite, "EXP for 6th    : %d\n", pcap->experienceforlevel[5] );
    fprintf( filewrite, "Starting EXP   : %d\n", pchr->experience );
    fprintf( filewrite, "EXP worth      : %d\n", pcap->experienceworth );
    fprintf( filewrite, "EXP exchange   : %5.3f\n", pcap->experienceexchange );
    fprintf( filewrite, "EXPSECRET      : %4.2f\n", pcap->experiencerate[0] );
    fprintf( filewrite, "EXPQUEST       : %4.2f\n", pcap->experiencerate[1] );
    fprintf( filewrite, "EXPDARE        : %4.2f\n", pcap->experiencerate[2] );
    fprintf( filewrite, "EXPKILL        : %4.2f\n", pcap->experiencerate[3] );
    fprintf( filewrite, "EXPMURDER      : %4.2f\n", pcap->experiencerate[4] );
    fprintf( filewrite, "EXPREVENGE     : %4.2f\n", pcap->experiencerate[5] );
    fprintf( filewrite, "EXPTEAMWORK    : %4.2f\n", pcap->experiencerate[6] );
    fprintf( filewrite, "EXPROLEPLAY    : %4.2f\n", pcap->experiencerate[7] );
    fprintf( filewrite, "\n" );

    // IDSZ identification tags
    fprintf( filewrite, "IDSZ Parent    : [%s]\n", undo_idsz( pcap->idsz[IDSZ_PARENT] ) );
    fprintf( filewrite, "IDSZ Type      : [%s]\n", undo_idsz( pcap->idsz[IDSZ_TYPE] ) );
    fprintf( filewrite, "IDSZ Skill     : [%s]\n", undo_idsz( pcap->idsz[IDSZ_SKILL] ) );
    fprintf( filewrite, "IDSZ Special   : [%s]\n", undo_idsz( pcap->idsz[IDSZ_SPECIAL] ) );
    fprintf( filewrite, "IDSZ Hate      : [%s]\n", undo_idsz( pcap->idsz[IDSZ_HATE] ) );
    fprintf( filewrite, "IDSZ Vulnie    : [%s]\n", undo_idsz( pcap->idsz[IDSZ_VULNERABILITY] ) );
    fprintf( filewrite, "\n" );

    // Item and damage flags
    ftruthf( filewrite, "Is an item     : ", pcap->isitem );
    ftruthf( filewrite, "Is a mount     : ", pcap->ismount );
    ftruthf( filewrite, "Is stackable   : ", pcap->isstackable );
    ftruthf( filewrite, "Name known     : ", pchr->nameknown );
    ftruthf( filewrite, "Usage known    : ", pcap->usageknown );
    ftruthf( filewrite, "Is exportable  : ", pcap->cancarrytonextmodule );
    ftruthf( filewrite, "Requires skill : ", pcap->needskillidtouse );
    ftruthf( filewrite, "Is platform    : ", pcap->platform );
    ftruthf( filewrite, "Collects money : ", pcap->cangrabmoney );
    ftruthf( filewrite, "Can open stuff : ", pcap->canopenstuff );
    fprintf( filewrite, "\n" );

    // Other item and damage stuff
    fdamagf( filewrite, "Damage type    : ", pcap->damagetargettype );
    factiof( filewrite, "Attack type    : ", pcap->weaponaction );
    fprintf( filewrite, "\n" );

    // Particle attachments
    fprintf( filewrite, "Attached parts : %d\n", pcap->attachedprtamount );
    fdamagf( filewrite, "Reaffirm type  : ", pcap->attachedprtreaffirmdamagetype );
    fprintf( filewrite, "Particle type  : %d\n", pcap->attachedprttype );
    fprintf( filewrite, "\n" );

    // Character hands
    ftruthf( filewrite, "Left valid     : ", pcap->slotvalid[SLOT_LEFT] );
    ftruthf( filewrite, "Right valid    : ", pcap->slotvalid[SLOT_RIGHT] );
    fprintf( filewrite, "\n" );

    // Particle spawning on attack
    ftruthf( filewrite, "Part on weapon : ", pcap->attackattached );
    fprintf( filewrite, "Part type      : %d\n", pcap->attackprttype );
    fprintf( filewrite, "\n" );

    // Particle spawning for GoPoof
    fprintf( filewrite, "Poof amount    : %d\n", pcap->gopoofprtamount );
    fprintf( filewrite, "Facing add     : %d\n", pcap->gopoofprtfacingadd );
    fprintf( filewrite, "Part type      : %d\n", pcap->gopoofprttype );
    fprintf( filewrite, "\n" );

    // Particle spawning for blud
    ftruthf( filewrite, "Blud valid     : ", pcap->bludvalid );
    fprintf( filewrite, "Part type      : %d\n", pcap->bludprttype );
    fprintf( filewrite, "\n" );

    // Extra stuff
    ftruthf( filewrite, "Waterwalking   : ", pcap->waterwalk );
    fprintf( filewrite, "Bounce dampen  : %5.3f\n", pcap->dampen );
    fprintf( filewrite, "\n" );

    // More stuff
    fprintf( filewrite, "NOT USED       : %5.3f\n", pcap->lifeheal / 256.0f );       //These two are seriously outdated
    fprintf( filewrite, "NOT USED       : %5.3f\n", pcap->manacost / 256.0f );       //and shouldnt be used. Use scripts instead.
    fprintf( filewrite, "Regeneration   : %d\n", pcap->lifereturn );
    fprintf( filewrite, "Stopped by     : %d\n", pcap->stoppedby );
    funderf( filewrite, "Skin 0 name    : ", pcap->skinname[0] );
    funderf( filewrite, "Skin 1 name    : ", pcap->skinname[1] );
    funderf( filewrite, "Skin 2 name    : ", pcap->skinname[2] );
    funderf( filewrite, "Skin 3 name    : ", pcap->skinname[3] );
    fprintf( filewrite, "Skin 0 cost    : %d\n", pcap->skincost[0] );
    fprintf( filewrite, "Skin 1 cost    : %d\n", pcap->skincost[1] );
    fprintf( filewrite, "Skin 2 cost    : %d\n", pcap->skincost[2] );
    fprintf( filewrite, "Skin 3 cost    : %d\n", pcap->skincost[3] );
    fprintf( filewrite, "STR dampen     : %5.3f\n", pcap->strengthdampen );
    fprintf( filewrite, "\n" );

    // Another memory lapse
    ftruthf( filewrite, "No rider attak : ", btrue - pcap->ridercanattack );
    ftruthf( filewrite, "Can be dazed   : ", pcap->canbedazed );
    ftruthf( filewrite, "Can be grogged : ", pcap->canbegrogged );
    fprintf( filewrite, "NOT USED       : 0\n" );
    fprintf( filewrite, "NOT USED       : 0\n" );
    ftruthf( filewrite, "Can see invisi : ", pcap->canseeinvisible );
    fprintf( filewrite, "Kursed chance  : %d\n", pchr->iskursed*100 );
    fprintf( filewrite, "Footfall sound : %d\n", pcap->soundindex[SOUND_FOOTFALL] );
    fprintf( filewrite, "Jump sound     : %d\n", pcap->soundindex[SOUND_JUMP] );
    fprintf( filewrite, "\n" );

    // Expansions
    if ( pcap->skindressy&1 )
        fprintf( filewrite, ":[DRES] 0\n" );

    if ( pcap->skindressy&2 )
        fprintf( filewrite, ":[DRES] 1\n" );

    if ( pcap->skindressy&4 )
        fprintf( filewrite, ":[DRES] 2\n" );

    if ( pcap->skindressy&8 )
        fprintf( filewrite, ":[DRES] 3\n" );

    if ( pcap->resistbumpspawn )
        fprintf( filewrite, ":[STUK] 0\n" );

    if ( pcap->istoobig )
        fprintf( filewrite, ":[PACK] 0\n" );

    if ( !pcap->reflect )
        fprintf( filewrite, ":[VAMP] 1\n" );

    if ( pcap->alwaysdraw )
        fprintf( filewrite, ":[DRAW] 1\n" );

    if ( pcap->isranged )
        fprintf( filewrite, ":[RANG] 1\n" );

    if ( pcap->hidestate != NOHIDE )
        fprintf( filewrite, ":[HIDE] %d\n", pcap->hidestate );

    if ( pcap->isequipment )
        fprintf( filewrite, ":[EQUI] 1\n" );

    if ( pcap->bumpsizebig == ( pcap->bumpsize << 1 ) )
        fprintf( filewrite, ":[SQUA] 1\n" );

    if ( pcap->icon != pcap->usageknown )
        fprintf( filewrite, ":[ICON] %d\n", pcap->icon );

    if ( pcap->forceshadow )
        fprintf( filewrite, ":[SHAD] 1\n" );

    if ( pcap->ripple == pcap->isitem )
        fprintf( filewrite, ":[RIPP] %d\n", pcap->ripple );

    if ( pcap->isvaluable != -1 )
        fprintf( filewrite, ":[VALU] %d\n", pcap->isvaluable );

    //Basic stuff that is always written
    fprintf( filewrite, ":[GOLD] %d\n", pchr->money );
    fprintf( filewrite, ":[PLAT] %d\n", pcap->canuseplatforms );
    fprintf( filewrite, ":[SKIN] %d\n", pchr->skin );
    fprintf( filewrite, ":[CONT] %d\n", pchr->ai.content );
    fprintf( filewrite, ":[STAT] %d\n", pchr->ai.state );
    fprintf( filewrite, ":[LEVL] %d\n", pchr->experiencelevel );
	fprintf( filewrite, ":[LIFE] %4.2f\n", FP8_TO_FLOAT(pchr->life) );
	fprintf( filewrite, ":[MANA] %4.2f\n", FP8_TO_FLOAT(pchr->mana) );

    //Copy all skill expansions

    if ( pchr->shieldproficiency > 0 )
        fprintf( filewrite, ":[SHPR] %d\n", pchr->shieldproficiency );

    if ( pchr->canuseadvancedweapons > 0 )
        fprintf( filewrite, ":[AWEP] %d\n", pchr->canuseadvancedweapons );

    if ( pchr->canjoust )
        fprintf( filewrite, ":[JOUS] %d\n", pchr->canjoust );

    if ( pchr->candisarm )
        fprintf( filewrite, ":[DISA] %d\n", pchr->candisarm );

    if ( pcap->canseekurse )
        fprintf( filewrite, ":[CKUR] %d\n", pcap->canseekurse );

    if ( pchr->canusepoison )
        fprintf( filewrite, ":[POIS] %d\n", pchr->canusepoison );

    if ( pchr->canread )
        fprintf( filewrite, ":[READ] %d\n", pchr->canread );

    if ( pchr->canbackstab )
        fprintf( filewrite, ":[STAB] %d\n", pchr->canbackstab );

    if ( pchr->canusedivine )
        fprintf( filewrite, ":[HMAG] %d\n", pchr->canusedivine );

    if ( pchr->canusearcane )
        fprintf( filewrite, ":[WMAG] %d\n", pchr->canusearcane );

    if ( pchr->canusetech )
        fprintf( filewrite, ":[TECH] %d\n", pchr->canusetech );

    //The end
    fclose( filewrite );


    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_skin( const char *szSaveName, Uint16 character )
{
    // ZZ> This function creates a skin.txt file for the given character.
    FILE* filewrite;

    if ( INVALID_CHR(character) ) return bfalse;

    // Open the file
    filewrite = fopen( szSaveName, "w" );
    if ( NULL == filewrite ) return bfalse;

    fprintf( filewrite, "//This file is used only by the import menu\n" );
    fprintf( filewrite, ": %d\n", ChrList[character].skin );
    fclose( filewrite );

    return btrue;
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

    if ( !VALID_CAP_RANGE( object ) ) return MAX_PROFILE;
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
    if ( 'T' == toupper(cTmp) || !cfg.gourard_req )  pcap->uniformlit = btrue;

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
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pcap->stoppedby = iTmp | MPDFX_IMPASS;
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
        else if ( idsz == Make_IDSZ( "LIFE" ) ) pcap->spawnlife = 256*fget_float( fileread );
        else if ( idsz == Make_IDSZ( "MANA" ) ) pcap->spawnmana = 256*fget_float( fileread );

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

    if ( INVALID_CHR(character) ) return;

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
            manadamage = MAX(ChrList[character].mana - damage, 0);
            ChrList[character].mana = manadamage;
            damage -= manadamage;
            ChrList[character].ai.alert |= ALERTIF_ATTACKED;
            ChrList[character].ai.attacklast = attacker;
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

                //Hard mode deals 50% extra to players damage!
                if ( cfg.difficulty >= GAME_HARD && !ChrList[attacker].isplayer && ChrList[character].isplayer ) damage *= 1.5f;

                //East mode deals 25% extra damage by players and 25% less to players
                if ( cfg.difficulty <= GAME_EASY )
                {
                    if ( ChrList[attacker].isplayer && !ChrList[character].isplayer ) damage *= 1.25f;
                    if ( !ChrList[attacker].isplayer && ChrList[character].isplayer ) damage *= 0.75f;
                }

                if ( 0 == ( effects & DAMFX_NBLOC ) )
                {
                    // Only damage if hitting from proper direction
                    if ( Md2FrameList[ChrList[character].inst.frame_nxt].framefx & MADFX_INVICTUS )
                    {
                        // I Frame...
                        direction -= CapList[model].iframefacing;
                        left  = 0xFFFF - CapList[model].iframeangle;
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
                                    left  = 0xFFFF - CapList[ChrList[ChrList[character].holdingwhich[SLOT_LEFT]].model].iframeangle;
                                    right = CapList[ChrList[ChrList[character].holdingwhich[SLOT_LEFT]].model].iframeangle;
                                }
                            }
                            else
                            {
                                // Check right hand
                                if ( ChrList[character].holdingwhich[SLOT_RIGHT] != MAX_CHR )
                                {
                                    left  = 0xFFFF - CapList[ChrList[ChrList[character].holdingwhich[SLOT_RIGHT]].model].iframeangle;
                                    right = CapList[ChrList[ChrList[character].holdingwhich[SLOT_RIGHT]].model].iframeangle;
                                }
                            }
                        }
                    }
                    else
                    {
                        // N Frame
                        direction -= CapList[model].nframefacing;
                        left = 0xFFFF - CapList[model].nframeangle;
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
                        ChrList[character].life -= FF_MUL( damage, ChrList[character].defense );
                    }
                    if ( basedamage > HURTDAMAGE )
                    {
                        // Spawn blud particles
                        if ( CapList[model].bludvalid && ( damagetype < DAMAGE_HOLY || CapList[model].bludvalid == ULTRABLUDY ) )
                        {
                            spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].pos.z,
                                                ChrList[character].turn_z + direction, ChrList[character].model, CapList[model].bludprttype,
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
                        //disenchant_character( character );
                        ChrList[character].waskilled = btrue;
                        ChrList[character].keepaction = btrue;
                        ChrList[character].life = -1;
                        ChrList[character].platform = btrue;
                        ChrList[character].bumpdampen = ChrList[character].bumpdampen / 2.0f;
                        action = ACTION_KA;

                        // Give kill experience
                        experience = CapList[model].experienceworth + ( ChrList[character].experience * CapList[model].experienceexchange );
                        if ( VALID_CHR(attacker) )
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
                        ChrList[character].ai.timer = update_wld + 1;  // No timeout...
                        looped_stop_object_sounds( character );          //Stop sound loops

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
                    spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].pos.z, ChrList[character].turn_z, MAX_PROFILE, DEFEND, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, 0, MAX_CHR );
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
void kill_character( Uint16 character, Uint16 killer )
{
    // ZZ> This function kills a character...  MAX_CHR killer for accidental death

    Uint8 modifier;
    chr_t * pchr;

    if ( INVALID_CHR( character ) ) return;
    pchr = ChrList + character;

    if ( pchr->alive )
    {
        pchr->damagetime = 0;
        pchr->life = 1;
        modifier = pchr->damagemodifier[DAMAGE_CRUSH];
        pchr->damagemodifier[DAMAGE_CRUSH] = 1;

        if ( VALID_CHR( killer ) )
        {
            damage_character( character, 0, 512, 1, DAMAGE_CRUSH, ChrList[killer].team, killer, DAMFX_ARMO | DAMFX_NBLOC, btrue );
        }
        else
        {
            damage_character( character, 0, 512, 1, DAMAGE_CRUSH, DAMAGETEAM, pchr->ai.bumplast, DAMFX_ARMO | DAMFX_NBLOC, btrue );
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

    if ( INVALID_CHR( character ) ) return;

    sTmp = ChrList[character].turn_z;
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

    pself->order_counter = rank;
    pself->order_value   = 0;

    pself->wp_tail = 0;
    pself->wp_head = 0;
    for ( tnc = 0; tnc < MAXSTOR; tnc++ )
    {
        pself->x[tnc] = 0;
        pself->y[tnc] = 0;
    }
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_character( GLvector3 pos, Uint16 profile, Uint8 team,
                            Uint8 skin, Uint16 facing, const char *name, int override )
{
    // ZZ> This function spawns a character and returns the character's index number
    //     if it worked, MAX_CHR otherwise

    Uint16 ichr, kursechance;
    int cnt, tnc;
    chr_t * pchr;
    cap_t * pcap;
    float nrm[2];
    GLvector3 pos_tmp;

    if ( profile >= MAX_PROFILE )
    {
        log_warning( "spawn_one_character() - profile value too large %d out of %d\n", profile, MAX_PROFILE );
        return MAX_CHR;
    }

    if ( !MadList[profile].loaded )
    {
        if ( profile > PMod->importamount * 9 )
        {
            log_warning( "spawn_one_character() - trying to spawn using invalid profile %d\n", profile );
        }

        return MAX_CHR;
    }

    // allocate a new character
    ichr = MAX_CHR;
    if ( VALID_CHR_RANGE(override) )
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

    if ( !VALID_CHR_RANGE(ichr) )
    {
        log_warning( "spawn_one_character() - failed to spawn character\n" );
        return ichr;
    }
    pchr = ChrList + ichr;
    pcap = CapList + profile;

    // make a copy of the data in pos
    pos_tmp = pos;

    // clear out all data
    memset(pchr, 0, sizeof(chr_t));

    // Make sure the team is valid
    team = MIN( team, MAXTEAM - 1 );

    // IMPORTANT!!!
    pchr->isequipped = bfalse;
    pchr->sparkle = NOSPARKLE;
    pchr->overlay = bfalse;
    pchr->missilehandler = ichr;
    pchr->loopedsound_channel = -1;

    // sound stuff...  copy from the cap
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pchr->soundindex[tnc] = pcap->soundindex[tnc];
    }

    // Set up model stuff
    pchr->reloadtime = 0;
    pchr->inwhich_slot = SLOT_LEFT;
    pchr->waskilled = bfalse;
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
    kursechance = pcap->kursechance;
    if ( cfg.difficulty >= GAME_HARD )                        kursechance *= 2.0f;  //Hard mode doubles chance for Kurses
    if ( cfg.difficulty < GAME_NORMAL && kursechance != 100 ) kursechance *= 0.5f;  //Easy mode halves chance for Kurses
    pchr->iskursed = ( ( rand() % 100 ) < kursechance );
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
    pchr->lifereturn = pcap->lifereturn;
    pchr->manamax = generate_number( pcap->manabase, pcap->manarand );
    pchr->manaflow = generate_number( pcap->manaflowbase, pcap->manaflowrand );
    pchr->manareturn = generate_number( pcap->manareturnbase, pcap->manareturnrand );
    
	//Load current life and mana or refill them (based on difficulty)
	if( cfg.difficulty >= GAME_NORMAL ) pchr->life = CLIP( pcap->spawnlife, LOWSTAT, pchr->lifemax );
	else pchr->life = pchr->lifemax;
	if( cfg.difficulty >= GAME_NORMAL ) pchr->mana = CLIP( pcap->spawnmana, 0, pchr->manamax );
	else pchr->mana = pchr->manamax;

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
    pchr->jump_power = pcap->jump;
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
    for (cnt = 0; cnt < SLOT_COUNT; cnt++)
    {
        pchr->holdingwhich[cnt] = MAX_CHR;
    }

    // pack/inventory info
    pchr->pack_ispacked = bfalse;
    pchr->pack_next = MAX_CHR;
    pchr->pack_count = 0;
    for (cnt = 0; cnt < INVEN_COUNT; cnt++)
    {
        pchr->inventory[cnt] = MAX_CHR;
    }

    // Image rendering
    pchr->uoffvel = pcap->uoffvel;
    pchr->voffvel = pcap->voffvel;

    // Movement
    pchr->sneakspd = pcap->sneakspd;
    pchr->walkspd = pcap->walkspd;
    pchr->runspd = pcap->runspd;

    // Set up position
    pchr->floor_level = get_mesh_level( PMesh, pos_tmp.x, pos_tmp.y, pchr->waterwalk ) + RAISE;
    if ( pos_tmp.z < pchr->floor_level ) pos_tmp.z = pchr->floor_level;
    pchr->pos = pos_tmp;

    pchr->vel.x = 0;
    pchr->vel.y = 0;
    pchr->vel.z = 0;

    pchr->turn_z     = facing;
    pchr->map_turn_y = 32768;  // These two mean on level surface
    pchr->map_turn_x = 32768;

    pchr->pos_safe = pchr->pos;
    pchr->pos_stt  = pchr->pos;

    pchr->pos_old  = pchr->pos;
    pchr->vel_old.x = 0;
    pchr->vel_old.y = 0;
    pchr->vel_old.z = 0;
    pchr->turn_old_z = pchr->turn_z;

    pchr->phys.level    = pchr->floor_level;
    pchr->onwhichfan    = mesh_get_tile( PMesh, pchr->pos.x, pchr->pos.y );
    pchr->onwhichblock  = mesh_get_block( PMesh, pchr->pos.x, pchr->pos.y );

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
    chr_instance_init( &(pchr->inst), profile, skin );

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
		pchr->iskursed = bfalse;				//Shop items are never kursed
    }
    else
    {
        pchr->isshopitem = bfalse;
        if (pchr->isitem && !pchr->pack_ispacked && pchr->attachedto == MAX_CHR)
        {
            float tlx, tly, brx, bry;
            Uint16 passage = 0;
            float bumpsize;

            bumpsize = pchr->bumpsize;

            while (passage < numpassage)
            {
                // Passage area
                tlx = ( passtlx[passage] << TILE_BITS ) - CLOSETOLERANCE;
                tly = ( passtly[passage] << TILE_BITS ) - CLOSETOLERANCE;
                brx = ( ( passbrx[passage] + 1 ) << TILE_BITS ) + CLOSETOLERANCE;
                bry = ( ( passbry[passage] + 1 ) << TILE_BITS ) + CLOSETOLERANCE;

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

    if ( 0 == __chrhitawall(ichr, nrm) )
    {
        pchr->safe_valid = btrue;
    };

    pchr->on = btrue;

    return ichr;
}

//--------------------------------------------------------------------------------------------
void respawn_character( Uint16 character )
{
    // ZZ> This function respawns a character
    Uint16 item;

    chr_t * pchr;
    cap_t * pcap;

    if ( INVALID_CHR( character ) || ChrList[character].alive ) return;
    pchr = ChrList + character;

    if ( INVALID_CAP(pchr->model) ) return;
    pcap = CapList + pchr->model;

    spawn_poof( character, pchr->model );
    disaffirm_attached_particles( character );

    pchr->alive = btrue;
    pchr->boretime = BORETIME;
    pchr->carefultime = CAREFULTIME;
    pchr->life = pchr->lifemax;
    pchr->mana = pchr->manamax;
    pchr->pos  = pchr->pos_stt;
    pchr->vel.x = 0;
    pchr->vel.y = 0;
    pchr->vel.z = 0;
    pchr->team = pchr->baseteam;
    pchr->canbecrushed = bfalse;
    pchr->map_turn_y = 32768;  // These two mean on level surface
    pchr->map_turn_x = 32768;
    if ( NOLEADER == TeamList[pchr->team].leader )  TeamList[pchr->team].leader = character;
    if ( !pchr->invictus )  TeamList[pchr->baseteam].morale++;

    pchr->actionready = btrue;
    pchr->keepaction = bfalse;
    pchr->loopaction = bfalse;
    pchr->action = ACTION_DA;
    pchr->nextaction = ACTION_DA;

    pchr->platform = pcap->platform;
    pchr->flyheight = pcap->flyheight;
    pchr->bumpdampen = pcap->bumpdampen;
    pchr->bumpsize = pcap->bumpsize * pchr->fat;
    pchr->bumpsizebig = pcap->bumpsizebig * pchr->fat;
    pchr->bumpheight = pcap->bumpheight * pchr->fat;

    pchr->bumpsizesave = pcap->bumpsize;
    pchr->bumpsizebigsave = pcap->bumpsizebig;
    pchr->bumpheightsave = pcap->bumpheight;

    pchr->ai.alert = 0;
    pchr->ai.target = character;
    pchr->ai.timer  = 0;

    pchr->grogtime = 0;
    pchr->dazetime = 0;
    reaffirm_attached_particles( character );

    // Let worn items come back
    for ( item = pchr->pack_next; item < MAX_CHR; item = ChrList[item].pack_next )
    {
        if ( ChrList[item].on && ChrList[item].isequipped )
        {
            ChrList[item].isequipped = bfalse;
            ChrList[item].ai.alert |= ALERTIF_ATLASTWAYPOINT;  // doubles as PutAway
        }
    }

    // re-initialize the instance
    chr_instance_init( &(pchr->inst), pchr->model, pchr->skin );
}

//--------------------------------------------------------------------------------------------
int chr_change_skin( Uint16 character, int skin )
{
    Uint16 model, imad;

    if ( INVALID_CHR(character) ) return 0;

    model = ChrList[character].model;
    if ( model >= MAX_PROFILE || !MadList[model].loaded )
    {
        ChrList[character].skin    = 0;
        ChrList[character].inst.texture = TX_WATER_TOP;
        return 0;
    }

    // make sure that the instance has a valid model
    imad = ChrList[character].inst.imad;
    if ( imad >= MAX_PROFILE || !MadList[imad].loaded )
    {
        imad = model;
        ChrList[character].inst.imad = model;
    }

    // do the best we can to change the skin
    if ( MadList[imad].skins == 0 )
    {
        ChrList[character].skin    = 0;
        ChrList[character].inst.texture = TX_WATER_TOP;
    }
    else
    {
        // limit the skin
        if ( skin > MadList[imad].skins) skin = 0;

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

    if ( INVALID_CHR(character) ) return 0;

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
void change_character_full( Uint16 ichr, Uint16 profile, Uint8 skin, Uint8 leavewhich )
{
    //This function polymorphs a character permanently so that it can be exported properly
    //A character turned into a frog with this function will also export as a frog!
    if ( profile > MAX_PROFILE || !MadList[profile].loaded ) return;

    strcpy(MadList[ChrList[ichr].model].name, MadList[profile].name);
    change_character( ichr, profile, skin, leavewhich );
    ChrList[ichr].basemodel = profile;
}

//--------------------------------------------------------------------------------------------
void set_weapongrip( Uint16 iitem, Uint16 iholder, Uint16 vrt_off )
{
    int i, tnc;
    chr_t * pitem, *pholder;
    mad_t * pholder_mad;

    if( INVALID_CHR(iitem) ) return;
    pitem = ChrList + iitem;

    // reset the vertices
    for (i = 0; i < GRIP_VERTS; i++)
    {
        pitem->weapongrip[i] = 0xFFFF;
    }

    if( INVALID_CHR(iholder) ) return;
    pholder = ChrList + iholder;

    if( INVALID_MAD(pholder->inst.imad) ) return;
    pholder_mad = MadList + pholder->inst.imad;

    tnc = pholder_mad->md2.vertices - vrt_off;
    for (i = 0; i < GRIP_VERTS; i++)
    {
        if (tnc + i < pholder_mad->md2.vertices )
        {
            pitem->weapongrip[i] = tnc + i;
        }
        else
        {
            pitem->weapongrip[i] = 0xFFFF;
        }
    }
}

//--------------------------------------------------------------------------------------------
void change_character( Uint16 ichr, Uint16 profile, Uint8 skin, Uint8 leavewhich )
{
    // ZZ> This function polymorphs a character, changing stats, dropping weapons
    int tnc, enchant;
    Uint16 sTmp, item;
    chr_t * pchr;
    cap_t * pcap;
    mad_t * pmad;

    if ( INVALID_MAD(profile) || INVALID_CHR(ichr) ) return;
    pchr = ChrList + ichr;
    pcap = CapList + profile;
    pmad = MadList + profile;

    // Drop left weapon
    sTmp = pchr->holdingwhich[SLOT_LEFT];
    if ( sTmp != MAX_CHR && ( !pcap->slotvalid[SLOT_LEFT] || pcap->ismount ) )
    {
        detach_character_from_mount( sTmp, btrue, btrue );
        if ( pchr->ismount )
        {
            ChrList[sTmp].vel.z = DISMOUNTZVEL;
            ChrList[sTmp].pos.z += DISMOUNTZVEL;
            ChrList[sTmp].jumptime = JUMPDELAY;
        }
    }

    // Drop right weapon
    sTmp = pchr->holdingwhich[SLOT_RIGHT];
    if ( sTmp != MAX_CHR && !pcap->slotvalid[SLOT_RIGHT] )
    {
        detach_character_from_mount( sTmp, btrue, btrue );
        if ( pchr->ismount )
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
        enchant = pchr->firstenchant;
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
    pchr->model = profile;
    pchr->stoppedby = pcap->stoppedby;
    pchr->lifeheal = pcap->lifeheal;
    pchr->manacost = pcap->manacost;

    // Ammo
    pchr->ammomax = pcap->ammomax;
    pchr->ammo = pcap->ammo;

    // Gender
    if ( pcap->gender != GENRANDOM )  // GENRANDOM means keep old gender
    {
        pchr->gender = pcap->gender;
    }

    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pchr->soundindex[tnc] = pcap->soundindex[tnc];
    }

    // AI stuff
    pchr->ai.type = pmad->ai;
    pchr->ai.state = 0;
    pchr->ai.timer = 0;

    pchr->latchx = 0;
    pchr->latchy = 0;
    pchr->latchbutton = 0;
    pchr->turnmode = TURNMODEVELOCITY;

    // Flags
    pchr->stickybutt = pcap->stickybutt;
    pchr->openstuff = pcap->canopenstuff;
    pchr->transferblend = pcap->transferblend;
    pchr->platform = pcap->platform;
    pchr->isitem = pcap->isitem;
    pchr->invictus = pcap->invictus;
    pchr->ismount = pcap->ismount;
    pchr->cangrabmoney = pcap->cangrabmoney;
    pchr->jumptime = JUMPDELAY;

    // Character size and bumping
    pchr->shadowsize = (Uint8)(pcap->shadowsize * pchr->fat);
    pchr->bumpsize = (Uint8) (pcap->bumpsize * pchr->fat);
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

    // Character scales...  Magic numbers
    if ( VALID_CHR(pchr->attachedto) )
    {
        set_weapongrip( ichr, pchr->attachedto, slot_to_grip_offset(pchr->inwhich_slot) );
    }

    item = pchr->holdingwhich[SLOT_LEFT];
    if ( VALID_CHR(item) )
    {
        set_weapongrip( item, ichr, GRIP_LEFT );
    }

    item = pchr->holdingwhich[SLOT_RIGHT];
    if ( VALID_CHR(item) )
    {
        set_weapongrip( item, ichr, GRIP_RIGHT );
    }


    // Image rendering
    pchr->uoffvel = pcap->uoffvel;
    pchr->voffvel = pcap->voffvel;

    // Movement
    pchr->sneakspd = pcap->sneakspd;
    pchr->walkspd = pcap->walkspd;
    pchr->runspd = pcap->runspd;

    // AI and action stuff
    pchr->actionready = btrue;
    pchr->keepaction = bfalse;
    pchr->loopaction = bfalse;
    pchr->action = ACTION_DA;
    pchr->nextaction = ACTION_DA;
    pchr->holdingweight = 0;
    pchr->onwhichplatform = MAX_CHR;

    // initialize the instance
    chr_instance_init( &(pchr->inst), profile, skin );

    // Set the skin
    change_armor( ichr, skin );

    // Reaffirm them particles...
    pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;
    reaffirm_attached_particles( ichr );
}

//--------------------------------------------------------------------------------------------
bool_t cost_mana( Uint16 character, int amount, Uint16 killer )
{
    // ZZ> This function takes mana from a character ( or gives mana ),
    //     and returns btrue if the character had enough to pay, or bfalse
    //     otherwise. This can kill a character in hard mode.

    int mana_final;
    bool_t mana_paid;

    chr_t * pchr;

    if ( INVALID_CHR(character) ) return bfalse;
    pchr = ChrList + character;

    mana_paid  = bfalse;
    mana_final = pchr->mana - amount;

    if ( mana_final < 0 )
    {
        int mana_debt = -mana_final;

        pchr->mana = 0;

        if ( pchr->canchannel )
        {
            pchr->life -= mana_debt;

            if ( pchr->life <= 0 && cfg.difficulty >= GAME_HARD )
            {
                kill_character( character, INVALID_CHR(killer) ? character : killer );
            }

            mana_paid = btrue;
        }
    }
    else
    {
        int mana_surplus = 0;

        pchr->mana = mana_final;

        if ( mana_final > pchr->manamax )
        {
            mana_surplus = mana_final - pchr->manamax;
            pchr->mana   = pchr->manamax;
        }

        // allow surplus mana to go to health if you can channel?
        if ( pchr->canchannel && mana_surplus > 0 )
        {
            // use some factor, like divide by 2?
            pchr->life += mana_surplus;

            if ( pchr->life > pchr->lifemax )
            {
                pchr->life = pchr->lifemax;
            }
        }

        mana_paid = btrue;

    }

    return mana_paid;
}

//--------------------------------------------------------------------------------------------
void switch_team( Uint16 character, Uint8 team )
{
    // ZZ> This function makes a character join another team...

    if ( INVALID_CHR(character) || team >= MAXTEAM ) return;

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

//--------------------------------------------------------------------------------------------
void issue_clean( Uint16 character )
{
    // ZZ> This function issues a clean up order to all teammates
    Uint8 team;
    Uint16 cnt;

    if ( INVALID_CHR(character) ) return;

    team = ChrList[character].team;
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ChrList[cnt].on || team != ChrList[cnt].team ) continue;

        if ( !ChrList[cnt].alive )
        {
            ChrList[cnt].ai.timer  = update_wld + 2;  // Don't let it think too much...
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

    if ( INVALID_CHR(character) ) return 0;

    amount = 0;
    model = ChrList[character].model;
    if ( INVALID_CAP(model) ) return 0;

    if ( CapList[model].idsz[IDSZ_PARENT] == idsz || CapList[model].idsz[IDSZ_TYPE] == idsz )
    {
        if ( ChrList[character].ammo < ChrList[character].ammomax )
        {
            amount = ChrList[character].ammomax - ChrList[character].ammo;
            ChrList[character].ammo = ChrList[character].ammomax;
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
                    QuestLevel = adjustment;
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
const char * undo_idsz( IDSZ idsz )
{
    // ZZ> This function takes an integer and makes a text IDSZ out of it.

    static char value_string[5] = {"NONE"};

    if ( idsz == IDSZ_NONE )
    {
        strncpy( value_string, "NONE", SDL_arraysize( value_string ) );
    }
    else
    {
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
    else if ( Make_IDSZ( "READ" ) == whichskill ) result = ChrList[who].canread || ( ChrList[who].canseeinvisible && ChrList[who].canseekurse ); //Truesight allows reading

    return result;
}

//--------------------------------------------------------------------------------------------
void move_characters( void )
{
    // ZZ> This function handles character physics
    Uint16 cnt;
    Uint8 twist, actionready;
    Uint8 speed, framelip, allowedtoattack;
    float level, friction_xy, friction_z;
    float dvx, dvy, dvmax;
    Uint16 action, weapon, mount, item;
    bool_t watchtarget;
    float nrm[2], ftmp;
    const float blah_friction = 0.9868f;
    float fric_acc_x, fric_acc_y;

    bool_t is_slippy, is_watery;

    // Move every character
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        float zlerp;
        float new_vx, new_vy;
        float traction;

        chr_t * pchr;

        if ( !ChrList[cnt].on || ChrList[cnt].pack_ispacked ) continue;
        pchr = ChrList + cnt;

        // Down that ol' damage timer
        if (pchr->damagetime > 0) pchr->damagetime--;

        // Character's old location
        pchr->pos_old    = pchr->pos;
        pchr->vel_old    = pchr->vel;
        pchr->turn_old_z = pchr->turn_z;

        new_vx = pchr->vel.x;
        new_vy = pchr->vel.y;

        // Is the character in the air?
        level = pchr->phys.level;
        zlerp = (pchr->pos.z - level) / PLATTOLERANCE;
        zlerp = CLIP(zlerp, 0, 1);

        pchr->phys.grounded = (0 == pchr->flyheight) && (zlerp < 0.25f);

        // get the tile twist
        twist = TWIST_FLAT;
        if ( VALID_TILE(PMesh, pchr->onwhichfan) && INVALID_CHR(pchr->onwhichplatform) )
        {
            twist = PMesh->mmem.tile_list[pchr->onwhichfan].twist;
        }
        is_watery = water_data.is_water && pchr->inwater;
        is_slippy = !is_watery && (0 != mesh_test_fx( PMesh, pchr->onwhichfan, MPDFX_SLIPPY ));

        traction = 1.0f;
        if ( 0 != pchr->flyheight )
        {
            // any traction factor here
            /* traction = ??; */
        }
        else if ( VALID_CHR( pchr->onwhichplatform ) )
        {
            // any traction factor here
            /* traction = ??; */
        }
        else if ( VALID_TILE(PMesh, pchr->onwhichfan) )
        {
            traction = ABS(map_twist_nrm[twist].z) * (1.0f - zlerp) + 0.25 * zlerp;

            if ( is_slippy )
            {
                traction /= hillslide * (1.0f - zlerp) + 1.0f * zlerp;
            }
        }

        // Texture movement
        pchr->inst.uoffset += pchr->uoffvel;
        pchr->inst.voffset += pchr->voffvel;

        if ( VALID_CHR(pchr->onwhichplatform) )
        {
            chr_t * pplat = ChrList + pchr->onwhichplatform;

            fric_acc_x = (pplat->vel.x - pplat->vel_old.x) * (1.0f - zlerp) * platstick;
            fric_acc_y = (pplat->vel.y - pplat->vel_old.y) * (1.0f - zlerp) * platstick;
        }
        else if ( !pchr->alive )
        {
            fric_acc_x = -pchr->vel.x * (1.0f - zlerp) * 0.5f;
            fric_acc_y = -pchr->vel.y * (1.0f - zlerp) * 0.5f;
        }
        else
        {
            fric_acc_x = 0.0f;
            fric_acc_y = 0.0f;
        }

        // do friction with the floor before volontary motion
        if ( 0 == pchr->flyheight )
        {
            pchr->vel.x += fric_acc_x * traction;
            pchr->vel.y += fric_acc_y * traction;
        }

        if ( pchr->alive )
        {
            new_vx = pchr->vel.x;
            new_vy = pchr->vel.y;
            if ( pchr->attachedto == MAX_CHR )
            {
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

                new_vx = dvx * pchr->maxaccel * airfriction / (1.0f - airfriction);
                new_vy = dvy * pchr->maxaccel * airfriction / (1.0f - airfriction);

                if ( VALID_CHR(pchr->onwhichplatform) )
                {
                    chr_t * pplat = ChrList + pchr->onwhichplatform;

                    new_ax = (pplat->vel.x + new_vx - pchr->vel.x);
                    new_ay = (pplat->vel.y + new_vy - pchr->vel.y);
                }
                else
                {
                    new_ax = (new_vx - pchr->vel.x);
                    new_ay = (new_vy - pchr->vel.y);
                }

                dvmax = pchr->maxaccel;
                if ( new_ax < -dvmax ) new_ax = -dvmax;
                if ( new_ax >  dvmax ) new_ax =  dvmax;
                if ( new_ay < -dvmax ) new_ay = -dvmax;
                if ( new_ay >  dvmax ) new_ay =  dvmax;

                new_vx = new_ax * airfriction / (1.0f - airfriction);
                new_vy = new_ay * airfriction / (1.0f - airfriction);

                new_ax *= traction;
                new_ay *= traction;

                // Get direction from the DESIRED change in velocity
                if ( pchr->turnmode == TURNMODEWATCH )
                {
                    if ( ( ABS( dvx ) > WATCHMIN || ABS( dvy ) > WATCHMIN ) )
                    {
                        pchr->turn_z = terp_dir( pchr->turn_z, ( ATAN2( dvx, dvy ) + PI ) * 0xFFFF / ( TWO_PI ) );
                    }
                }

                // Face the target
                watchtarget = ( pchr->turnmode == TURNMODEWATCHTARGET );
                if ( watchtarget )
                {
                    if ( cnt != pchr->ai.target )
                    {
                        pchr->turn_z = terp_dir( pchr->turn_z, ( ATAN2( ChrList[pchr->ai.target].pos.y - pchr->pos.y, ChrList[pchr->ai.target].pos.x - pchr->pos.x ) + PI ) * 0xFFFF / ( TWO_PI ) );
                    }
                }

                if ( Md2FrameList[pchr->inst.frame_nxt].framefx & MADFX_STOP )
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
                            pchr->turn_z = terp_dir_fast( pchr->turn_z, ( ATAN2( dvy, dvx ) + PI ) * 0xFFFF / ( TWO_PI ) );
                        }
                        else
                        {
                            // AI turn slowly
                            pchr->turn_z = terp_dir( pchr->turn_z, ( ATAN2( dvy, dvx ) + PI ) * 0xFFFF / ( TWO_PI ) );
                        }
                    }
                }

                // Otherwise make it spin
                else if ( pchr->turnmode == TURNMODESPIN )
                {
                    pchr->turn_z += SPINRATE;
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
                            sound_play_chunk( pchr->pos, CapList[pchr->model].wavelist[ijump] );
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
                                pchr->vel.z = WATERJUMP * 1.5;
                            }
                            else
                            {
                                pchr->vel.z = pchr->jump_power * 1.5;
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
                                    sound_play_chunk( pchr->pos, CapList[pchr->model].wavelist[ijump] );
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
                            if ( pchr->isplayer && CapList[ChrList[item].model].istoobig )
                            {
                                STRING text;
                                snprintf( text, sizeof(text), "The %s is too big to be put away...", CapList[ChrList[item].model].classname);
                                debug_message( text );
                            }
                        }
                        else
                        {
                            // Put the item into the pack
                            inventory_add_item( item, cnt );
                        }
                    }
                    else
                    {
                        // Get a new one out and put it in hand
                        inventory_get_item( cnt, GRIP_LEFT, bfalse );
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
                            if ( pchr->isplayer && CapList[ChrList[item].model].istoobig )
                            {
                                STRING text;
                                snprintf( text, sizeof(text), "The %s is too big to be put away...", CapList[ChrList[item].model].classname);
                                debug_message( text );
                            }
                        }
                        else
                        {
                            // Put the item into the pack
                            inventory_add_item( item, cnt );
                        }
                    }
                    else
                    {
                        // Get a new one out and put it in hand
                        inventory_get_item( cnt, GRIP_RIGHT, bfalse );
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
                                bool_t mana_paid = cost_mana( cnt, ChrList[weapon].manacost, weapon );

                                if ( mana_paid )
                                {
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
                                bool_t mana_paid = cost_mana( cnt, ChrList[weapon].manacost, weapon );
                                // Check mana cost
                                if ( mana_paid )
                                {
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

        //  Flying
        if ( 0 != pchr->flyheight )
        {
            level = pchr->phys.level;
            if ( level < 0 ) level = 0;  // fly above pits...

            pchr->vel.z += ( level + pchr->flyheight - pchr->pos.z ) * FLYDAMPEN;
        }

        // determine the character environment
        friction_z = blah_friction;       // like real-life air friction
        if ( 0 != pchr->flyheight )
        {
            //  Flying
            pchr->jumpready = bfalse;

            // Airborne characters still get friction_xy to make control easier
            friction_xy = blah_friction;
            friction_z = 1.0f;
        }
        else
        {
            float temp_friction_xy = blah_friction;

            // Character is in the air
            pchr->jumpready = pchr->phys.grounded;

            // Down jump timer
            if ( pchr->jumptime > 0 ) pchr->jumptime--;

            // Do ground hits
            if ( pchr->phys.grounded && pchr->vel.z < -STOPBOUNCING && pchr->hitready )
            {
                pchr->ai.alert |= ALERTIF_HITGROUND;
                pchr->hitready = bfalse;
            }

            // Make the characters slide
            temp_friction_xy = noslipfriction;
            if ( VALID_TILE(PMesh, pchr->onwhichfan) )
            {
                if ( is_slippy )
                {
                    // It's slippy all right...
                    temp_friction_xy = slippyfriction;

                    if ( map_twist_flat[twist] )
                    {
                        // Reset jumping on flat areas of slippiness
                        pchr->jumpnumber = pchr->jumpnumberreset;
                    }
                }
                else
                {
                    // Reset jumping
                    temp_friction_xy = noslipfriction;
                    pchr->jumpnumber = pchr->jumpnumberreset;
                }
            }

            friction_xy = zlerp * blah_friction + (1.0f - zlerp) * temp_friction_xy;
        }

        // override friction with water friction, if appropriate
        if ( is_watery )
        {
            friction_z  = waterfriction;
            friction_xy = waterfriction;
        }

        // do gravity
        if ( 0 == pchr->flyheight )
        {
            if ( is_slippy && pchr->weight != 0xFFFFFFFF &&
                    twist != TWIST_FLAT && zlerp < 1.0f)
            {
                // Slippy hills make characters slide

                GLvector3 gpara, gperp;

                gperp.x = map_twistvel_x[twist];
                gperp.y = map_twistvel_y[twist];
                gperp.z = map_twistvel_z[twist];

                gpara.x = 0       - gperp.x;
                gpara.y = 0       - gperp.y;
                gpara.z = gravity - gperp.z;

                pchr->vel.x += gpara.x + gperp.x * zlerp;
                pchr->vel.y += gpara.y + gperp.y * zlerp;
                pchr->vel.z += gpara.z + gperp.z * zlerp;
            }
            else
            {
                pchr->vel.z += zlerp * gravity;
            }
        }

        // Move the character
        pchr->pos.z += pchr->vel.z;
        LOG_NAN(pchr->pos.z);
        if ( pchr->pos.z < pchr->floor_level )
        {
            pchr->vel.z *= -pchr->bumpdampen;

            if ( ABS(pchr->vel.z) < STOPBOUNCING )
            {
                pchr->vel.z = 0;
                pchr->pos.z = pchr->phys.level;
            }
            else
            {
                float diff = pchr->phys.level - pchr->pos.z;
                pchr->pos.z = pchr->phys.level + diff;
            }
        }
        else
        {
            pchr->pos_safe.z = pchr->pos.z;
        }

        if ( 0 != pchr->flyheight )
        {
            if ( pchr->pos.z < 0 )
            {
                pchr->pos.z = 0;  // Don't fall in pits...
            }
        }

        ftmp = pchr->pos.x;
        pchr->pos.x += pchr->vel.x;
        LOG_NAN(pchr->pos.x);
        if ( __chrhitawall( cnt, nrm ) )
        {
            if ( ABS(pchr->vel.x) + ABS(pchr->vel.y) > 0 )
            {
                if ( ABS(nrm[XX]) + ABS(nrm[YY]) > 0 )
                {
                    float dotprod;
                    float vpara[2], vperp[2];

                    dotprod = pchr->vel.x * nrm[XX] + pchr->vel.y * nrm[YY];
                    if ( dotprod < 0 )
                    {
                        vperp[XX] = dotprod * nrm[XX];
                        vperp[YY] = dotprod * nrm[YY];

                        vpara[XX] = pchr->vel.x - vperp[XX];
                        vpara[YY] = pchr->vel.y - vperp[YY];

                        pchr->vel.x = vpara[XX] - /* pchr->bumpdampen * */ vperp[XX];
                        pchr->vel.y = vpara[YY] - /* pchr->bumpdampen * */ vperp[YY];
                    }
                }
                else
                {
                    pchr->vel.x *= -1 /* pchr->bumpdampen * */;
                }
            }

            if ( !pchr->safe_valid )
            {
                pchr->pos.x += nrm[XX] * 5;
            }
            else
            {
                pchr->pos.x = pchr->pos_safe.x;
            }
        }
        else
        {
            pchr->pos_safe.x = pchr->pos.x;
        }

        ftmp = pchr->pos.y;
        pchr->pos.y += pchr->vel.y;
        LOG_NAN(pchr->pos.y);
        if ( __chrhitawall( cnt, nrm ) )
        {
            if ( ABS(pchr->vel.x) + ABS(pchr->vel.y) > 0 )
            {
                if ( ABS(nrm[XX]) + ABS(nrm[YY]) > 0 )
                {
                    float dotprod;
                    float vpara[2], vperp[2];

                    dotprod = pchr->vel.x * nrm[XX] + pchr->vel.y * nrm[YY];
                    if ( dotprod < 0 )
                    {
                        vperp[XX] = dotprod * nrm[XX];
                        vperp[YY] = dotprod * nrm[YY];

                        vpara[XX] = pchr->vel.x - vperp[XX];
                        vpara[YY] = pchr->vel.y - vperp[YY];

                        pchr->vel.x = vpara[XX] - /* pchr->bumpdampen * */ vperp[XX];
                        pchr->vel.y = vpara[YY] - /* pchr->bumpdampen * */ vperp[YY];
                    }
                }
                else
                {
                    pchr->vel.y *= - 1 /* pchr->bumpdampen * */;
                }
            }

            if ( !pchr->safe_valid )
            {
                pchr->pos.y += nrm[YY] * 5;
            }
            else
            {
                pchr->pos.y = pchr->pos_safe.y;
            }
        }
        else
        {
            pchr->pos_safe.y = pchr->pos.y;
        }

        if ( __chrhitawall(cnt, nrm) )
        {
            pchr->safe_valid = btrue;
        }

        // Apply fluid friction for next time
        pchr->vel.x *= friction_xy;
        pchr->vel.y *= friction_xy;
        pchr->vel.z *= friction_z;

        // Characters with sticky butts lie on the surface of the mesh
        if ( pchr->stickybutt || !pchr->alive )
        {
            float fkeep = (7 + zlerp) / 8.0f;
            float fnew  = (1 - zlerp) / 8.0f;

            if ( fnew > 0 )
            {
                pchr->map_turn_x = pchr->map_turn_x * fkeep + map_twist_x[twist] * fnew;
                pchr->map_turn_y = pchr->map_turn_y * fkeep + map_twist_y[twist] * fnew;
            }
        }

        // Animate the character
        pchr->inst.lip = ( pchr->inst.lip + 64 );
        if ( pchr->inst.lip == 192 )
        {
            // Check frame effects
            if ( Md2FrameList[pchr->inst.frame_nxt].framefx&MADFX_ACTLEFT )
                character_swipe( cnt, 0 );
            if ( Md2FrameList[pchr->inst.frame_nxt].framefx&MADFX_ACTRIGHT )
                character_swipe( cnt, 1 );
            if ( Md2FrameList[pchr->inst.frame_nxt].framefx&MADFX_GRABLEFT )
                character_grab_stuff( cnt, GRIP_LEFT, bfalse );
            if ( Md2FrameList[pchr->inst.frame_nxt].framefx&MADFX_GRABRIGHT )
                character_grab_stuff( cnt, GRIP_RIGHT, bfalse );
            if ( Md2FrameList[pchr->inst.frame_nxt].framefx&MADFX_CHARLEFT )
                character_grab_stuff( cnt, GRIP_LEFT, btrue );
            if ( Md2FrameList[pchr->inst.frame_nxt].framefx&MADFX_CHARRIGHT )
                character_grab_stuff( cnt, GRIP_RIGHT, btrue );
            if ( Md2FrameList[pchr->inst.frame_nxt].framefx&MADFX_DROPLEFT )
                detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], bfalse, btrue );
            if ( Md2FrameList[pchr->inst.frame_nxt].framefx&MADFX_DROPRIGHT )
                detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], bfalse, btrue );
            if ( Md2FrameList[pchr->inst.frame_nxt].framefx&MADFX_POOF && !pchr->isplayer )
                pchr->ai.poof_time = update_wld;
            if ( Md2FrameList[pchr->inst.frame_nxt].framefx&MADFX_FOOTFALL )
            {
                int ifoot = CapList[pchr->model].soundindex[SOUND_FOOTFALL];
                if ( ifoot >= 0 && ifoot < MAX_WAVE )
                {
                    sound_play_chunk( pchr->pos, CapList[pchr->model].wavelist[ifoot] );
                }
            }
        }

        if ( pchr->inst.lip == 0 )
        {
            // Change frames
            pchr->inst.frame_lst = pchr->inst.frame_nxt;
            pchr->inst.frame_nxt++;
            if ( pchr->inst.frame_nxt == MadList[pchr->inst.imad].actionend[pchr->action] )
            {
                // Action finished
                if ( pchr->keepaction )
                {
                    // Keep the last frame going
                    pchr->inst.frame_nxt = pchr->inst.frame_lst;
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

                    pchr->inst.frame_nxt = MadList[pchr->inst.imad].actionstart[pchr->action];
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
            framelip = Md2FrameList[pchr->inst.frame_nxt].framelip;  // 0 - 15...  Way through animation
            if ( pchr->actionready && pchr->inst.lip == 0 && pchr->phys.grounded && pchr->flyheight == 0 && ( framelip&7 ) < 2 )
            {
                // Do the motion stuff
                speed = ABS( new_vx ) + ABS( new_vy );
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
                            pchr->inst.frame_nxt = MadList[pchr->inst.imad].actionstart[pchr->action];
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
                            pchr->inst.frame_nxt = MadList[pchr->inst.imad].frameliptowalkframe[LIPWA][framelip];
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
                                pchr->inst.frame_nxt = MadList[pchr->inst.imad].frameliptowalkframe[LIPWB][framelip];
                                pchr->action = ACTION_WB;
                            }
                        }
                        else
                        {
                            pchr->nextaction = ACTION_WC;
                            if ( pchr->action != ACTION_WC )
                            {
                                pchr->inst.frame_nxt = MadList[pchr->inst.imad].frameliptowalkframe[LIPWC][framelip];
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
        if ( !ChrList[cnt].on || !(ChrList[cnt].ai.poof_time >= 0 && ChrList[cnt].ai.poof_time <= (Sint32)update_wld)  ) continue;

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

        looped_stop_object_sounds( cnt );
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

//--------------------------------------------------------------------------------------------
bool_t is_invictus_direction( Uint16 direction, Uint16 character, Uint16 effects )
{
    Uint16 left, right;

    chr_t * pchr;
    cap_t * pcap;
    mad_t * pmad;

    bool_t is_invictus;

    if ( INVALID_CHR(character) ) return btrue;
    pchr = ChrList + character;

    if ( INVALID_MAD(pchr->inst.imad) ) return btrue;
    pmad = MadList + pchr->inst.imad;

    if ( INVALID_CAP( pchr->model ) ) return btrue;
    pcap = CapList + pchr->model;

    // if the invictus flag is set, we are invictus
    if ( pchr->invictus ) return btrue;

    // if the character's frame is invictus, then check the angles
    if ( Md2FrameList[pchr->inst.frame_nxt].framefx & MADFX_INVICTUS )
    {
        // I Frame
        if ( effects & DAMFX_NBLOC )
        {
            left  = 0xFFFF;
            right = 0;
        }
        else
        {
            direction -= pcap->iframefacing;
            left       = 0xFFFF - pcap->iframeangle;
            right      = pcap->iframeangle;
        }
    }
    else
    {
        // N Frame
        direction -= pcap->nframefacing;
        left       = 0xFFFF - pcap->nframeangle;
        right      = pcap->nframeangle;
    }

    // Check that direction
    is_invictus = btrue;
    if ( direction <= left && direction >= right )
    {
        is_invictus = bfalse;
    }

    return is_invictus;
}

//--------------------------------------------------------------------------------------------
bool_t chr_instance_init( chr_instance_t * pinst, Uint16 profile, Uint8 skin )
{
    int tnc;
	Sint8 greensave = 0, redsave = 0, bluesave = 0;

    mad_t * pmad;
    cap_t * pcap;

    if ( NULL == pinst ) return bfalse;

	//Remember any previous color shifts in case of lasting enchantments
	greensave = pinst->grnshift;
	redsave = pinst->redshift;
	bluesave = pinst->blushift;

    // clear the instance
    memset(pinst, 0, sizeof(chr_instance_t));
    pinst->imad = MAX_PROFILE;

    if ( INVALID_MAD(profile) ) return bfalse;
    pmad = MadList + profile;

    if ( INVALID_CAP(profile) ) return bfalse;
    pcap = CapList + profile;

    pinst->imad      = profile;
    pinst->texture   = pmad->skinstart + skin;
    pinst->enviro    = pcap->enviro;
    pinst->alpha     = pcap->alpha;
    pinst->light     = pcap->light;
    pinst->sheen     = pcap->sheen;
	pinst->grnshift  = greensave;
	pinst->redshift  = redsave;
	pinst->blushift  = bluesave;

    pinst->frame_nxt = pmad->md2.framestart;
    pinst->frame_lst = pinst->frame_nxt;

    // Set up initial fade in lighting
    pinst->color_amb = 0;
    for ( tnc = 0; tnc < pmad->transvertices; tnc++ )
    {
        pinst->vlst[tnc].color_dir = 0;
    }

    // initialize the save frame
    pinst->save_flip      = 0;
    pinst->save_frame     = 0;
    pinst->save_vmin      = 0xFFFF;
    pinst->save_vmax      = -1;
    pinst->save_frame_nxt = 0;
    pinst->save_frame_lst = 0;

    chr_instance_update_vertices( pinst, -1, -1 );

    return btrue;
}

//--------------------------------------------------------------------------------------------
grip_offset_t slot_to_grip_offset( slot_t slot )
{
    grip_offset_t retval = GRIP_ORIGIN;

    retval = ( slot + 1 ) * GRIP_VERTS;

    return retval;
}

//--------------------------------------------------------------------------------------------
slot_t grip_offset_to_slot( grip_offset_t grip_off )
{
    slot_t retval = SLOT_LEFT;

    if ( 0 != grip_off % GRIP_VERTS )
    {
        // this does not correspond to a valid slot
        // coerce it to the "default" slot
        retval = SLOT_LEFT;
    }
    else
    {
        int islot = ((int)grip_off / GRIP_VERTS) - 1;

        // coerce the slot number to fit within the valid range
        islot = CLIP(islot, 0, SLOT_COUNT);

        retval = (slot_t) islot;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void init_slot_idsz()
{
    inventory_idsz[INVEN_PACK]  = IDSZ_NONE;
    inventory_idsz[INVEN_NECK]  = Make_IDSZ( "NECK" );
    inventory_idsz[INVEN_WRIS]  = Make_IDSZ( "WRIS" );
    inventory_idsz[INVEN_FOOT]  = Make_IDSZ( "FOOT" );
}

//--------------------------------------------------------------------------------------------
bool_t ai_add_order( ai_state_t * pai, Uint32 value, Uint16 counter )
{
    bool_t retval;

    if (NULL == pai) return bfalse;

    // this function is only truely valid if there is no other order
    retval = (0 == (pai->alert & ALERTIF_ORDERED));

    pai->alert        |= ALERTIF_ORDERED;
    pai->order_value   = value;
    pai->order_counter = counter;

    return retval;
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
      if ( dead != ChrList[charb].alive && ( seeinvisible || FF_MUL( ChrList[charb].inst.alpha, ChrList[charb].inst.max_light ) > INVISIBLE ) ) )
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
      charb = ChrList[charb].fanblock_next;
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
  x = ( ( int )ChrList[character].pos.x ) >> BLOCK_BITS;
  y = ( ( int )ChrList[character].pos.y ) >> BLOCK_BITS;
  return get_target_in_block( x, y, character, items, friends, enemies, dead, seeinvisible, idsz, 0 );
}*/

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
      if ( ChrList[cnt].attachedto == MAX_CHR && !ChrList[cnt].pack_ispacked )
      {
        if ( TeamList[team].hatesteam[ChrList[cnt].team] && ChrList[cnt].alive && !ChrList[cnt].invictus )
        {
          if ( ChrList[character].canseeinvisible || FF_MUL( ChrList[cnt].inst.alpha, ChrList[cnt].inst.max_light ) > INVISIBLE ) )
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
      if ( dead != ChrList[charb].alive && ( seeinvisible || FF_MUL( ChrList[charb].inst.alpha, ChrList[charb].inst.max_light ) > INVISIBLE ) ) )
      {
        if ( ( enemies && TeamList[team].hatesteam[ChrList[charb].team] ) ||
             ( items && ChrList[charb].isitem ) ||
             ( friends && ChrList[charb].team == team ) ||
             ( friends && enemies ) )
        {
          if ( charb != character && ChrList[character].attachedto != charb && ChrList[charb].attachedto == MAX_CHR && !ChrList[charb].pack_ispacked )
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
      charb = ChrList[charb].fanblock_next;
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
  x = ( ( int )ChrList[character].pos.x ) >> BLOCK_BITS;
  y = ( ( int )ChrList[character].pos.y ) >> BLOCK_BITS;

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
  x = ( ( int )ChrList[character].pos.x ) >> BLOCK_BITS;
  y = ( ( int )ChrList[character].pos.y ) >> BLOCK_BITS;
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
