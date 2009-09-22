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

/* Egoboo - char.c
 */

#include "char.h"
#include "particle.h"
#include "enchant.h"
#include "mad.h"
#include "profile.h"

#include "log.h"
#include "script.h"
#include "menu.h"
#include "sound.h"
#include "camera.h"
#include "input.h"
#include "Md2.h"
#include "passage.h"
#include "graphic.h"
#include "game.h"
#include "texture.h"
#include "ui.h"

#include "egoboo_vfs.h"
#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo_math.h"
#include "egoboo.h"

#include "SDL_extensions.h"

#include <assert.h>
#include <float.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static IDSZ    inventory_idsz[INVEN_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
cap_t        CapList[MAX_PROFILE];
team_t       TeamList[TEAM_MAX];

DECLARE_LIST ( ACCESS_TYPE_NONE, chr_t,  ChrList  );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t chr_instance_init( chr_instance_t * pinst, Uint16 profile, Uint8 skin );
static Uint16 pack_has_a_stack( Uint16 item, Uint16 character );
static bool_t pack_add_item( Uint16 item, Uint16 character );
static Uint16 pack_get_item( Uint16 character, grip_offset_t grip_off, bool_t ignorekurse );
static void set_weapongrip( Uint16 iitem, Uint16 iholder, Uint16 vrt_off );

static int chr_add_billboard( Uint16 ichr, Uint32 lifetime_secs );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int chr_count_free()
{
    return ChrList.free_count;
}

//--------------------------------------------------------------------------------------------
void flash_character_height( Uint16 character, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high )
{
    // ZZ> This function sets a character's lighting depending on vertex height...
    //    Can make feet dark and head light...
    int cnt;
    Sint16 z;

    mad_t * pmad;
    chr_instance_t * pinst;

    pinst = chr_get_pinstance(character);
    if( NULL == pinst ) return;

    pmad = chr_get_pmad(character);
    if( NULL == pmad ) return;

    for ( cnt = 0; cnt < pmad->transvertices; cnt++  )
    {
        z = Md2FrameList[pinst->frame_nxt].vrtz[cnt];

        if ( z < low )
        {
            pinst->color_amb = valuelow;
        }
        else
        {
            if ( z > high )
            {
                pinst->color_amb = valuehigh;
            }
            else
            {
                pinst->color_amb = ( valuehigh * ( z - low ) / ( high - low ) ) +
                                   ( valuelow * ( high - z ) / ( high - low ) );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders()
{
    // ZZ> This function keeps weapons near their holders
    int cnt, iattached;

    // !!!BAD!!!  May need to do 3 levels of attachment...

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        chr_t * pchr;

        if ( !ChrList.lst[cnt].on ) continue;
        pchr = ChrList.lst + cnt;

        iattached = pchr->attachedto;
        if ( INVALID_CHR(iattached) )
        {
            pchr->attachedto = MAX_CHR;

            // Keep inventory with iattached
            if ( !pchr->pack_ispacked )
            {
                iattached = pchr->pack_next;

                while ( iattached != MAX_CHR )
                {
                    ChrList.lst[iattached].pos = pchr->pos;

                    // Copy olds to make SendMessageNear work
                    ChrList.lst[iattached].pos_old = pchr->pos_old;

                    iattached = ChrList.lst[iattached].pack_next;
                }
            }
        }
        else
        {
            chr_t * pattached = ChrList.lst + iattached;

            // Keep in hand weapons with iattached
            if ( pchr->inst.matrixvalid )
            {
                pchr->pos.x = pchr->inst.matrix.CNV( 3, 0 );
                pchr->pos.y = pchr->inst.matrix.CNV( 3, 1 );
                pchr->pos.z = pchr->inst.matrix.CNV( 3, 2 );
            }
            else
            {
                pchr->pos = pattached->pos;
            }

            pchr->turn_z = pattached->turn_z;

            // Copy this stuff ONLY if it's a weapon, not for mounts
            if ( pattached->transferblend && pchr->isitem )
            {

                // Items become partially invisible in hands of players
                if ( pattached->isplayer && pattached->inst.alpha != 255 )
                {
                    pchr->inst.alpha = SEEINVISIBLE;
                }
                else
                {
                    // Only if not naturally transparent
                    if ( chr_get_pcap(cnt)->alpha == 255 )
                    {
                        pchr->inst.alpha = pattached->inst.alpha;
                    }
                    else
                    {
                        pchr->inst.alpha = chr_get_pcap(cnt)->alpha;
                    }
                }

                // Do light too
                if ( pattached->isplayer && pattached->inst.light != 255 )
                {
                    pchr->inst.light = SEEINVISIBLE;
                }
                else
                {
                    // Only if not naturally transparent
                    if ( chr_get_pcap(cnt)->light == 255 )
                    {
                        pchr->inst.light = pattached->inst.light;
                    }
                    else
                    {
                        pchr->inst.light = chr_get_pcap(cnt)->light;
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

    chr_t * pchr;
    chr_instance_t * pinst;

    if ( INVALID_CHR( cnt ) ) return;
    pchr = ChrList.lst + cnt;
    pinst = &(pchr->inst);

    pinst->matrixvalid = bfalse;
    if ( VALID_CHR(pchr->overlay) )
    {
        // Overlays are kept with their target...
        chr_t * povl = ChrList.lst + pchr->overlay;

        if ( VALID_CHR(povl->ai.target) )
        {
            chr_t * ptarget = ChrList.lst + povl->ai.target;

            povl->pos              = ptarget->pos;
            povl->inst.matrixvalid = ptarget->inst.matrixvalid;
            CopyMatrix( &(povl->inst.matrix), &(ptarget->inst.matrix) );

            pchr->pos              = ptarget->pos;
            pchr->inst.matrixvalid = ptarget->inst.matrixvalid;
            CopyMatrix( &(pchr->inst.matrix), &(ptarget->inst.matrix) );
        }
    }
    else
    {
        pinst->matrix = ScaleXYZRotateXYZTranslate( pchr->fat, pchr->fat, pchr->fat,
                                       pchr->turn_z >> 2,
                                       ( ( Uint16 ) ( pchr->map_turn_x + 32768 ) ) >> 2,
                                       ( ( Uint16 ) ( pchr->map_turn_y + 32768 ) ) >> 2,
                                       pchr->pos.x, pchr->pos.y, pchr->pos.z );
        pinst->matrixvalid = btrue;
    }
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_free_one( Uint16 ichr )
{
    // ZZ> This function sticks a character back on the free enchant stack

    bool_t retval;

    if ( !VALID_CHR_RANGE(ichr) ) return bfalse;

    // enchant "destructor"
    // sets all boolean values to false, incluting the "on" flag
    memset( ChrList.lst + ichr, 0, sizeof(chr_t) );

#if defined(DEBUG)
    {
        int cnt;
        // determine whether this character is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < ChrList.free_count; cnt++ )
        {
            if ( ichr == ChrList.free_ref[cnt] ) return bfalse;
        }
    }
#endif

    // push it on the free stack
    retval = bfalse;
    if ( ChrList.free_count < MAX_CHR )
    {
        ChrList.free_ref[ChrList.free_count] = ichr;
        ChrList.free_count++;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void free_one_character( Uint16 character )
{
    if ( !VALID_CHR_RANGE( character ) ) return;

    if ( ChrList_free_one( character ) )
    {
        int     cnt;
        chr_t * pchr = ChrList.lst + character;

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
    }

}

//--------------------------------------------------------------------------------------------
void free_one_character_in_game( Uint16 character )
{
    // ZZ> This function sticks a character back on the free character stack
    int cnt;

    if ( VALID_CHR( character ) )
    {
        chr_t * pchr = ChrList.lst + character;

        // Remove from stat list
        if ( pchr->staton )
        {
            bool_t stat_found;

            pchr->staton = bfalse;

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
            ai_state_t * pai;

            if ( !ChrList.lst[cnt].on || cnt == character ) continue;
            pai = chr_get_pai(cnt);

            if ( pai->target == character )
            {
                pai->alert |= ALERTIF_TARGETKILLED;
                pai->target = cnt;
            }

            if ( chr_get_pteam(cnt)->leader == character )
            {
                pai->alert |= ALERTIF_LEADERKILLED;
            }
        }

        // Handle the team
        if ( pchr->alive && !chr_get_pcap(character)->invictus )
        {
            TeamList[pchr->baseteam].morale--;
        }

        if ( TeamList[pchr->team].leader == character )
        {
            TeamList[pchr->team].leader = NOLEADER;
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

    cnt = ChrList.lst[character].pack_next;
    while ( cnt < MAX_CHR )
    {
        next = ChrList.lst[cnt].pack_next;
        free_one_character_in_game( cnt );
        cnt = next;
    }
}

//--------------------------------------------------------------------------------------------
void attach_particle_to_character( Uint16 particle, Uint16 character, int vertex_offset )
{
    // ZZ> This function sets one particle's position to be attached to a character.
    //    It will kill the particle if the character is no longer around

    Uint16 vertex;
    GLvector4 point[1], nupoint[1];

    chr_t * pchr;
    prt_t * pprt;

    if( INVALID_PRT(particle) ) return;
    pprt = PrtList.lst + particle;

    // Check validity of attachment
    if ( INVALID_CHR(character) || ChrList.lst[character].pack_ispacked )
    {
        pprt->time   = frame_all + 1;
        pprt->poofme = btrue;
        return;
    }
    pchr = ChrList.lst + character;

    // Do we have a matrix???
    if ( pchr->inst.matrixvalid )// PMesh->mmem.inrenderlist[pchr->onwhichfan])
    {
        // Transform the weapon vertex_offset from model to world space
        mad_t * pmad = chr_get_pmad(character);

        if ( vertex_offset == GRIP_ORIGIN )
        {
            pprt->pos.x = pchr->inst.matrix.CNV( 3, 0 );
            pprt->pos.y = pchr->inst.matrix.CNV( 3, 1 );
            pprt->pos.z = pchr->inst.matrix.CNV( 3, 2 );
            return;
        }

        vertex = 0;
        if( NULL != pmad )
        {
            vertex = pmad->md2_data.vertices - vertex_offset;

            // do the automatic update
            chr_instance_update_vertices( &(pchr->inst), vertex, vertex );

            // Calculate vertex_offset point locations with linear interpolation and other silly things
            point[0].x = pchr->inst.vlst[vertex].pos[XX];
            point[0].y = pchr->inst.vlst[vertex].pos[YY];
            point[0].z = pchr->inst.vlst[vertex].pos[ZZ];
            point[0].w = 1.0f;
        }
        else
        {
            point[0].x =
            point[0].y =
            point[0].z = 0.0f;
            point[0].w = 1.0f;
        }

        // Do the transform
        TransformVertices( &(pchr->inst.matrix), point, nupoint, 1 );

        pprt->pos.x = nupoint[0].x;
        pprt->pos.y = nupoint[0].y;
        pprt->pos.z = nupoint[0].z;
    }
    else
    {
        // No matrix, so just wing it...
        pprt->pos = pchr->pos;
    }
}

//--------------------------------------------------------------------------------------------
void make_one_weapon_matrix( Uint16 iweap, Uint16 iholder, bool_t do_physics )
{
    // ZZ> This function sets one weapon's matrix, based on who it's attached to
    int       cnt, vertex;
    Uint16    iholder_frame, iholder_lastframe;
    Uint8     iholder_lip;
    float     iholder_flip;
    GLvector4 point[GRIP_VERTS], nupoint[GRIP_VERTS];
    GLvector3 ptemp;
    int       iweap_points;

    chr_t * pweap, * pholder;

    // turn this off for now
    do_physics = bfalse;

    if ( INVALID_CHR(iweap) ) return;
    pweap = ChrList.lst + iweap;

    if(  INVALID_CHR(iholder) ) return;
    pholder = ChrList.lst + iholder;

    // make sure that the matrix is invalid incase of an error
    pweap->inst.matrixvalid = bfalse;

    // Transform the weapon grip from model space to world space
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
        point[0].x = ChrList.lst[0].pos.x;
        point[0].y = ChrList.lst[0].pos.y;
        point[0].z = ChrList.lst[0].pos.z;
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
        // GLvector3 vcom;

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
        // vcom.x = (ABS(wt_weap) * dx + ABS(wt_holder) * pholder->vel.x) / ( ABS(wt_weap) + ABS(wt_holder) );
        // vcom.y = (ABS(wt_weap) * dy + ABS(wt_holder) * pholder->vel.y) / ( ABS(wt_weap) + ABS(wt_holder) );
        // vcom.z = (ABS(wt_weap) * dz + ABS(wt_holder) * pholder->vel.z) / ( ABS(wt_weap) + ABS(wt_holder) );

        if ( wt_weap >= 0.0f )
        {
            // the object has already been moved the full distance
            // move it back some

            float ratio = 1.0f - (float)ABS(wt_holder) / ((float)ABS(wt_weap) + (float)ABS(wt_holder));

            pweap->phys.apos_1.x -= dx * ratio;
            pweap->phys.apos_1.y -= dy * ratio;
            pweap->phys.apos_1.z -= dz * ratio;

            // pweap->phys.avel.x += (dx-vcom.x)*damp + vcom.x - pweap->vel.x;
            // pweap->phys.avel.y += (dy-vcom.y)*damp + vcom.y - pweap->vel.y;
            // pweap->phys.avel.z += (dz-vcom.z)*damp + vcom.z - pweap->vel.z;
        }

        if ( wt_holder >= 0.0f )
        {
            float ratio = (float)ABS(wt_weap) / ((float)ABS(wt_weap) + (float)ABS(wt_holder));

            pholder->phys.apos_1.x -= dx * ratio;
            pholder->phys.apos_1.y -= dy * ratio;
            pholder->phys.apos_1.z -= dz * ratio;

            // pholder->phys.avel.x += (pholder->vel.x-vcom.x)*damp + vcom.x - pholder->vel.x;
            // pholder->phys.avel.y += (pholder->vel.y-vcom.y)*damp + vcom.y - pholder->vel.y;
            // pholder->phys.avel.z += (pholder->vel.z-vcom.z)*damp + vcom.z - pholder->vel.z;
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
        ChrList.lst[ichr].inst.matrixvalid = bfalse;
    }

    // blank the accumulators
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        ChrList.lst[ichr].phys.apos_0.x = 0.0f;
        ChrList.lst[ichr].phys.apos_0.y = 0.0f;
        ChrList.lst[ichr].phys.apos_0.z = 0.0f;

        ChrList.lst[ichr].phys.apos_1.x = 0.0f;
        ChrList.lst[ichr].phys.apos_1.y = 0.0f;
        ChrList.lst[ichr].phys.apos_1.z = 0.0f;

        ChrList.lst[ichr].phys.avel.x = 0.0f;
        ChrList.lst[ichr].phys.avel.y = 0.0f;
        ChrList.lst[ichr].phys.avel.z = 0.0f;
    }

    // Do base characters
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        if ( !ChrList.lst[ichr].on ) continue;

        if ( INVALID_CHR( ChrList.lst[ichr].attachedto ) )
        {
            make_one_character_matrix( ichr );
        }
    }

    // do all levels of attachment
    done = bfalse;
    while ( !done )
    {
        for ( cnt = 0, ichr = 0; ichr < MAX_CHR; ichr++ )
        {
            chr_t * pchr;
            Uint16 imount;

            if ( !ChrList.lst[ichr].on ) continue;
            pchr = ChrList.lst + ichr;

            if ( pchr->inst.matrixvalid ) continue;

            imount = pchr->attachedto;
            if ( INVALID_CHR(imount) || imount == ichr )
            {
                pchr->attachedto = MAX_CHR;
                make_one_character_matrix( ichr );
                continue;
            }

            // can't evaluate this link yet
            if ( !chr_get_pinstance(imount)->matrixvalid )
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

            if ( !ChrList.lst[ichr].on ) continue;
            pchr = ChrList.lst + ichr;

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
                    // pchr->vel.x += pchr->phys.apos_1.x;
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
                    // pchr->vel.y += pchr->phys.apos_1.y;
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
                    // pchr->vel.z += pchr->phys.apos_1.z;
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
            chr_t * pchr;

            if ( !ChrList.lst[ichr].on ) continue;
            pchr = ChrList.lst + ichr;

            if( !pchr->inst.matrixvalid ) continue;

            pchr->inst.matrix.CNV( 3, 0 ) = pchr->pos.x;
            pchr->inst.matrix.CNV( 3, 1 ) = pchr->pos.y;
            pchr->inst.matrix.CNV( 3, 2 ) = pchr->pos.z;
        }
    }
}

//--------------------------------------------------------------------------------------------
Uint16 ChrList_get_free()
{
    // ZZ> This function returns the next free character or MAX_CHR if there are none

    Uint16 retval = MAX_CHR;

    if ( ChrList.free_count > 0 )
    {
        ChrList.free_count--;
        retval = ChrList.free_ref[ChrList.free_count];
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void ChrList_free_all()
{
    int cnt;

    ChrList.free_count = 0;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList_free_one( cnt );
    }
 }

//--------------------------------------------------------------------------------------------
void free_all_chraracters()
{
    // ZZ> This function resets the character allocation list

    int cnt;

    ChrList.free_count = 0;
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        free_one_character( cnt );
    }

    // free_all_players
    PlaList_count = 0;
    local_numlpla = 0;
    local_noplayers = btrue;

    // free_all_stats
    numstat = 0;
}

//--------------------------------------------------------------------------------------------
Uint32 __chrhitawall( Uint16 character, float nrm[] )
{
    // ZZ> This function returns nonzero if the character hit a wall that the
    //    character is not allowed to cross

    Uint32 pass;
    float x, y, bs;
    Uint32 itile;
    int tx_min, tx_max, ty_min, ty_max;
    int ix, iy, tx0, ty0;
    bool_t invalid;
    chr_t * pchr;

    if ( INVALID_CHR(character) ) return 0;
    pchr = ChrList.lst + character;

    if ( 0 == pchr->bump.size || 0xFFFFFFFF == pchr->weight ) return 0;

    y  = pchr->pos.y;
    x  = pchr->pos.x;
    bs = pchr->bump.size * 0.5f;

    tx_min = floor( (x - bs) / TILE_SIZE );
    tx_max = (int)( (x + bs) / TILE_SIZE );
    ty_min = floor( (y - bs) / TILE_SIZE );
    ty_max = (int)( (y + bs) / TILE_SIZE );

    tx0 = (int)x / TILE_ISIZE;
    ty0 = (int)y / TILE_ISIZE;

    pass = 0;
    nrm[XX] = nrm[YY] = 0.0f;
    for ( iy = ty_min; iy <= ty_max; iy++ )
    {
        invalid = bfalse;

        if ( iy < 0 || iy >= PMesh->info.tiles_y )
        {
            pass  |=  MPDFX_IMPASS | MPDFX_WALL;
            nrm[YY]  += (iy + 0.5f) * TILE_SIZE - y;
            invalid = btrue;
        }

        for ( ix = tx_min; ix <= tx_max; ix++ )
        {
            if ( ix < 0 || ix >= PMesh->info.tiles_x )
            {
                pass  |=  MPDFX_IMPASS | MPDFX_WALL;
                nrm[XX]  += (ix + 0.5f) * TILE_SIZE - x;
                invalid = btrue;
            }

            if ( !invalid )
            {
                itile = mesh_get_tile_int( PMesh, ix, iy );
                if ( VALID_TILE(PMesh, itile) )
                {
                    if ( PMesh->mmem.tile_list[itile].fx & pchr->stoppedby )
                    {
                        nrm[XX] += (ix + 0.5f) * TILE_SIZE - x;
                        nrm[YY] += (iy + 0.5f) * TILE_SIZE - y;
                    }
                    else
                    {
                        nrm[XX] -= (ix + 0.5f) * TILE_SIZE - x;
                        nrm[YY] -= (iy + 0.5f) * TILE_SIZE - y;
                    }

                    pass |= PMesh->mmem.tile_list[itile].fx;
                }
            }
        }
    }

    if ( pass & pchr->stoppedby )
    {
        float dist2 = nrm[XX] * nrm[XX] + nrm[YY] * nrm[YY];
        if ( dist2 > 0 )
        {
            float dist = SQRT( dist2 );
            nrm[XX] /= -dist;
            nrm[YY] /= -dist;
        }
    }

    return pass & pchr->stoppedby;

}

//--------------------------------------------------------------------------------------------
void reset_character_accel( Uint16 character )
{
    // ZZ> This function fixes a character's max acceleration
    Uint16 enchant;
    chr_t * pchr;
    cap_t * pcap;

    if ( INVALID_CHR(character) ) return;
    pchr = ChrList.lst + character;

    // Okay, remove all acceleration enchants
    enchant = pchr->firstenchant;
    while ( enchant < MAX_ENC )
    {
        remove_enchant_value( enchant, ADDACCEL );
        enchant = EncList.lst[enchant].nextenchant;
    }

    // Set the starting value
    pchr->maxaccel = 0;
    pcap = chr_get_pcap(character);
    if( NULL != pcap )
    {
        pchr->maxaccel = pcap->maxaccel[pchr->skin];
    }

    // Put the acceleration enchants back on
    enchant = pchr->firstenchant;
    while ( enchant < MAX_ENC )
    {
        add_enchant_value( enchant, ADDACCEL, enc_get_ieve(enchant) );
        enchant = EncList.lst[enchant].nextenchant;
    }

}

//--------------------------------------------------------------------------------------------
void detach_character_from_mount( Uint16 character, Uint8 ignorekurse,
                                  Uint8 doshop )
{
    // ZZ> This function drops an item
    Uint16 mount, hand, enchant, owner = NOOWNER;
    bool_t inshop;
    float price;
    float nrm[2];
    chr_t * pchr, * pmount;

    // Make sure the character is valid
    if ( INVALID_CHR(character) ) return;
    pchr = ChrList.lst + character;

    // Make sure the character is mounted
    mount = ChrList.lst[character].attachedto;
    if ( INVALID_CHR(mount) ) return;
    pmount = ChrList.lst + mount;

    // Don't allow living characters to drop kursed weapons
    if ( !ignorekurse && pchr->iskursed && pmount->alive && pchr->isitem )
    {
        pchr->ai.alert |= ALERTIF_NOTDROPPED;
        return;
    }

    // set the dismount timer
    pchr->phys.dismount_timer = PHYS_DISMOUNT_TIME;

    // Figure out which hand it's in
    hand = pchr->inwhich_slot;

    // Rip 'em apart
    pchr->attachedto = MAX_CHR;
    if ( pmount->holdingwhich[SLOT_LEFT] == character )
        pmount->holdingwhich[SLOT_LEFT] = MAX_CHR;

    if ( pmount->holdingwhich[SLOT_RIGHT] == character )
        pmount->holdingwhich[SLOT_RIGHT] = MAX_CHR;

    if ( pchr->alive )
    {
        // play the falling animation...
        chr_play_action( character, ACTION_JB + hand, bfalse );
    }
    else if ( pchr->action < ACTION_KA || pchr->action > ACTION_KD )
    {
        // play the "killed" animation...
        chr_play_action( character, ACTION_KA + hand, bfalse );
        pchr->keepaction = btrue;
    }

    // Set the positions
    if ( pchr->inst.matrixvalid )
    {
        pchr->pos.x = pchr->inst.matrix.CNV( 3, 0 );
        pchr->pos.y = pchr->inst.matrix.CNV( 3, 1 );
        pchr->pos.z = pchr->inst.matrix.CNV( 3, 2 );
    }
    else
    {
        pchr->pos = pmount->pos;
    }

    // Make sure it's not dropped in a wall...
    if ( __chrhitawall( character, nrm ) )
    {
        pchr->pos.x = pmount->pos.x;
        pchr->pos.y = pmount->pos.y;
    }
    else
    {
        pchr->safe_valid = btrue;
        pchr->pos_safe = pchr->pos;
    }

    // Check for shop passages
    inshop = bfalse;
    if ( pchr->isitem && ShopStack.count > 0 && doshop )
    {
        int ix = pchr->pos.x / TILE_SIZE;
        int iy = pchr->pos.y / TILE_SIZE;

        // This is a hack that makes spellbooks in shops cost correctly
        if ( pmount->isshopitem ) pchr->isshopitem = btrue;

        owner = shop_get_owner( ix, iy );
        if ( VALID_CHR(owner) ) inshop = btrue;

        if ( inshop )
        {
            // Give the mount its money back, alert the shop owner
            Uint16 skin, icap;

            // Make sure spell books are priced according to their spell and not the book itself
            if ( pchr->iprofile == SPELLBOOK )
            {
                icap = pro_get_icap(pchr->basemodel);
                skin = 0;
            }
            else
            {
                icap  = pro_get_icap(pchr->iprofile);
                skin = pchr->skin;
            }
            price = CapList[icap].skincost[skin];

            // Are they are trying to sell junk or quest items?
            if ( price == 0 ) ai_add_order( chr_get_pai(owner), (Uint32) price, SHOP_BUY );
            else
            {
                // Items spawned within shops are more valuable
                if (!pchr->isshopitem) price *= 0.5;

                // cost it based on the number/charges of the item
                if ( CapList[icap].isstackable )
                {
                    price *= pchr->ammo;
                }
                else if (CapList[icap].isranged && pchr->ammo < pchr->ammomax)
                {
                    if ( 0 != pchr->ammo )
                    {
                        price *= (float)pchr->ammo / pchr->ammomax;
                    }
                }

                pmount->money += (Sint16) price;
                pmount->money  = CLIP(pmount->money, 0, MAXMONEY);

                ChrList.lst[owner].money -= (Sint16) price;
                ChrList.lst[owner].money  = CLIP(ChrList.lst[owner].money, 0, MAXMONEY);

                ai_add_order( chr_get_pai(owner), (Uint32) price, SHOP_BUY );
            }
        }
    }

    // Make sure it works right
    pchr->hitready = btrue;
    if ( inshop )
    {
        // Drop straight down to avoid theft
        pchr->vel.x = 0;
        pchr->vel.y = 0;
    }
    else
    {
        pchr->vel.x = pmount->vel.x;
        pchr->vel.y = pmount->vel.y;
    }

    pchr->vel.z = DROPZVEL;

    // Turn looping off
    pchr->loopaction = bfalse;

    // Reset the team if it is a mount
    if ( pmount->ismount )
    {
        pmount->team = pmount->baseteam;
        pmount->ai.alert |= ALERTIF_DROPPED;
    }

    pchr->team = pchr->baseteam;
    pchr->ai.alert |= ALERTIF_DROPPED;

    // Reset transparency
    if ( pchr->isitem && pmount->transferblend )
    {
        // Okay, reset transparency
        enchant = pchr->firstenchant;

        while ( enchant < MAX_ENC )
        {
            unset_enchant_value( enchant, SETALPHABLEND );
            unset_enchant_value( enchant, SETLIGHTBLEND );
            enchant = EncList.lst[enchant].nextenchant;
        }

        pchr->inst.alpha = pchr->basealpha;
        pchr->inst.light = pchr->baselight;
        enchant = pchr->firstenchant;

        while ( enchant < MAX_ENC )
        {
            set_enchant_value( enchant, SETALPHABLEND, enc_get_ieve(enchant) );
            set_enchant_value( enchant, SETLIGHTBLEND, enc_get_ieve(enchant) );
            enchant = EncList.lst[enchant].nextenchant;
        }
    }

    // Set twist
    pchr->map_turn_y = 32768;
    pchr->map_turn_x = 32768;
}

//--------------------------------------------------------------------------------------------
void reset_character_alpha( Uint16 character )
{
    // ZZ> This function fixes an item's transparency
    Uint16 enchant, mount;
    chr_t * pchr, * pmount;

    // Make sure the character is valid
    if ( INVALID_CHR(character) ) return;
    pchr = ChrList.lst + character;

    // Make sure the character is mounted
    mount = ChrList.lst[character].attachedto;
    if ( INVALID_CHR(mount) ) return;
    pmount = ChrList.lst + mount;

    if ( pchr->isitem && pmount->transferblend )
    {
        // Okay, reset transparency
        enchant = pchr->firstenchant;
        while ( enchant < MAX_ENC )
        {
            unset_enchant_value( enchant, SETALPHABLEND );
            unset_enchant_value( enchant, SETLIGHTBLEND );
            enchant = EncList.lst[enchant].nextenchant;
        }

        pchr->inst.alpha = pchr->basealpha;
        pchr->inst.light = pchr->baselight;

        enchant = pchr->firstenchant;
        while ( enchant < MAX_ENC )
        {
            set_enchant_value( enchant, SETALPHABLEND, enc_get_ieve(enchant) );
            set_enchant_value( enchant, SETLIGHTBLEND, enc_get_ieve(enchant) );
            enchant = EncList.lst[enchant].nextenchant;
        }
    }
}

//--------------------------------------------------------------------------------------------
void attach_character_to_mount( Uint16 iitem, Uint16 iholder, grip_offset_t grip_off )
{
    // ZZ> This function attaches one character/item to another ( the holder/mount )
    //    at a certain vertex offset ( grip_off )

    slot_t slot;

    chr_t * pitem, * pholder;

    // Make sure the character/item is valid
    if ( INVALID_CHR(iitem) || ChrList.lst[iitem].pack_ispacked ) return;
    pitem = ChrList.lst + iitem;

    // make a reasonable time for the character to remount something
    // for characters jumping out of pots, etc
    if( !pitem->isitem && pitem->phys.dismount_timer > 0 )
        return;

    // Make sure the holder/mount is valid
    if ( INVALID_CHR(iholder) || ChrList.lst[iholder].pack_ispacked ) return;
    pholder = ChrList.lst + iholder;

#if !defined(ENABLE_BODY_GRAB)
    if (!pitem->alive) return;
#endif

    // Figure out which slot this grip_off relates to
    slot = grip_offset_to_slot( grip_off );

    // Make sure the the slot is valid
    if ( !chr_get_pcap(iholder)->slotvalid[slot] ) return;

	// This is a small fix that allows special grabbable mounts not to be mountable while
	// held by another character (such as the magic carpet for example)
	if( !pitem->isitem && pholder->ismount && pholder->attachedto != MAX_CHR ) return;

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
    pitem = ChrList.lst + item;

    // don't allow sub-inventories
    if ( pitem->pack_ispacked || pitem->isequipped ) return bfalse;

    pcap_item = pro_get_pcap(pitem->iprofile);
    if ( NULL == pcap_item ) return bfalse;

    if ( INVALID_CHR(character) ) return bfalse;
    pchr = ChrList.lst + character;

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
    pchr = ChrList.lst + ichr;

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
                ChrList.lst[iitem].isequipped = bfalse;
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
    //    to the one given.  If it finds one, it returns the similar item's
    //    index number, otherwise it returns MAX_CHR.

    Uint16 pack_ispacked, id;
    bool_t allok;

    if ( INVALID_CHR( item ) ) return MAX_CHR;

    if ( chr_get_pcap(item)->isstackable )
    {
        pack_ispacked = ChrList.lst[character].pack_next;

        allok = bfalse;
        while ( VALID_CHR(pack_ispacked) && !allok )
        {
            allok = btrue;
            if ( ChrList.lst[pack_ispacked].iprofile != ChrList.lst[item].iprofile )
            {
                if ( !chr_get_pcap(pack_ispacked)->isstackable )
                {
                    allok = bfalse;
                }
                if ( ChrList.lst[pack_ispacked].ammomax != ChrList.lst[item].ammomax )
                {
                    allok = bfalse;
                }

                for ( id = 0; id < IDSZ_COUNT && allok; id++ )
                {
                    if ( chr_get_idsz(pack_ispacked,id) != chr_get_idsz(item,id) )
                    {
                        allok = bfalse;
                    }
                }
            }
            if ( !allok )
            {
                pack_ispacked = ChrList.lst[pack_ispacked].pack_next;
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
    if ( ChrList.lst[item].pack_ispacked || ChrList.lst[character].pack_ispacked || ChrList.lst[character].isitem )
        return bfalse;

    stack = pack_has_a_stack( item, character );
    if ( VALID_CHR(stack) )
    {
        // We found a similar, stackable item in the pack
        if ( ChrList.lst[item].nameknown || ChrList.lst[stack].nameknown )
        {
            ChrList.lst[item].nameknown = btrue;
            ChrList.lst[stack].nameknown = btrue;
        }
        if ( chr_get_pcap(item)->usageknown || chr_get_pcap(stack)->usageknown )
        {
            chr_get_pcap(item)->usageknown = btrue;
            chr_get_pcap(stack)->usageknown = btrue;
        }

        newammo = ChrList.lst[item].ammo + ChrList.lst[stack].ammo;
        if ( newammo <= ChrList.lst[stack].ammomax )
        {
            // All transfered, so kill the in hand item
            ChrList.lst[stack].ammo = newammo;
            if ( ChrList.lst[item].attachedto != MAX_CHR )
            {
                detach_character_from_mount( item, btrue, bfalse );
            }

            free_one_character_in_game( item );
        }
        else
        {
            // Only some were transfered,
            ChrList.lst[item].ammo = ChrList.lst[item].ammo + ChrList.lst[stack].ammo - ChrList.lst[stack].ammomax;
            ChrList.lst[stack].ammo = ChrList.lst[stack].ammomax;
            chr_get_pai(character)->alert |= ALERTIF_TOOMUCHBAGGAGE;
        }
    }
    else
    {
        // Make sure we have room for another item
        if ( ChrList.lst[character].pack_count >= MAXNUMINPACK )
        {
            chr_get_pai(character)->alert |= ALERTIF_TOOMUCHBAGGAGE;
            return bfalse;
        }

        // Take the item out of hand
        if ( ChrList.lst[item].attachedto != MAX_CHR )
        {
            detach_character_from_mount( item, btrue, bfalse );
            chr_get_pai(item)->alert &= ( ~ALERTIF_DROPPED );
        }

        // Remove the item from play
        ChrList.lst[item].hitready = bfalse;
        ChrList.lst[item].pack_ispacked = btrue;

        // Insert the item into the pack as the first one
        oldfirstitem = ChrList.lst[character].pack_next;
        ChrList.lst[character].pack_next = item;
        ChrList.lst[item].pack_next = oldfirstitem;
        ChrList.lst[character].pack_count++;

        if ( chr_get_pcap(item)->isequipment )
        {
            // AtLastWaypoint doubles as PutAway
            chr_get_pai(item)->alert |= ALERTIF_ATLASTWAYPOINT;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 pack_get_item( Uint16 character, grip_offset_t grip_off, bool_t ignorekurse )
{
    // ZZ> This function takes the last item in the character's pack and puts
    //    it into the designated hand.  It returns the item number or MAX_CHR.

    Uint16 item, nexttolastitem;

    // does the character exist?
    if ( INVALID_CHR( character ) )
        return MAX_CHR;

    // Can the character have a pack?
    if ( ChrList.lst[character].pack_ispacked || ChrList.lst[character].isitem )
        return MAX_CHR;

    // is the pack empty?
    if ( MAX_CHR == ChrList.lst[character].pack_next || 0 == ChrList.lst[character].pack_count )
        return MAX_CHR;

    // Find the last item in the pack
    nexttolastitem = character;
    item = ChrList.lst[character].pack_next;
    while ( VALID_CHR(ChrList.lst[item].pack_next) )
    {
        nexttolastitem = item;
        item = ChrList.lst[item].pack_next;
    }

    // Figure out what to do with it
    if ( ChrList.lst[item].iskursed && ChrList.lst[item].isequipped && !ignorekurse )
    {
        // Flag the last item as not removed
        chr_get_pai(item)->alert |= ALERTIF_NOTPUTAWAY;  // Doubles as IfNotTakenOut

        // Cycle it to the front
        ChrList.lst[item].pack_next = ChrList.lst[character].pack_next;
        ChrList.lst[nexttolastitem].pack_next = MAX_CHR;
        ChrList.lst[character].pack_next = item;
        if ( character == nexttolastitem )
        {
            ChrList.lst[item].pack_next = MAX_CHR;
        }

        item = MAX_CHR;
    }
    else
    {
        // Remove the last item from the pack
        ChrList.lst[item].pack_ispacked = bfalse;
        ChrList.lst[item].isequipped = bfalse;
        ChrList.lst[nexttolastitem].pack_next = MAX_CHR;
        ChrList.lst[character].pack_count--;
        ChrList.lst[item].team = chr_get_iteam(character);

        // Attach the item to the character's hand
        attach_character_to_mount( item, character, grip_off );
        chr_get_pai(item)->alert &= ( ~ALERTIF_GRABBED );
        chr_get_pai(item)->alert |= ( ALERTIF_TAKENOUT );
    }

    return item;
}

//--------------------------------------------------------------------------------------------
void drop_keys( Uint16 character )
{
    // ZZ> This function drops all keys ( [KEYA] to [KEYZ] ) that are in a character's
    //    inventory ( Not hands ).

    chr_t * pchr;
    Uint16 item, lastitem, nextitem, direction;
    IDSZ testa, testz;

    if ( INVALID_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    if ( pchr->pos.z > -2 ) // Don't lose keys in pits...
    {
        // The IDSZs to find
        testa = MAKE_IDSZ( 'K', 'E', 'Y', 'A' );  // [KEYA]
        testz = MAKE_IDSZ( 'K', 'E', 'Y', 'Z' );  // [KEYZ]

        lastitem = character;
        item = pchr->pack_next;

        while ( item != MAX_CHR )
        {
            nextitem = ChrList.lst[item].pack_next;
            if ( VALID_CHR(item) && item != character )  // Should never happen...
            {
                chr_t * pitem = ChrList.lst + item;

                if ( ( chr_get_idsz(item,IDSZ_PARENT) >= testa && chr_get_idsz(item,IDSZ_PARENT) <= testz ) ||
                     ( chr_get_idsz(item,IDSZ_TYPE  ) >= testa && chr_get_idsz(item,IDSZ_TYPE  ) <= testz ) )
                {
                    // We found a key...

                    // unpack the item
                    ChrList.lst[lastitem].pack_next = pitem->pack_next;
                    pitem->pack_next = MAX_CHR;
                    pchr->pack_count--;
                    pitem->attachedto = MAX_CHR;
                    pitem->ai.alert |= ALERTIF_DROPPED;
                    pitem->hitready = btrue;
                    pitem->pack_ispacked = bfalse;
                    pitem->isequipped    = bfalse;

                    direction              = RANDIE;
                    pitem->turn_z          = direction + 32768;
                    pitem->phys.level      = pchr->phys.level;
                    pitem->floor_level     = pchr->floor_level;
                    pitem->onwhichplatform = pchr->onwhichplatform;
                    pitem->pos             = pchr->pos;
                    pitem->vel.x           = turntocos[ (direction >> 2) & TRIG_TABLE_MASK ] * DROPXYVEL;
                    pitem->vel.y           = turntosin[ (direction >> 2) & TRIG_TABLE_MASK ] * DROPXYVEL;
                    pitem->vel.z           = DROPZVEL;
                    pitem->team            = pitem->baseteam;
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
    chr_t * pchr;

    if ( INVALID_CHR(character) ) return bfalse;
    pchr = ChrList.lst + character;

    detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], btrue, bfalse );
    detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], btrue, bfalse );
    if ( pchr->pack_count > 0 )
    {
        direction = pchr->turn_z + 32768;
        diradd    = 0xFFFF / pchr->pack_count;

        while ( pchr->pack_count > 0 )
        {
            item = inventory_get_item( character, GRIP_LEFT, bfalse );

            if ( VALID_CHR(item) )
            {
                chr_t * pitem = ChrList.lst + item;

                detach_character_from_mount( item, btrue, btrue );

                pitem->hitready        = btrue;
                pitem->ai.alert       |= ALERTIF_DROPPED;
                pitem->pos             = pchr->pos;
                pitem->phys.level      = pchr->phys.level;
                pitem->floor_level     = pchr->floor_level;
                pitem->onwhichplatform = pchr->onwhichplatform;
                pitem->turn_z          = direction + 32768;
                pitem->vel.x           = turntocos[ (direction>>2) & TRIG_TABLE_MASK ] * DROPXYVEL;
                pitem->vel.y           = turntosin[ (direction>>2) & TRIG_TABLE_MASK ] * DROPXYVEL;
                pitem->vel.z           = DROPZVEL;
                pitem->team            = pitem->baseteam;
            }

            direction += diradd;
        }
    }

    return btrue;

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_grab_data
{
    Uint16 ichr;
    float  dist;
};
typedef struct s_grab_data grab_data_t;

//--------------------------------------------------------------------------------------------
int grab_data_cmp( const void * pleft, const void * pright )
{
    int rv;
    float diff;

    grab_data_t * dleft  = (grab_data_t *)pleft;
    grab_data_t * dright = (grab_data_t *)pright;

    diff = dleft->dist - dright->dist;

    if ( diff < 0.0f )
    {
        rv = -1;
    }
    else if ( diff > 0.0f )
    {
        rv = 1;
    }
    else
    {
        rv = 0;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
bool_t character_grab_stuff( Uint16 ichr_a, grip_offset_t grip_off, bool_t grab_people )
{
    // ZZ> This function makes the character pick up an item if there's one around

    int    cnt;
    Uint16 ichr_b;
    Uint16 vertex, frame_nxt;
    slot_t slot;
    GLvector4 point[1], nupoint[1];
    SDL_Color color_red = {0xFF, 0x7F, 0x7F, 0xFF};
    SDL_Color color_grn = {0x7F, 0xFF, 0x7F, 0xFF};
 //   SDL_Color color_blu = {0x7F, 0x7F, 0xFF, 0xFF};

    chr_t * pchr_a;

    int ticks;

    bool_t retval;

    // valid objects that can be grabbed
    int         grab_count = 0;
    grab_data_t grab_list[MAX_CHR];

    // valid objects that cannot be grabbed
    int         ungrab_count = 0;
    grab_data_t ungrab_list[MAX_CHR];

    if( INVALID_CHR(ichr_a) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    ticks = SDL_GetTicks();

    // Make life easier
    slot = grip_offset_to_slot( grip_off );  // 0 is left, 1 is right

    // Make sure the character doesn't have something already, and that it has hands
    if ( VALID_CHR(pchr_a->holdingwhich[slot]) || !chr_get_pcap(ichr_a)->slotvalid[slot] )
        return bfalse;

    // Do we have a matrix???
    if ( pchr_a->inst.matrixvalid )
    {
        // Transform the weapon grip_off from pchr_a->iprofile to world space
        frame_nxt = pchr_a->inst.frame_nxt;
        vertex    = chr_get_pmad(ichr_a)->md2_data.vertices - grip_off;

        // do the automatic update
        chr_instance_update_vertices( &(pchr_a->inst), vertex, vertex );

        // Calculate grip_off point locations with linear interpolation and other silly things
        point[0].x = pchr_a->inst.vlst[vertex].pos[XX];
        point[0].y = pchr_a->inst.vlst[vertex].pos[YY];
        point[0].z = pchr_a->inst.vlst[vertex].pos[ZZ];
        point[0].w = 1.0f;

        // Do the transform
        TransformVertices( &(pchr_a->inst.matrix), point, nupoint, 1 );
    }
    else
    {
        // Just wing it
        nupoint[0].x = pchr_a->pos.x;
        nupoint[0].y = pchr_a->pos.y;
        nupoint[0].z = pchr_a->pos.z;
        nupoint[0].w = 1.0f;
    }

    // Go through all characters to find the best match
    for ( ichr_b = 0; ichr_b < MAX_CHR; ichr_b++ )
    {
        GLvector3 pos_b;
        float     dx, dy, dz, dxy;
        chr_t   * pchr_b;
        bool_t    can_grab = btrue;

        if ( !ChrList.lst[ichr_b].on ) continue;
        pchr_b = ChrList.lst + ichr_b;

        // do nothing to yourself
        if( ichr_a == ichr_b ) continue;

        if ( pchr_b->pack_ispacked ) continue;        // pickpocket not allowed yet
        if ( MAX_CHR != pchr_b->attachedto) continue; // disarm not allowed yet

        // do not pick up your mount
        if ( pchr_b->holdingwhich[SLOT_LEFT] == ichr_a ||
             pchr_b->holdingwhich[SLOT_RIGHT] == ichr_a ) continue;

        pos_b = pchr_b->pos;

        // First check absolute value diamond
        dx = ABS( nupoint[0].x - pos_b.x );
        dy = ABS( nupoint[0].y - pos_b.y );
        dz = ABS( nupoint[0].z - pos_b.z );
        dxy = dx + dy;

        if ( dxy > TILE_SIZE * 2 || dz > MAX(pchr_b->bump.height, GRABSIZE) ) continue;

        // reasonable carrying capacity
        if ( pchr_b->weight > pchr_a->weight + pchr_a->strength * INV_FF )
        {
            can_grab = bfalse;
        }

        // grab_people == btrue allows you to pick up living non-items
        // grab_people == false allows you to pick up living (functioning) items
        if ( pchr_b->alive && (grab_people == pchr_b->isitem) )
        {
            can_grab = bfalse;
        }

        if( can_grab )
        {
            grab_list[grab_count].ichr = ichr_b;
            grab_list[grab_count].dist = dxy;
            grab_count++;

            //iline = get_free_line();
            //if( iline >= 0)
            //{
            //    line_list[iline].src     = nupoint[0];
            //    line_list[iline].dst     = pchr_b->pos;
            //    line_list[iline].color.r = color_grn.r * INV_FF;
            //    line_list[iline].color.g = color_grn.g * INV_FF;
            //    line_list[iline].color.b = color_grn.b * INV_FF;
            //    line_list[iline].color.a = 1.0f;
            //    line_list[iline].time    = ticks + ONESECOND * 5;
            //}
        }
        else
        {
            ungrab_list[grab_count].ichr = ichr_b;
            ungrab_list[grab_count].dist = dxy;
            ungrab_count++;

            //iline = get_free_line();
            //if( iline >= 0)
            //{
            //    line_list[iline].src     = nupoint[0];
            //    line_list[iline].dst     = pchr_b->pos;
            //    line_list[iline].color.r = color_red.r * INV_FF;
            //    line_list[iline].color.g = color_red.g * INV_FF;
            //    line_list[iline].color.b = color_red.b * INV_FF;
            //    line_list[iline].color.a = 1.0f;
            //    line_list[iline].time    = ticks + ONESECOND * 5;
            //}
        }
    }

    // sort the grab list
    if ( grab_count > 1 )
    {
        qsort( grab_list, grab_count, sizeof(grab_data_t), grab_data_cmp );
    }

    // try to grab something
    retval = bfalse;
    for ( cnt = 0; cnt < grab_count; cnt++ )
    {
        bool_t can_grab;
        float price;
        chr_t * pchr_b;

        ichr_b = grab_list[cnt].ichr;
        pchr_b = ChrList.lst + ichr_b;

        if ( grab_list[cnt].dist > GRABSIZE ) continue;

        // Check for shop
        can_grab = btrue;
        if ( pchr_b->isitem && ShopStack.count > 0 )
        {
            int    ix, iy;
            bool_t inshop;
            Uint16 owner;

            ix = pchr_b->pos.x / TILE_SIZE;
            iy = pchr_b->pos.y / TILE_SIZE;

            owner  = shop_get_owner( ix, iy );
            inshop = VALID_CHR(owner);

            if ( inshop )
            {
                // Pay the shop owner, or don't allow grab...
                bool_t is_invis, can_steal;

                is_invis  = FF_MUL(pchr_a->inst.alpha, pchr_a->inst.max_light) < INVISIBLE;
                can_steal = is_invis || pchr_a->isitem;

                if ( can_steal )
                {
                    // Pets can try to steal in addition to invisible characters
                    IPair  tmp_rand = {1,100};
                    Uint8  detection = generate_number( tmp_rand );

                    // Check if it was detected. 50% chance +2% per pet DEX and -2% per shopkeeper wisdom. There is always a 5% chance it will fail.
                    if ( ChrList.lst[owner].canseeinvisible || detection <= 5 || detection - ( pchr_a->dexterity >> 7 ) + ( ChrList.lst[owner].wisdom >> 7 ) > 50 )
                    {
                        debug_printf( "%s was detected!!", pchr_a->name );

                        ai_add_order( chr_get_pai(owner), STOLEN, SHOP_THEFT );
                        chr_get_pai(owner)->target = ichr_a;
                    }
                    else
                    {
                        debug_printf( "%s stole something! (%s)", pchr_a->name, chr_get_pcap(ichr_b)->classname );
                    }
                }
                else
                {
                    Uint16 icap, iskin;

                    // Make sure spell books are priced according to their spell and not the book itself
                    if ( pchr_b->iprofile == SPELLBOOK )
                    {
                        icap = pro_get_icap(pchr_b->basemodel);
                        iskin = 0;
                    }
                    else
                    {
                        icap  = pro_get_icap(pchr_b->iprofile);
                        iskin = pchr_b->skin;
                    }
                    price = (float) CapList[icap].skincost[iskin];

                    // Items spawned in shops are more valuable
                    if (!pchr_b->isshopitem) price *= 0.5f;

                    // base the cost on the number of items/charges
                    if ( CapList[icap].isstackable )
                    {
                        price *= pchr_b->ammo;
                    }
                    else if (CapList[icap].isranged && pchr_b->ammo < pchr_b->ammomax)
                    {
                        if ( 0 != pchr_b->ammo )
                        {
                            price *= (float) pchr_b->ammo / (float) pchr_b->ammomax;
                        }
                    }

                    // round to int value
                    price = (Sint32) price;

                    if ( pchr_a->money >= price )
                    {
                        // Okay to sell
                        ai_add_order( chr_get_pai(owner), (Uint32) price, SHOP_SELL );

                        pchr_a->money -= (Sint16) price;
                        pchr_a->money  = CLIP(pchr_a->money, 0, MAXMONEY);

                        ChrList.lst[owner].money += (Sint16) price;
                        ChrList.lst[owner].money  = CLIP(ChrList.lst[owner].money, 0, MAXMONEY);
                    }
                    else
                    {
                        // Don't allow purchase
                        ai_add_order( chr_get_pai(owner), (Uint32) price, SHOP_NOAFFORD );
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
            retval = btrue;
            break;
        }
        else
        {
            // Lift the item a little and quit...
            pchr_b->vel.z = DROPZVEL;
            pchr_b->hitready = btrue;
            pchr_b->ai.alert |= ALERTIF_DROPPED;
            break;
        }

    }

    if( !retval )
    {
        GLvector3 vforward;

        //---- generate billboards for things that players can interact with
        if( pchr_a->isplayer )
        {
            // things that can be grabbed (5 secs and green)
            for ( cnt = 0; cnt < grab_count; cnt++ )
            {
                ichr_b = grab_list[cnt].ichr;
                chr_make_text_billboard( ichr_b, chr_get_name(ichr_b), color_grn, 5 );
            }

            // things that can't be grabbed (5 secs and red)
            for ( cnt = 0; cnt < ungrab_count; cnt++ )
            {
                ichr_b = ungrab_list[cnt].ichr;
                chr_make_text_billboard( ichr_b, chr_get_name(ichr_b), color_red, 5 );
            }
        }

        //---- if you can't grab anything, activate something using ALERTIF_BUMPED

        vforward = mat_getChrForward( pchr_a->inst.matrix );

        // sort the ungrab list
        if ( ungrab_count > 1 )
        {
            qsort( ungrab_list, ungrab_count, sizeof(grab_data_t), grab_data_cmp );
        }

        for ( cnt = 0; cnt < ungrab_count; cnt++ )
        {
            float       ftmp;
            GLvector3   diff;
            chr_t     * pchr_b;

            if( grab_list[cnt].dist > GRABSIZE ) continue;

            ichr_b = grab_list[cnt].ichr;
            pchr_b = ChrList.lst + ichr_b;

            diff = VSub(pchr_a->pos, pchr_b->pos);

            // ignore vertical displacement in the dot product
            ftmp = vforward.x * diff.x + vforward.y * diff.y;
            if( ftmp > 0.0f )
            {
                pchr_b->ai.bumplast  = ichr_a;
                pchr_b->ai.alert    |= ALERTIF_BUMPED;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void character_swipe( Uint16 cnt, slot_t slot )
{
    // ZZ> This function spawns an attack particle
    int weapon, particle, spawngrip, thrown;
    Uint8 action;
    Uint16 tTmp;
    float dampen;
    float velocity;
    chr_t * pchr, * pweapon;
    cap_t * pweapon_cap;

    if( INVALID_CHR(cnt) ) return;
    pchr = ChrList.lst + cnt;

    weapon = pchr->holdingwhich[slot];
    spawngrip = GRIP_LAST;
    action = pchr->action;

    // See if it's an unarmed attack...
    if ( weapon == MAX_CHR )
    {
        weapon = cnt;
        spawngrip = slot_to_grip_offset( slot );  // 0 -> GRIP_LEFT, 1 -> GRIP_RIGHT
    }

    if( INVALID_CHR(weapon) ) return;
    pweapon = ChrList.lst + weapon;

    pweapon_cap = chr_get_pcap(weapon);
    if( NULL == pweapon_cap ) return;

    if ( weapon != cnt && ( ( pweapon_cap->isstackable && pweapon->ammo > 1 ) || ( action >= ACTION_FA && action <= ACTION_FD ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation

        thrown = spawn_one_character( pchr->pos, pweapon->iprofile, chr_get_iteam(cnt), 0, pchr->turn_z, pweapon->name, MAX_CHR );
        if ( VALID_CHR(thrown) )
        {
            chr_t * pthrown = ChrList.lst + thrown;

            pthrown->iskursed = bfalse;
            pthrown->ammo = 1;
            pthrown->ai.alert |= ALERTIF_THROWN;
            velocity = pchr->strength / ( pthrown->weight * THROWFIX );
            velocity += MINTHROWVELOCITY;
            if ( velocity > MAXTHROWVELOCITY )
            {
                velocity = MAXTHROWVELOCITY;
            }

            tTmp = pchr->turn_z >> 2;
            pthrown->vel.x += turntocos[( tTmp+8192 ) & TRIG_TABLE_MASK] * velocity;
            pthrown->vel.y += turntosin[( tTmp+8192 ) & TRIG_TABLE_MASK] * velocity;
            pthrown->vel.z = DROPZVEL;
            if ( pweapon->ammo <= 1 )
            {
                // Poof the item
                detach_character_from_mount( weapon, btrue, bfalse );
                free_one_character_in_game( weapon );
            }
            else
            {
                pweapon->ammo--;
            }
        }
    }
    else
    {
        // Spawn an attack particle
        if ( pweapon->ammomax == 0 || pweapon->ammo != 0 )
        {
            if ( pweapon->ammo > 0 && !pweapon_cap->isstackable )
            {
                pweapon->ammo--;  // Ammo usage
            }

            // Spawn an attack particle
            if ( pweapon_cap->attackprttype != -1 )
            {
                particle = spawn_one_particle( pweapon->pos.x, pweapon->pos.y, pweapon->pos.z, pchr->turn_z, pweapon->iprofile, pweapon_cap->attackprttype, weapon, spawngrip, chr_get_iteam(cnt), cnt, 0, MAX_CHR );
                if ( VALID_PRT(particle) )
                {
                    prt_t * pprt = PrtList.lst + particle;

                    if ( !pweapon_cap->attackattached )
                    {
                        // Detach the particle
                        if ( !prt_get_ppip(particle)->startontarget || pprt->target == MAX_CHR )
                        {
                            attach_particle_to_character( particle, weapon, spawngrip );

                            // Correct Z spacing base, but nothing else...
                            pprt->pos.z += prt_get_ppip(particle)->zspacing_pair.base;
                        }

                        pprt->attachedtocharacter = MAX_CHR;

                        // Don't spawn in walls
                        if ( __prthitawall( particle ) )
                        {
                            pprt->pos.x = pweapon->pos.x;
                            pprt->pos.y = pweapon->pos.y;
                            if ( __prthitawall( particle ) )
                            {
                                pprt->pos.x = pchr->pos.x;
                                pprt->pos.y = pchr->pos.y;
                            }
                        }
                    }
                    else
                    {
                        // Attached particles get a strength bonus for reeling...
                        dampen = REELBASE + ( pchr->strength / REEL );
                        pprt->vel.x = pprt->vel.x * dampen;
                        pprt->vel.y = pprt->vel.y * dampen;
                        pprt->vel.z = pprt->vel.z * dampen;
                    }

                    // Initial particles get a strength bonus, which may be 0.00f
                    pprt->damage.base += ( pchr->strength * pweapon_cap->strengthdampen );

                    // Initial particles get an enchantment bonus
                    pprt->damage.base += pweapon->damageboost;

                    // Initial particles inherit damage type of weapon
                    // pprt->damagetype = pweapon->damagetargettype;  // Zefz: not sure if we want this. we can have weapons with different damage types
                }
            }
        }
        else
        {
            pweapon->ammoknown = btrue;
        }
    }
}

//--------------------------------------------------------------------------------------------
void drop_money( Uint16 character, Uint16 money )
{
    // ZZ> This function drops some of a character's money
    Uint16 huns, tfives, fives, ones, cnt;

    if ( INVALID_CHR( character ) ) return;

    if ( money > ChrList.lst[character].money )  money = ChrList.lst[character].money;
    if ( money > 0 && ChrList.lst[character].pos.z > -2 )
    {
        ChrList.lst[character].money = ChrList.lst[character].money - money;
        huns = money / 100;  money -= ( huns << 7 ) - ( huns << 5 ) + ( huns << 2 );
        tfives = money / 25;  money -= ( tfives << 5 ) - ( tfives << 3 ) + tfives;
        fives = money / 5;  money -= ( fives << 2 ) + fives;
        ones = money;

        for ( cnt = 0; cnt < ones; cnt++ )
        {
            spawn_one_particle( ChrList.lst[character].pos.x, ChrList.lst[character].pos.y,  ChrList.lst[character].pos.z, 0, MAX_PROFILE, COIN1, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, cnt, MAX_CHR );
        }

        for ( cnt = 0; cnt < fives; cnt++ )
        {
            spawn_one_particle( ChrList.lst[character].pos.x, ChrList.lst[character].pos.y,  ChrList.lst[character].pos.z, 0, MAX_PROFILE, COIN5, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, cnt, MAX_CHR );
        }

        for ( cnt = 0; cnt < tfives; cnt++ )
        {
            spawn_one_particle( ChrList.lst[character].pos.x, ChrList.lst[character].pos.y,  ChrList.lst[character].pos.z, 0, MAX_PROFILE, COIN25, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, cnt, MAX_CHR );
        }

        for ( cnt = 0; cnt < huns; cnt++ )
        {
            spawn_one_particle( ChrList.lst[character].pos.x, ChrList.lst[character].pos.y,  ChrList.lst[character].pos.z, 0, MAX_PROFILE, COIN100, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, cnt, MAX_CHR );
        }

        ChrList.lst[character].damagetime = DAMAGETIME;  // So it doesn't grab it again
    }
}

//--------------------------------------------------------------------------------------------
void call_for_help( Uint16 character )
{
    // ZZ> This function issues a call for help to all allies
    Uint8 team;
    Uint16 cnt;

    if ( INVALID_CHR( character ) ) return;

    team = chr_get_iteam(character);
    TeamList[team].sissy = character;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( ChrList.lst[cnt].on && cnt != character && !team_hates_team(cnt,team) )
        {
            chr_get_pai(cnt)->alert |= ALERTIF_CALLEDFORHELP;
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t setup_xp_table( Uint16 icap )
{
    // This calculates the xp needed to reach next level and stores it in an array for later use
    Uint8 level;
    cap_t * pcap;

    if ( INVALID_CAP(icap) ) return bfalse;
    pcap = CapList + icap;

    // Calculate xp needed
    for (level = MAXBASELEVEL; level < MAXLEVEL; level++)
    {
        Uint32 xpneeded = pcap->experienceforlevel[MAXBASELEVEL - 1];
        xpneeded += ( level * level * level * 15 );
        xpneeded -= ( ( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * 15 );
        pcap->experienceforlevel[level] = xpneeded;
    }
    return btrue;
}

//--------------------------------------------------------------------------------------------
void do_level_up( Uint16 character )
{
    // BB > level gains are done here, but only once a second

    Uint8 curlevel;
    int number;
    chr_t * pchr;
    cap_t * pcap;

    if ( INVALID_CHR(character) ) return;
    pchr = ChrList.lst + character;

    pcap = chr_get_pcap( character );
    if( NULL == pcap ) return;

    // Do level ups and stat changes
    curlevel = pchr->experiencelevel + 1;
    if ( curlevel < MAXLEVEL )
    {
        Uint32 xpcurrent, xpneeded;

        xpcurrent = pchr->experience;
        xpneeded  = pcap->experienceforlevel[curlevel];
        if ( xpcurrent >= xpneeded )
        {
            // do the level up
            pchr->experiencelevel++;
            xpneeded = pcap->experienceforlevel[curlevel];

            // The character is ready to advance...
            if ( pchr->isplayer )
            {
                debug_printf( "%s gained a level!!!", pchr->name );
                sound_play_chunk( PCamera->track_pos, g_wavelist[GSND_LEVELUP] );
            }

            // Size
            pchr->fat_goto += pcap->sizeperlevel * 0.25f;  // Limit this?
            pchr->fat_goto_time += SIZETIME;

            // Strength
            number = generate_number( pcap->strength_stat.perlevel );
            number += pchr->strength;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->strength = number;

            // Wisdom
            number = generate_number( pcap->wisdom_stat.perlevel );
            number += pchr->wisdom;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->wisdom = number;

            // Intelligence
            number = generate_number( pcap->intelligence_stat.perlevel );
            number += pchr->intelligence;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->intelligence = number;

            // Dexterity
            number = generate_number( pcap->dexterity_stat.perlevel );
            number += pchr->dexterity;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->dexterity = number;

            // Life
            number = generate_number( pcap->life_stat.perlevel );
            number += pchr->lifemax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            pchr->life += ( number - pchr->lifemax );
            pchr->lifemax = number;

            // Mana
            number = generate_number( pcap->mana_stat.perlevel );
            number += pchr->manamax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            pchr->mana += ( number - pchr->manamax );
            pchr->manamax = number;

            // Mana Return
            number = generate_number( pcap->manareturn_stat.perlevel );
            number += pchr->manareturn;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->manareturn = number;

            // Mana Flow
            number = generate_number( pcap->manaflow_stat.perlevel );
            number += pchr->manaflow;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->manaflow = number;
        }
    }
}

//--------------------------------------------------------------------------------------------
void give_experience( Uint16 character, int amount, Uint8 xptype, bool_t override_invictus )
{
    // ZZ> This function gives a character experience

    int newamount;

    chr_t * pchr;
    cap_t * pcap;

    if ( INVALID_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    pcap = chr_get_pcap(character);
    if( NULL == pcap ) return;

    if ( 0 == amount ) return;

    if ( !pchr->invictus || override_invictus )
    {
        float intadd = (FP8_TO_INT(pchr->intelligence) - 10.0f) / 200.0f;
        float wisadd = (FP8_TO_INT(pchr->wisdom) - 10.0f)       / 400.0f;

        // Figure out how much experience to give
        newamount = amount;
        if ( xptype < XP_COUNT )
        {
            newamount = amount * pcap->experiencerate[xptype];
        }

        // Intelligence and slightly wisdom increases xp gained (0,5% per int and 0,25% per wisdom above 10)
        newamount *= 1.00f + intadd + wisadd;

        // Apply XP bonus/penality depending on game difficulty
        if ( cfg.difficulty >= GAME_HARD ) newamount *= 1.10f;          // 10% extra on hard

        pchr->experience += newamount;
    }
}

//--------------------------------------------------------------------------------------------
void give_team_experience( Uint8 team, int amount, Uint8 xptype )
{
    // ZZ> This function gives every character on a team experience
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( chr_get_iteam(cnt) == team && ChrList.lst[cnt].on )
        {
            give_experience( cnt, amount, xptype, bfalse );
        }
    }
}

//--------------------------------------------------------------------------------------------
void resize_characters()
{
    // ZZ> This function makes the characters get bigger or smaller, depending
    //    on their fat_goto and fat_goto_time

    bool_t willgetcaught;
    int ichr;
    float newsize;

    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        chr_t * pchr;

        if ( !ChrList.lst[ichr].on ) continue;
        pchr = ChrList.lst + ichr;

        if(  0 != pchr->fat_goto_time ) continue;

        if ( pchr->fat_goto != pchr->fat )
        {
            int bump_increase;
            float nrm[2];

            bump_increase = ( pchr->fat_goto - pchr->fat ) * 0.10f * pchr->bump.size;

            // Make sure it won't get caught in a wall
            willgetcaught = bfalse;
            if ( pchr->fat_goto > pchr->fat )
            {
                pchr->bump.size += bump_increase;

                if ( __chrhitawall( ichr, nrm ) )
                {
                    willgetcaught = btrue;
                }

                pchr->bump.size -= bump_increase;
            }

            // If it is getting caught, simply halt growth until later
            if ( !willgetcaught )
            {
                // Figure out how big it is
                pchr->fat_goto_time--;

                newsize = pchr->fat_goto;
                if ( pchr->fat_goto_time > 0 )
                {
                    newsize = ( pchr->fat * 0.90f ) + ( newsize * 0.10f );
                }

                // Make it that big...
                chr_set_fat( pchr, newsize );

                if ( chr_get_pcap(ichr)->weight == 0xFF )
                {
                    pchr->weight = 0xFFFFFFFF;
                }
                else
                {
                    Uint32 itmp = chr_get_pcap(ichr)->weight * pchr->fat * pchr->fat * pchr->fat;
                    pchr->weight = MIN( itmp, (Uint32)0xFFFFFFFE );
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_name( const char *szSaveName, Uint16 character )
{
    // ZZ> This function makes the naming.txt file for the character
    vfs_FILE* filewrite;
    char cTmp;
    int cnt, tnc;

    if ( INVALID_CHR(character) ) return bfalse;

    // Can it export?
    filewrite = vfs_openWrite( szSaveName );
    if ( NULL == filewrite ) return bfalse;

    cnt = 0;
    cTmp = ChrList.lst[character].name[0];
    cnt++;

    while ( cnt < MAXCAPNAMESIZE && cTmp != 0 )
    {
        vfs_printf( filewrite, ":" );
        tnc = 0;

        while ( tnc < 8 && cTmp != 0 )
        {
            if ( ' ' == cTmp )
            {
                vfs_printf( filewrite, "_" );
            }
            else
            {
                vfs_printf( filewrite, "%c", cTmp );
            }

            cTmp = ChrList.lst[character].name[cnt];
            tnc++;
            cnt++;
        }

        vfs_printf( filewrite, "\n" );
        vfs_printf( filewrite, ":STOP\n\n" );
    }

    vfs_close( filewrite );

    return btrue;

}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_profile( const char *szSaveName, Uint16 character )
{
    // ZZ> This function creates a data.txt file for the given character.
    //    it is assumed that all enchantments have been done away with

    vfs_FILE* filewrite;
    int icap;
    int damagetype, skin;
    char types[10] = "SCPHEFIZ";
    char codes[4];
    chr_t * pchr;
    cap_t * pcap;

    if ( INVALID_CHR(character) ) return bfalse;
    pchr = ChrList.lst + character;

    // General stuff
    icap = chr_get_icap( character );
    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return bfalse;

    // Open the file
    filewrite = vfs_openWrite( szSaveName );
    if ( NULL == filewrite ) return bfalse;

    // Real general data
    vfs_printf( filewrite, "Slot number    : -1\n" );  // -1 signals a flexible load thing
    fput_string_under( filewrite, "Class name     : ", pcap->classname );
    fput_bool( filewrite, "Uniform light  : ", pcap->uniformlit );
    vfs_printf( filewrite, "Maximum ammo   : %d\n", pcap->ammomax );
    vfs_printf( filewrite, "Current ammo   : %d\n", pchr->ammo );
    fput_gender( filewrite, "Gender         : ", pchr->gender );
    vfs_printf( filewrite, "\n" );

    // Object stats
    vfs_printf( filewrite, "Life color     : %d\n", pchr->lifecolor );
    vfs_printf( filewrite, "Mana color     : %d\n", pchr->manacolor );
    vfs_printf( filewrite, "Life           : %4.2f\n", pchr->lifemax / 256.0f );
    fput_pair( filewrite, "Life up        : ", pcap->life_stat.perlevel );
    vfs_printf( filewrite, "Mana           : %4.2f\n", pchr->manamax / 256.0f );
    fput_pair( filewrite, "Mana up        : ", pcap->mana_stat.perlevel );
    vfs_printf( filewrite, "Mana return    : %4.2f\n", pchr->manareturn / 256.0f );
    fput_pair( filewrite, "Mana return up : ", pcap->manareturn_stat.perlevel );
    vfs_printf( filewrite, "Mana flow      : %4.2f\n", pchr->manaflow / 256.0f );
    fput_pair( filewrite, "Mana flow up   : ", pcap->manaflow_stat.perlevel );
    vfs_printf( filewrite, "STR            : %4.2f\n", pchr->strength / 256.0f );
    fput_pair( filewrite, "STR up         : ", pcap->strength_stat.perlevel );
    vfs_printf( filewrite, "WIS            : %4.2f\n", pchr->wisdom / 256.0f );
    fput_pair( filewrite, "WIS up         : ", pcap->wisdom_stat.perlevel );
    vfs_printf( filewrite, "INT            : %4.2f\n", pchr->intelligence / 256.0f );
    fput_pair( filewrite, "INT up         : ", pcap->intelligence_stat.perlevel );
    vfs_printf( filewrite, "DEX            : %4.2f\n", pchr->dexterity / 256.0f );
    fput_pair( filewrite, "DEX up         : ", pcap->dexterity_stat.perlevel );
    vfs_printf( filewrite, "\n" );

    // More physical attributes
    vfs_printf( filewrite, "Size           : %4.2f\n", pchr->fat_goto );
    vfs_printf( filewrite, "Size up        : %4.2f\n", pcap->sizeperlevel );
    vfs_printf( filewrite, "Shadow size    : %d\n", pcap->shadowsize );
    vfs_printf( filewrite, "Bump size      : %d\n", pcap->bumpsize );
    vfs_printf( filewrite, "Bump height    : %d\n", pcap->bumpheight );
    vfs_printf( filewrite, "Bump dampen    : %4.2f\n", pcap->bumpdampen );
    vfs_printf( filewrite, "Weight         : %d\n", pcap->weight );
    vfs_printf( filewrite, "Jump power     : %4.2f\n", pcap->jump );
    vfs_printf( filewrite, "Jump number    : %d\n", pcap->jumpnumber );
    vfs_printf( filewrite, "Sneak speed    : %d\n", pcap->sneakspd );
    vfs_printf( filewrite, "Walk speed     : %d\n", pcap->walkspd );
    vfs_printf( filewrite, "Run speed      : %d\n", pcap->runspd );
    vfs_printf( filewrite, "Fly to height  : %d\n", pcap->flyheight );
    vfs_printf( filewrite, "Flashing AND   : %d\n", pcap->flashand );
    vfs_printf( filewrite, "Alpha blending : %d\n", pcap->alpha );
    vfs_printf( filewrite, "Light blending : %d\n", pcap->light );
    fput_bool( filewrite, "Transfer blend : ", pcap->transferblend );
    vfs_printf( filewrite, "Sheen          : %d\n", pcap->sheen );
    fput_bool( filewrite, "Phong mapping  : ", pcap->enviro );
    vfs_printf( filewrite, "Texture X add  : %4.2f\n", pcap->uoffvel / (float)0xFFFF );
    vfs_printf( filewrite, "Texture Y add  : %4.2f\n", pcap->voffvel / (float)0xFFFF );
    fput_bool( filewrite, "Sticky butt    : ", pcap->stickybutt );
    vfs_printf( filewrite, "\n" );

    // Invulnerability data
    fput_bool( filewrite, "Invictus       : ", pcap->invictus );
    vfs_printf( filewrite, "NonI facing    : %d\n", pcap->nframefacing );
    vfs_printf( filewrite, "NonI angle     : %d\n", pcap->nframeangle );
    vfs_printf( filewrite, "I facing       : %d\n", pcap->iframefacing );
    vfs_printf( filewrite, "I angle        : %d\n", pcap->iframeangle );
    vfs_printf( filewrite, "\n" );

    // Skin defenses
    vfs_printf( filewrite, "Base defense   : %3d %3d %3d %3d\n", 255 - pcap->defense[0], 255 - pcap->defense[1],
             255 - pcap->defense[2], 255 - pcap->defense[3] );

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        vfs_printf( filewrite, "%c damage shift : %3d %3d %3d %3d\n", types[damagetype],
                 pcap->damagemodifier[damagetype][0]&DAMAGESHIFT,
                 pcap->damagemodifier[damagetype][1]&DAMAGESHIFT,
                 pcap->damagemodifier[damagetype][2]&DAMAGESHIFT,
                 pcap->damagemodifier[damagetype][3]&DAMAGESHIFT );
    }

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        skin = 0;

        while ( skin < MAX_SKIN )
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
        vfs_printf( filewrite, "%c damage code  : %3c %3c %3c %3c\n", types[damagetype], codes[0], codes[1], codes[2], codes[3] );
    }

    vfs_printf( filewrite, "Acceleration   : %3.0f %3.0f %3.0f %3.0f\n", pcap->maxaccel[0]*80,
             pcap->maxaccel[1]*80,
             pcap->maxaccel[2]*80,
             pcap->maxaccel[3]*80 );
    vfs_printf( filewrite, "\n" );

    // Experience and level data
    vfs_printf( filewrite, "EXP for 2nd    : %d\n", pcap->experienceforlevel[1] );
    vfs_printf( filewrite, "EXP for 3rd    : %d\n", pcap->experienceforlevel[2] );
    vfs_printf( filewrite, "EXP for 4th    : %d\n", pcap->experienceforlevel[3] );
    vfs_printf( filewrite, "EXP for 5th    : %d\n", pcap->experienceforlevel[4] );
    vfs_printf( filewrite, "EXP for 6th    : %d\n", pcap->experienceforlevel[5] );
    vfs_printf( filewrite, "Starting EXP   : %d\n", pchr->experience );
    vfs_printf( filewrite, "EXP worth      : %d\n", pcap->experienceworth );
    vfs_printf( filewrite, "EXP exchange   : %5.3f\n", pcap->experienceexchange );
    vfs_printf( filewrite, "EXPSECRET      : %4.2f\n", pcap->experiencerate[0] );
    vfs_printf( filewrite, "EXPQUEST       : %4.2f\n", pcap->experiencerate[1] );
    vfs_printf( filewrite, "EXPDARE        : %4.2f\n", pcap->experiencerate[2] );
    vfs_printf( filewrite, "EXPKILL        : %4.2f\n", pcap->experiencerate[3] );
    vfs_printf( filewrite, "EXPMURDER      : %4.2f\n", pcap->experiencerate[4] );
    vfs_printf( filewrite, "EXPREVENGE     : %4.2f\n", pcap->experiencerate[5] );
    vfs_printf( filewrite, "EXPTEAMWORK    : %4.2f\n", pcap->experiencerate[6] );
    vfs_printf( filewrite, "EXPROLEPLAY    : %4.2f\n", pcap->experiencerate[7] );
    vfs_printf( filewrite, "\n" );

    // IDSZ identification tags
    vfs_printf( filewrite, "IDSZ Parent    : [%s]\n", undo_idsz( pcap->idsz[IDSZ_PARENT] ) );
    vfs_printf( filewrite, "IDSZ Type      : [%s]\n", undo_idsz( pcap->idsz[IDSZ_TYPE] ) );
    vfs_printf( filewrite, "IDSZ Skill     : [%s]\n", undo_idsz( pcap->idsz[IDSZ_SKILL] ) );
    vfs_printf( filewrite, "IDSZ Special   : [%s]\n", undo_idsz( pcap->idsz[IDSZ_SPECIAL] ) );
    vfs_printf( filewrite, "IDSZ Hate      : [%s]\n", undo_idsz( pcap->idsz[IDSZ_HATE] ) );
    vfs_printf( filewrite, "IDSZ Vulnie    : [%s]\n", undo_idsz( pcap->idsz[IDSZ_VULNERABILITY] ) );
    vfs_printf( filewrite, "\n" );

    // Item and damage flags
    fput_bool( filewrite, "Is an item     : ", pcap->isitem );
    fput_bool( filewrite, "Is a mount     : ", pcap->ismount );
    fput_bool( filewrite, "Is stackable   : ", pcap->isstackable );
    fput_bool( filewrite, "Name known     : ", pchr->nameknown );
    fput_bool( filewrite, "Usage known    : ", pcap->usageknown );
    fput_bool( filewrite, "Is exportable  : ", pcap->cancarrytonextmodule );
    fput_bool( filewrite, "Requires skill : ", pcap->needskillidtouse );
    fput_bool( filewrite, "Is platform    : ", pcap->platform );
    fput_bool( filewrite, "Collects money : ", pcap->cangrabmoney );
    fput_bool( filewrite, "Can open stuff : ", pcap->canopenstuff );
    vfs_printf( filewrite, "\n" );

    // Other item and damage stuff
    fput_damage_type( filewrite, "Damage type    : ", pcap->damagetargettype );
    fput_action( filewrite, "Attack type    : ", pcap->weaponaction );
    vfs_printf( filewrite, "\n" );

    // Particle attachments
    vfs_printf( filewrite, "Attached parts : %d\n", pcap->attachedprt_amount );
    fput_damage_type( filewrite, "Reaffirm type  : ", pcap->attachedprt_reaffirmdamagetype );
    vfs_printf( filewrite, "Particle type  : %d\n", pcap->attachedprt_pip );
    vfs_printf( filewrite, "\n" );

    // Character hands
    fput_bool( filewrite, "Left valid     : ", pcap->slotvalid[SLOT_LEFT] );
    fput_bool( filewrite, "Right valid    : ", pcap->slotvalid[SLOT_RIGHT] );
    vfs_printf( filewrite, "\n" );

    // Particle spawning on attack
    fput_bool( filewrite, "Part on weapon : ", pcap->attackattached );
    vfs_printf( filewrite, "Part type      : %d\n", pcap->attackprttype );
    vfs_printf( filewrite, "\n" );

    // Particle spawning for GoPoof
    vfs_printf( filewrite, "Poof amount    : %d\n", pcap->gopoofprtamount );
    vfs_printf( filewrite, "Facing add     : %d\n", pcap->gopoofprtfacingadd );
    vfs_printf( filewrite, "Part type      : %d\n", pcap->gopoofprttype );
    vfs_printf( filewrite, "\n" );

    // Particle spawning for blud
    fput_bool( filewrite, "Blud valid     : ", pcap->bludvalid );
    vfs_printf( filewrite, "Part type      : %d\n", pcap->bludprttype );
    vfs_printf( filewrite, "\n" );

    // Extra stuff
    fput_bool( filewrite, "Waterwalking   : ", pcap->waterwalk );
    vfs_printf( filewrite, "Bounce dampen  : %5.3f\n", pcap->dampen );
    vfs_printf( filewrite, "\n" );

    // More stuff
    vfs_printf( filewrite, "NOT USED       : %5.3f\n", pcap->lifeheal / 256.0f );       // These two are seriously outdated
    vfs_printf( filewrite, "NOT USED       : %5.3f\n", pcap->manacost / 256.0f );       // and shouldnt be used. Use scripts instead.
    vfs_printf( filewrite, "Regeneration   : %d\n", pcap->lifereturn );
    vfs_printf( filewrite, "Stopped by     : %d\n", pcap->stoppedby );
    fput_string_under( filewrite, "Skin 0 name    : ", pcap->skinname[0] );
    fput_string_under( filewrite, "Skin 1 name    : ", pcap->skinname[1] );
    fput_string_under( filewrite, "Skin 2 name    : ", pcap->skinname[2] );
    fput_string_under( filewrite, "Skin 3 name    : ", pcap->skinname[3] );
    vfs_printf( filewrite, "Skin 0 cost    : %d\n", pcap->skincost[0] );
    vfs_printf( filewrite, "Skin 1 cost    : %d\n", pcap->skincost[1] );
    vfs_printf( filewrite, "Skin 2 cost    : %d\n", pcap->skincost[2] );
    vfs_printf( filewrite, "Skin 3 cost    : %d\n", pcap->skincost[3] );
    vfs_printf( filewrite, "STR dampen     : %5.3f\n", pcap->strengthdampen );
    vfs_printf( filewrite, "\n" );

    // Another memory lapse
    fput_bool( filewrite, "No rider attak : ", btrue - pcap->ridercanattack );
    fput_bool( filewrite, "Can be dazed   : ", pcap->canbedazed );
    fput_bool( filewrite, "Can be grogged : ", pcap->canbegrogged );
    vfs_printf( filewrite, "NOT USED       : 0\n" );
    vfs_printf( filewrite, "NOT USED       : 0\n" );
    fput_bool( filewrite, "Can see invisi : ", pcap->canseeinvisible );
    vfs_printf( filewrite, "Kursed chance  : %d\n", pchr->iskursed*100 );
    vfs_printf( filewrite, "Footfall sound : %d\n", pcap->soundindex[SOUND_FOOTFALL] );
    vfs_printf( filewrite, "Jump sound     : %d\n", pcap->soundindex[SOUND_JUMP] );
    vfs_printf( filewrite, "\n" );

    // Expansions
    if ( pcap->skindressy&1 )
        vfs_printf( filewrite, ":[DRES] 0\n" );

    if ( pcap->skindressy&2 )
        vfs_printf( filewrite, ":[DRES] 1\n" );

    if ( pcap->skindressy&4 )
        vfs_printf( filewrite, ":[DRES] 2\n" );

    if ( pcap->skindressy&8 )
        vfs_printf( filewrite, ":[DRES] 3\n" );

    if ( pcap->resistbumpspawn )
        vfs_printf( filewrite, ":[STUK] 0\n" );

    if ( pcap->istoobig )
        vfs_printf( filewrite, ":[PACK] 0\n" );

    if ( !pcap->reflect )
        vfs_printf( filewrite, ":[VAMP] 1\n" );

    if ( pcap->alwaysdraw )
        vfs_printf( filewrite, ":[DRAW] 1\n" );

    if ( pcap->isranged )
        vfs_printf( filewrite, ":[RANG] 1\n" );

    if ( pcap->hidestate != NOHIDE )
        vfs_printf( filewrite, ":[HIDE] %d\n", pcap->hidestate );

    if ( pcap->isequipment )
        vfs_printf( filewrite, ":[EQUI] 1\n" );

    if ( pcap->bumpsizebig >= pcap->bumpsize * 2 )
        vfs_printf( filewrite, ":[SQUA] 1\n" );

    if ( pcap->icon != pcap->usageknown )
        vfs_printf( filewrite, ":[ICON] %d\n", pcap->icon );

    if ( pcap->forceshadow )
        vfs_printf( filewrite, ":[SHAD] 1\n" );

    if ( pcap->ripple == pcap->isitem )
        vfs_printf( filewrite, ":[RIPP] %d\n", pcap->ripple );

    if ( pcap->isvaluable != -1 )
        vfs_printf( filewrite, ":[VALU] %d\n", pcap->isvaluable );

    // Basic stuff that is always written
    vfs_printf( filewrite, ":[GOLD] %d\n", pchr->money );
    vfs_printf( filewrite, ":[PLAT] %d\n", pcap->canuseplatforms );
    vfs_printf( filewrite, ":[SKIN] %d\n", pchr->skin );
    vfs_printf( filewrite, ":[CONT] %d\n", pchr->ai.content );
    vfs_printf( filewrite, ":[STAT] %d\n", pchr->ai.state );
    vfs_printf( filewrite, ":[LEVL] %d\n", pchr->experiencelevel );
    vfs_printf( filewrite, ":[LIFE] %4.2f\n", FP8_TO_FLOAT(pchr->life) );
    vfs_printf( filewrite, ":[MANA] %4.2f\n", FP8_TO_FLOAT(pchr->mana) );

    // Copy all skill expansions

    if ( pchr->shieldproficiency > 0 )
        vfs_printf( filewrite, ":[SHPR] %d\n", pchr->shieldproficiency );

    if ( pchr->canuseadvancedweapons > 0 )
        vfs_printf( filewrite, ":[AWEP] %d\n", pchr->canuseadvancedweapons );

    if ( pchr->canjoust )
        vfs_printf( filewrite, ":[JOUS] %d\n", pchr->canjoust );

    if ( pchr->candisarm )
        vfs_printf( filewrite, ":[DISA] %d\n", pchr->candisarm );

    if ( pcap->canseekurse )
        vfs_printf( filewrite, ":[CKUR] %d\n", pcap->canseekurse );

    if ( pchr->canusepoison )
        vfs_printf( filewrite, ":[POIS] %d\n", pchr->canusepoison );

    if ( pchr->canread )
        vfs_printf( filewrite, ":[READ] %d\n", pchr->canread );

    if ( pchr->canbackstab )
        vfs_printf( filewrite, ":[STAB] %d\n", pchr->canbackstab );

    if ( pchr->canusedivine )
        vfs_printf( filewrite, ":[HMAG] %d\n", pchr->canusedivine );

    if ( pchr->canusearcane )
        vfs_printf( filewrite, ":[WMAG] %d\n", pchr->canusearcane );

    if ( pchr->canusetech )
        vfs_printf( filewrite, ":[TECH] %d\n", pchr->canusetech );

    // The end
    vfs_close( filewrite );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_skin( const char *szSaveName, Uint16 character )
{
    // ZZ> This function creates a skin.txt file for the given character.
    vfs_FILE* filewrite;

    if ( INVALID_CHR(character) ) return bfalse;

    // Open the file
    filewrite = vfs_openWrite( szSaveName );
    if ( NULL == filewrite ) return bfalse;

    vfs_printf( filewrite, "// This file is used only by the import menu\n" );
    vfs_printf( filewrite, ": %d\n", ChrList.lst[character].skin );
    vfs_close( filewrite );

    return btrue;
}

//--------------------------------------------------------------------------------------------
int load_one_character_profile( const char * tmploadname, int slot_override, bool_t required )
{
    // ZZ> This function fills a character profile with data from data.txt, returning
    // the icap slot that the profile was stuck into.  It may cause the program
    // to abort if bad things happen.

    Sint16  icap = -1;
    cap_t * pcap;
    STRING  szLoadName;

    if( VALID_PRO_RANGE( slot_override ) )
    {
        icap = slot_override;
    }
    else
    {
        icap = pro_get_slot( tmploadname, MAX_PROFILE );
    }

    if( !VALID_CAP_RANGE(icap) )
    {
        // The data file wasn't found
        if ( required )
        {
            log_error( "load_one_character_profile() - \"%s\" was not found!\n", szLoadName );
        }
        else if( VALID_CAP_RANGE(slot_override) && slot_override > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "load_one_character_profile() - Not able to open file \"%s\"\n", szLoadName );
        }

        return MAX_PROFILE;
    }

    pcap = CapList + icap;

    // if there is data in this profile, release it
    if( pcap->loaded )
    {
        // Make sure global objects don't load over existing models
        if ( required && SPELLBOOK == icap )
        {
            log_error( "Object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, szLoadName );
        }
        else if ( required && overrideslots )
        {
            log_error( "Object slot %i used twice (%s, %s)\n", icap, pcap->name, szLoadName );
        }
        else
        {
            // Stop, we don't want to override it
            return MAX_PROFILE;
        }

        // If loading over an existing model is allowed (?how?) then make sure to release the old one
        release_one_cap(icap);
    }

    if( NULL == load_one_cap_file( tmploadname, pcap ) )
    {
        return MAX_PROFILE;
    }

    // do the rest of the levels not listed in data.txt
    setup_xp_table(icap);

    if( cfg.gourard_req )
    {
        pcap->uniformlit = bfalse;
    }

    // limit the wave indices to rational values
    pcap->soundindex[SOUND_FOOTFALL] = CLIP(pcap->soundindex[SOUND_FOOTFALL], INVALID_SOUND, MAX_WAVE);
    pcap->soundindex[SOUND_JUMP]     = CLIP(pcap->soundindex[SOUND_JUMP], INVALID_SOUND, MAX_WAVE);

    // bumpdampen == 0 means infinite mass, and causes some problems
    pcap->bumpdampen = MAX(INV_FF, pcap->bumpdampen);

    return icap;
}

//--------------------------------------------------------------------------------------------
bool_t heal_character( Uint16 character, Uint16 healer, int amount, bool_t ignoreinvincible)
{
    // ZF> This function gives some pure life points to the target, ignoring any resistances and so forth
    if ( INVALID_CHR(character) || amount == 0 || !ChrList.lst[character].alive || (ChrList.lst[character].invictus && !ignoreinvincible) ) return bfalse;

    ChrList.lst[character].life = CLIP(ChrList.lst[character].life, ChrList.lst[character].life + ABS(amount), ChrList.lst[character].lifemax);

    // Dont alert that we healed ourselves
    if ( healer != character && ChrList.lst[healer].attachedto != character )
    {
        chr_get_pai(character)->alert |= ALERTIF_HEALED;
        chr_get_pai(character)->attacklast = healer;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void damage_character( Uint16 character, Uint16 direction,
                       IPair  damage, Uint8 damagetype, Uint8 team,
                       Uint16 attacker, Uint16 effects, bool_t ignoreinvincible )
{
    // ZZ> This function calculates and applies actual_damage to a character.  It also
    //    sets alerts and begins actions.  Blocking and frame invincibility
    //    are done here too.  Direction is 0 if the attack is coming head on,
    //    16384 if from the right, 32768 if from the back, 49152 if from the
    //    left.

    int tnc;
    Uint16 action;
    int    actual_damage, base_damage;
    Uint16 experience, left, right;
    chr_t * pchr;

    if ( INVALID_CHR(character) ) return;
    pchr = ChrList.lst + character;

    if ( pchr->alive && damage.base >= 0 && damage.rand >= 1 )
    {
        // Lessen actual_damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
        // This can also be used to lessen effectiveness of healing
        actual_damage = generate_number( damage );
        base_damage   = actual_damage;
        actual_damage = actual_damage >> ( pchr->damagemodifier[damagetype] & DAMAGESHIFT );

        // Allow actual_damage to be dealt to mana (mana shield spell)
        if ( pchr->damagemodifier[damagetype]&DAMAGEMANA )
        {
            int manadamage;
            manadamage = MAX(pchr->mana - actual_damage, 0);
            pchr->mana = manadamage;
            actual_damage -= manadamage;
            pchr->ai.alert |= ALERTIF_ATTACKED;
            pchr->ai.attacklast = attacker;
        }

        // Allow charging (Invert actual_damage to mana)
        if ( pchr->damagemodifier[damagetype]&DAMAGECHARGE )
        {
            pchr->mana += actual_damage;
            if ( pchr->mana > pchr->manamax )
            {
                pchr->mana = pchr->manamax;
            }
            return;
        }

        // Invert actual_damage to heal
        if ( pchr->damagemodifier[damagetype]&DAMAGEINVERT )
            actual_damage = -actual_damage;

        // Remember the actual_damage type
        pchr->ai.damagetypelast = damagetype;
        pchr->ai.directionlast = direction;

        // Do it already
        if ( actual_damage > 0 )
        {
            // Only actual_damage if not invincible
            if ( (0 == pchr->damagetime || ignoreinvincible) && !pchr->invictus )
            {
                // Hard mode deals 25% extra actual_damage to players!
                if ( cfg.difficulty >= GAME_HARD && pchr->isplayer && !ChrList.lst[attacker].isplayer ) actual_damage *= 1.25f;

                // East mode deals 25% extra actual_damage by players and 25% less to players
                if ( cfg.difficulty <= GAME_EASY )
                {
                    if ( ChrList.lst[attacker].isplayer && !pchr->isplayer ) actual_damage *= 1.25f;
                    if ( !ChrList.lst[attacker].isplayer && pchr->isplayer ) actual_damage *= 0.75f;
                }

                if ( HAS_NO_BITS( effects, DAMFX_NBLOC ) )
                {
                    // Only actual_damage if hitting from proper direction
                    if ( Md2FrameList[pchr->inst.frame_nxt].framefx & MADFX_INVICTUS )
                    {
                        // I Frame...
                        direction -= chr_get_pcap(character)->iframefacing;
                        left  = 0xFFFF - chr_get_pcap(character)->iframeangle;
                        right = chr_get_pcap(character)->iframeangle;

                        // Check for shield
                        if ( pchr->action >= ACTION_PA && pchr->action <= ACTION_PD )
                        {
                            // Using a shield?
                            if ( pchr->action < ACTION_PC )
                            {
                                // Check left hand
                                if ( pchr->holdingwhich[SLOT_LEFT] != MAX_CHR )
                                {
                                    left  = 0xFFFF - chr_get_pcap(pchr->holdingwhich[SLOT_LEFT])->iframeangle;
                                    right = chr_get_pcap(pchr->holdingwhich[SLOT_LEFT])->iframeangle;
                                }
                            }
                            else
                            {
                                // Check right hand
                                if ( pchr->holdingwhich[SLOT_RIGHT] != MAX_CHR )
                                {
                                    left  = 0xFFFF - chr_get_pcap(pchr->holdingwhich[SLOT_RIGHT])->iframeangle;
                                    right = chr_get_pcap(pchr->holdingwhich[SLOT_RIGHT])->iframeangle;
                                }
                            }
                        }
                    }
                    else
                    {
                        // N Frame
                        direction -= chr_get_pcap(character)->nframefacing;
                        left = 0xFFFF - chr_get_pcap(character)->nframeangle;
                        right = chr_get_pcap(character)->nframeangle;
                    }

                    // Check that direction
                    if ( direction > left || direction < right )
                    {
                        actual_damage = 0;
                    }
                }

                if ( actual_damage != 0 )
                {
                    if ( effects&DAMFX_ARMO )
                    {
                        pchr->life -= actual_damage;
                    }
                    else
                    {
                        pchr->life -= FF_MUL( actual_damage, pchr->defense );
                    }
                    if ( base_damage > HURTDAMAGE )
                    {
                        // Spawn blud particles
                        if ( chr_get_pcap(character)->bludvalid && ( damagetype < DAMAGE_HOLY || chr_get_pcap(character)->bludvalid == ULTRABLUDY ) )
                        {
                            spawn_one_particle( pchr->pos.x, pchr->pos.y, pchr->pos.z,
                                                pchr->turn_z + direction, pchr->iprofile, chr_get_pcap(character)->bludprttype,
                                                MAX_CHR, GRIP_LAST, pchr->team, character, 0, MAX_CHR );
                        }

                        // Set attack alert if it wasn't an accident
                        if ( team == TEAM_DAMAGE )
                        {
                            pchr->ai.attacklast = MAX_CHR;
                        }
                        else
                        {
                            // Don't alert the character too much if under constant fire
                            if ( pchr->carefultime == 0 )
                            {
                                // Don't let characters chase themselves...  That would be silly
                                if ( attacker != character )
                                {
                                    pchr->ai.alert |= ALERTIF_ATTACKED;
                                    pchr->ai.attacklast = attacker;
                                    pchr->carefultime = CAREFULTIME;
                                }
                            }
                        }
                    }

                    // Taking actual_damage action
                    action = ACTION_HA;
                    if ( pchr->life < 0 )
                    {
                        Uint16 iTmp = pchr->firstenchant;

                        // Character has died
                        pchr->alive = bfalse;
                        while ( iTmp != MAX_ENC )
                        {
                            Uint16 sTmp = EncList.lst[iTmp].nextenchant;
                            if ( !enc_get_peve(iTmp)->stayifdead  )
                            {
                                remove_enchant( iTmp );
                            }
                            iTmp = sTmp;
                        }
                        pchr->waskilled = btrue;
                        pchr->keepaction = btrue;
                        pchr->life = -1;
                        pchr->platform = btrue;
                        pchr->bumpdampen = pchr->bumpdampen / 2.0f;
                        action = ACTION_KA;

                        // Give kill experience
                        experience = chr_get_pcap(character)->experienceworth + ( pchr->experience * chr_get_pcap(character)->experienceexchange );
                        if ( VALID_CHR(attacker) )
                        {
                            // Set target
                            pchr->ai.target = attacker;
                            if ( team == TEAM_DAMAGE )  pchr->ai.target = character;
                            if ( team == TEAM_NULL )  pchr->ai.target = character;

                            // Award direct kill experience
                            if ( team_hates_team(attacker,pchr->team) )
                            {
                                give_experience( attacker, experience, XP_KILLENEMY, bfalse );
                            }

                            // Check for hated
                            if ( chr_get_idsz(attacker,IDSZ_HATE) == chr_get_idsz(character,IDSZ_PARENT) ||
                                 chr_get_idsz(attacker,IDSZ_HATE) == chr_get_idsz(character,IDSZ_TYPE) )
                            {
                                give_experience( attacker, experience, XP_KILLHATED, bfalse );
                            }
                        }

                        // Clear all shop passages that it owned...
                        for (tnc = 0; tnc < ShopStack.count; tnc++ )
                        {
                            if ( ShopStack.lst[tnc].owner != character ) continue;
                            ShopStack.lst[tnc].owner = NOOWNER;
                        }

                        //Set various alerts to let others know it has died
                        for ( tnc = 0; tnc < MAX_CHR; tnc++ )
                        {
                            chr_t * plistener;

                            if ( !ChrList.lst[tnc].on ) continue;
                            plistener = ChrList.lst + tnc;

                            if( !plistener->alive ) continue;

                            // Let the other characters know it died
                            if ( plistener->ai.target == character )
                            {
                                plistener->ai.alert |= ALERTIF_TARGETKILLED;
                            }

                            // All allies get team experience, but only if they also hate the dead guy's team
                            if ( !team_hates_team(tnc,team) && ( team_hates_team(tnc,pchr->team) ) )
                            {
                                give_experience( tnc, experience, XP_TEAMKILL, bfalse );
                            }

                            // Check if it was a leader
                            if ( TeamList[pchr->team].leader == character && chr_get_iteam(tnc) == pchr->team )
                            {
                                // All folks on the leaders team get the alert
                                plistener->ai.alert |= ALERTIF_LEADERKILLED;
                            }
                        }

                        // The team now has no leader if the character is the leader
                        if ( TeamList[pchr->team].leader == character )
                        {
                            TeamList[pchr->team].leader = NOLEADER;
                        }

                        detach_character_from_mount( character, btrue, bfalse );

                        // Play the death animation
                        action += generate_randmask( 0, 3 );
                        chr_play_action( character, action, bfalse );

                        // If it's a player, let it die properly before enabling respawn
                        if (pchr->isplayer) revivetimer = ONESECOND; // 1 second

                        // Afford it one last thought if it's an AI
                        TeamList[pchr->baseteam].morale--;
                        pchr->team = pchr->baseteam;
                        pchr->ai.alert |= ALERTIF_KILLED;
                        pchr->sparkle = NOSPARKLE;
                        pchr->ai.timer = update_wld + 1;  // No timeout...
                        looped_stop_object_sounds( character );          // Stop sound loops

                        let_character_think( character );
                    }
                    else
                    {
                        if ( base_damage > HURTDAMAGE )
                        {
                            action += generate_randmask( 0, 3 );
                            chr_play_action( character, action, bfalse );

                            // Make the character invincible for a limited time only
                            if ( !( effects & DAMFX_TIME ) )
                                pchr->damagetime = DAMAGETIME;
                        }
                    }
                }
                else
                {
                    // Spawn a defend particle
                    spawn_one_particle( pchr->pos.x, pchr->pos.y, pchr->pos.z, pchr->turn_z, MAX_PROFILE, DEFEND, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, 0, MAX_CHR );
                    pchr->damagetime    = DEFENDTIME;
                    pchr->ai.alert     |= ALERTIF_BLOCKED;
                    pchr->ai.attacklast = attacker;     // For the ones attacking a shield
                }
            }
        }
        else if ( actual_damage < 0 )
        {
            // Heal 'em
            heal_character( character, attacker, actual_damage, ignoreinvincible);

            // Isssue an alert
            if ( team != TEAM_DAMAGE )
            {
                pchr->ai.attacklast = MAX_CHR;
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
    pchr = ChrList.lst + character;

    if ( pchr->alive )
    {
        IPair tmp_damage = {512,1};

        pchr->damagetime = 0;
        pchr->life = 1;
        modifier = pchr->damagemodifier[DAMAGE_CRUSH];
        pchr->damagemodifier[DAMAGE_CRUSH] = 1;

        if ( VALID_CHR( killer ) )
        {
            damage_character( character, ATK_FRONT, tmp_damage, DAMAGE_CRUSH, chr_get_iteam(killer), killer, DAMFX_ARMO | DAMFX_NBLOC, btrue );
        }
        else
        {
            damage_character( character, ATK_FRONT, tmp_damage, DAMAGE_CRUSH, TEAM_DAMAGE, pchr->ai.bumplast, DAMFX_ARMO | DAMFX_NBLOC, btrue );
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

    chr_t * pchr;
    cap_t * pcap;

    if ( INVALID_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    pcap = pro_get_pcap( profile );
    if( NULL == pcap ) return;

    origin = pchr->ai.owner;
    sTmp = pchr->turn_z;
    for ( iTmp = 0; iTmp < pcap->gopoofprtamount; iTmp++ )
    {
        spawn_one_particle( pchr->pos_old.x, pchr->pos_old.y, pchr->pos_old.z,
                            sTmp, profile, pcap->gopoofprttype,
                            MAX_CHR, GRIP_LAST, pchr->team, origin, iTmp, MAX_CHR );

        sTmp += pcap->gopoofprtfacingadd;
    }
}

//--------------------------------------------------------------------------------------------
char * chop_create( Uint16 profile )
{
    // ZZ> This function generates a random name
    int read, write, section, mychop;
    char cTmp;

    static char buffer[MAXCAPNAMESIZE];// The name returned by the function

    if ( 0 == ProList.lst[profile].chop_sectionsize[0] )
    {
        strncpy(buffer, CapList[profile].classname, SDL_arraysize(buffer) );
    }
    else
    {
        write = 0;

        for ( section = 0; section < MAXSECTION; section++ )
        {
            if ( 0 != ProList.lst[profile].chop_sectionsize[section] )
            {
                int irand = RANDIE;

                mychop = ProList.lst[profile].chop_sectionstart[section] + ( irand % ProList.lst[profile].chop_sectionsize[section] );

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
void init_ai_state( ai_state_t * pself, Uint16 index, Uint16 icap, Uint16 iobj, Uint16 rank )
{
    int tnc;
    if ( NULL == pself || index >= MAX_CHR ) return;

    // clear out everything
    memset( pself, 0, sizeof(ai_state_t) );

    pself->index      = index;
    pself->type       = ProList.lst[iobj].iai;
    pself->alert      = ALERTIF_SPAWNED;
    pself->state      = CapList[icap].stateoverride;
    pself->content    = CapList[icap].contentoverride;
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
    pself->searchlast = MAX_CHR;

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
void chr_init( chr_t * pchr )
{
    // BB> initialize the character data to safe values
    //     since we use memset(..., 0, ...), all = 0, = false, and = 0.0f
    //     statements are redundant

    int cnt;

    if( NULL == pchr ) return;

    // clear out all data
    memset(pchr, 0, sizeof(chr_t));

    // IMPORTANT!!!
    pchr->ibillboard = INVALID_BILLBOARD;
    /* pchr->isequipped = bfalse; */
    pchr->sparkle = NOSPARKLE;
    /* pchr->overlay = bfalse; */
    pchr->loopedsound_channel = INVALID_SOUND;

    // Set up model stuff
    /* pchr->reloadtime = 0; */
    pchr->inwhich_slot = SLOT_LEFT;
    /* pchr->waskilled = bfalse; */
    /* pchr->inwater = bfalse; */
    pchr->hitready = btrue;
    pchr->boretime = BORETIME;
    pchr->carefultime = CAREFULTIME;
    /* pchr->canbecrushed = bfalse; */
    /* pchr->damageboost = 0; */

    // Enchant stuff
    pchr->firstenchant = MAX_ENC;
    pchr->undoenchant = MAX_ENC;
    /* pchr->canchannel = bfalse; */
    pchr->missiletreatment = MISNORMAL;
    /* pchr->missilecost = 0; */

    // Player stuff
    /* pchr->isplayer = bfalse; */
    /* pchr->islocalplayer = bfalse; */

    // latches
    /* pchr->latchx = 0; */
    /* pchr->latchy = 0; */
    /* pchr->latchbutton = 0; */

    pchr->turnmode = TURNMODE_VELOCITY;

    // Life and Mana
    pchr->alive = btrue;

    // Jumping
    /* pchr->jumpnumber = 0; */
    pchr->jumptime = JUMPDELAY;

    // Grip info
    pchr->attachedto = MAX_CHR;
    for (cnt = 0; cnt < SLOT_COUNT; cnt++)
    {
        pchr->holdingwhich[cnt] = MAX_CHR;
    }

    // pack/inventory info
    /* pchr->pack_ispacked = bfalse; */
    pchr->pack_next = MAX_CHR;
    /* pchr->pack_count = 0; */
    for (cnt = 0; cnt < INVEN_COUNT; cnt++)
    {
        pchr->inventory[cnt] = MAX_CHR;
    }

    // Set up position
    /* pchr->vel.x = 0; */
    /* pchr->vel.y = 0; */
    /* pchr->vel.z = 0; */

    /* pchr->vel_old = pchr->vel; */

    pchr->map_turn_y = 32768;  // These two mean on level surface
    pchr->map_turn_x = 32768;

    // action stuff
    pchr->actionready = btrue;
    /* pchr->keepaction = bfalse; */
    /* pchr->loopaction = bfalse; */
    pchr->action = ACTION_DA;
    pchr->nextaction = ACTION_DA;

    /* pchr->holdingweight = 0; */
    pchr->onwhichplatform = MAX_CHR;

    // Timers set to 0
    /* pchr->grogtime = 0; */
    /* pchr->dazetime = 0; */
}

//--------------------------------------------------------------------------------------------
Uint16 chr_get_free( Uint16 override )
{
    int    tnc;
    Uint16 ichr = MAX_CHR;

    ichr = MAX_CHR;
    if ( VALID_CHR_RANGE(override) )
    {
        ichr = ChrList_get_free();
        if ( ichr != override )
        {
            // Picked the wrong one, so put this one back and find the right one

            for ( tnc = 0; tnc < MAX_CHR; tnc++ )
            {
                if ( ChrList.free_ref[tnc] == override )
                {
                    ChrList.free_ref[tnc] = ichr;
                    break;
                }
            }

            ichr = override;
        }

        if ( MAX_CHR == ichr )
        {
            log_warning( "chr_get_free() - failed to override a character? character %d already spawned? \n", override );
        }
    }
    else
    {
        ichr = ChrList_get_free();
        if ( MAX_CHR == ichr )
        {
            log_warning( "chr_get_free() - failed to allocate a new character\n" );
        }
    }

    return ichr;
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_character( GLvector3 pos, Uint16 profile, Uint8 team,
                            Uint8 skin, Uint16 facing, const char *name, Uint16 override )
{
    // ZZ> This function spawns a character and returns the character's index number
    //    if it worked, MAX_CHR otherwise

    Uint16 ichr, kursechance;
    int cnt, tnc;
    chr_t * pchr;
    cap_t * pcap;
    float nrm[2];
    GLvector3 pos_tmp;

    Uint16 icap;

    if ( profile >= MAX_PROFILE )
    {
        log_warning( "spawn_one_character() - profile value too large %d out of %d\n", profile, MAX_PROFILE );
        return MAX_CHR;
    }

    if ( INVALID_PRO(profile) )
    {
        if ( profile > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "spawn_one_character() - trying to spawn using invalid profile %d\n", profile );
        }
        return MAX_CHR;
    }

    // allocate a new character
    ichr = chr_get_free( override );
    if ( !VALID_CHR_RANGE(ichr) )
    {
        log_warning( "spawn_one_character() - failed to spawn character (invalid index number %d?)\n", ichr );
        return MAX_CHR;
    }

    pchr = ChrList.lst + ichr;

    // can't use chr_get_pcap() because pchr is not a valid character yet
    icap = pro_get_icap( profile );
    pcap = pro_get_pcap( profile );
    assert(NULL != pcap);

    // make a copy of the data in pos
    pos_tmp = pos;

    // set default values
    chr_init( pchr );

    // turn the character on here. you can't fail to spawn after this point.
    pchr->on = btrue;

    // Make sure the team is valid
    team = MIN( team, TEAM_MAX - 1 );

    // IMPORTANT!!!
    pchr->missilehandler = ichr;

    // sound stuff...  copy from the cap
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pchr->soundindex[tnc] = pcap->soundindex[tnc];
    }

    // Set up model stuff
    pchr->iprofile = profile;
    pchr->basemodel = profile;
    pchr->stoppedby = pcap->stoppedby;
    pchr->lifeheal = pcap->lifeheal;
    pchr->manacost = pcap->manacost;
    pchr->nameknown = pcap->nameknown;
    pchr->ammoknown = pcap->nameknown;
    pchr->icon = pcap->icon;

    // Enchant stuff
    pchr->canseeinvisible = pcap->canseeinvisible;

    // Skillz
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
    if ( pcap->isitem )
    {
        IPair loc_rand = {0,100};

        kursechance = pcap->kursechance;
        if ( cfg.difficulty >= GAME_HARD )                        kursechance *= 2.0f;  // Hard mode doubles chance for Kurses
        if ( cfg.difficulty < GAME_NORMAL && kursechance != 100 ) kursechance *= 0.5f;  // Easy mode halves chance for Kurses
        pchr->iskursed = ( generate_number(loc_rand) <= kursechance );
    }

    // Ammo
    pchr->ammomax = pcap->ammomax;
    pchr->ammo = pcap->ammo;

    // Gender
    pchr->gender = pcap->gender;
    if ( pchr->gender == GENDER_RANDOM )  pchr->gender = generate_randmask( GENDER_FEMALE, 1 );

    // AI stuff
    init_ai_state( &(pchr->ai), ichr, icap, pchr->iprofile, TeamList[team].morale );

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
        skin = pcap->skinoverride % MAX_SKIN;
    }
    if ( skin >= ProList.lst[profile].skins )
    {
        skin = 0;
        if ( ProList.lst[profile].skins > 1 )
        {
            int irand = RANDIE;
            skin = irand % ProList.lst[profile].skins;
        }
    }

    pchr->skin    = skin;

    // Life and Mana
    pchr->lifecolor = pcap->lifecolor;
    pchr->manacolor = pcap->manacolor;
    pchr->lifemax = generate_number( pcap->life_stat.val );
    pchr->lifereturn = pcap->lifereturn;
    pchr->manamax = generate_number( pcap->mana_stat.val );
    pchr->manaflow = generate_number( pcap->manaflow_stat.val );
    pchr->manareturn = generate_number( pcap->manareturn_stat.val );

    // Load current life and mana or refill them (based on difficulty)
    if ( cfg.difficulty >= GAME_NORMAL ) pchr->life = CLIP( pcap->spawnlife, LOWSTAT, pchr->lifemax );
    else pchr->life = pchr->lifemax;
    if ( cfg.difficulty >= GAME_NORMAL ) pchr->mana = CLIP( pcap->spawnmana, 0, pchr->manamax );
    else pchr->mana = pchr->manamax;

    // SWID
    pchr->strength = generate_number( pcap->strength_stat.val );
    pchr->wisdom = generate_number( pcap->wisdom_stat.val );
    pchr->intelligence = generate_number( pcap->intelligence_stat.val );
    pchr->dexterity = generate_number( pcap->dexterity_stat.val );

    // Damage
    pchr->defense = pcap->defense[skin];
    pchr->reaffirmdamagetype = pcap->attachedprt_reaffirmdamagetype;
    pchr->damagetargettype = pcap->damagetargettype;
    for ( tnc = 0; tnc < DAMAGE_COUNT; tnc++ )
    {
        pchr->damagemodifier[tnc] = pcap->damagemodifier[tnc][skin];
    }

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
    pchr->jumpnumberreset = pcap->jumpnumber;

    // Other junk
    pchr->flyheight = pcap->flyheight;
    pchr->maxaccel  = pcap->maxaccel[skin];
    pchr->basealpha = pcap->alpha;
    pchr->baselight = pcap->light;
    pchr->flashand  = pcap->flashand;
    pchr->dampen    = pcap->dampen;

    // Character size and bumping
    chr_init_size( pchr, pcap );
    pchr->fat_goto      = pchr->fat;
    pchr->fat_goto_time = 0;

    pchr->bumpdampen = pcap->bumpdampen;
    if ( pcap->weight == 0xFF )
    {
        pchr->weight = 0xFFFFFFFF;
    }
    else
    {
        Uint32 itmp = pcap->weight * pchr->fat * pchr->fat * pchr->fat;
        pchr->weight = MIN( itmp, (Uint32)0xFFFFFFFE );
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

    pchr->pos      = pos_tmp;
    pchr->pos_safe = pchr->pos;
    pchr->pos_stt  = pchr->pos;
    pchr->pos_old  = pchr->pos;

    pchr->turn_z     = facing;
    pchr->turn_old_z = pchr->turn_z;

    pchr->phys.level    = pchr->floor_level;
    pchr->onwhichfan    = mesh_get_tile( PMesh, pchr->pos.x, pchr->pos.y );
    pchr->onwhichblock  = mesh_get_block( PMesh, pchr->pos.x, pchr->pos.y );

    // Money is added later
    pchr->money = pcap->money;

    // Name the character
    if ( name == NULL )
    {
        // Generate a random name
        snprintf( pchr->name, SDL_arraysize( pchr->name), "%s", chop_create( profile ) );
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
    for ( tnc = 0; tnc < pcap->attachedprt_amount; tnc++ )
    {
        spawn_one_particle( pchr->pos.x, pchr->pos.y, pchr->pos.z,
                            0, pchr->iprofile, pcap->attachedprt_pip,
                            ichr, GRIP_LAST + tnc, pchr->team, ichr, tnc, MAX_CHR );
    }
    pchr->reaffirmdamagetype = pcap->attachedprt_reaffirmdamagetype;

    // Experience
    pchr->experience = MIN(generate_number( pcap->experience ), MAXXP);
    pchr->experiencelevel = pcap->leveloverride;

    // Items that are spawned inside shop passages are more expensive than normal
    pchr->isshopitem = bfalse;
    if ( pchr->isitem && ShopStack.count > 0 && !pchr->pack_ispacked && pchr->attachedto == MAX_CHR )
    {
        for ( cnt = 0; cnt < ShopStack.count; cnt++ )
        {
            // Make sure the owner is not dead
            if ( ShopStack.lst[cnt].owner == NOOWNER ) continue;

            if ( object_is_in_passage( ShopStack.lst[cnt].passage, pchr->pos.x, pchr->pos.y, pchr->bump.size) )
            {
                pchr->isshopitem = btrue;               // Full value
                pchr->iskursed   = bfalse;              // Shop items are never kursed
                pchr->nameknown  = btrue;               // Identify items in shop
                break;
            }
        }
    }

    // Flagged as always valuable?
    if ( pcap->isvaluable ) pchr->isshopitem = btrue;

    if ( 0 == __chrhitawall(ichr, nrm) )
    {
        pchr->safe_valid = btrue;
    };

    return ichr;
}

//--------------------------------------------------------------------------------------------
void respawn_character( Uint16 character )
{
    // ZZ> This function respawns a character
    Uint16 item;

    chr_t * pchr;
    cap_t * pcap;

    if ( INVALID_CHR( character ) || ChrList.lst[character].alive ) return;
    pchr = ChrList.lst + character;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return;

    spawn_poof( character, pchr->iprofile );
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

    // reset all of the bump size information
    {
        float old_fat = pchr->fat;
        chr_init_size( pchr, pcap );
        chr_set_fat( pchr, old_fat );
    }

    pchr->platform     = pcap->platform;
    pchr->flyheight    = pcap->flyheight;
    pchr->bumpdampen   = pcap->bumpdampen;

    pchr->ai.alert = ALERTIF_CLEANEDUP;
    pchr->ai.target = character;
    pchr->ai.timer  = 0;

    pchr->grogtime = 0;
    pchr->dazetime = 0;
    reaffirm_attached_particles( character );

    // Let worn items come back
    for ( item = pchr->pack_next; item < MAX_CHR; item = ChrList.lst[item].pack_next )
    {
        if ( ChrList.lst[item].on && ChrList.lst[item].isequipped )
        {
            ChrList.lst[item].isequipped = bfalse;
            chr_get_pai(item)->alert |= ALERTIF_ATLASTWAYPOINT;  // doubles as PutAway
        }
    }

    // re-initialize the instance
    chr_instance_init( &(pchr->inst), pchr->iprofile, pchr->skin );
}

//--------------------------------------------------------------------------------------------
int chr_change_skin( Uint16 character, int skin )
{
    Uint16 imad;

    chr_t * pchr;
    pro_t * pobj;
    chr_instance_t * pinst;

    if ( INVALID_CHR(character) ) return 0;
    pchr  = ChrList.lst + character;
    pinst = &(pchr->inst);

    pobj = chr_get_pobj(character);

    imad = chr_get_imad(character);
    if( INVALID_MAD(imad) )
    {
        pchr->skin     = 0;
        pinst->texture = TX_WATER_TOP;
    }
    else
    {
        mad_t * pmad;

        // make sure that the instance has a valid imad
        if ( INVALID_MAD(pinst->imad) )
        {
            pinst->imad = imad;
        }
        pmad = MadList + pinst->imad;

        // do the best we can to change the skin
        if ( 0 == pobj->skins )
        {
            pobj->skins = 1;
            pobj->tex_ref[0] = TX_WATER_TOP;
        }

        // limit the skin
        if ( skin > pobj->skins) skin = 0;

        pchr->skin     = skin;
        pinst->texture = pobj->tex_ref[skin];
    }

    return pchr->skin;
};

//--------------------------------------------------------------------------------------------
Uint16 change_armor( Uint16 character, Uint16 skin )
{
    // ZZ> This function changes the armor of the character

    Uint16  enchant;
    int     iTmp;
    cap_t * pcap;

    if ( INVALID_CHR(character) ) return 0;

    // Remove armor enchantments
    enchant = ChrList.lst[character].firstenchant;
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
        enchant = EncList.lst[enchant].nextenchant;
    }

    // Change the skin
    pcap = chr_get_pcap(character);
    skin = chr_change_skin( character, skin );

    // Change stats associated with skin
    ChrList.lst[character].defense = pcap->defense[skin];
    iTmp = 0;

    while ( iTmp < DAMAGE_COUNT )
    {
        ChrList.lst[character].damagemodifier[iTmp] = pcap->damagemodifier[iTmp][skin];
        iTmp++;
    }

    ChrList.lst[character].maxaccel = pcap->maxaccel[skin];

    // Reset armor enchantments
    // These should really be done in reverse order ( Start with last enchant ), but
    // I don't care at this point !!!BAD!!!
    enchant = ChrList.lst[character].firstenchant;
    while ( enchant < MAX_ENC )
    {
        Uint16 ieve = enc_get_ieve(enchant);

        if( VALID_EVE(ieve) )
        {
            set_enchant_value( enchant, SETSLASHMODIFIER, ieve );
            set_enchant_value( enchant, SETCRUSHMODIFIER, ieve );
            set_enchant_value( enchant, SETPOKEMODIFIER,  ieve );
            set_enchant_value( enchant, SETHOLYMODIFIER,  ieve );
            set_enchant_value( enchant, SETEVILMODIFIER,  ieve );
            set_enchant_value( enchant, SETFIREMODIFIER,  ieve );
            set_enchant_value( enchant, SETICEMODIFIER,   ieve );
            set_enchant_value( enchant, SETZAPMODIFIER,   ieve );

            add_enchant_value( enchant, ADDACCEL,         ieve );
            add_enchant_value( enchant, ADDDEFENSE,       ieve );
        }

        enchant = EncList.lst[enchant].nextenchant;
    }

    return skin;
}

//--------------------------------------------------------------------------------------------
void change_character_full( Uint16 ichr, Uint16 profile, Uint8 skin, Uint8 leavewhich )
{
    // ZF> This function polymorphs a character permanently so that it can be exported properly
    // A character turned into a frog with this function will also export as a frog!

    Uint16 imad_old, imad_new;

    if ( INVALID_PRO(profile) ) return;

    imad_new = pro_get_imad( profile );
    if( INVALID_MAD(imad_new) ) return;

    imad_old = chr_get_imad( ichr );
    if( INVALID_MAD(imad_old) ) return;

    // copy the new name
    strncpy(MadList[imad_old].name, MadList[imad_new].name, SDL_arraysize(MadList[imad_old].name));

    // change ther model
    change_character( ichr, profile, skin, leavewhich );

    // set the base model to the new model, too
    ChrList.lst[ichr].basemodel = profile;
}

//--------------------------------------------------------------------------------------------
void set_weapongrip( Uint16 iitem, Uint16 iholder, Uint16 vrt_off )
{
    int i, tnc;
    chr_t * pitem, *pholder;
    mad_t * pholder_mad;

    if ( INVALID_CHR(iitem) ) return;
    pitem = ChrList.lst + iitem;

    // reset the vertices
    for (i = 0; i < GRIP_VERTS; i++)
    {
        pitem->weapongrip[i] = 0xFFFF;
    }

    if ( INVALID_CHR(iholder) ) return;
    pholder = ChrList.lst + iholder;

    pholder_mad = chr_get_pmad(iholder);
    if ( NULL == pholder_mad ) return;

    tnc = pholder_mad->md2_data.vertices - vrt_off;
    for (i = 0; i < GRIP_VERTS; i++)
    {
        if (tnc + i < pholder_mad->md2_data.vertices )
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

    pro_t * pobj;
    cap_t * pcap;
    mad_t * pmad;

    if ( INVALID_MAD(profile) || INVALID_CHR(ichr) ) return;
    pchr = ChrList.lst + ichr;

    if( INVALID_PRO(profile) ) return;
    pobj = ProList.lst + profile;

    pcap = pro_get_pcap( profile );
    pmad = pro_get_pmad( profile );

    // Drop left weapon
    sTmp = pchr->holdingwhich[SLOT_LEFT];
    if ( VALID_CHR(sTmp) && ( !pcap->slotvalid[SLOT_LEFT] || pcap->ismount ) )
    {
        detach_character_from_mount( sTmp, btrue, btrue );
        if ( pchr->ismount )
        {
            ChrList.lst[sTmp].vel.z = DISMOUNTZVEL;
            ChrList.lst[sTmp].pos.z += DISMOUNTZVEL;
            ChrList.lst[sTmp].jumptime = JUMPDELAY;
        }
    }

    // Drop right weapon
    sTmp = pchr->holdingwhich[SLOT_RIGHT];
    if ( VALID_CHR(sTmp) && !pcap->slotvalid[SLOT_RIGHT] )
    {
        detach_character_from_mount( sTmp, btrue, btrue );
        if ( pchr->ismount )
        {
            ChrList.lst[sTmp].vel.z = DISMOUNTZVEL;
            ChrList.lst[sTmp].pos.z += DISMOUNTZVEL;
            ChrList.lst[sTmp].jumptime = JUMPDELAY;
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
            while ( EncList.lst[enchant].nextenchant != MAX_ENC )
            {
                remove_enchant( EncList.lst[enchant].nextenchant );
            }
        }
    }

    if ( leavewhich == LEAVENONE )
    {
        // Remove all enchantments
        disenchant_character( ichr );
    }

    // Stuff that must be set
    pchr->iprofile  = profile;
    pchr->stoppedby = pcap->stoppedby;
    pchr->lifeheal  = pcap->lifeheal;
    pchr->manacost  = pcap->manacost;

    // Ammo
    pchr->ammomax = pcap->ammomax;
    pchr->ammo = pcap->ammo;

    // Gender
    if ( pcap->gender != GENDER_RANDOM )  // GENDER_RANDOM means keep old gender
    {
        pchr->gender = pcap->gender;
    }

    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pchr->soundindex[tnc] = pcap->soundindex[tnc];
    }

    // AI stuff
    pchr->ai.type = pobj->iai;
    pchr->ai.state = 0;
    pchr->ai.timer = 0;

    pchr->latchx = 0;
    pchr->latchy = 0;
    pchr->latchbutton = 0;
    pchr->turnmode = TURNMODE_VELOCITY;

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

    // set the character size so that the new model is the same size as the old model
    // the model will then morph its size to the correct size over time
    {
        float old_fat = pchr->fat;
        float new_fat;

        if( 0.0f == pchr->bump.size )
        {
            new_fat = pcap->size;
        }
        else
        {
            new_fat = (pcap->bumpsize * pcap->size) / pchr->bump.size;
        }

        // copy all the cap size info over, as normal
        chr_init_size( pchr, pcap );

        // make the model's size congruent
        if( 0.0f != new_fat && new_fat != old_fat )
        {
            chr_set_fat( pchr, new_fat );
            pchr->fat_goto      = old_fat;
            pchr->fat_goto_time = SIZETIME;
        }
        else
        {
            chr_set_fat( pchr, old_fat );
            pchr->fat_goto      = old_fat;
            pchr->fat_goto_time = 0;
        }
    }

    pchr->bumpdampen = pcap->bumpdampen;

    if ( pcap->weight == 0xFF )
    {
        pchr->weight = 0xFFFFFFFF;
    }
    else
    {
        Uint32 itmp = pcap->weight * pchr->fat * pchr->fat * pchr->fat;
        pchr->weight = MIN( itmp, (Uint32)0xFFFFFFFE );
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
    pchr->actionready = bfalse;
    pchr->keepaction = bfalse;
    pchr->action = ACTION_DA;
    pchr->nextaction = ACTION_DA;
    pchr->loopaction = bfalse;
    pchr->holdingweight = 0;
    pchr->onwhichplatform = MAX_CHR;

    // initialize the instance
    chr_instance_init( &(pchr->inst), profile, skin );

    // Set the skin
    change_armor( ichr, skin );

    // Reaffirm them particles...
    pchr->reaffirmdamagetype = pcap->attachedprt_reaffirmdamagetype;
    reaffirm_attached_particles( ichr );
}

//--------------------------------------------------------------------------------------------
bool_t cost_mana( Uint16 character, int amount, Uint16 killer )
{
    // ZZ> This function takes mana from a character ( or gives mana ),
    //    and returns btrue if the character had enough to pay, or bfalse
    //    otherwise. This can kill a character in hard mode.

    int mana_final;
    bool_t mana_paid;

    chr_t * pchr;

    if ( INVALID_CHR(character) ) return bfalse;
    pchr = ChrList.lst + character;

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
            // use some factor, divide by 2
            heal_character( pchr->ai.index, killer, mana_surplus << 1, btrue);
        }

        mana_paid = btrue;

    }

    return mana_paid;
}

//--------------------------------------------------------------------------------------------
void switch_team( Uint16 character, Uint8 team )
{
    // ZZ> This function makes a character join another team...

    if ( INVALID_CHR(character) || team >= TEAM_MAX ) return;

    if ( !ChrList.lst[character].invictus )
    {
        chr_get_pteam_base(character)->morale--;
        TeamList[team].morale++;
    }
    if ( ( !ChrList.lst[character].ismount || ChrList.lst[character].holdingwhich[SLOT_LEFT] == MAX_CHR ) &&
            ( !ChrList.lst[character].isitem || ChrList.lst[character].attachedto == MAX_CHR ) )
    {
        ChrList.lst[character].team = team;
    }

    ChrList.lst[character].baseteam = team;
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

    team = chr_get_iteam(character);
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        chr_t * pchr;

        if ( !ChrList.lst[cnt].on) continue;
        pchr = ChrList.lst + cnt;

        if(  team != chr_get_iteam(cnt) ) continue;

        if ( !pchr->alive )
        {
            pchr->ai.timer  = update_wld + 2;  // Don't let it think too much...
        }

        pchr->ai.alert |= ALERTIF_CLEANEDUP;
    }
}

//--------------------------------------------------------------------------------------------
int restock_ammo( Uint16 character, IDSZ idsz )
{
    // ZZ> This function restocks the characters ammo, if it needs ammo and if
    //    either its parent or type idsz match the given idsz.  This
    //    function returns the amount of ammo given.
    int amount;

    chr_t * pchr;

    if( INVALID_CHR(character) ) return 0;
    pchr = ChrList.lst + character;

    amount = 0;
    if ( chr_is_type_idsz(character, idsz) )
    {
        if ( ChrList.lst[character].ammo < ChrList.lst[character].ammomax )
        {
            amount = ChrList.lst[character].ammomax - ChrList.lst[character].ammo;
            ChrList.lst[character].ammo = ChrList.lst[character].ammomax;
        }
    }

    return amount;
}

//--------------------------------------------------------------------------------------------
int check_skills( Uint16 who, IDSZ whichskill )
{
    // @details ZF@> This checks if the specified character has the required skill. Returns the level
    // of the skill. Also checks Skill expansions.

    bool_t result = bfalse;

    // First check the character Skill ID matches
    // Then check for expansion skills too.
    if ( chr_get_idsz(who,IDSZ_SKILL)  == whichskill ) result = btrue;
    else if ( MAKE_IDSZ( 'A', 'W', 'E', 'P' ) == whichskill ) result = ChrList.lst[who].canuseadvancedweapons;
    else if ( MAKE_IDSZ( 'C', 'K', 'U', 'R' ) == whichskill ) result = ChrList.lst[who].canseekurse;
    else if ( MAKE_IDSZ( 'J', 'O', 'U', 'S' ) == whichskill ) result = ChrList.lst[who].canjoust;
    else if ( MAKE_IDSZ( 'S', 'H', 'P', 'R' ) == whichskill ) result = ChrList.lst[who].shieldproficiency;
    else if ( MAKE_IDSZ( 'T', 'E', 'C', 'H' ) == whichskill ) result = ChrList.lst[who].canusetech;
    else if ( MAKE_IDSZ( 'W', 'M', 'A', 'G' ) == whichskill ) result = ChrList.lst[who].canusearcane;
    else if ( MAKE_IDSZ( 'H', 'M', 'A', 'G' ) == whichskill ) result = ChrList.lst[who].canusedivine;
    else if ( MAKE_IDSZ( 'D', 'I', 'S', 'A' ) == whichskill ) result = ChrList.lst[who].candisarm;
    else if ( MAKE_IDSZ( 'S', 'T', 'A', 'B' ) == whichskill ) result = ChrList.lst[who].canbackstab;
    else if ( MAKE_IDSZ( 'P', 'O', 'I', 'S' ) == whichskill ) result = ChrList.lst[who].canusepoison;
    else if ( MAKE_IDSZ( 'R', 'E', 'A', 'D' ) == whichskill ) result = ChrList.lst[who].canread || ( ChrList.lst[who].canseeinvisible && ChrList.lst[who].canseekurse ); // Truesight allows reading

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_chr_environment
{
    float  air_friction, ice_friction;
    bool_t is_slippy,    is_watery;

    Uint8 twist;
    float level, zlerp;

    bool_t is_slipping;

    float fluid_friction_xy, fluid_friction_z;
    float traction, friction_xy;

    float new_vx, new_vy;
    GLvector3 acc;
};
typedef struct s_chr_environment chr_environment_t;

//--------------------------------------------------------------------------------------------
void move_characters_get_environment( chr_t * pchr, chr_environment_t * penviro )
{
    Uint32 itile;

    if( NULL == pchr || NULL == penviro ) return;

    //---- character "floor" level
    if ( 0 != pchr->flyheight )
    {
        penviro->level = pchr->phys.level;
        if ( penviro->level < 0 ) penviro->level = 0;  // fly above pits...
    }
    else
    {
        penviro->level = pchr->phys.level;
    }
    penviro->zlerp = (pchr->pos.z - penviro->level) / PLATTOLERANCE;
    penviro->zlerp = CLIP(penviro->zlerp, 0, 1);

    pchr->phys.grounded = (0 == pchr->flyheight) && (penviro->zlerp < 0.25f);

    //---- the "twist" of the floor
    penviro->twist = TWIST_FLAT;
    itile          = INVALID_TILE;
    if( VALID_CHR(pchr->onwhichplatform) )
    {
        // this only works for 1 level of attachment
        itile = ChrList.lst[pchr->onwhichplatform].onwhichfan;
    }
    else
    {
        itile = pchr->onwhichfan;
    }

    if( VALID_TILE(PMesh,itile) )
    {
        penviro->twist = PMesh->mmem.tile_list[itile].twist;
    }

    // the "watery-ness" of whatever water might be here
    penviro->is_watery = water.is_water && pchr->inwater;
    penviro->is_slippy = !penviro->is_watery && (0 != mesh_test_fx( PMesh, pchr->onwhichfan, MPDFX_SLIPPY ));

    //---- traction
    penviro->traction = 1.0f;
    if ( 0 != pchr->flyheight )
    {
        // any traction factor here
        /* traction = ??; */
    }
    else if ( VALID_CHR( pchr->onwhichplatform ) )
    {
        // in case the platform is tilted
        GLvector3 platform_up = mat_getChrUp( chr_get_pinstance(pchr->onwhichplatform)->matrix );
        platform_up = VNormalize(platform_up);

        penviro->traction = ABS(platform_up.z) * (1.0f - penviro->zlerp) + 0.25 * penviro->zlerp;

        if ( penviro->is_slippy )
        {
            penviro->traction /= hillslide * (1.0f - penviro->zlerp) + 1.0f * penviro->zlerp;
        }
    }
    else if ( VALID_TILE(PMesh, pchr->onwhichfan) )
    {
        penviro->traction = ABS(map_twist_nrm[penviro->twist].z) * (1.0f - penviro->zlerp) + 0.25 * penviro->zlerp;

        if ( penviro->is_slippy )
        {
            penviro->traction /= hillslide * (1.0f - penviro->zlerp) + 1.0f * penviro->zlerp;
        }
    }

    //---- the friction of the fluid we are in
    if ( penviro->is_watery )
    {
        penviro->fluid_friction_z  = waterfriction;
        penviro->fluid_friction_xy = waterfriction;
    }
    else
    {
        penviro->fluid_friction_xy = penviro->air_friction;       // like real-life air friction
        penviro->fluid_friction_z  = penviro->air_friction;
    }

    //---- friction
    penviro->friction_xy       = 1.0f;
    if ( 0 != pchr->flyheight )
    {
        if( pchr->platform )
        {
            // override the z friction for platforms.
            // friction in the z direction will make the bouncing stop
            penviro->fluid_friction_z = 1.0f;
        }
    }
    else
    {
        // Make the characters slide
        float temp_friction_xy = noslipfriction;
        if ( VALID_TILE(PMesh, pchr->onwhichfan) && penviro->is_slippy )
        {
            // It's slippy all right...
            temp_friction_xy = slippyfriction;
        }

        penviro->friction_xy = penviro->zlerp * 1.0f + (1.0f - penviro->zlerp) * temp_friction_xy;
    }

    //---- jump stuff
    if ( 0 != pchr->flyheight )
    {
        // Flying
        pchr->jumpready = bfalse;
    }
    else
    {
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

        // Special considerations for slippy surfaces
        if ( penviro->is_slippy )
        {
            if ( map_twist_flat[penviro->twist] )
            {
                // Reset jumping on flat areas of slippiness
                if ( pchr->phys.grounded && pchr->jumptime == 0 )
                {
                    pchr->jumpnumber = pchr->jumpnumberreset;
                }
            }
        }
        else if ( pchr->phys.grounded && pchr->jumptime == 0 )
        {
            // Reset jumping
            pchr->jumpnumber = pchr->jumpnumberreset;
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_characters_do_floor_friction( chr_t * pchr, chr_environment_t * penviro )
{
    // BB> Friction is complicated when you want to have sliding characters :P

    float temp_friction_xy;
    GLvector3 vup, floor_acc, fric, fric_floor;

    if( NULL == pchr || NULL == penviro ) return;

    if( 0 != pchr->flyheight ) return;

    // figure out the acceleration due to the current "floor"
    floor_acc.x = floor_acc.y = floor_acc.z = 0.0f;
    temp_friction_xy = 1.0f;
    if ( VALID_CHR(pchr->onwhichplatform) )
    {
        chr_t * pplat = ChrList.lst + pchr->onwhichplatform;

        temp_friction_xy = platstick;

        floor_acc.x = pplat->vel.x - pplat->vel_old.x;
        floor_acc.y = pplat->vel.y - pplat->vel_old.y;
        floor_acc.z = pplat->vel.z - pplat->vel_old.z;

        vup = mat_getChrUp( pplat->inst.matrix );
    }
    else if ( !pchr->alive || pchr->isitem )
    {
        temp_friction_xy = 0.5f;
        floor_acc.x = -pchr->vel.x;
        floor_acc.y = -pchr->vel.y;
        floor_acc.z = -pchr->vel.z;

        if( TWIST_FLAT == penviro->twist )
        {
            vup.x = vup.y = 0.0f;
            vup.z = 1.0f;
        }
        else
        {
            vup = map_twist_nrm[penviro->twist];
        }

    }
    else
    {
        temp_friction_xy = penviro->friction_xy;

        if( TWIST_FLAT == penviro->twist )
        {
            vup.x = vup.y = 0.0f;
            vup.z = 1.0f;
        }
        else
        {
            vup = map_twist_nrm[penviro->twist];
        }

        if( ABS(pchr->vel.x) + ABS(pchr->vel.y) + ABS(pchr->vel.z) > 0.0f )
        {
            float ftmp;
            GLvector3 vfront = mat_getChrForward( pchr->inst.matrix );

            floor_acc.x = -pchr->vel.x;
            floor_acc.y = -pchr->vel.y;
            floor_acc.z = -pchr->vel.z;

            //---- get the "bad" velocity (perpendicular to the direction of motion)
            vfront = VNormalize(vfront);
            ftmp = VDotProduct( floor_acc, vfront );

            floor_acc.x -= ftmp * vfront.x;
            floor_acc.y -= ftmp * vfront.y;
            floor_acc.z -= ftmp * vfront.z;
        }
    }

    // the first guess about the floor friction
    fric_floor.x = floor_acc.x * (1.0f - penviro->zlerp) * (1.0f-temp_friction_xy) * penviro->traction;
    fric_floor.y = floor_acc.y * (1.0f - penviro->zlerp) * (1.0f-temp_friction_xy) * penviro->traction;
    fric_floor.z = floor_acc.z * (1.0f - penviro->zlerp) * (1.0f-temp_friction_xy) * penviro->traction;

    // the total "friction" due to the floor
    fric.x = fric_floor.x + penviro->acc.x;
    fric.y = fric_floor.y + penviro->acc.y;
    fric.z = fric_floor.z + penviro->acc.z;

    //---- limit the friction to whatever is horizontal to the mesh
    if( TWIST_FLAT == penviro->twist )
    {
        floor_acc.z = 0.0f;
        fric.z      = 0.0f;
    }
    else
    {
        float ftmp;
        GLvector3 vup = map_twist_nrm[penviro->twist];

        ftmp = VDotProduct( floor_acc, vup );

        floor_acc.x -= ftmp * vup.x;
        floor_acc.y -= ftmp * vup.y;
        floor_acc.z -= ftmp * vup.z;

        ftmp = VDotProduct( fric, vup );

        fric.x -= ftmp * vup.x;
        fric.y -= ftmp * vup.y;
        fric.z -= ftmp * vup.z;
    }

    // test to see if the player has any more friction left?
    penviro->is_slipping = (ABS(fric.x) + ABS(fric.y) + ABS(fric.z) > penviro->friction_xy);

    if( penviro->is_slipping )
    {
        penviro->traction *= 0.5f;
        temp_friction_xy  = SQRT(temp_friction_xy);

        fric_floor.x = floor_acc.x * (1.0f - penviro->zlerp) * (1.0f-temp_friction_xy) * penviro->traction;
        fric_floor.y = floor_acc.y * (1.0f - penviro->zlerp) * (1.0f-temp_friction_xy) * penviro->traction;
        fric_floor.z = floor_acc.z * (1.0f - penviro->zlerp) * (1.0f-temp_friction_xy) * penviro->traction;
    }

    //apply the floor friction
    pchr->vel.x += fric_floor.x;
    pchr->vel.y += fric_floor.y;
    pchr->vel.z += fric_floor.z;

    // Apply fluid friction from last time
    pchr->vel.x += -pchr->vel.x * (1.0f - penviro->fluid_friction_xy);
    pchr->vel.y += -pchr->vel.y * (1.0f - penviro->fluid_friction_xy);
    pchr->vel.z += -pchr->vel.z * (1.0f - penviro->fluid_friction_z );
}

//--------------------------------------------------------------------------------------------
void move_characters_do_volontary( chr_t * pchr, chr_environment_t * penviro )
{
    float  dvx, dvy, dvmax;
    bool_t watchtarget;
    float maxspeed;
    float dv2;
    float new_ax, new_ay;

    if( NULL == pchr || NULL == penviro ) return;

    if( !pchr->alive ) return;

    // do volontary motion

    penviro->new_vx = pchr->vel.x;
    penviro->new_vy = pchr->vel.y;

    if ( VALID_CHR(pchr->attachedto) ) return;

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

    // this is the maximum speed that a character could go under the v2.22 system
    maxspeed = pchr->maxaccel * airfriction / (1.0f - airfriction);

    penviro->new_vx = penviro->new_vy = 0.0f;
    if( ABS(dvx) + ABS(dvy) > 0 )
    {
        dv2 = dvx*dvx + dvy*dvy;

        if( pchr->isplayer )
        {
            float speed;
            float dv = POW(dv2, 0.25f);

            if(maxspeed < pchr->runspd)
            {
                maxspeed = pchr->runspd;
                dv *= 0.75f;
            }

            if( dv >= 1.0f )
            {
                speed = maxspeed;
            }
            else if( dv >= 0.75f )
            {
                speed = (dv - 0.75f) / 0.25f * maxspeed + (1.0f - dv) / 0.25f * pchr->runspd;
            }
            else if( dv >= 0.50f )
            {
                speed = (dv - 0.50f) / 0.25f * pchr->runspd + (0.75f - dv) / 0.25f * pchr->walkspd;
            }
            else if ( dv >= 0.25f )
            {
                speed = (dv - 0.25f) / 0.25f * pchr->walkspd + (0.25f - dv) / 0.25f * pchr->sneakspd;
            }
            else
            {
                speed = dv / 0.25f * pchr->sneakspd;
            }

            penviro->new_vx = speed * dvx / dv;
            penviro->new_vy = speed * dvy / dv;
        }
        else
        {
            float scale = 1.0f;

            if( dv2 > 1.0f )
            {
                scale = 1.0f / POW(dv2, 0.5f);
            }
            else
            {
                scale = POW(dv2, 0.25f) / POW(dv2, 0.5f);
            }

            penviro->new_vx = dvx * maxspeed * scale;
            penviro->new_vy = dvy * maxspeed * scale;
        }
    }

    if ( VALID_CHR(pchr->onwhichplatform) )
    {
        chr_t * pplat = ChrList.lst + pchr->onwhichplatform;

        new_ax = (pplat->vel.x + penviro->new_vx - pchr->vel.x);
        new_ay = (pplat->vel.y + penviro->new_vy - pchr->vel.y);
    }
    else
    {
        new_ax = (penviro->new_vx - pchr->vel.x);
        new_ay = (penviro->new_vy - pchr->vel.y);
    }

    dvmax = pchr->maxaccel;
    if ( new_ax < -dvmax ) new_ax = -dvmax;
    if ( new_ax >  dvmax ) new_ax =  dvmax;
    if ( new_ay < -dvmax ) new_ay = -dvmax;
    if ( new_ay >  dvmax ) new_ay =  dvmax;

    //penviro->new_vx = new_ax * airfriction / (1.0f - airfriction);
    //penviro->new_vy = new_ay * airfriction / (1.0f - airfriction);

    new_ax *= penviro->traction;
    new_ay *= penviro->traction;

    // Get direction from the DESIRED change in velocity
    if ( pchr->turnmode == TURNMODE_WATCH )
    {
        if ( ( ABS( dvx ) > WATCHMIN || ABS( dvy ) > WATCHMIN ) )
        {
            pchr->turn_z = terp_dir( pchr->turn_z, ( ATAN2( dvx, dvy ) + PI ) * 0xFFFF / ( TWO_PI ) );
        }
    }

    // Face the target
    watchtarget = ( pchr->turnmode == TURNMODE_WATCHTARGET );
    if ( watchtarget )
    {
        if ( pchr->ai.index != pchr->ai.target )
        {
            pchr->turn_z = terp_dir( pchr->turn_z, ( ATAN2( ChrList.lst[pchr->ai.target].pos.y - pchr->pos.y, ChrList.lst[pchr->ai.target].pos.x - pchr->pos.x ) + PI ) * 0xFFFF / ( TWO_PI ) );
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
    if ( pchr->turnmode == TURNMODE_VELOCITY )
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
    else if ( pchr->turnmode == TURNMODE_SPIN )
    {
        pchr->turn_z += SPINRATE;
    }

}

//--------------------------------------------------------------------------------------------
bool_t chr_do_latch_button( chr_t * pchr )
{
    // BB> Character latches for generalized buttons

    Uint16 ichr;
    ai_state_t * pai;
    Uint8 actionready;
    Uint8 allowedtoattack;
    Uint16 action, weapon, mount, item;

    if ( NULL == pchr || !pchr->on ) return bfalse;
    pai = &(pchr->ai);
    ichr = pai->index;

    if ( !pchr->alive || 0 == pchr->latchbutton ) return btrue;

    if ( HAS_SOME_BITS( pchr->latchbutton, LATCHBUTTON_JUMP ) )
    {
        if ( pchr->attachedto != MAX_CHR && pchr->jumptime == 0 )
        {
            int ijump;

            detach_character_from_mount( ichr, btrue, btrue );
            pchr->jumptime = JUMPDELAY;
            pchr->vel.z = DISMOUNTZVEL;
            if ( pchr->flyheight != 0 )
                pchr->vel.z = DISMOUNTZVELFLY;

            pchr->pos.z += pchr->vel.z;
            if ( pchr->jumpnumberreset != JUMPINFINITE && pchr->jumpnumber != 0 )
                pchr->jumpnumber--;

            // Play the jump sound
            ijump = pro_get_pcap(pchr->iprofile)->soundindex[SOUND_JUMP];
            if ( VALID_SND( ijump ) )
            {
                sound_play_chunk( pchr->pos, chr_get_chunk_ptr( pchr,ijump ) );
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
                if ( pchr->actionready )    chr_play_action( ichr, ACTION_JA, btrue );

                // Play the jump sound (Boing!)
                {
                    int ijump = pro_get_pcap(pchr->iprofile)->soundindex[SOUND_JUMP];
                    if ( VALID_SND( ijump ) )
                    {
                        sound_play_chunk( pchr->pos, chr_get_chunk_ptr( pchr,ijump ) );
                    }
                }
            }
        }
    }
    if ( HAS_SOME_BITS( pchr->latchbutton, LATCHBUTTON_ALTLEFT ) && pchr->actionready && 0 == pchr->reloadtime )
    {
        pchr->reloadtime = GRABDELAY;
        if ( pchr->holdingwhich[SLOT_LEFT] == MAX_CHR )
        {
            // Grab left
            chr_play_action( ichr, ACTION_ME, bfalse );
        }
        else
        {
            // Drop left
            chr_play_action( ichr, ACTION_MA, bfalse );
        }
    }
    if ( HAS_SOME_BITS( pchr->latchbutton, LATCHBUTTON_ALTRIGHT ) && pchr->actionready && 0 == pchr->reloadtime )
    {
        pchr->reloadtime = GRABDELAY;
        if ( pchr->holdingwhich[SLOT_RIGHT] == MAX_CHR )
        {
            // Grab right
            chr_play_action( ichr, ACTION_MF, bfalse );
        }
        else
        {
            // Drop right
            chr_play_action( ichr, ACTION_MB, bfalse );
        }
    }
    if ( HAS_SOME_BITS( pchr->latchbutton, LATCHBUTTON_PACKLEFT ) && pchr->actionready && 0 == pchr->reloadtime )
    {
        pchr->reloadtime = PACKDELAY;
        item = pchr->holdingwhich[SLOT_LEFT];

        if ( VALID_CHR(item) )
        {
            chr_t * pitem = ChrList.lst + item;

            if ( ( pitem->iskursed || chr_get_pcap(item)->istoobig ) && !chr_get_pcap(item)->isequipment )
            {
                // The item couldn't be put away
                pitem->ai.alert |= ALERTIF_NOTPUTAWAY;
                if ( pchr->isplayer && chr_get_pcap(item)->istoobig )
                {
                    debug_printf( "The %s is too big to be put away...", chr_get_pcap(item)->classname );
                }
            }
            else
            {
                // Put the item into the pack
                inventory_add_item( item, ichr );
            }
        }
        else
        {
            // Get a new one out and put it in hand
            inventory_get_item( ichr, GRIP_LEFT, bfalse );
        }

        // Make it take a little time
        chr_play_action( ichr, ACTION_MG, bfalse );
    }
    if ( HAS_SOME_BITS( pchr->latchbutton, LATCHBUTTON_PACKRIGHT ) && pchr->actionready && 0 == pchr->reloadtime )
    {
        pchr->reloadtime = PACKDELAY;
        item = pchr->holdingwhich[SLOT_RIGHT];
        if ( item != MAX_CHR )
        {
            chr_t * pitem = ChrList.lst + item;

            if ( ( pitem->iskursed || chr_get_pcap(item)->istoobig ) && !chr_get_pcap(item)->isequipment )
            {
                // The item couldn't be put away
                pitem->ai.alert |= ALERTIF_NOTPUTAWAY;
                if ( pchr->isplayer && chr_get_pcap(item)->istoobig )
                {
                    debug_printf( "The %s is too big to be put away...", chr_get_pcap(item)->classname );
                }
            }
            else
            {
                // Put the item into the pack
                inventory_add_item( item, ichr );
            }
        }
        else
        {
            // Get a new one out and put it in hand
            inventory_get_item( ichr, GRIP_RIGHT, bfalse );
        }

        // Make it take a little time
        chr_play_action( ichr, ACTION_MG, bfalse );
    }

    // LATCHBUTTON_LEFT and LATCHBUTTON_RIGHT are mutually exclusive
    if ( HAS_SOME_BITS( pchr->latchbutton, LATCHBUTTON_LEFT ) && 0 == pchr->reloadtime )
    {
        chr_t * pweapon;

        // Which weapon?
        weapon = pchr->holdingwhich[SLOT_LEFT];
        if ( weapon == MAX_CHR )
        {
            // Unarmed means character itself is the weapon
            weapon = ichr;
        }
        pweapon = ChrList.lst + weapon;

        action = chr_get_pcap(weapon)->weaponaction;

        // Can it do it?
        allowedtoattack = btrue;

        // First check if reload time and action is okay
        if ( !chr_get_pmad(ichr)->actionvalid[action] || pweapon->reloadtime > 0 ) allowedtoattack = bfalse;
        else
        {
            // Then check if a skill is needed
            if ( chr_get_pcap(weapon)->needskillidtouse )
            {
                if (check_skills( ichr, chr_get_idsz(weapon,IDSZ_SKILL)) == bfalse )
                    allowedtoattack = bfalse;
            }
        }
        if ( !allowedtoattack )
        {
            if ( pweapon->reloadtime == 0 )
            {
                // This character can't use this weapon
                pweapon->reloadtime = 50;
                if ( pchr->staton )
                {
                    // Tell the player that they can't use this weapon
                    debug_printf( "%s can't use this item...", pchr->name );
                }
            }
        }
        if ( action == ACTION_DA )
        {
            allowedtoattack = bfalse;
            if ( pweapon->reloadtime == 0 )
            {
                pweapon->ai.alert |= ALERTIF_USED;
            }
        }
        if ( allowedtoattack )
        {
            // Rearing mount
            mount = pchr->attachedto;
            if ( mount != MAX_CHR )
            {
                allowedtoattack = chr_get_pcap(mount)->ridercanattack;
                if ( ChrList.lst[mount].ismount && ChrList.lst[mount].alive && !ChrList.lst[mount].isplayer && ChrList.lst[mount].actionready )
                {
                    if ( ( action != ACTION_PA || !allowedtoattack ) && pchr->actionready )
                    {
                        chr_play_action( pchr->attachedto,  generate_randmask( ACTION_UA, 1 ), bfalse );
                        chr_get_pai(pchr->attachedto)->alert |= ALERTIF_USED;
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
                if ( pchr->actionready && chr_get_pmad(ichr)->actionvalid[action] )
                {
                    // Check mana cost
                    bool_t mana_paid = cost_mana( ichr, pweapon->manacost, weapon );

                    if ( mana_paid )
                    {
                        // Check life healing
                        pchr->life += pweapon->lifeheal;
                        if ( pchr->life > pchr->lifemax )  pchr->life = pchr->lifemax;

                        actionready = bfalse;
                        if ( action == ACTION_PA )
                            actionready = btrue;

                        action += generate_randmask( 0, 1 );
                        chr_play_action( ichr, action, actionready );
                        if ( weapon != ichr )
                        {
                            // Make the weapon attack too
                            chr_play_action( weapon, ACTION_MJ, bfalse );
                            pweapon->ai.alert |= ALERTIF_USED;
                            pchr->ai.lastitemused = weapon;
                        }
                        else
                        {
                            // Flag for unarmed attack
                            pchr->ai.alert |= ALERTIF_USED;
                            pchr->ai.lastitemused = ichr;
                        }
                    }
                }
            }
        }
    }
    else if ( HAS_SOME_BITS( pchr->latchbutton, LATCHBUTTON_RIGHT ) && 0 == pchr->reloadtime )
    {
        chr_t * pweapon;

        // Which weapon?
        weapon = pchr->holdingwhich[SLOT_RIGHT];
        if ( weapon == MAX_CHR )
        {
            // Unarmed means character itself is the weapon
            weapon = ichr;
        }
        pweapon = ChrList.lst + weapon;

        action = chr_get_pcap(weapon)->weaponaction + 2;

        // Can it do it? (other hand)
        allowedtoattack = btrue;

        // First check if reload time and action is okay
        if ( !chr_get_pmad(ichr)->actionvalid[action] || pweapon->reloadtime > 0 )
        {
            allowedtoattack = bfalse;
        }
        else
        {
            // Then check if a skill is needed
            if ( chr_get_pcap(weapon)->needskillidtouse )
            {
                IDSZ idsz = chr_get_idsz(weapon,IDSZ_SKILL);

                if ( check_skills( ichr, idsz) == bfalse   )
                    allowedtoattack = bfalse;
            }
        }

        if ( !allowedtoattack )
        {
            if ( pweapon->reloadtime == 0 )
            {
                // This character can't use this weapon
                pweapon->reloadtime = 50;
                if ( pchr->staton )
                {
                    // Tell the player that they can't use this weapon
                    debug_printf( "%s can't use this item...", pchr->name );
                }
            }
        }
        if ( action == ACTION_DC )
        {
            allowedtoattack = bfalse;
            if ( pweapon->reloadtime == 0 )
            {
                pweapon->ai.alert |= ALERTIF_USED;
                pchr->ai.lastitemused = weapon;
            }
        }
        if ( allowedtoattack )
        {
            // Rearing mount
            mount = pchr->attachedto;
            if ( mount != MAX_CHR )
            {
                allowedtoattack = chr_get_pcap(mount)->ridercanattack;
                if ( ChrList.lst[mount].ismount && ChrList.lst[mount].alive && !ChrList.lst[mount].isplayer && ChrList.lst[mount].actionready )
                {
                    if ( ( action != ACTION_PC || !allowedtoattack ) && pchr->actionready )
                    {
                        chr_play_action( pchr->attachedto,  generate_randmask( ACTION_UC, 1 ), bfalse );
                        chr_get_pai(pchr->attachedto)->alert |= ALERTIF_USED;
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
                if ( pchr->actionready && chr_get_pmad(ichr)->actionvalid[action] )
                {
                    bool_t mana_paid = cost_mana( ichr, pweapon->manacost, weapon );
                    // Check mana cost
                    if ( mana_paid )
                    {
                        // Check life healing
                        pchr->life += pweapon->lifeheal;
                        if ( pchr->life > pchr->lifemax )  pchr->life = pchr->lifemax;

                        actionready = bfalse;
                        if ( action == ACTION_PC )
                            actionready = btrue;

                        action += generate_randmask( 0, 1 );
                        chr_play_action( ichr, action, actionready );
                        if ( weapon != ichr )
                        {
                            // Make the weapon attack too
                            chr_play_action( weapon, ACTION_MJ, bfalse );
                            pweapon->ai.alert |= ALERTIF_USED;
                            pchr->ai.lastitemused = weapon;
                        }
                        else
                        {
                            // Flag for unarmed attack
                            pchr->ai.alert |= ALERTIF_USED;
                            pchr->ai.lastitemused = ichr;
                        }
                    }
                }
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void move_characters_do_z_motion( chr_t * pchr, chr_environment_t * penviro )
{
    if( NULL == pchr || NULL == penviro ) return;

    //---- do z acceleration
    if ( 0 != pchr->flyheight )
    {
        pchr->vel.z += ( penviro->level + pchr->flyheight - pchr->pos.z ) * FLYDAMPEN;
    }
    else
    {
        if ( penviro->is_slippy && pchr->weight != 0xFFFFFFFF &&
             penviro->twist != TWIST_FLAT && penviro->zlerp < 1.0f)
        {
            // Slippy hills make characters slide

            GLvector3 gpara, gperp;

            gperp.x = map_twistvel_x[penviro->twist];
            gperp.y = map_twistvel_y[penviro->twist];
            gperp.z = map_twistvel_z[penviro->twist];

            gpara.x = 0       - gperp.x;
            gpara.y = 0       - gperp.y;
            gpara.z = gravity - gperp.z;

            pchr->vel.x += gpara.x + gperp.x * penviro->zlerp;
            pchr->vel.y += gpara.y + gperp.y * penviro->zlerp;
            pchr->vel.z += gpara.z + gperp.z * penviro->zlerp;
        }
        else
        {
            pchr->vel.z += penviro->zlerp * gravity;
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t move_characters_integrate_motion( chr_t * pchr )
{
    // BB> Figure out the next position of the character.
    //    Include collisions with the mesh in this step.

    float nrm[2], ftmp;
    Uint16 ichr;
    ai_state_t * pai;

    if ( NULL == pchr || !pchr->on ) return bfalse;
    pai = &(pchr->ai);
    ichr = pai->index;

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
    if ( __chrhitawall( ichr, nrm ) )
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
    if ( __chrhitawall( ichr, nrm ) )
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

    if ( __chrhitawall(ichr, nrm) )
    {
        pchr->safe_valid = btrue;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void move_characters_do_animation( chr_t * pchr, chr_environment_t * penviro )
{
    Uint8 speed, framelip;
    Uint16 ichr;

    chr_instance_t * pinst;
    mad_t          * pmad;

    if( NULL == penviro ) return;

    if( NULL == pchr || !pchr->onwhichblock ) return;
    ichr  = pchr->ai.index;
    pinst = &(pchr->inst);

    pmad = chr_get_pmad(ichr);
    if( NULL == pmad ) return;

    // Animate the character
    pchr->inst.lip = ( pchr->inst.lip + 64 );

    // handle frame FX for the new frame
    if ( pinst->lip == 192 )
    {
        // Check frame effects
        if ( Md2FrameList[pinst->frame_nxt].framefx&MADFX_ACTLEFT )
            character_swipe( ichr, SLOT_LEFT );

        if ( Md2FrameList[pinst->frame_nxt].framefx&MADFX_ACTRIGHT )
            character_swipe( ichr, SLOT_RIGHT );

        if ( Md2FrameList[pinst->frame_nxt].framefx&MADFX_GRABLEFT )
            character_grab_stuff( ichr, GRIP_LEFT, bfalse );

        if ( Md2FrameList[pinst->frame_nxt].framefx&MADFX_GRABRIGHT )
            character_grab_stuff( ichr, GRIP_RIGHT, bfalse );

        if ( Md2FrameList[pinst->frame_nxt].framefx&MADFX_CHARLEFT )
            character_grab_stuff( ichr, GRIP_LEFT, btrue );

        if ( Md2FrameList[pinst->frame_nxt].framefx&MADFX_CHARRIGHT )
            character_grab_stuff( ichr, GRIP_RIGHT, btrue );

        if ( Md2FrameList[pinst->frame_nxt].framefx&MADFX_DROPLEFT )
            detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], bfalse, btrue );

        if ( Md2FrameList[pinst->frame_nxt].framefx&MADFX_DROPRIGHT )
            detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], bfalse, btrue );

        if ( Md2FrameList[pinst->frame_nxt].framefx&MADFX_POOF && !pchr->isplayer )
            pchr->ai.poof_time = update_wld;

        if ( Md2FrameList[pinst->frame_nxt].framefx&MADFX_FOOTFALL )
        {
            int ifoot = pro_get_pcap(pchr->iprofile)->soundindex[SOUND_FOOTFALL];
            if ( VALID_SND( ifoot ) )
            {
                sound_play_chunk( pchr->pos, chr_get_chunk_ptr( pchr,ifoot ) );
            }
        }
    }

    if ( pinst->lip == 0 )
    {
        // Change frames
        pinst->frame_lst = pinst->frame_nxt;
        pinst->frame_nxt++;

        if ( pinst->frame_nxt == chr_get_pmad(ichr)->actionend[pchr->action] )
        {
            // Action finished
            if ( pchr->keepaction )
            {
                // Keep the last frame going
                pinst->frame_nxt = pinst->frame_lst;
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

                pinst->frame_nxt = chr_get_pmad(ichr)->actionstart[pchr->action];
            }

            pchr->actionready = btrue;
        }
    }

    // Get running, walking, sneaking, or dancing, from speed
    if ( !pchr->keepaction && !pchr->loopaction )
    {
        framelip = Md2FrameList[pinst->frame_nxt].framelip;  // 0 - 15...  Way through animation
        if ( pchr->actionready && pinst->lip == 0 && pchr->phys.grounded && pchr->flyheight == 0 && ( framelip&7 ) < 2 )
        {
            // Do the motion stuff
            speed = ABS( penviro->new_vx ) + ABS( penviro->new_vy );
            if ( speed < pchr->sneakspd )
            {
                //                       pchr->nextaction = ACTION_DA;
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
                        pinst->frame_nxt = pmad->actionstart[pchr->action];
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
                        pinst->frame_nxt = pmad->frameliptowalkframe[LIPWA][framelip];
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
                            pinst->frame_nxt = pmad->frameliptowalkframe[LIPWB][framelip];
                            pchr->action = ACTION_WB;
                        }
                    }
                    else
                    {
                        pchr->nextaction = ACTION_WC;
                        if ( pchr->action != ACTION_WC )
                        {
                            pinst->frame_nxt = pmad->frameliptowalkframe[LIPWC][framelip];
                            pchr->action     = ACTION_WC;
                        }
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_characters( void )
{
    // ZZ> This function handles character physics

    const float air_friction = 0.9868f;  // gives the same terminal velocity in terms of the size of the game characters
    const float ice_friction = 0.9738f;  // the square of air_friction

    Uint16 cnt;

    chr_environment_t enviro;
    chr_environment_t * penviro = &enviro;

    // prime the environment
    enviro.air_friction = air_friction;
    enviro.ice_friction = ice_friction;

    // Move every character
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        chr_t * pchr;

        if ( !ChrList.lst[cnt].on || ChrList.lst[cnt].pack_ispacked ) continue;
        pchr = ChrList.lst + cnt;

        // save the acceleration from the last time-step
        penviro->acc = VSub(pchr->vel, pchr->vel_old);

        // Character's old location
        pchr->pos_old    = pchr->pos;
        pchr->vel_old    = pchr->vel;
        pchr->turn_old_z = pchr->turn_z;

        penviro->new_vx = pchr->vel.x;
        penviro->new_vy = pchr->vel.y;

        move_characters_get_environment(pchr, penviro);

        // do friction with the floor before volontary motion
        move_characters_do_floor_friction(pchr, penviro);

        move_characters_do_volontary(pchr, penviro);

        chr_do_latch_button( pchr );

        move_characters_do_z_motion(pchr, penviro);

        move_characters_integrate_motion( pchr );

        move_characters_do_animation(pchr, penviro);

        // Characters with sticky butts lie on the surface of the mesh
        if ( pchr->stickybutt || !pchr->alive )
        {
            float fkeep = (7 + penviro->zlerp) / 8.0f;
            float fnew  = (1 - penviro->zlerp) / 8.0f;

            if ( fnew > 0 )
            {
                pchr->map_turn_x = pchr->map_turn_x * fkeep + map_twist_x[penviro->twist] * fnew;
                pchr->map_turn_y = pchr->map_turn_y * fkeep + map_twist_y[penviro->twist] * fnew;
            }
        }
    }

    // update every character
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        chr_t * pchr;

        if ( !ChrList.lst[cnt].on ) continue;
        pchr = ChrList.lst + cnt;

        // Texture movement
        pchr->inst.uoffset += pchr->uoffvel;
        pchr->inst.voffset += pchr->voffvel;

        if( !pchr->pack_ispacked )
        {
            // Down that ol' damage timer
            if (pchr->damagetime > 0)
            {
                pchr->damagetime--;
            }

            // Do "Be careful!" delay
            if ( pchr->carefultime > 0 )
            {
                pchr->carefultime--;
            }
        }
    }

    // Do poofing
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ChrList.lst[cnt].on || !(chr_get_pai(cnt)->poof_time >= 0 && chr_get_pai(cnt)->poof_time <= (Sint32)update_wld)  ) continue;

        if ( ChrList.lst[cnt].attachedto != MAX_CHR )
        {
            detach_character_from_mount( cnt, btrue, bfalse );
        }

        if ( ChrList.lst[cnt].holdingwhich[SLOT_LEFT] != MAX_CHR )
        {
            detach_character_from_mount( ChrList.lst[cnt].holdingwhich[SLOT_LEFT], btrue, bfalse );
        }

        if ( ChrList.lst[cnt].holdingwhich[SLOT_RIGHT] != MAX_CHR )
        {
            detach_character_from_mount( ChrList.lst[cnt].holdingwhich[SLOT_RIGHT], btrue, bfalse );
        }

        looped_stop_object_sounds( cnt );
        free_inventory( cnt );
        free_one_character_in_game( cnt );
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
    pchr = ChrList.lst + character;

    pmad = chr_get_pmad(character);
    if ( NULL == pmad ) return btrue;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return btrue;

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

    pro_t * pobj;
    mad_t * pmad;
    cap_t * pcap;

    if ( NULL == pinst ) return bfalse;

    // Remember any previous color shifts in case of lasting enchantments
    greensave = pinst->grnshift;
    redsave = pinst->redshift;
    bluesave = pinst->blushift;

    // clear the instance
    memset(pinst, 0, sizeof(chr_instance_t));
    pinst->imad = MAX_PROFILE;

    if ( INVALID_PRO(profile) ) return bfalse;
    pobj = ProList.lst + profile;

    pcap = pro_get_pcap(profile);
    pmad = pro_get_pmad(profile);

    pinst->imad      = profile;
    pinst->texture   = pobj->tex_ref[skin];
    pinst->enviro    = pcap->enviro;
    pinst->alpha     = pcap->alpha;
    pinst->light     = pcap->light;
    pinst->sheen     = pcap->sheen;
    pinst->grnshift  = greensave;
    pinst->redshift  = redsave;
    pinst->blushift  = bluesave;

    pinst->frame_nxt = pmad->md2_data.framestart;
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
    int retval = GRIP_ORIGIN;

    retval = ( slot + 1 ) * GRIP_VERTS;

    return (grip_offset_t)retval;
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
    inventory_idsz[INVEN_NECK]  = MAKE_IDSZ( 'N', 'E', 'C', 'K' );
    inventory_idsz[INVEN_WRIS]  = MAKE_IDSZ( 'W', 'R', 'I', 'S' );
    inventory_idsz[INVEN_FOOT]  = MAKE_IDSZ( 'F', 'O', 'O', 'T' );
}

//--------------------------------------------------------------------------------------------
bool_t ai_add_order( ai_state_t * pai, Uint32 value, Uint16 counter )
{
    bool_t retval;

    if (NULL == pai) return bfalse;

    // this function is only truely valid if there is no other order
    retval = HAS_NO_BITS( pai->alert, ALERTIF_ORDERED );

    pai->alert        |= ALERTIF_ORDERED;
    pai->order_value   = value;
    pai->order_counter = counter;

    return retval;
}

//--------------------------------------------------------------------------------------------
int chr_add_billboard( Uint16 ichr, Uint32 lifetime_secs )
{
    // BB> Attach a basic billboard to a character. You set the billboard texture
    //     at any time after this. Returns the index of the billboard or INVALID_BILLBOARD
    //     if the allocation fails.
    //
    //    must be called with a valid character, so be careful if you call this function from within
    //    spawn_one_character()

    chr_t * pchr;

    if ( INVALID_CHR(ichr) ) return INVALID_BILLBOARD;
    pchr = ChrList.lst + ichr;

    if ( INVALID_BILLBOARD != pchr->ibillboard )
    {
        BillboardList_free_one(pchr->ibillboard);
        pchr->ibillboard = INVALID_BILLBOARD;
    }

    pchr->ibillboard = BillboardList_get_free(lifetime_secs);

    // attachr the billboard to the character
    if ( INVALID_BILLBOARD != pchr->ibillboard )
    {
        billboard_data_t * pbb = BillboardList.lst + pchr->ibillboard;

        pbb->ichr = ichr;
    }

    return pchr->ibillboard;
}

//--------------------------------------------------------------------------------------------
bool_t chr_make_text_billboard( Uint16 ichr, const char * txt, SDL_Color color, int lifetime_secs )
{
    chr_t            * pchr;
    billboard_data_t * pbb;

    bool_t retval = bfalse;
    int    ibb;

    if ( INVALID_CHR(ichr) ) return bfalse;
    pchr = ChrList.lst + ichr;

    // create a new billboard or override the old billboard
    ibb = chr_add_billboard( ichr, lifetime_secs );
    if ( INVALID_BILLBOARD == ibb ) return bfalse;

    pbb = BillboardList_get_ptr( pchr->ibillboard );
    if ( NULL != pbb)
    {
        int rv = billboard_data_printf_ttf( pbb, ui_getFont(), color, "%s", txt );

        if ( rv < 0 )
        {
            pchr->ibillboard = INVALID_BILLBOARD;
            BillboardList_free_one(ibb);
        }
        else
        {
            retval = btrue;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char * chr_get_name( Uint16 ichr )
{
    static STRING szName;

    // the default name
    strncpy( szName, "Unknown", SDL_arraysize(szName) );

    if ( VALID_CHR(ichr) )
    {
        chr_t * pchr = ChrList.lst + ichr;

        if ( pchr->nameknown )
        {
            snprintf( szName, SDL_arraysize( szName), "%s", pchr->name );
        }
        else
        {
            char lTmp;
            const char * article = "a";

            lTmp = toupper( pro_get_pcap(pchr->iprofile)->classname[0] );
            if ( 'A' == lTmp || 'E' == lTmp || 'I' == lTmp || 'O' == lTmp || 'U' == lTmp )
            {
                article = "an";
            }

            snprintf( szName, SDL_arraysize( szName), "%s %s", article, pro_get_pcap(pchr->iprofile)->classname );
        }
    }

    // ? capitalize the name ?
    szName[0] = toupper( szName[0] );

    return szName;
}

//--------------------------------------------------------------------------------------------
bool_t release_one_cap( Uint16 icap )
{
    cap_t * pcap;

    if( !VALID_CAP_RANGE(icap) ) return bfalse;
    pcap = CapList + icap;

    if( !pcap->loaded ) return btrue;

    memset( pcap, 0, sizeof(cap_t) );

    pcap->loaded  = bfalse;
    pcap->name[0] = '\0';

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint16 chr_get_iobj(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return MAX_PROFILE;
    pchr = ChrList.lst + ichr;

    if( INVALID_PRO(pchr->iprofile) ) return MAX_PROFILE;

    return pchr->iprofile;
}

//--------------------------------------------------------------------------------------------
Uint16 chr_get_icap(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return MAX_CHR;
    pchr = ChrList.lst + ichr;

    return pro_get_icap(pchr->iprofile);
}

//--------------------------------------------------------------------------------------------
Uint16 chr_get_imad(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return MAX_MAD;
    pchr = ChrList.lst + ichr;

    // try to repair a bad model if it exists
    if( INVALID_MAD(pchr->inst.imad) )
    {
        Uint16 imad_tmp = pro_get_imad(pchr->iprofile);
        if( VALID_MAD( imad_tmp ) )
        {
            pchr->inst.imad = imad_tmp;
        }
    }

    if( INVALID_MAD(pchr->inst.imad) ) return MAX_MAD;

    return pchr->inst.imad;
}

//--------------------------------------------------------------------------------------------
Uint16 chr_get_ieve(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return MAX_EVE;
    pchr = ChrList.lst + ichr;

    return pro_get_ieve(pchr->iprofile);
}

//--------------------------------------------------------------------------------------------
Uint16 chr_get_ipip(Uint16 ichr, Uint16 ipip)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return MAX_PIP;
    pchr = ChrList.lst + ichr;

    return pro_get_ipip(pchr->iprofile, ipip);
}

//--------------------------------------------------------------------------------------------
Uint16 chr_get_iteam(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return TEAM_MAX;
    pchr = ChrList.lst + ichr;

    if( pchr->team < 0 && pchr->team >= TEAM_MAX ) return TEAM_MAX;

    return pchr->team;
}

//--------------------------------------------------------------------------------------------
Uint16 chr_get_iteam_base(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return TEAM_MAX;
    pchr = ChrList.lst + ichr;

    if( pchr->baseteam < 0 && pchr->baseteam >= TEAM_MAX ) return TEAM_MAX;

    return pchr->baseteam;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
pro_t * chr_get_pobj(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return NULL;
    pchr = ChrList.lst + ichr;

    if( INVALID_PRO(pchr->iprofile) ) return NULL;

    return ProList.lst + pchr->iprofile;
}

//--------------------------------------------------------------------------------------------
cap_t * chr_get_pcap(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return NULL;
    pchr = ChrList.lst + ichr;

    return pro_get_pcap(pchr->iprofile);
}

//--------------------------------------------------------------------------------------------
mad_t * chr_get_pmad(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return NULL;
    pchr = ChrList.lst + ichr;

    // try to repair a bad model if it exists
    if( INVALID_MAD(pchr->inst.imad) )
    {
        Uint16 imad_tmp = pro_get_imad(pchr->iprofile);
        if( VALID_MAD( imad_tmp ) )
        {
            pchr->inst.imad = imad_tmp;
        }
    }

    if( INVALID_MAD(pchr->inst.imad) ) return NULL;

    return MadList + pchr->inst.imad;
}

//--------------------------------------------------------------------------------------------
eve_t * chr_get_peve(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return NULL;
    pchr = ChrList.lst + ichr;

    return pro_get_peve(pchr->iprofile);
}

//--------------------------------------------------------------------------------------------
pip_t * chr_get_ppip(Uint16 ichr, Uint16 ipip)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return NULL;
    pchr = ChrList.lst + ichr;

    return pro_get_ppip(pchr->iprofile, ipip);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Mix_Chunk * chr_get_chunk( Uint16 ichr, int index )
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return NULL;
    pchr = ChrList.lst + ichr;

    return pro_get_chunk( pchr->iprofile, index );
}

//--------------------------------------------------------------------------------------------
Mix_Chunk * chr_get_chunk_ptr( chr_t * pchr, int index )
{
    if( NULL == pchr || !pchr->on ) return NULL;

    return pro_get_chunk( pchr->iprofile, index );
}

//--------------------------------------------------------------------------------------------
team_t * chr_get_pteam(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return NULL;
    pchr = ChrList.lst + ichr;

    if( pchr->team < 0 && pchr->team >= TEAM_MAX ) return NULL;

    return TeamList + pchr->team;
}

//--------------------------------------------------------------------------------------------
team_t * chr_get_pteam_base(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return NULL;
    pchr = ChrList.lst + ichr;

    if( pchr->baseteam < 0 && pchr->baseteam >= TEAM_MAX ) return NULL;

    return TeamList + pchr->baseteam;
}

//--------------------------------------------------------------------------------------------
ai_state_t * chr_get_pai(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return NULL;
    pchr = ChrList.lst + ichr;

    return &(pchr->ai);
}

//--------------------------------------------------------------------------------------------
chr_instance_t * chr_get_pinstance(Uint16 ichr)
{
    chr_t * pchr;

    if( INVALID_CHR(ichr) ) return NULL;
    pchr = ChrList.lst + ichr;

    return &(pchr->inst);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint16 team_get_ileader( Uint16 iteam )
{
    int ichr;

    if( iteam >= TEAM_MAX ) return MAX_CHR;

    ichr = TeamList[iteam].leader;
    if( INVALID_CHR(ichr) ) return MAX_CHR;

    return ichr;
}

//--------------------------------------------------------------------------------------------
chr_t  * team_get_pleader( Uint16 iteam )
{
    int ichr;

    if( iteam >= TEAM_MAX ) return NULL;

    ichr = TeamList[iteam].leader;
    if( INVALID_CHR(ichr) ) return NULL;

    return ChrList.lst + ichr;
}

//--------------------------------------------------------------------------------------------
bool_t team_hates_team(Uint16 ipredator, Uint16 iprey)\
{
    if( ipredator >= TEAM_MAX || iprey >= TEAM_MAX ) return bfalse;

    return TeamList[ipredator].hatesteam[iprey];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
IDSZ chr_get_idsz(Uint16 ichr, Uint16 type)
{
    cap_t * pcap;

    if(type >= IDSZ_COUNT) return IDSZ_NONE;

    pcap = chr_get_pcap( ichr );
    if( NULL == pcap) return IDSZ_NONE;

    return pcap->idsz[type];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void chr_update_collision_size( chr_t * pchr )
{
    // TODO: use this function to update the pchr->collision with
    //       values that reflect the best possible collision volume
    //
    //       to make much sense, this may require a bounding box that
    //       can be oblong in shape, similar to the collision volume stuff
    //       in the svn trunk

    //int cnt;
    //
    //int       vcount;   // the actual number of vertices, in case the object is square
    //GLvector4 src[16];  // for the upper and lower octagon points
    //GLvector4 dst[16];  // for the upper and lower octagon points

    //float min_x, max_x;
    //float min_y, max_y;
    //float min_xy, max_xy;
    //float min_yx, max_yx;
    //float min_z, max_z;

    mad_t * pmad;


    if( NULL == pchr || !pchr->onwhichblock ) return;

    pmad = chr_get_pmad( pchr->ai.index );
    if( NULL == pmad ) return;

    // do this for now...
    pchr->collision = pchr->bump;

    //// determine a bounding box for the model
    //min_x  = max_x  = pchr->inst.vlst[0].pos[XX];
    //min_y  = max_y  = pchr->inst.vlst[0].pos[YY];
    //min_z  = max_z  = pchr->inst.vlst[0].pos[ZZ];
    //min_xy = max_xy = min_x + min_y;
    //min_yx = max_yx = min_x - min_y;

    //for( cnt = 1; cnt < pmad->transvertices; cnt++ )
    //{
    //    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;

    //    tmp_x = pchr->inst.vlst[cnt].pos[XX];
    //    tmp_y = pchr->inst.vlst[cnt].pos[YY];
    //    tmp_z = pchr->inst.vlst[cnt].pos[ZZ];

    //    min_x  = MIN( min_x, tmp_x ); 
    //    max_x  = MAX( max_x, tmp_x ); 

    //    min_y  = MIN( min_y, tmp_y ); 
    //    max_y  = MAX( max_y, tmp_y ); 

    //    min_z  = MIN( min_z, tmp_z ); 
    //    max_z  = MAX( max_z, tmp_z ); 

    //    tmp_xy = tmp_x + tmp_y;
    //    min_xy = MIN( min_xy, tmp_xy ); 
    //    max_xy = MAX( max_xy, tmp_xy ); 

    //    tmp_yx = tmp_x - tmp_y;
    //    min_yx = MIN( min_yx, tmp_yx ); 
    //    max_yx = MAX( max_yx, tmp_yx ); 
    //}

    //// determine the intersection points
    //TransformVertices( pchr->inst.matrix, src, dst, vcount );

    //// since we do not have a full collision volume, do the best we can with the one we have

    //min_x  = max_x  = dst[0].x;
    //min_y  = max_y  = dst[0].y;
    //min_z  = max_z  = dst[0].z;
    //min_xy = max_xy = min_x + min_y;
    //min_yx = max_yx = min_x - min_y;

    //for( cnt = 1; cnt < vcount; cnt++ )
    //{
    //    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;

    //    tmp_x = dst[cnt].x;
    //    tmp_y = dst[cnt].y;
    //    tmp_z = dst[cnt].z;

    //    min_x  = MIN( min_x, tmp_x ); 
    //    max_x  = MAX( max_x, tmp_x ); 

    //    min_y  = MIN( min_y, tmp_y ); 
    //    max_y  = MAX( max_y, tmp_y ); 

    //    min_z  = MIN( min_z, tmp_z ); 
    //    max_z  = MAX( max_z, tmp_z ); 

    //    tmp_xy = tmp_x + tmp_y;
    //    min_xy = MIN( min_xy, tmp_xy ); 
    //    max_xy = MAX( max_xy, tmp_xy ); 

    //    tmp_yx = tmp_x - tmp_y;
    //    min_yx = MIN( min_yx, tmp_yx ); 
    //    max_yx = MAX( max_yx, tmp_yx ); 
    //}


    //pchr->collision.size   = MAX(MAX(max_x-pchr->pos.x, pchr->pos.x-min_x), MAX(max_y-pchr->pos.y, pchr->pos.y-min_y));
    //pchr->collision.height = (max_z-pchr->pos.z);
}

//--------------------------------------------------------------------------------------------
void chr_update_size( chr_t * pchr )
{
    if( NULL == pchr || !pchr->on ) return;

    pchr->shadowsize   = pchr->shadowsizesave    * pchr->fat;
    pchr->bump.size    = pchr->bump_save.size    * pchr->fat;
    pchr->bump.sizebig = pchr->bump_save.sizebig * pchr->fat;
    pchr->bump.height  = pchr->bump_save.height  * pchr->fat;

    chr_update_collision_size(pchr);
}

//--------------------------------------------------------------------------------------------
void chr_init_size( chr_t * pchr, cap_t * pcap )
{
    if( NULL == pchr || !pchr->on     ) return;
    if( NULL == pcap || !pcap->loaded ) return;

    pchr->fat               = pcap->size;

    pchr->shadowsizesave    = pcap->shadowsize;
    pchr->bump_save.size    = pcap->bumpsize;
    pchr->bump_save.sizebig = pcap->bumpsizebig;
    pchr->bump_save.height  = pcap->bumpheight;

    chr_update_size( pchr );
}

//--------------------------------------------------------------------------------------------
void chr_set_size( chr_t * pchr, float size )
{
    float ratio;

    if( NULL == pchr || !pchr->on ) return;

    ratio = size / pchr->bump.size;

    pchr->shadowsizesave    *= ratio;
    pchr->bump_save.size    *= ratio;
    pchr->bump_save.sizebig *= ratio;
    pchr->bump_save.height  *= ratio;

    chr_update_size( pchr );
}

//--------------------------------------------------------------------------------------------
void chr_set_width( chr_t * pchr, float width )
{
    float ratio;

    if( NULL == pchr || !pchr->on ) return;

    ratio = width / pchr->bump.size;

    pchr->shadowsizesave    *= ratio;
    pchr->bump_save.size    *= ratio;
    pchr->bump_save.sizebig *= ratio;

    chr_update_size( pchr );
}

//--------------------------------------------------------------------------------------------
void chr_set_height( chr_t * pchr, float height )
{
    if( NULL == pchr || !pchr->on ) return;

    pchr->bump_save.height = height;

    chr_update_size( pchr );
}

//--------------------------------------------------------------------------------------------
void chr_set_shadow( chr_t * pchr, float width )
{
    if( NULL == pchr || !pchr->on ) return;

    pchr->shadowsizesave = width;

    chr_update_size( pchr );
}

//--------------------------------------------------------------------------------------------
void chr_set_fat( chr_t * pchr, float fat )
{
    if( NULL == pchr || !pchr->on ) return;

    pchr->fat = fat;

    chr_update_size( pchr );
}

//--------------------------------------------------------------------------------------------
bool_t cap_has_idsz( Uint16 icap, IDSZ idsz )
{
    int     cnt;
    cap_t * pcap;
    bool_t  retval;

    if( INVALID_CAP(icap) ) return bfalse;
    pcap = CapList + icap;

    retval = bfalse;
    if( IDSZ_NONE == idsz )
    {
        retval = btrue;
    }
    else
    { 
        for ( cnt = 0; cnt < IDSZ_COUNT; cnt++ )
        {
            if ( pcap->idsz[cnt] == idsz )
            {
                retval = btrue;
               break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t cap_is_type_idsz( Uint16 icap, IDSZ test_idsz )
{
    // BB> check IDSZ_PARENT and IDSZ_TYPE to see if the test_idsz matches. If we are not 
    //     picky (i.e. IDSZ_NONE == test_idsz), then it matches any valid item.

    cap_t * pcap;

    if( INVALID_CAP(icap) ) return bfalse;
    pcap = CapList + icap;

    if( IDSZ_NONE == test_idsz               ) return btrue;
    if( IDSZ_NONE == pcap->idsz[IDSZ_TYPE  ] ) return btrue;
    if( IDSZ_NONE == pcap->idsz[IDSZ_PARENT] ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t chr_has_idsz( Uint16 ichr, IDSZ idsz )
{
    Uint16 icap = chr_get_icap(ichr);

    return cap_has_idsz( icap, idsz );
}

//--------------------------------------------------------------------------------------------
bool_t chr_is_type_idsz( Uint16 item, IDSZ test_idsz )
{
    // BB> check IDSZ_PARENT and IDSZ_TYPE to see if the test_idsz matches. If we are not 
    //     picky (i.e. IDSZ_NONE == test_idsz), then it matches any valid item.

    Uint16 icap;

    icap = chr_get_icap( item );

    return cap_is_type_idsz( icap, test_idsz );
}

//--------------------------------------------------------------------------------------------
bool_t chr_has_vulnie( Uint16 item, Uint16 test_profile )
{
    IDSZ vulnie;

    if( INVALID_CHR(item) ) return bfalse;
    vulnie = chr_get_idsz(item, IDSZ_VULNERABILITY);

    // not vulnerable if there is no specific weakness
    if( IDSZ_NONE == vulnie ) return bfalse;

    // check vs. every IDSZ that could have something to do with attacking
    if( vulnie == pro_get_idsz(test_profile, IDSZ_TYPE  ) ) return btrue;
    if( vulnie == pro_get_idsz(test_profile, IDSZ_PARENT) ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
/*Uint16 get_target_in_block( int x, int y, Uint16 character, char items,
                            char friends, char enemies, char dead, char seeinvisible, IDSZ idsz,
                            char excludeid )
{
  // ZZ> This is a good little helper, that returns != MAX_CHR if a suitable target
  //    was found
  int cnt;
  Uint16 charb;
  Uint32 fanblock;
  Uint8 team;
  if ( x >= 0 && x < meshbloksx && y >= 0 && y < meshbloksy )
  {
    team = chr_get_iteam(character);
    fanblock = mesh_get_block_int(PMesh, x,y);
    charb = bumplist[fanblock].chr;
    cnt = 0;
    while ( cnt < bumplist[fanblock].chrnum )
    {
      if ( dead != ChrList.lst[charb].alive && ( seeinvisible || FF_MUL( chr_get_pinstance(charb)->alpha, chr_get_pinstance(charb)->max_light ) > INVISIBLE ) ) )
      {
        if ( ( enemies && TeamList[team].hatesteam[chr_get_iteam(charb)] && !ChrList.lst[charb].invictus ) ||
             ( items && ChrList.lst[charb].isitem ) ||
             ( friends && ChrList.lst[charb].baseteam == team ) )
        {
          if ( charb != character && ChrList.lst[character].attachedto != charb )
          {
            if ( !ChrList.lst[charb].isitem || items )
            {
              if ( idsz != IDSZ_NONE )
              {
                if ( chr_get_idsz(charb,IDSZ_PARENT) == idsz ||
                     chr_get_idsz(charb,IDSZ_TYPE) == idsz )
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
      charb = ChrList.lst[charb].fanblock_next;
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
  seeinvisible = ChrList.lst[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )ChrList.lst[character].pos.x ) >> BLOCK_BITS;
  y = ( ( int )ChrList.lst[character].pos.y ) >> BLOCK_BITS;
  return get_target_in_block( x, y, character, items, friends, enemies, dead, seeinvisible, idsz, 0 );
}*/

//--------------------------------------------------------------------------------------------
/*Uint16 find_distant_target( Uint16 character, int maxdistance )
{
  // ZZ> This function finds a target, or it returns MAX_CHR if it can't find one...
  //    maxdistance should be the square of the actual distance you want to use
  //    as the cutoff...
  int cnt, distance, xdistance, ydistance;
  Uint8 team;

  team = chr_get_iteam(character);
  cnt = 0;
  while ( cnt < MAX_CHR )
  {
    if ( ChrList.lst[cnt].on )
    {
      if ( ChrList.lst[cnt].attachedto == MAX_CHR && !ChrList.lst[cnt].pack_ispacked )
      {
        if ( TeamList[team].hatesteam[chr_get_iteam(cnt)] && ChrList.lst[cnt].alive && !ChrList.lst[cnt].invictus )
        {
          if ( ChrList.lst[character].canseeinvisible || FF_MUL( chr_get_pinstance(cnt)->alpha, chr_get_pinstance(cnt)->max_light ) > INVISIBLE ) )
          {
            xdistance = (int) (ChrList.lst[cnt].pos.x - ChrList.lst[character].pos.x);
            ydistance = (int) (ChrList.lst[cnt].pos.y - ChrList.lst[character].pos.y);
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
    team = chr_get_iteam(character);
    fanblock = mesh_get_block_int(PMesh, x,y);
    charb = bumplist[fanblock].chr;
    cnt = 0;
    while ( cnt < bumplist[fanblock].chrnum )
    {
      if ( dead != ChrList.lst[charb].alive && ( seeinvisible || FF_MUL( chr_get_pinstance(charb)->alpha, chr_get_pinstance(charb)->max_light ) > INVISIBLE ) ) )
      {
        if ( ( enemies && TeamList[team].hatesteam[chr_get_iteam(charb)] ) ||
             ( items && ChrList.lst[charb].isitem ) ||
             ( friends && chr_get_iteam(charb) == team ) ||
             ( friends && enemies ) )
        {
          if ( charb != character && ChrList.lst[character].attachedto != charb && ChrList.lst[charb].attachedto == MAX_CHR && !ChrList.lst[charb].pack_ispacked )
          {
            if ( !ChrList.lst[charb].invictus || items )
            {
              if ( idsz != IDSZ_NONE )
              {
                if ( chr_get_idsz(charb,IDSZ_PARENT) == idsz ||
                     chr_get_idsz(charb,IDSZ_TYPE) == idsz )
                {
                  xdis = ChrList.lst[character].pos.x - ChrList.lst[charb].pos.x;
                  ydis = ChrList.lst[character].pos.y - ChrList.lst[charb].pos.y;
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
                xdis = ChrList.lst[character].pos.x - ChrList.lst[charb].pos.x;
                ydis = ChrList.lst[character].pos.y - ChrList.lst[charb].pos.y;
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
      charb = ChrList.lst[charb].fanblock_next;
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
  seeinvisible = ChrList.lst[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )ChrList.lst[character].pos.x ) >> BLOCK_BITS;
  y = ( ( int )ChrList.lst[character].pos.y ) >> BLOCK_BITS;

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
  seeinvisible = ChrList.lst[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )ChrList.lst[character].pos.x ) >> BLOCK_BITS;
  y = ( ( int )ChrList.lst[character].pos.y ) >> BLOCK_BITS;
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
