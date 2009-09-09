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
#include "Md2.h"
#include "passage.h"
#include "graphic.h"
#include "mad.h"
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
chop_data_t  chop = {0, 0};
cap_import_t import_data;
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
    Uint16 frame_nxt;
    Sint16 z;

    if ( INVALID_CHR( character ) ) return;

    frame_nxt = ChrList.lst[character].inst.frame_nxt;

    for ( cnt = 0; cnt < MadList[ChrList.lst[character].inst.imad].transvertices; cnt++  )
    {
        z = Md2FrameList[frame_nxt].vrtz[cnt];
        if ( z < low )
        {
            ChrList.lst[character].inst.color_amb = valuelow;
        }
        else
        {
            if ( z > high )
            {
                ChrList.lst[character].inst.color_amb = valuehigh;
            }
            else
            {
                ChrList.lst[character].inst.color_amb = ( valuehigh * ( z - low ) / ( high - low ) ) +
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
        if ( !ChrList.lst[cnt].on ) continue;

        character = ChrList.lst[cnt].attachedto;
        if ( INVALID_CHR(character) )
        {
            ChrList.lst[cnt].attachedto = MAX_CHR;

            // Keep inventory with character
            if ( !ChrList.lst[cnt].pack_ispacked )
            {
                character = ChrList.lst[cnt].pack_next;

                while ( character != MAX_CHR )
                {
                    ChrList.lst[character].pos = ChrList.lst[cnt].pos;

                    // Copy olds to make SendMessageNear work
                    ChrList.lst[character].pos_old = ChrList.lst[cnt].pos_old;

                    character = ChrList.lst[character].pack_next;
                }
            }
        }
        else
        {
            // Keep in hand weapons with character
            if ( ChrList.lst[cnt].inst.matrixvalid )
            {
                ChrList.lst[cnt].pos.x = ChrList.lst[cnt].inst.matrix.CNV( 3, 0 );
                ChrList.lst[cnt].pos.y = ChrList.lst[cnt].inst.matrix.CNV( 3, 1 );
                ChrList.lst[cnt].pos.z = ChrList.lst[cnt].inst.matrix.CNV( 3, 2 );
            }
            else
            {
                ChrList.lst[cnt].pos = ChrList.lst[character].pos;
            }

            ChrList.lst[cnt].turn_z = ChrList.lst[character].turn_z;

            // Copy this stuff ONLY if it's a weapon, not for mounts
            if ( ChrList.lst[character].transferblend && ChrList.lst[cnt].isitem )
            {

                // Items become partially invisible in hands of players
                if ( ChrList.lst[character].isplayer && ChrList.lst[character].inst.alpha != 255 )
                {
                    ChrList.lst[cnt].inst.alpha = SEEINVISIBLE;
                }
                else
                {
                    // Only if not naturally transparent
                    if ( CapList[ChrList.lst[cnt].model].alpha == 255 )
                    {
                        ChrList.lst[cnt].inst.alpha = ChrList.lst[character].inst.alpha;
                    }
                    else
                    {
                        ChrList.lst[cnt].inst.alpha = CapList[ChrList.lst[cnt].model].alpha;
                    }
                }

                // Do light too
                if ( ChrList.lst[character].isplayer && ChrList.lst[character].inst.light != 255 )
                {
                    ChrList.lst[cnt].inst.light = SEEINVISIBLE;
                }
                else
                {
                    // Only if not naturally transparent
                    if ( CapList[ChrList.lst[cnt].model].light == 255 )
                    {
                        ChrList.lst[cnt].inst.light = ChrList.lst[character].inst.light;
                    }
                    else
                    {
                        ChrList.lst[cnt].inst.light = CapList[ChrList.lst[cnt].model].light;
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

    ChrList.lst[cnt].inst.matrixvalid = bfalse;

    if ( ChrList.lst[cnt].overlay )
    {
        // Overlays are kept with their target...
        tnc = ChrList.lst[cnt].ai.target;

        if ( VALID_CHR(tnc) )
        {
            ChrList.lst[cnt].pos = ChrList.lst[tnc].pos;
            ChrList.lst[cnt].inst.matrixvalid = ChrList.lst[tnc].inst.matrixvalid;
            CopyMatrix( &(ChrList.lst[cnt].inst.matrix), &(ChrList.lst[tnc].inst.matrix) );
        }
    }
    else
    {
        ChrList.lst[cnt].inst.matrix = ScaleXYZRotateXYZTranslate( ChrList.lst[cnt].fat, ChrList.lst[cnt].fat, ChrList.lst[cnt].fat,
                                       ChrList.lst[cnt].turn_z >> 2,
                                       ( ( Uint16 ) ( ChrList.lst[cnt].map_turn_x + 32768 ) ) >> 2,
                                       ( ( Uint16 ) ( ChrList.lst[cnt].map_turn_y + 32768 ) ) >> 2,
                                       ChrList.lst[cnt].pos.x, ChrList.lst[cnt].pos.y, ChrList.lst[cnt].pos.z );
        ChrList.lst[cnt].inst.matrixvalid = btrue;
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
    memset( ChrList.lst + ichr, 0, sizeof(enc_t) );

#if defined(DEBUG)
    {
        int cnt;
        // determine whether this texture is already in the list of free textures
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
        // Remove from stat list
        if ( ChrList.lst[character].staton )
        {
            bool_t stat_found;

            ChrList.lst[character].staton = bfalse;

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
            if ( !ChrList.lst[cnt].on || cnt == character ) continue;

            if ( ChrList.lst[cnt].ai.target == character )
            {
                ChrList.lst[cnt].ai.alert |= ALERTIF_TARGETKILLED;
                ChrList.lst[cnt].ai.target = cnt;
            }

            if ( TeamList[ChrList.lst[cnt].team].leader == character )
            {
                ChrList.lst[cnt].ai.alert |= ALERTIF_LEADERKILLED;
            }
        }

        // Handle the team
        if ( ChrList.lst[character].alive && !CapList[ChrList.lst[character].model].invictus )
        {
            TeamList[ChrList.lst[character].baseteam].morale--;
        }

        if ( TeamList[ChrList.lst[character].team].leader == character )
        {
            TeamList[ChrList.lst[character].team].leader = NOLEADER;
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

    Uint16 vertex, model;
    GLvector4 point[1], nupoint[1];

    // Check validity of attachment
    if ( INVALID_CHR(character) || ChrList.lst[character].pack_ispacked )
    {
        PrtList.lst[particle].time   = frame_all + 1;
        PrtList.lst[particle].poofme = btrue;
        return;
    }

    // Do we have a matrix???
    if ( ChrList.lst[character].inst.matrixvalid )// PMesh->mmem.inrenderlist[ChrList.lst[character].onwhichfan])
    {
        // Transform the weapon vertex_offset from model to world space
        model = ChrList.lst[character].inst.imad;

        if ( vertex_offset == GRIP_ORIGIN )
        {
            PrtList.lst[particle].pos.x = ChrList.lst[character].inst.matrix.CNV( 3, 0 );
            PrtList.lst[particle].pos.y = ChrList.lst[character].inst.matrix.CNV( 3, 1 );
            PrtList.lst[particle].pos.z = ChrList.lst[character].inst.matrix.CNV( 3, 2 );
            return;
        }

        vertex = MadList[model].md2_data.vertices - vertex_offset;

        // do the automatic update
        chr_instance_update_vertices( &(ChrList.lst[character].inst), vertex, vertex );

        // Calculate vertex_offset point locations with linear interpolation and other silly things
        point[0].x = ChrList.lst[character].inst.vlst[vertex].pos[XX];
        point[0].y = ChrList.lst[character].inst.vlst[vertex].pos[YY];
        point[0].z = ChrList.lst[character].inst.vlst[vertex].pos[ZZ];
        point[0].w = 1.0f;

        // Do the transform
        TransformVertices( &(ChrList.lst[character].inst.matrix), point, nupoint, 1 );

        PrtList.lst[particle].pos.x = nupoint[0].x;
        PrtList.lst[particle].pos.y = nupoint[0].y;
        PrtList.lst[particle].pos.z = nupoint[0].z;
    }
    else
    {
        // No matrix, so just wing it...
        PrtList.lst[particle].pos = ChrList.lst[character].pos;
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
    pweap = ChrList.lst + iweap;
    pholder = ChrList.lst + iholder;

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
            Uint16 imount;

            if ( !ChrList.lst[ichr].on ) continue;
            if ( ChrList.lst[ichr].inst.matrixvalid ) continue;

            imount = ChrList.lst[ichr].attachedto;
            if ( INVALID_CHR(imount) || imount == ichr ) 
			{ 
				ChrList.lst[ichr].attachedto = MAX_CHR; 
				make_one_character_matrix( ichr );
				continue; 
			}

            // can't evaluate this link yet
            if ( !ChrList.lst[imount].inst.matrixvalid )
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
            if ( !ChrList.lst[ichr].on || !ChrList.lst[ichr].inst.matrixvalid ) continue;

            ChrList.lst[ichr].inst.matrix.CNV( 3, 0 ) = ChrList.lst[ichr].pos.x;
            ChrList.lst[ichr].inst.matrix.CNV( 3, 1 ) = ChrList.lst[ichr].pos.y;
            ChrList.lst[ichr].inst.matrix.CNV( 3, 2 ) = ChrList.lst[ichr].pos.z;
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

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList_free_one( cnt );
    }

	ChrList.free_count = 0;
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
    //    character is not allowed to cross

    Uint32 pass;
    float x, y, bs;
    Uint32 itile;
    int tx_min, tx_max, ty_min, ty_max;
    int ix, iy, tx0, ty0;
    bool_t invalid;

    if ( INVALID_CHR(character) ) return 0;

    if ( 0 == ChrList.lst[character].bumpsize || 0xFFFFFFFF == ChrList.lst[character].weight ) return 0;

    y = ChrList.lst[character].pos.y;
    x = ChrList.lst[character].pos.x;
    bs = ChrList.lst[character].bumpsize >> 1;

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
                    if ( PMesh->mmem.tile_list[itile].fx & ChrList.lst[character].stoppedby )
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

    if ( pass & ChrList.lst[character].stoppedby )
    {
        float dist2 = nrm[XX] * nrm[XX] + nrm[YY] * nrm[YY];
        if ( dist2 > 0 )
        {
            float dist = SQRT( dist2 );
            nrm[XX] /= -dist;
            nrm[YY] /= -dist;
        }
    }

    return pass & ChrList.lst[character].stoppedby;

}

//--------------------------------------------------------------------------------------------
void reset_character_accel( Uint16 character )
{
    // ZZ> This function fixes a character's max acceleration
    Uint16 enchant;

    if ( INVALID_CHR( character ) ) return;

    // Okay, remove all acceleration enchants
    enchant = ChrList.lst[character].firstenchant;
    while ( enchant < MAX_ENC )
    {
        remove_enchant_value( enchant, ADDACCEL );
        enchant = EncList.lst[enchant].nextenchant;
    }

    // Set the starting value
    ChrList.lst[character].maxaccel = CapList[ChrList.lst[character].model].maxaccel[ChrList.lst[character].skin];

    // Put the acceleration enchants back on
    enchant = ChrList.lst[character].firstenchant;
    while ( enchant < MAX_ENC )
    {
        add_enchant_value( enchant, ADDACCEL, EncList.lst[enchant].eve );
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

    // Make sure the character is valid
    if ( INVALID_CHR(character) ) return;

    // Make sure the character is mounted
    mount = ChrList.lst[character].attachedto;
    if ( INVALID_CHR(mount) ) return;

    // Don't allow living characters to drop kursed weapons
    if ( !ignorekurse && ChrList.lst[character].iskursed && ChrList.lst[mount].alive && ChrList.lst[character].isitem )
    {
        ChrList.lst[character].ai.alert |= ALERTIF_NOTDROPPED;
        return;
    }

    // set the dismount timer
    ChrList.lst[character].phys.dismount_timer = PHYS_DISMOUNT_TIME;

    // Figure out which hand it's in
    hand = ChrList.lst[character].inwhich_slot;

    // Rip 'em apart
    ChrList.lst[character].attachedto = MAX_CHR;
    if ( ChrList.lst[mount].holdingwhich[SLOT_LEFT] == character )
        ChrList.lst[mount].holdingwhich[SLOT_LEFT] = MAX_CHR;
    if ( ChrList.lst[mount].holdingwhich[SLOT_RIGHT] == character )
        ChrList.lst[mount].holdingwhich[SLOT_RIGHT] = MAX_CHR;

    if ( ChrList.lst[character].alive )
    {
        // play the falling animation...
        chr_play_action( character, ACTION_JB + hand, bfalse );
    }
    else if ( ChrList.lst[character].action < ACTION_KA || ChrList.lst[character].action > ACTION_KD )
    {
        // play the "killed" animation...
        chr_play_action( character, ACTION_KA + hand, bfalse );
        ChrList.lst[character].keepaction = btrue;
    }

    // Set the positions
    if ( ChrList.lst[character].inst.matrixvalid )
    {
        ChrList.lst[character].pos.x = ChrList.lst[character].inst.matrix.CNV( 3, 0 );
        ChrList.lst[character].pos.y = ChrList.lst[character].inst.matrix.CNV( 3, 1 );
        ChrList.lst[character].pos.z = ChrList.lst[character].inst.matrix.CNV( 3, 2 );
    }
    else
    {
        ChrList.lst[character].pos = ChrList.lst[mount].pos;
    }

    // Make sure it's not dropped in a wall...
    if ( __chrhitawall( character, nrm ) )
    {
        ChrList.lst[character].pos.x = ChrList.lst[mount].pos.x;
        ChrList.lst[character].pos.y = ChrList.lst[mount].pos.y;
    }
    else
    {
        ChrList.lst[character].safe_valid = btrue;
        ChrList.lst[character].pos_safe = ChrList.lst[character].pos;
    }

    // Check for shop passages
    inshop = bfalse;
    if ( ChrList.lst[character].isitem && ShopStack.count > 0 && doshop )
    {
        int ix = ChrList.lst[character].pos.x / TILE_SIZE;
        int iy = ChrList.lst[character].pos.y / TILE_SIZE;

        // This is a hack that makes spellbooks in shops cost correctly
        if ( ChrList.lst[mount].isshopitem ) ChrList.lst[character].isshopitem = btrue;

        owner = shop_get_owner( ix, iy );
        if ( VALID_CHR(owner) ) inshop = btrue;

        if ( inshop )
        {
            // Give the mount its money back, alert the shop owner
            Uint16 skin, icap;

            // Make sure spell books are priced according to their spell and not the book itself
            if ( ChrList.lst[character].model == SPELLBOOK )
            {
                icap = ChrList.lst[character].basemodel;
                skin = 0;
            }
            else
            {
                icap  = ChrList.lst[character].model;
                skin = ChrList.lst[character].skin;
            }
            price = CapList[icap].skincost[skin];

            // Are they are trying to sell junk or quest items?
            if ( price == 0 ) ai_add_order( &(ChrList.lst[owner].ai), (Uint32) price, SHOP_BUY );
            else
            {
                // Items spawned within shops are more valuable
                if (!ChrList.lst[character].isshopitem) price *= 0.5;

                // cost it based on the number/charges of the item
                if ( CapList[icap].isstackable )
                {
                    price *= ChrList.lst[character].ammo;
                }
                else if (CapList[icap].isranged && ChrList.lst[character].ammo < ChrList.lst[character].ammomax)
                {
                    if ( 0 != ChrList.lst[character].ammo )
                    {
                        price *= (float)ChrList.lst[character].ammo / ChrList.lst[character].ammomax;
                    }
                }

                ChrList.lst[mount].money += (Sint16) price;
                ChrList.lst[mount].money  = CLIP(ChrList.lst[mount].money, 0, MAXMONEY);

                ChrList.lst[owner].money -= (Sint16) price;
                ChrList.lst[owner].money  = CLIP(ChrList.lst[owner].money, 0, MAXMONEY);

                ai_add_order( &(ChrList.lst[owner].ai), (Uint32) price, SHOP_BUY );
            }
        }
    }

    // Make sure it works right
    ChrList.lst[character].hitready = btrue;
    if ( inshop )
    {
        // Drop straight down to avoid theft
        ChrList.lst[character].vel.x = 0;
        ChrList.lst[character].vel.y = 0;
    }
    else
    {
        ChrList.lst[character].vel.x = ChrList.lst[mount].vel.x;
        ChrList.lst[character].vel.y = ChrList.lst[mount].vel.y;
    }

    ChrList.lst[character].vel.z = DROPZVEL;

    // Turn looping off
    ChrList.lst[character].loopaction = bfalse;

    // Reset the team if it is a mount
    if ( ChrList.lst[mount].ismount )
    {
        ChrList.lst[mount].team = ChrList.lst[mount].baseteam;
        ChrList.lst[mount].ai.alert |= ALERTIF_DROPPED;
    }

    ChrList.lst[character].team = ChrList.lst[character].baseteam;
    ChrList.lst[character].ai.alert |= ALERTIF_DROPPED;

    // Reset transparency
    if ( ChrList.lst[character].isitem && ChrList.lst[mount].transferblend )
    {
        // Okay, reset transparency
        enchant = ChrList.lst[character].firstenchant;

        while ( enchant < MAX_ENC )
        {
            unset_enchant_value( enchant, SETALPHABLEND );
            unset_enchant_value( enchant, SETLIGHTBLEND );
            enchant = EncList.lst[enchant].nextenchant;
        }

        ChrList.lst[character].inst.alpha = ChrList.lst[character].basealpha;
        ChrList.lst[character].inst.light = CapList[ChrList.lst[character].model].light;
        enchant = ChrList.lst[character].firstenchant;

        while ( enchant < MAX_ENC )
        {
            set_enchant_value( enchant, SETALPHABLEND, EncList.lst[enchant].eve );
            set_enchant_value( enchant, SETLIGHTBLEND, EncList.lst[enchant].eve );
            enchant = EncList.lst[enchant].nextenchant;
        }
    }

    // Set twist
    ChrList.lst[character].map_turn_y = 32768;
    ChrList.lst[character].map_turn_x = 32768;
}
//--------------------------------------------------------------------------------------------
void reset_character_alpha( Uint16 character )
{
    // ZZ> This function fixes an item's transparency
    Uint16 enchant, mount;

    if ( INVALID_CHR( character ) ) return;

    mount = ChrList.lst[character].attachedto;
    if ( INVALID_CHR( mount ) ) return;

    if ( ChrList.lst[character].isitem && ChrList.lst[mount].transferblend )
    {
        // Okay, reset transparency
        enchant = ChrList.lst[character].firstenchant;
        while ( enchant < MAX_ENC )
        {
            unset_enchant_value( enchant, SETALPHABLEND );
            unset_enchant_value( enchant, SETLIGHTBLEND );
            enchant = EncList.lst[enchant].nextenchant;
        }

        ChrList.lst[character].inst.alpha = ChrList.lst[character].basealpha;
        ChrList.lst[character].inst.light = CapList[ChrList.lst[character].model].light;

        enchant = ChrList.lst[character].firstenchant;
        while ( enchant < MAX_ENC )
        {
            set_enchant_value( enchant, SETALPHABLEND, EncList.lst[enchant].eve );
            set_enchant_value( enchant, SETLIGHTBLEND, EncList.lst[enchant].eve );
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
    if( pitem->phys.dismount_timer > 0 ) 
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
    pitem = ChrList.lst + item;

    // don't allow sub-inventories
    if ( pitem->pack_ispacked || pitem->isequipped ) return bfalse;

    if ( INVALID_CAP(pitem->model) ) return bfalse;
    pcap_item = CapList + pitem->model;

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

    if ( CapList[ChrList.lst[item].model].isstackable )
    {
        pack_ispacked = ChrList.lst[character].pack_next;

        allok = bfalse;
        while ( VALID_CHR(pack_ispacked) && !allok )
        {
            allok = btrue;
            if ( ChrList.lst[pack_ispacked].model != ChrList.lst[item].model )
            {
                if ( !CapList[ChrList.lst[pack_ispacked].model].isstackable )
                {
                    allok = bfalse;
                }
                if ( ChrList.lst[pack_ispacked].ammomax != ChrList.lst[item].ammomax )
                {
                    allok = bfalse;
                }

                for ( id = 0; id < IDSZ_COUNT && allok; id++ )
                {
                    if ( CapList[ChrList.lst[pack_ispacked].model].idsz[id] != CapList[ChrList.lst[item].model].idsz[id] )
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
        if ( CapList[ChrList.lst[item].model].usageknown || CapList[ChrList.lst[stack].model].usageknown )
        {
            CapList[ChrList.lst[item].model].usageknown = btrue;
            CapList[ChrList.lst[stack].model].usageknown = btrue;
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
            ChrList.lst[character].ai.alert |= ALERTIF_TOOMUCHBAGGAGE;
        }
    }
    else
    {
        // Make sure we have room for another item
        if ( ChrList.lst[character].pack_count >= MAXNUMINPACK )
        {
            ChrList.lst[character].ai.alert |= ALERTIF_TOOMUCHBAGGAGE;
            return bfalse;
        }

        // Take the item out of hand
        if ( ChrList.lst[item].attachedto != MAX_CHR )
        {
            detach_character_from_mount( item, btrue, bfalse );
            ChrList.lst[item].ai.alert &= ( ~ALERTIF_DROPPED );
        }

        // Remove the item from play
        ChrList.lst[item].hitready = bfalse;
        ChrList.lst[item].pack_ispacked = btrue;

        // Insert the item into the pack as the first one
        oldfirstitem = ChrList.lst[character].pack_next;
        ChrList.lst[character].pack_next = item;
        ChrList.lst[item].pack_next = oldfirstitem;
        ChrList.lst[character].pack_count++;

        if ( CapList[ChrList.lst[item].model].isequipment )
        {
            // AtLastWaypoint doubles as PutAway
            ChrList.lst[item].ai.alert |= ALERTIF_ATLASTWAYPOINT;
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
        ChrList.lst[item].ai.alert |= ALERTIF_NOTPUTAWAY;  // Doubles as IfNotTakenOut

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
        ChrList.lst[item].team = ChrList.lst[character].team;

        // Attach the item to the character's hand
        attach_character_to_mount( item, character, grip_off );
        ChrList.lst[item].ai.alert &= ( ~ALERTIF_GRABBED );
        ChrList.lst[item].ai.alert |= ( ALERTIF_TAKENOUT );
    }

    return item;
}

//--------------------------------------------------------------------------------------------
void drop_keys( Uint16 character )
{
    // ZZ> This function drops all keys ( [KEYA] to [KEYZ] ) that are in a character's
    //    inventory ( Not hands ).

    Uint16 item, lastitem, nextitem, direction;
    IDSZ testa, testz;

    if ( INVALID_CHR( character ) ) return;

    if ( ChrList.lst[character].pos.z > -2 ) // Don't lose keys in pits...
    {
        // The IDSZs to find
        testa = MAKE_IDSZ( 'K', 'E', 'Y', 'A' );  // [KEYA]
        testz = MAKE_IDSZ( 'K', 'E', 'Y', 'Z' );  // [KEYZ]

        lastitem = character;
        item = ChrList.lst[character].pack_next;

        while ( item != MAX_CHR )
        {
            nextitem = ChrList.lst[item].pack_next;
            if ( item != character )  // Should never happen...
            {
                if ( ( CapList[ChrList.lst[item].model].idsz[IDSZ_PARENT] >= testa &&
                        CapList[ChrList.lst[item].model].idsz[IDSZ_PARENT] <= testz ) ||
                        ( CapList[ChrList.lst[item].model].idsz[IDSZ_TYPE] >= testa &&
                          CapList[ChrList.lst[item].model].idsz[IDSZ_TYPE] <= testz ) )
                {
                    // We found a key...
                    ChrList.lst[item].pack_ispacked = bfalse;
                    ChrList.lst[item].isequipped = bfalse;
                    ChrList.lst[lastitem].pack_next = nextitem;
                    ChrList.lst[item].pack_next = MAX_CHR;
                    ChrList.lst[character].pack_count--;
                    ChrList.lst[item].attachedto = MAX_CHR;
                    ChrList.lst[item].ai.alert |= ALERTIF_DROPPED;
                    ChrList.lst[item].hitready = btrue;

                    direction = RANDIE;
                    ChrList.lst[item].turn_z = direction + 32768;
                    ChrList.lst[item].phys.level = ChrList.lst[character].phys.level;
                    ChrList.lst[item].floor_level = ChrList.lst[character].floor_level;
                    ChrList.lst[item].onwhichplatform = ChrList.lst[character].onwhichplatform;
                    ChrList.lst[item].pos   = ChrList.lst[character].pos;
                    ChrList.lst[item].vel.x = turntocos[ (direction >> 2) & TRIG_TABLE_MASK ] * DROPXYVEL;
                    ChrList.lst[item].vel.y = turntosin[ (direction >> 2) & TRIG_TABLE_MASK ] * DROPXYVEL;
                    ChrList.lst[item].vel.z = DROPZVEL;
                    ChrList.lst[item].team = ChrList.lst[item].baseteam;
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

    detach_character_from_mount( ChrList.lst[character].holdingwhich[SLOT_LEFT], btrue, bfalse );
    detach_character_from_mount( ChrList.lst[character].holdingwhich[SLOT_RIGHT], btrue, bfalse );
    if ( ChrList.lst[character].pack_count > 0 )
    {
        direction = ChrList.lst[character].turn_z + 32768;
        diradd = 0xFFFF / ChrList.lst[character].pack_count;

        while ( ChrList.lst[character].pack_count > 0 )
        {
            item = inventory_get_item( character, GRIP_LEFT, bfalse );

            if ( VALID_CHR(item) )
            {
                detach_character_from_mount( item, btrue, btrue );
                ChrList.lst[item].hitready = btrue;
                ChrList.lst[item].ai.alert |= ALERTIF_DROPPED;
                ChrList.lst[item].pos         = ChrList.lst[character].pos;
                ChrList.lst[item].phys.level  = ChrList.lst[character].phys.level;
                ChrList.lst[item].floor_level = ChrList.lst[character].floor_level;
                ChrList.lst[item].onwhichplatform = ChrList.lst[character].onwhichplatform;
                ChrList.lst[item].turn_z = direction + 32768;
                ChrList.lst[item].vel.x = turntocos[ (direction>>2) & TRIG_TABLE_MASK ] * DROPXYVEL;
                ChrList.lst[item].vel.y = turntosin[ (direction>>2) & TRIG_TABLE_MASK ] * DROPXYVEL;
                ChrList.lst[item].vel.z = DROPZVEL;
                ChrList.lst[item].team = ChrList.lst[item].baseteam;
            }

            direction += diradd;
        }
    }

    return btrue;

}

struct s_grab_data
{
    Uint16 ichr;
    float  dist;
};
typedef struct s_grab_data grab_data_t;

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
    Uint16 ichr_b;
    Uint16 model, vertex, frame_nxt;
    slot_t slot;
    GLvector4 point[1], nupoint[1];

    bool_t retval;

    int         cnt;
    int         grab_count = 0;
    grab_data_t grab_list[MAX_CHR];

    // Make life easier
    model = ChrList.lst[ichr_a].model;
    slot = grip_offset_to_slot( grip_off );  // 0 is left, 1 is right

    // Make sure the character doesn't have something already, and that it has hands
    if ( VALID_CHR(ChrList.lst[ichr_a].holdingwhich[slot]) || !CapList[model].slotvalid[slot] )
        return bfalse;

    // Do we have a matrix???
    if ( ChrList.lst[ichr_a].inst.matrixvalid )
    {
        // Transform the weapon grip_off from model to world space
        frame_nxt = ChrList.lst[ichr_a].inst.frame_nxt;
        vertex = MadList[model].md2_data.vertices - grip_off;

        // do the automatic update
        chr_instance_update_vertices( &(ChrList.lst[ichr_a].inst), vertex, vertex );

        // Calculate grip_off point locations with linear interpolation and other silly things
        point[0].x = ChrList.lst[ichr_a].inst.vlst[vertex].pos[XX];
        point[0].y = ChrList.lst[ichr_a].inst.vlst[vertex].pos[YY];
        point[0].z = ChrList.lst[ichr_a].inst.vlst[vertex].pos[ZZ];
        point[0].w = 1.0f;

        // Do the transform
        TransformVertices( &(ChrList.lst[ichr_a].inst.matrix), point, nupoint, 1 );
    }
    else
    {
        // Just wing it
        nupoint[0].x = ChrList.lst[ichr_a].pos.x;
        nupoint[0].y = ChrList.lst[ichr_a].pos.y;
        nupoint[0].z = ChrList.lst[ichr_a].pos.z;
        nupoint[0].w = 1.0f;
    }

    // Go through all characters to find the best match
    for ( ichr_b = 0; ichr_b < MAX_CHR; ichr_b++ )
    {
        GLvector3 pos_b;
        float dx, dy, dz, dxy;

        if ( !ChrList.lst[ichr_b].on ) continue;

        if ( ChrList.lst[ichr_b].pack_ispacked ) continue;              // pickpocket not allowed yet
        if ( MAX_CHR != ChrList.lst[ichr_b].attachedto) continue; // disarm not allowed yet

        if ( ChrList.lst[ichr_b].weight > ChrList.lst[ichr_a].weight + ChrList.lst[ichr_a].strength ) continue; // reasonable carrying capacity

        // grab_people == btrue allows you to pick up living non-items
        // grab_people == false allows you to pick up living (functioning) items
        if ( ChrList.lst[ichr_b].alive && (grab_people == ChrList.lst[ichr_b].isitem) ) continue;

        // do not pick up your mount
        if ( ChrList.lst[ichr_b].holdingwhich[SLOT_LEFT] == ichr_a || ChrList.lst[ichr_b].holdingwhich[SLOT_RIGHT] == ichr_a ) continue;

        pos_b = ChrList.lst[ichr_b].pos;

        // First check absolute value diamond
        dx = ABS( nupoint[0].x - pos_b.x );
        dy = ABS( nupoint[0].y - pos_b.y );
        dz = ABS( nupoint[0].z - pos_b.z );
        dxy = dx + dy;

        if ( dxy > TILE_SIZE * 2 || dz > MAX(ChrList.lst[ichr_b].bumpheight, GRABSIZE) ) continue;

        grab_list[grab_count].ichr = ichr_b;
        grab_list[grab_count].dist = dxy;
        grab_count++;
    }

    if ( 0 == grab_count ) return bfalse;

    // generate billboards for all the things that can be grabbed (5 secs and green)
    for ( cnt = 0; cnt < grab_count; cnt++ )
    {
        SDL_Color color = {0x7F, 0xFF, 0x7F, 0xFF};

        ichr_b = grab_list[cnt].ichr;

        chr_make_text_billboard( ichr_b, chr_get_name(ichr_b), color, 5 );
    }

    // sort the grab list
    if ( grab_count > 1 )
    {
        qsort( grab_list, grab_count, sizeof(grab_data_t), grab_data_cmp );
    }

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

                is_invis  = FF_MUL(ChrList.lst[ichr_a].inst.alpha, ChrList.lst[ichr_a].inst.max_light) < INVISIBLE;
                can_steal = is_invis || ChrList.lst[ichr_a].isitem;

                if ( can_steal )
                {
                    // Pets can try to steal in addition to invisible characters
                    IPair  tmp_rand = {1,100};
                    Uint8  detection = generate_number( tmp_rand );

                    // Check if it was detected. 50% chance +2% per pet DEX and -2% per shopkeeper wisdom. There is always a 5% chance it will fail.
                    if ( ChrList.lst[owner].canseeinvisible || detection <= 5 || detection - ( ChrList.lst[ichr_a].dexterity >> 7 ) + ( ChrList.lst[owner].wisdom >> 7 ) > 50 )
                    {
                        debug_printf( "%s was detected!!", ChrList.lst[ichr_a].name );

                        ai_add_order( &(ChrList.lst[owner].ai), STOLEN, SHOP_THEFT );
                        ChrList.lst[owner].ai.target = ichr_a;
                    }
                    else
                    {
                        debug_printf( "%s stole something! (%s)", ChrList.lst[ichr_a].name, CapList[pchr_b->model].classname );
                    }
                }
                else
                {
                    Uint16 icap, iskin;

                    // Make sure spell books are priced according to their spell and not the book itself
                    if ( pchr_b->model == SPELLBOOK )
                    {
                        icap = pchr_b->basemodel;
                        iskin = 0;
                    }
                    else
                    {
                        icap  = pchr_b->model;
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

                    if ( ChrList.lst[ichr_a].money >= price )
                    {
                        // Okay to sell
                        ai_add_order( &(ChrList.lst[owner].ai), (Uint32) price, SHOP_SELL );

                        ChrList.lst[ichr_a].money -= (Sint16) price;
                        ChrList.lst[ichr_a].money  = CLIP(ChrList.lst[ichr_a].money, 0, MAXMONEY);

                        ChrList.lst[owner].money += (Sint16) price;
                        ChrList.lst[owner].money  = CLIP(ChrList.lst[owner].money, 0, MAXMONEY);
                    }
                    else
                    {
                        // Don't allow purchase
                        ai_add_order( &(ChrList.lst[owner].ai), (Uint32) price, SHOP_NOAFFORD );
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

    weapon = ChrList.lst[cnt].holdingwhich[slot];
    spawngrip = GRIP_LAST;
    action = ChrList.lst[cnt].action;

    // See if it's an unarmed attack...
    if ( weapon == MAX_CHR )
    {
        weapon = cnt;
        spawngrip = slot_to_grip_offset( slot );  // 0 -> GRIP_LEFT, 1 -> GRIP_RIGHT
    }
    if ( weapon != cnt && ( ( CapList[ChrList.lst[weapon].model].isstackable && ChrList.lst[weapon].ammo > 1 ) || ( action >= ACTION_FA && action <= ACTION_FD ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation

        thrown = spawn_one_character( ChrList.lst[cnt].pos, ChrList.lst[weapon].model, ChrList.lst[cnt].team, 0, ChrList.lst[cnt].turn_z, ChrList.lst[weapon].name, MAX_CHR );
        if ( VALID_CHR(thrown) )
        {
            ChrList.lst[thrown].iskursed = bfalse;
            ChrList.lst[thrown].ammo = 1;
            ChrList.lst[thrown].ai.alert |= ALERTIF_THROWN;
            velocity = ChrList.lst[cnt].strength / ( ChrList.lst[thrown].weight * THROWFIX );
            velocity += MINTHROWVELOCITY;
            if ( velocity > MAXTHROWVELOCITY )
            {
                velocity = MAXTHROWVELOCITY;
            }

            tTmp = ChrList.lst[cnt].turn_z >> 2;
            ChrList.lst[thrown].vel.x += turntocos[( tTmp+8192 ) & TRIG_TABLE_MASK] * velocity;
            ChrList.lst[thrown].vel.y += turntosin[( tTmp+8192 ) & TRIG_TABLE_MASK] * velocity;
            ChrList.lst[thrown].vel.z = DROPZVEL;
            if ( ChrList.lst[weapon].ammo <= 1 )
            {
                // Poof the item
                detach_character_from_mount( weapon, btrue, bfalse );
                free_one_character_in_game( weapon );
            }
            else
            {
                ChrList.lst[weapon].ammo--;
            }
        }
    }
    else
    {
        // Spawn an attack particle
        if ( ChrList.lst[weapon].ammomax == 0 || ChrList.lst[weapon].ammo != 0 )
        {
            if ( ChrList.lst[weapon].ammo > 0 && !CapList[ChrList.lst[weapon].model].isstackable )
            {
                ChrList.lst[weapon].ammo--;  // Ammo usage
            }

            // Spawn an attack particle
            if ( CapList[ChrList.lst[weapon].model].attackprttype != -1 )
            {
                particle = spawn_one_particle( ChrList.lst[weapon].pos.x, ChrList.lst[weapon].pos.y, ChrList.lst[weapon].pos.z, ChrList.lst[cnt].turn_z, ChrList.lst[weapon].model, CapList[ChrList.lst[weapon].model].attackprttype, weapon, spawngrip, ChrList.lst[cnt].team, cnt, 0, MAX_CHR );
                if ( particle != TOTAL_MAX_PRT )
                {
                    if ( !CapList[ChrList.lst[weapon].model].attackattached )
                    {
                        // Detach the particle
                        if ( !PipStack.lst[PrtList.lst[particle].pip].startontarget || PrtList.lst[particle].target == MAX_CHR )
                        {
                            attach_particle_to_character( particle, weapon, spawngrip );
                            // Correct Z spacing base, but nothing else...
                            PrtList.lst[particle].pos.z += PipStack.lst[PrtList.lst[particle].pip].zspacing_pair.base;
                        }

                        PrtList.lst[particle].attachedtocharacter = MAX_CHR;

                        // Don't spawn in walls
                        if ( __prthitawall( particle ) )
                        {
                            PrtList.lst[particle].pos.x = ChrList.lst[weapon].pos.x;
                            PrtList.lst[particle].pos.y = ChrList.lst[weapon].pos.y;
                            if ( __prthitawall( particle ) )
                            {
                                PrtList.lst[particle].pos.x = ChrList.lst[cnt].pos.x;
                                PrtList.lst[particle].pos.y = ChrList.lst[cnt].pos.y;
                            }
                        }
                    }
                    else
                    {
                        // Attached particles get a strength bonus for reeling...
                        dampen = REELBASE + ( ChrList.lst[cnt].strength / REEL );
                        PrtList.lst[particle].vel.x = PrtList.lst[particle].vel.x * dampen;
                        PrtList.lst[particle].vel.y = PrtList.lst[particle].vel.y * dampen;
                        PrtList.lst[particle].vel.z = PrtList.lst[particle].vel.z * dampen;
                    }

                    // Initial particles get a strength bonus, which may be 0.00f
                    PrtList.lst[particle].damage.base += ( ChrList.lst[cnt].strength * CapList[ChrList.lst[weapon].model].strengthdampen );

                    // Initial particles get an enchantment bonus
                    PrtList.lst[particle].damage.base += ChrList.lst[weapon].damageboost;

                    // Initial particles inherit damage type of weapon
                    // PrtList.lst[particle].damagetype = ChrList.lst[weapon].damagetargettype;  // Zefz: not sure if we want this. we can have weapons with different damage types
                }
            }
        }
        else
        {
            ChrList.lst[weapon].ammoknown = btrue;
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

    team = ChrList.lst[character].team;
    TeamList[team].sissy = character;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( ChrList.lst[cnt].on && cnt != character && !TeamList[ChrList.lst[cnt].team].hatesteam[team] )
        {
            ChrList.lst[cnt].ai.alert |= ALERTIF_CALLEDFORHELP;
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t setup_xp_table(Uint16 profile )
{
    // This calculates the xp needed to reach next level and stores it in an array for later use
    Uint8 level;

    if ( INVALID_CAP(profile) ) return bfalse;

    // Calculate xp needed
    for (level = MAXBASELEVEL; level <= MAXLEVEL; level++)
    {
        Uint32 xpneeded = CapList[profile].experienceforlevel[MAXBASELEVEL - 1];
        xpneeded += ( level * level * level * 15 );
        xpneeded -= ( ( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * 15 );
        CapList[profile].experienceforlevel[level] = xpneeded;
    }
    return btrue;
}

//--------------------------------------------------------------------------------------------
void do_level_up( Uint16 character )
{
    // BB > level gains are done here, but only once a second

    Uint8 curlevel;
    int number;
    Uint16 profile;

    if ( INVALID_CHR(character) ) return;

    profile = ChrList.lst[character].model;
    if ( INVALID_CAP(profile) ) return;

    // Do level ups and stat changes
    curlevel = ChrList.lst[character].experiencelevel + 1;
    if ( curlevel < MAXLEVEL )
    {
        Uint32 xpcurrent, xpneeded;

        xpcurrent = ChrList.lst[character].experience;
        xpneeded  = CapList[profile].experienceforlevel[curlevel];
        if ( xpcurrent >= xpneeded )
        {
            // do the level up
            ChrList.lst[character].experiencelevel++;
            xpneeded = CapList[profile].experienceforlevel[curlevel];

            // The character is ready to advance...
            if ( ChrList.lst[character].isplayer )
            {
                debug_printf( "%s gained a level!!!", ChrList.lst[character].name );
                sound_play_chunk( PCamera->track_pos, g_wavelist[GSND_LEVELUP] );
            }

            // Size
            ChrList.lst[character].sizegoto += CapList[profile].sizeperlevel * 0.25f;  // Limit this?
            ChrList.lst[character].sizegototime += SIZETIME;

            // Strength
            number = generate_number( CapList[profile].strength_stat.perlevel );
            number += ChrList.lst[character].strength;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList.lst[character].strength = number;

            // Wisdom
            number = generate_number( CapList[profile].wisdom_stat.perlevel );
            number += ChrList.lst[character].wisdom;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList.lst[character].wisdom = number;

            // Intelligence
            number = generate_number( CapList[profile].intelligence_stat.perlevel );
            number += ChrList.lst[character].intelligence;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList.lst[character].intelligence = number;

            // Dexterity
            number = generate_number( CapList[profile].dexterity_stat.perlevel );
            number += ChrList.lst[character].dexterity;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList.lst[character].dexterity = number;

            // Life
            number = generate_number( CapList[profile].life_stat.perlevel );
            number += ChrList.lst[character].lifemax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            ChrList.lst[character].life += ( number - ChrList.lst[character].lifemax );
            ChrList.lst[character].lifemax = number;

            // Mana
            number = generate_number( CapList[profile].mana_stat.perlevel );
            number += ChrList.lst[character].manamax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            ChrList.lst[character].mana += ( number - ChrList.lst[character].manamax );
            ChrList.lst[character].manamax = number;

            // Mana Return
            number = generate_number( CapList[profile].manareturn_stat.perlevel );
            number += ChrList.lst[character].manareturn;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList.lst[character].manareturn = number;

            // Mana Flow
            number = generate_number( CapList[profile].manaflow_stat.perlevel );
            number += ChrList.lst[character].manaflow;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            ChrList.lst[character].manaflow = number;
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

    if ( !ChrList.lst[character].invictus || override_invictus )
    {
        float intadd = (FP8_TO_INT(ChrList.lst[character].intelligence) - 10.0f) / 200.0f;
        float wisadd = (FP8_TO_INT(ChrList.lst[character].wisdom) - 10.0f)       / 400.0f;

        // Figure out how much experience to give
        profile = ChrList.lst[character].model;
        newamount = amount;
        if ( xptype < XP_COUNT )
        {
            newamount = amount * CapList[profile].experiencerate[xptype];
        }

        // Intelligence and slightly wisdom increases xp gained (0,5% per int and 0,25% per wisdom above 10)
        newamount *= 1.00f + intadd + wisadd;

        // Apply XP bonus/penality depending on game difficulty
        if ( cfg.difficulty >= GAME_HARD ) newamount *= 1.10f;          // 10% extra on hard

        ChrList.lst[character].experience += newamount;
    }
}

//--------------------------------------------------------------------------------------------
void give_team_experience( Uint8 team, int amount, Uint8 xptype )
{
    // ZZ> This function gives every character on a team experience
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( ChrList.lst[cnt].team == team && ChrList.lst[cnt].on )
        {
            give_experience( cnt, amount, xptype, bfalse );
        }
    }
}

//--------------------------------------------------------------------------------------------
void resize_characters()
{
    // ZZ> This function makes the characters get bigger or smaller, depending
    //    on their sizegoto and sizegototime
    int cnt = 0;
    bool_t willgetcaught;
    float newsize;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ChrList.lst[cnt].on || ChrList.lst[cnt].sizegototime != 0 ) continue;

        if ( ChrList.lst[cnt].sizegoto != ChrList.lst[cnt].fat )
        {
            int bump_increase;
            float nrm[2];

            bump_increase = ( ChrList.lst[cnt].sizegoto - ChrList.lst[cnt].fat ) * 0.10f * ChrList.lst[cnt].bumpsize;

            // Make sure it won't get caught in a wall
            willgetcaught = bfalse;
            if ( ChrList.lst[cnt].sizegoto > ChrList.lst[cnt].fat )
            {
                ChrList.lst[cnt].bumpsize += bump_increase;

                if ( __chrhitawall( cnt, nrm ) )
                {
                    willgetcaught = btrue;
                }

                ChrList.lst[cnt].bumpsize -= bump_increase;
            }

            // If it is getting caught, simply halt growth until later
            if ( !willgetcaught )
            {
                // Figure out how big it is
                ChrList.lst[cnt].sizegototime--;

                newsize = ChrList.lst[cnt].sizegoto;
                if ( ChrList.lst[cnt].sizegototime > 0 )
                {
                    newsize = ( ChrList.lst[cnt].fat * 0.90f ) + ( newsize * 0.10f );
                }

                // Make it that big...
                ChrList.lst[cnt].fat   = newsize;
                ChrList.lst[cnt].shadowsize = ChrList.lst[cnt].shadowsizesave * newsize;
                ChrList.lst[cnt].bumpsize = ChrList.lst[cnt].bumpsizesave * newsize;
                ChrList.lst[cnt].bumpsizebig = ChrList.lst[cnt].bumpsizebigsave * newsize;
                ChrList.lst[cnt].bumpheight = ChrList.lst[cnt].bumpheightsave * newsize;

                if ( CapList[ChrList.lst[cnt].model].weight == 0xFF )
                {
                    ChrList.lst[cnt].weight = 0xFFFFFFFF;
                }
                else
                {
                    Uint32 itmp = CapList[ChrList.lst[cnt].model].weight * ChrList.lst[cnt].fat * ChrList.lst[cnt].fat * ChrList.lst[cnt].fat;
                    ChrList.lst[cnt].weight = MIN( itmp, (Uint32)0xFFFFFFFE );
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
    icap = ChrList.lst[character].model;
    if ( INVALID_CAP(icap) ) return bfalse;
    pcap = CapList + icap;

    // Open the file
    filewrite = vfs_openWrite( szSaveName );
    if ( NULL == filewrite ) return bfalse;

    // Real general data
    vfs_printf( filewrite, "Slot number    : -1\n" );  // -1 signals a flexible load thing
    funderf( filewrite, "Class name     : ", pcap->classname );
    ftruthf( filewrite, "Uniform light  : ", pcap->uniformlit );
    vfs_printf( filewrite, "Maximum ammo   : %d\n", pcap->ammomax );
    vfs_printf( filewrite, "Current ammo   : %d\n", pchr->ammo );
    fgendef( filewrite, "Gender         : ", pchr->gender );
    vfs_printf( filewrite, "\n" );

    // Object stats
    vfs_printf( filewrite, "Life color     : %d\n", pchr->lifecolor );
    vfs_printf( filewrite, "Mana color     : %d\n", pchr->manacolor );
    vfs_printf( filewrite, "Life           : %4.2f\n", pchr->lifemax / 256.0f );
    fpairof( filewrite, "Life up        : ", pcap->life_stat.perlevel );
    vfs_printf( filewrite, "Mana           : %4.2f\n", pchr->manamax / 256.0f );
    fpairof( filewrite, "Mana up        : ", pcap->mana_stat.perlevel );
    vfs_printf( filewrite, "Mana return    : %4.2f\n", pchr->manareturn / 256.0f );
    fpairof( filewrite, "Mana return up : ", pcap->manareturn_stat.perlevel );
    vfs_printf( filewrite, "Mana flow      : %4.2f\n", pchr->manaflow / 256.0f );
    fpairof( filewrite, "Mana flow up   : ", pcap->manaflow_stat.perlevel );
    vfs_printf( filewrite, "STR            : %4.2f\n", pchr->strength / 256.0f );
    fpairof( filewrite, "STR up         : ", pcap->strength_stat.perlevel );
    vfs_printf( filewrite, "WIS            : %4.2f\n", pchr->wisdom / 256.0f );
    fpairof( filewrite, "WIS up         : ", pcap->wisdom_stat.perlevel );
    vfs_printf( filewrite, "INT            : %4.2f\n", pchr->intelligence / 256.0f );
    fpairof( filewrite, "INT up         : ", pcap->intelligence_stat.perlevel );
    vfs_printf( filewrite, "DEX            : %4.2f\n", pchr->dexterity / 256.0f );
    fpairof( filewrite, "DEX up         : ", pcap->dexterity_stat.perlevel );
    vfs_printf( filewrite, "\n" );

    // More physical attributes
    vfs_printf( filewrite, "Size           : %4.2f\n", pchr->sizegoto );
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
    ftruthf( filewrite, "Transfer blend : ", pcap->transferblend );
    vfs_printf( filewrite, "Sheen          : %d\n", pcap->sheen );
    ftruthf( filewrite, "Phong mapping  : ", pcap->enviro );
    vfs_printf( filewrite, "Texture X add  : %4.2f\n", pcap->uoffvel / (float)0xFFFF );
    vfs_printf( filewrite, "Texture Y add  : %4.2f\n", pcap->voffvel / (float)0xFFFF );
    ftruthf( filewrite, "Sticky butt    : ", pcap->stickybutt );
    vfs_printf( filewrite, "\n" );

    // Invulnerability data
    ftruthf( filewrite, "Invictus       : ", pcap->invictus );
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
    vfs_printf( filewrite, "\n" );

    // Other item and damage stuff
    fdamagf( filewrite, "Damage type    : ", pcap->damagetargettype );
    factiof( filewrite, "Attack type    : ", pcap->weaponaction );
    vfs_printf( filewrite, "\n" );

    // Particle attachments
    vfs_printf( filewrite, "Attached parts : %d\n", pcap->attachedprtamount );
    fdamagf( filewrite, "Reaffirm type  : ", pcap->attachedprtreaffirmdamagetype );
    vfs_printf( filewrite, "Particle type  : %d\n", pcap->attachedprttype );
    vfs_printf( filewrite, "\n" );

    // Character hands
    ftruthf( filewrite, "Left valid     : ", pcap->slotvalid[SLOT_LEFT] );
    ftruthf( filewrite, "Right valid    : ", pcap->slotvalid[SLOT_RIGHT] );
    vfs_printf( filewrite, "\n" );

    // Particle spawning on attack
    ftruthf( filewrite, "Part on weapon : ", pcap->attackattached );
    vfs_printf( filewrite, "Part type      : %d\n", pcap->attackprttype );
    vfs_printf( filewrite, "\n" );

    // Particle spawning for GoPoof
    vfs_printf( filewrite, "Poof amount    : %d\n", pcap->gopoofprtamount );
    vfs_printf( filewrite, "Facing add     : %d\n", pcap->gopoofprtfacingadd );
    vfs_printf( filewrite, "Part type      : %d\n", pcap->gopoofprttype );
    vfs_printf( filewrite, "\n" );

    // Particle spawning for blud
    ftruthf( filewrite, "Blud valid     : ", pcap->bludvalid );
    vfs_printf( filewrite, "Part type      : %d\n", pcap->bludprttype );
    vfs_printf( filewrite, "\n" );

    // Extra stuff
    ftruthf( filewrite, "Waterwalking   : ", pcap->waterwalk );
    vfs_printf( filewrite, "Bounce dampen  : %5.3f\n", pcap->dampen );
    vfs_printf( filewrite, "\n" );

    // More stuff
    vfs_printf( filewrite, "NOT USED       : %5.3f\n", pcap->lifeheal / 256.0f );       // These two are seriously outdated
    vfs_printf( filewrite, "NOT USED       : %5.3f\n", pcap->manacost / 256.0f );       // and shouldnt be used. Use scripts instead.
    vfs_printf( filewrite, "Regeneration   : %d\n", pcap->lifereturn );
    vfs_printf( filewrite, "Stopped by     : %d\n", pcap->stoppedby );
    funderf( filewrite, "Skin 0 name    : ", pcap->skinname[0] );
    funderf( filewrite, "Skin 1 name    : ", pcap->skinname[1] );
    funderf( filewrite, "Skin 2 name    : ", pcap->skinname[2] );
    funderf( filewrite, "Skin 3 name    : ", pcap->skinname[3] );
    vfs_printf( filewrite, "Skin 0 cost    : %d\n", pcap->skincost[0] );
    vfs_printf( filewrite, "Skin 1 cost    : %d\n", pcap->skincost[1] );
    vfs_printf( filewrite, "Skin 2 cost    : %d\n", pcap->skincost[2] );
    vfs_printf( filewrite, "Skin 3 cost    : %d\n", pcap->skincost[3] );
    vfs_printf( filewrite, "STR dampen     : %5.3f\n", pcap->strengthdampen );
    vfs_printf( filewrite, "\n" );

    // Another memory lapse
    ftruthf( filewrite, "No rider attak : ", btrue - pcap->ridercanattack );
    ftruthf( filewrite, "Can be dazed   : ", pcap->canbedazed );
    ftruthf( filewrite, "Can be grogged : ", pcap->canbegrogged );
    vfs_printf( filewrite, "NOT USED       : 0\n" );
    vfs_printf( filewrite, "NOT USED       : 0\n" );
    ftruthf( filewrite, "Can see invisi : ", pcap->canseeinvisible );
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
    // the object slot that the profile was stuck into.  It may cause the program
    // to abort if bad things happen.

    vfs_FILE* fileread;
    Sint16 object = -1;
    int iTmp;
    char cTmp;
    Uint8 damagetype, level, xptype;
    int idsz_cnt;
    IDSZ idsz;
    cap_t * pcap;
    int cnt;
    STRING szLoadName;

    make_newloadname( tmploadname, SLASH_STR "data.txt", szLoadName );

    // Open the file
    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread )
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

    parse_filename = szLoadName;  // For debugging goto_colon()

    // load the object's slot no matter what
    iTmp = fget_next_int( fileread );

    if( !VALID_CAP_RANGE(slot_override) )
    {
        // set the object slot
        object = iTmp;
        
        if ( object < 0 )
        {
            if ( import_data.object < 0 )
            {
                log_warning( "Object slot number cannot be negative (%s)\n", szLoadName );
            }
            else
            {
                object = import_data.object;
            }
        }
    }
    else
    {
        // just use the slot that was provided
        object = slot_override;

        // tell the cap that it is not loaded to avoid the error that will be generated below
        CapList[object].loaded = bfalse;

        // ?? do we need to free anything a cap_t, pip_t, mad_t or any other profile thing ??
    }

    if ( !VALID_CAP_RANGE( object ) ) return MAX_PROFILE;
    pcap = CapList + object;

    // Make sure global objects don't load over existing models
    if ( pcap->loaded )
    {
        if ( required && SPELLBOOK == object )
        {
            log_error( "Object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, szLoadName );
        }
        else if ( required && overrideslots )
        {
            log_error( "Object slot %i used twice (%s, %s)\n", object, pcap->name, szLoadName );
        }
        else 
        {
            // Stop, we don't want to override it
            return MAX_PROFILE;   
        }
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
    fget_next_name( fileread, pcap->classname, SDL_arraysize(pcap->classname) );

    // Light cheat
    pcap->uniformlit = fget_next_bool( fileread );
    if( cfg.gourard_req ) pcap->uniformlit = bfalse;

    // Ammo
    pcap->ammomax = fget_next_int( fileread );
    pcap->ammo = fget_next_int( fileread );

    // Gender
    cTmp = fget_next_char( fileread );
    pcap->gender = GENDER_OTHER;
    if ( 'F' == toupper(cTmp) )  pcap->gender = GENDER_FEMALE;
    if ( 'M' == toupper(cTmp) )  pcap->gender = GENDER_MALE;
    if ( 'R' == toupper(cTmp) )  pcap->gender = GENDER_RANDOM;

    // Read in the object stats
    pcap->lifecolor = fget_next_int( fileread );
    pcap->manacolor = fget_next_int( fileread );

    fget_next_pair( fileread ); pcap->life_stat.val = pair;
    fget_next_pair( fileread ); pcap->life_stat.perlevel = pair;

    fget_next_pair( fileread ); pcap->mana_stat.val = pair;
    fget_next_pair( fileread ); pcap->mana_stat.perlevel = pair;

    fget_next_pair( fileread ); pcap->manareturn_stat.val = pair;
    fget_next_pair( fileread ); pcap->manareturn_stat.perlevel = pair;

    fget_next_pair( fileread ); pcap->manaflow_stat.val = pair;
    fget_next_pair( fileread ); pcap->manaflow_stat.perlevel = pair;

    fget_next_pair( fileread ); pcap->strength_stat.val = pair;
    fget_next_pair( fileread ); pcap->strength_stat.perlevel = pair;

    fget_next_pair( fileread ); pcap->wisdom_stat.val = pair;
    fget_next_pair( fileread ); pcap->wisdom_stat.perlevel = pair;

    fget_next_pair( fileread ); pcap->intelligence_stat.val = pair;
    fget_next_pair( fileread ); pcap->intelligence_stat.perlevel = pair;

    fget_next_pair( fileread ); pcap->dexterity_stat.val = pair;
    fget_next_pair( fileread ); pcap->dexterity_stat.perlevel = pair;

    // More physical attributes
    pcap->size = fget_next_float( fileread );
    pcap->sizeperlevel = fget_next_float( fileread );
    pcap->shadowsize = fget_next_int( fileread );
    pcap->bumpsize = fget_next_int( fileread );
    pcap->bumpheight = fget_next_int( fileread );
    pcap->bumpdampen = fget_next_float( fileread );  pcap->bumpdampen = MAX(0.01, pcap->bumpdampen);
    pcap->weight = fget_next_int( fileread );
    pcap->jump = fget_next_float( fileread );
    pcap->jumpnumber = fget_next_int( fileread );
    pcap->sneakspd = fget_next_int( fileread );
    pcap->walkspd = fget_next_int( fileread );
    pcap->runspd = fget_next_int( fileread );
    pcap->flyheight = fget_next_int( fileread );
    pcap->flashand = fget_next_int( fileread );
    pcap->alpha = fget_next_int( fileread );
    pcap->light = fget_next_int( fileread );
    pcap->transferblend = fget_next_bool( fileread );

    pcap->sheen = fget_next_int( fileread );
    pcap->enviro = fget_next_bool( fileread );

    pcap->uoffvel = fget_next_float( fileread ) * 0xFFFF;
    pcap->voffvel = fget_next_float( fileread ) * 0xFFFF;
    pcap->stickybutt = fget_next_bool( fileread );

    // Invulnerability data
    pcap->invictus = fget_next_bool( fileread );

    pcap->nframefacing = fget_next_int( fileread );
    pcap->nframeangle = fget_next_int( fileread );
    pcap->iframefacing = fget_next_int( fileread );
    pcap->iframeangle = fget_next_int( fileread );

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
    for( cnt=0; cnt<MAXSKIN; cnt++ )
    {
        pcap->defense[cnt] = 255 - fget_int( fileread );
    }

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        goto_colon( NULL, fileread, bfalse );
        for( cnt=0; cnt<MAXSKIN; cnt++ )
        {
            pcap->damagemodifier[damagetype][cnt] = fget_int( fileread );
        }
    }

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        goto_colon( NULL, fileread, bfalse );

        for( cnt=0; cnt<MAXSKIN; cnt++ )
        {
            cTmp = fget_first_letter( fileread );
                 if ( 'T' == toupper(cTmp) )  pcap->damagemodifier[damagetype][cnt] |= DAMAGEINVERT;
            else if ( 'C' == toupper(cTmp) )  pcap->damagemodifier[damagetype][cnt] |= DAMAGECHARGE;
            else if ( 'M' == toupper(cTmp) )  pcap->damagemodifier[damagetype][cnt] |= DAMAGEMANA;
        }
    }

    goto_colon( NULL, fileread, bfalse );
    for( cnt=0; cnt<MAXSKIN; cnt++ )
    {
        pcap->maxaccel[cnt] = fget_float( fileread ) / 80.0f;
    }

    // Experience and level data
    pcap->experienceforlevel[0] = 0;
    for ( level = 1; level < MAXBASELEVEL; level++ )
    {
        pcap->experienceforlevel[level] = fget_next_int( fileread );
    }
    setup_xp_table(object);         //Do the rest of the levels not listed in data.txt

    fget_next_pair( fileread ); pcap->experience = pair;
    pcap->experience.base >>= 8;
    pcap->experience.rand >>= 8;
    if ( pcap->experience.rand < 1 )  pcap->experience.rand = 1;

    pcap->experienceworth    = fget_next_int( fileread );
    pcap->experienceexchange = fget_next_float( fileread );

    for ( xptype = 0; xptype < XP_COUNT; xptype++ )
    {
        pcap->experiencerate[xptype] = fget_next_float( fileread ) + 0.001f;
    }

    // IDSZ tags
    for ( idsz_cnt = 0; idsz_cnt < IDSZ_COUNT; idsz_cnt++ )
    {
        pcap->idsz[idsz_cnt] = fget_next_idsz( fileread );
    }

    // Item and damage flags
    pcap->isitem = fget_next_bool( fileread ); 
    pcap->ismount = fget_next_bool( fileread );
    pcap->isstackable = fget_next_bool( fileread );
    pcap->nameknown = fget_next_bool( fileread );
    pcap->usageknown = fget_next_bool( fileread );
    pcap->cancarrytonextmodule = fget_next_bool( fileread );
    pcap->needskillidtouse = fget_next_bool( fileread );
    pcap->platform = fget_next_bool( fileread );
    pcap->cangrabmoney = fget_next_bool( fileread );
    pcap->canopenstuff = fget_next_bool( fileread );

    pcap->ripple = !pcap->isitem;

    // More item and damage stuff
    pcap->damagetargettype = fget_next_damage_type( fileread );
    pcap->weaponaction     = action_which( fget_next_char( fileread ) );

    // Particle attachments
    pcap->attachedprtamount             = fget_next_int( fileread );
    pcap->attachedprtreaffirmdamagetype = fget_next_damage_type( fileread );
    pcap->attachedprttype               = fget_next_int( fileread );

    // Character hands
    pcap->slotvalid[SLOT_LEFT]  = fget_next_bool( fileread );
    pcap->slotvalid[SLOT_RIGHT] = fget_next_bool( fileread );

    // Attack order ( weapon )
    pcap->attackattached = fget_next_bool( fileread );
    pcap->attackprttype  = fget_next_int( fileread );

    // GoPoof
    pcap->gopoofprtamount    = fget_next_int( fileread );
    pcap->gopoofprtfacingadd = fget_next_int( fileread );
    pcap->gopoofprttype      = fget_next_int( fileread );

    // Blud
    cTmp = fget_next_char( fileread );
    pcap->bludvalid = bfalse;
    if ( 'T' == toupper(cTmp) )  pcap->bludvalid = btrue;
    if ( 'U' == toupper(cTmp) )  pcap->bludvalid = ULTRABLUDY;

    pcap->bludprttype = fget_next_int( fileread );

    // Stuff I forgot
    pcap->waterwalk = fget_next_bool( fileread );
    pcap->dampen    = fget_next_float( fileread );

    // More stuff I forgot
    pcap->lifeheal   = fget_next_float( fileread ) * 256;
    pcap->manacost   = fget_next_float( fileread ) * 256;
    pcap->lifereturn = fget_next_int( fileread );
    pcap->stoppedby  = fget_next_int( fileread ) | MPDFX_IMPASS;
    for(cnt=0; cnt<MAXSKIN; cnt++)
    {
        fget_next_name( fileread, pcap->skinname[cnt], sizeof(pcap->skinname[cnt]) );
    }
    for(cnt=0; cnt<MAXSKIN; cnt++)
    {
        pcap->skincost[cnt] = fget_next_int( fileread );
    }
    pcap->strengthdampen = fget_next_float( fileread );

    // Another memory lapse
    pcap->ridercanattack = !fget_next_bool( fileread );
    pcap->canbedazed     =  fget_next_bool( fileread );
    pcap->canbegrogged   =  fget_next_bool( fileread );

    goto_colon( NULL, fileread, bfalse );  // !!!BAD!!! Life add
    goto_colon( NULL, fileread, bfalse );  // !!!BAD!!! Mana add
    pcap->canseeinvisible = fget_next_bool( fileread );

    pcap->kursechance = fget_next_int( fileread );

    iTmp = fget_next_int( fileread );  // Footfall sound
    pcap->soundindex[SOUND_FOOTFALL] = CLIP(iTmp, -1, MAX_WAVE);

    iTmp = fget_next_int( fileread );  // Jump sound
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
    pcap->bumpsizebig = pcap->bumpsize * SQRT_TWO;
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
    pcap->spawnlife = PERFECTBIG;
    pcap->spawnmana = PERFECTBIG;

    // Skills
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

        if ( idsz == MAKE_IDSZ( 'D', 'R', 'E', 'S' ) ) pcap->skindressy |= 1 << fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'G', 'O', 'L', 'D' ) ) pcap->money = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'T', 'U', 'K' ) ) pcap->resistbumpspawn = 1 - fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'P', 'A', 'C', 'K' ) ) pcap->istoobig = 1 - fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'V', 'A', 'M', 'P' ) ) pcap->reflect = 1 - fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'D', 'R', 'A', 'W' ) ) pcap->alwaysdraw = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'R', 'A', 'N', 'G' ) ) pcap->isranged = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'H', 'I', 'D', 'E' ) ) pcap->hidestate = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'E', 'Q', 'U', 'I' ) ) pcap->isequipment = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'Q', 'U', 'A' ) ) pcap->bumpsizebig = pcap->bumpsize * 2;
        else if ( idsz == MAKE_IDSZ( 'I', 'C', 'O', 'N' ) ) pcap->icon = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'H', 'A', 'D' ) ) pcap->forceshadow = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'C', 'K', 'U', 'R' ) ) pcap->canseekurse = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'K', 'I', 'N' ) ) pcap->skinoverride = fget_int( fileread ) & 3;
        else if ( idsz == MAKE_IDSZ( 'C', 'O', 'N', 'T' ) ) pcap->contentoverride = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'T', 'A', 'T' ) ) pcap->stateoverride = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'L', 'E', 'V', 'L' ) ) pcap->leveloverride = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'P', 'L', 'A', 'T' ) ) pcap->canuseplatforms = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'R', 'I', 'P', 'P' ) ) pcap->ripple = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'V', 'A', 'L', 'U' ) ) pcap->isvaluable = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'L', 'I', 'F', 'E' ) ) pcap->spawnlife = 256 * fget_float( fileread );
        else if ( idsz == MAKE_IDSZ( 'M', 'A', 'N', 'A' ) ) pcap->spawnmana = 256 * fget_float( fileread );
        else if ( idsz == MAKE_IDSZ( 'B', 'O', 'O', 'K' ) ) pcap->is_spelleffect = fget_int( fileread );

        // Read Skills
        else if ( idsz == MAKE_IDSZ( 'A', 'W', 'E', 'P' ) ) pcap->canuseadvancedweapons = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'H', 'P', 'R' ) ) pcap->shieldproficiency = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'J', 'O', 'U', 'S' ) ) pcap->canjoust = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'W', 'M', 'A', 'G' ) ) pcap->canusearcane = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'H', 'M', 'A', 'G' ) ) pcap->canusedivine = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'T', 'E', 'C', 'H' ) ) pcap->canusetech = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'D', 'I', 'S', 'A' ) ) pcap->candisarm = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'T', 'A', 'B' ) ) pcap->canbackstab = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'P', 'O', 'I', 'S' ) ) pcap->canusepoison = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'R', 'E', 'A', 'D' ) ) pcap->canread = fget_int( fileread );
    }

    vfs_close( fileread );

    // Load the random naming table for this object
    make_newloadname( tmploadname, SLASH_STR "naming.txt", szLoadName );
    chop_load( object, szLoadName );

    //log_info( "load_one_character_profile() - loaded object %s (%d)\n", pcap->classname, object );

    return object;
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
        ChrList.lst[character].ai.alert |= ALERTIF_HEALED;
        ChrList.lst[character].ai.attacklast = healer;
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
    Uint16 experience, model, left, right;

    if ( INVALID_CHR(character) ) return;

    if ( ChrList.lst[character].alive && damage.base >= 0 && damage.rand >= 1 )
    {
        // Lessen actual_damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
        // This can also be used to lessen effectiveness of healing
        actual_damage = generate_number( damage );
        base_damage   = actual_damage;
        actual_damage = actual_damage >> ( ChrList.lst[character].damagemodifier[damagetype] & DAMAGESHIFT );

        // Allow actual_damage to be dealt to mana (mana shield spell)
        if ( ChrList.lst[character].damagemodifier[damagetype]&DAMAGEMANA )
        {
            int manadamage;
            manadamage = MAX(ChrList.lst[character].mana - actual_damage, 0);
            ChrList.lst[character].mana = manadamage;
            actual_damage -= manadamage;
            ChrList.lst[character].ai.alert |= ALERTIF_ATTACKED;
            ChrList.lst[character].ai.attacklast = attacker;
        }

        // Allow charging (Invert actual_damage to mana)
        if ( ChrList.lst[character].damagemodifier[damagetype]&DAMAGECHARGE )
        {
            ChrList.lst[character].mana += actual_damage;
            if ( ChrList.lst[character].mana > ChrList.lst[character].manamax )
            {
                ChrList.lst[character].mana = ChrList.lst[character].manamax;
            }
            return;
        }

        // Invert actual_damage to heal
        if ( ChrList.lst[character].damagemodifier[damagetype]&DAMAGEINVERT )
            actual_damage = -actual_damage;

        // Remember the actual_damage type
        ChrList.lst[character].ai.damagetypelast = damagetype;
        ChrList.lst[character].ai.directionlast = direction;

        // Do it already
        if ( actual_damage > 0 )
        {
            // Only actual_damage if not invincible
            if ( (0 == ChrList.lst[character].damagetime || ignoreinvincible) && !ChrList.lst[character].invictus )
            {
                model = ChrList.lst[character].model;

                // Hard mode deals 25% extra actual_damage to players!
                if ( cfg.difficulty >= GAME_HARD && ChrList.lst[character].isplayer && !ChrList.lst[attacker].isplayer ) actual_damage *= 1.25f;

                // East mode deals 25% extra actual_damage by players and 25% less to players
                if ( cfg.difficulty <= GAME_EASY )
                {
                    if ( ChrList.lst[attacker].isplayer && !ChrList.lst[character].isplayer ) actual_damage *= 1.25f;
                    if ( !ChrList.lst[attacker].isplayer && ChrList.lst[character].isplayer ) actual_damage *= 0.75f;
                }

                if ( HAS_NO_BITS( effects, DAMFX_NBLOC ) )
                {
                    // Only actual_damage if hitting from proper direction
                    if ( Md2FrameList[ChrList.lst[character].inst.frame_nxt].framefx & MADFX_INVICTUS )
                    {
                        // I Frame...
                        direction -= CapList[model].iframefacing;
                        left  = 0xFFFF - CapList[model].iframeangle;
                        right = CapList[model].iframeangle;

                        // Check for shield
                        if ( ChrList.lst[character].action >= ACTION_PA && ChrList.lst[character].action <= ACTION_PD )
                        {
                            // Using a shield?
                            if ( ChrList.lst[character].action < ACTION_PC )
                            {
                                // Check left hand
                                if ( ChrList.lst[character].holdingwhich[SLOT_LEFT] != MAX_CHR )
                                {
                                    left  = 0xFFFF - CapList[ChrList.lst[ChrList.lst[character].holdingwhich[SLOT_LEFT]].model].iframeangle;
                                    right = CapList[ChrList.lst[ChrList.lst[character].holdingwhich[SLOT_LEFT]].model].iframeangle;
                                }
                            }
                            else
                            {
                                // Check right hand
                                if ( ChrList.lst[character].holdingwhich[SLOT_RIGHT] != MAX_CHR )
                                {
                                    left  = 0xFFFF - CapList[ChrList.lst[ChrList.lst[character].holdingwhich[SLOT_RIGHT]].model].iframeangle;
                                    right = CapList[ChrList.lst[ChrList.lst[character].holdingwhich[SLOT_RIGHT]].model].iframeangle;
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
                        actual_damage = 0;
                    }
                }

                if ( actual_damage != 0 )
                {
                    if ( effects&DAMFX_ARMO )
                    {
                        ChrList.lst[character].life -= actual_damage;
                    }
                    else
                    {
                        ChrList.lst[character].life -= FF_MUL( actual_damage, ChrList.lst[character].defense );
                    }
                    if ( base_damage > HURTDAMAGE )
                    {
                        // Spawn blud particles
                        if ( CapList[model].bludvalid && ( damagetype < DAMAGE_HOLY || CapList[model].bludvalid == ULTRABLUDY ) )
                        {
                            spawn_one_particle( ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, ChrList.lst[character].pos.z,
                                                ChrList.lst[character].turn_z + direction, ChrList.lst[character].model, CapList[model].bludprttype,
                                                MAX_CHR, GRIP_LAST, ChrList.lst[character].team, character, 0, MAX_CHR );
                        }

                        // Set attack alert if it wasn't an accident
                        if ( team == TEAM_DAMAGE )
                        {
                            ChrList.lst[character].ai.attacklast = MAX_CHR;
                        }
                        else
                        {
                            // Don't alert the character too much if under constant fire
                            if ( ChrList.lst[character].carefultime == 0 )
                            {
                                // Don't let characters chase themselves...  That would be silly
                                if ( attacker != character )
                                {
                                    ChrList.lst[character].ai.alert |= ALERTIF_ATTACKED;
                                    ChrList.lst[character].ai.attacklast = attacker;
                                    ChrList.lst[character].carefultime = CAREFULTIME;
                                }
                            }
                        }
                    }

                    // Taking actual_damage action
                    action = ACTION_HA;
                    if ( ChrList.lst[character].life < 0 )
                    {
                        Uint16 iTmp = ChrList.lst[character].firstenchant;

                        // Character has died
                        ChrList.lst[character].alive = bfalse;
                        while ( iTmp != MAX_ENC )
                        {
                            Uint16 sTmp = EncList.lst[iTmp].nextenchant;
                            if ( !EveStack.lst[EncList.lst[iTmp].eve].stayifdead  )
                            {
                                remove_enchant( iTmp );
                            }
                            iTmp = sTmp;
                        }
                        ChrList.lst[character].waskilled = btrue;
                        ChrList.lst[character].keepaction = btrue;
                        ChrList.lst[character].life = -1;
                        ChrList.lst[character].platform = btrue;
                        ChrList.lst[character].bumpdampen = ChrList.lst[character].bumpdampen / 2.0f;
                        action = ACTION_KA;

                        // Give kill experience
                        experience = CapList[model].experienceworth + ( ChrList.lst[character].experience * CapList[model].experienceexchange );
                        if ( VALID_CHR(attacker) )
                        {
                            // Set target
                            ChrList.lst[character].ai.target = attacker;
                            if ( team == TEAM_DAMAGE )  ChrList.lst[character].ai.target = character;
                            if ( team == TEAM_NULL )  ChrList.lst[character].ai.target = character;

                            // Award direct kill experience
                            if ( TeamList[ChrList.lst[attacker].team].hatesteam[ChrList.lst[character].team] )
                            {
                                give_experience( attacker, experience, XP_KILLENEMY, bfalse );
                            }

                            // Check for hated
                            if ( CapList[ChrList.lst[attacker].model].idsz[IDSZ_HATE] == CapList[model].idsz[IDSZ_PARENT] ||
                                    CapList[ChrList.lst[attacker].model].idsz[IDSZ_HATE] == CapList[model].idsz[IDSZ_TYPE] )
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
                            if ( !ChrList.lst[tnc].on || !ChrList.lst[tnc].alive ) continue;

                            // Let the other characters know it died
                            if ( ChrList.lst[tnc].ai.target == character )
                            {
                                ChrList.lst[tnc].ai.alert |= ALERTIF_TARGETKILLED;
                            }

                            // All allies get team experience, but only if they also hate the dead guy's team
                            if ( !TeamList[ChrList.lst[tnc].team].hatesteam[team] && ( TeamList[ChrList.lst[tnc].team].hatesteam[ChrList.lst[character].team] ) )
                            {
                                give_experience( tnc, experience, XP_TEAMKILL, bfalse );
                            }

                            // Check if it was a leader
                            if ( TeamList[ChrList.lst[character].team].leader == character && ChrList.lst[tnc].team == ChrList.lst[character].team )
                            {
                                // All folks on the leaders team get the alert
                                ChrList.lst[tnc].ai.alert |= ALERTIF_LEADERKILLED;
                            }
                        }


                        // The team now has no leader if the character is the leader
                        if ( TeamList[ChrList.lst[character].team].leader == character )
                        {
                            TeamList[ChrList.lst[character].team].leader = NOLEADER;
                        }


                        detach_character_from_mount( character, btrue, bfalse );

                        // Play the death animation
                        action += generate_randmask( 0, 3 );
                        chr_play_action( character, action, bfalse );

                        // If it's a player, let it die properly before enabling respawn
                        if (ChrList.lst[character].isplayer) revivetimer = ONESECOND; // 1 second

                        // Afford it one last thought if it's an AI
                        TeamList[ChrList.lst[character].baseteam].morale--;
                        ChrList.lst[character].team = ChrList.lst[character].baseteam;
                        ChrList.lst[character].ai.alert |= ALERTIF_KILLED;
                        ChrList.lst[character].sparkle = NOSPARKLE;
                        ChrList.lst[character].ai.timer = update_wld + 1;  // No timeout...
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
                                ChrList.lst[character].damagetime = DAMAGETIME;
                        }
                    }
                }
                else
                {
                    // Spawn a defend particle
                    spawn_one_particle( ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, ChrList.lst[character].pos.z, ChrList.lst[character].turn_z, MAX_PROFILE, DEFEND, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, 0, MAX_CHR );
                    ChrList.lst[character].damagetime    = DEFENDTIME;
                    ChrList.lst[character].ai.alert     |= ALERTIF_BLOCKED;
                    ChrList.lst[character].ai.attacklast = attacker;     // For the ones attacking a shield
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
                ChrList.lst[character].ai.attacklast = MAX_CHR;
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
            damage_character( character, ATK_FRONT, tmp_damage, DAMAGE_CRUSH, ChrList.lst[killer].team, killer, DAMFX_ARMO | DAMFX_NBLOC, btrue );
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

    if ( INVALID_CHR( character ) ) return;

    sTmp = ChrList.lst[character].turn_z;
    iTmp = 0;
    origin = ChrList.lst[character].ai.owner;

    while ( iTmp < CapList[profile].gopoofprtamount )
    {
        spawn_one_particle( ChrList.lst[character].pos_old.x, ChrList.lst[character].pos_old.y, ChrList.lst[character].pos_old.z,
                            sTmp, profile, CapList[profile].gopoofprttype,
                            MAX_CHR, GRIP_LAST, ChrList.lst[character].team, origin, iTmp, MAX_CHR );
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
        strncpy(buffer, CapList[profile].classname, SDL_arraysize(buffer) );
    }
    else
    {
        write = 0;

        for ( section = 0; section < MAXSECTION; section++ )
        {
            if ( 0 != CapList[profile].chop_sectionsize[section] )
            {
                int irand = RANDIE;

                mychop = CapList[profile].chop_sectionstart[section] + ( irand % CapList[profile].chop_sectionsize[section] );

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

    if ( profile >= MAX_PROFILE )
    {
        log_warning( "spawn_one_character() - profile value too large %d out of %d\n", profile, MAX_PROFILE );
        return MAX_CHR;
    }

    if ( !VALID_MAD(profile) || !VALID_CAP(profile) )
    {
        if ( profile > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "spawn_one_character() - trying to spawn using invalid profile %d\n", profile );
        }

        return MAX_CHR;
    }

    // allocate a new character
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
            log_warning( "spawn_one_character() - failed to override a character? character %d already spawned? \n", override );
            return ichr;
        }
    }
    else
    {
        ichr = ChrList_get_free();
        if ( MAX_CHR == ichr )
        {
            log_warning( "spawn_one_character() - failed to allocate a new character\n" );
            return ichr;
        }
    }

    if ( !VALID_CHR_RANGE(ichr) )
    {
        log_warning( "spawn_one_character() - failed to spawn character (invalid index number %d?)\n", ichr );
        return ichr;
    }
    pchr = ChrList.lst + ichr;
    pcap = CapList + profile;

    // make a copy of the data in pos
    pos_tmp = pos;

    // clear out all data
    memset(pchr, 0, sizeof(chr_t));

    pchr->ibillboard = INVALID_BILLBOARD;

    // Make sure the team is valid
    team = MIN( team, TEAM_MAX - 1 );

    // IMPORTANT!!!
    pchr->isequipped = bfalse;
    pchr->sparkle = NOSPARKLE;
    pchr->overlay = bfalse;
    pchr->missilehandler = ichr;
    pchr->loopedsound_channel = INVALID_SOUND;

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
            int irand = RANDIE;
            skin = irand % MadList[profile].skins;
        }
    }

    pchr->skin    = skin;

    // Life and Mana
    pchr->alive = btrue;
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
    pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;
    pchr->damagetargettype = pcap->damagetargettype;
    tnc = 0;

    while ( tnc < DAMAGE_COUNT )
    {
        pchr->damagemodifier[tnc] = pcap->damagemodifier[tnc][skin];
        tnc++;
    }

    // latches
    pchr->latchx = 0;
    pchr->latchy = 0;
    pchr->latchbutton = 0;

    pchr->turnmode = TURNMODE_VELOCITY;

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
        Uint32 itmp = pcap->weight * pchr->fat * pchr->fat * pchr->fat;
        pchr->weight = MIN( itmp, (Uint32)0xFFFFFFFE );
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

            if ( is_in_passage( ShopStack.lst[cnt].passage, pchr->pos.x, pchr->pos.y, pchr->bumpsize) )
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

    if ( INVALID_CHR( character ) || ChrList.lst[character].alive ) return;
    pchr = ChrList.lst + character;

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
            ChrList.lst[item].ai.alert |= ALERTIF_ATLASTWAYPOINT;  // doubles as PutAway
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

    model = ChrList.lst[character].model;
    if ( model >= MAX_PROFILE || !MadList[model].loaded )
    {
        ChrList.lst[character].skin    = 0;
        ChrList.lst[character].inst.texture = TX_WATER_TOP;

        return 0;
    }

    // make sure that the instance has a valid model
    imad = ChrList.lst[character].inst.imad;
    if ( imad >= MAX_PROFILE || !MadList[imad].loaded )
    {
        imad = model;
        ChrList.lst[character].inst.imad = model;
    }

    // do the best we can to change the skin
    if ( 0 == MadList[imad].skins )
    {
        MadList[imad].skins = 1;
        MadList[imad].tex_ref[0] = TX_WATER_TOP;
    }

    // limit the skin
    if ( skin > MadList[imad].skins) skin = 0;

    ChrList.lst[character].skin         = skin;
    ChrList.lst[character].inst.texture = MadList[imad].tex_ref[skin];

    return ChrList.lst[character].skin;
};

//--------------------------------------------------------------------------------------------
Uint16 change_armor( Uint16 character, Uint16 skin )
{
    // ZZ> This function changes the armor of the character

    Uint16 enchant, sTmp;
    int iTmp;

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
    sTmp = ChrList.lst[character].model;
    skin = chr_change_skin( character, skin );

    // Change stats associated with skin
    ChrList.lst[character].defense = CapList[sTmp].defense[skin];
    iTmp = 0;

    while ( iTmp < DAMAGE_COUNT )
    {
        ChrList.lst[character].damagemodifier[iTmp] = CapList[sTmp].damagemodifier[iTmp][skin];
        iTmp++;
    }

    ChrList.lst[character].maxaccel = CapList[sTmp].maxaccel[skin];

    // Reset armor enchantments
    // These should really be done in reverse order ( Start with last enchant ), but
    // I don't care at this point !!!BAD!!!
    enchant = ChrList.lst[character].firstenchant;
    while ( enchant < MAX_ENC )
    {
        set_enchant_value( enchant, SETSLASHMODIFIER, EncList.lst[enchant].eve );
        set_enchant_value( enchant, SETCRUSHMODIFIER, EncList.lst[enchant].eve );
        set_enchant_value( enchant, SETPOKEMODIFIER, EncList.lst[enchant].eve );
        set_enchant_value( enchant, SETHOLYMODIFIER, EncList.lst[enchant].eve );
        set_enchant_value( enchant, SETEVILMODIFIER, EncList.lst[enchant].eve );
        set_enchant_value( enchant, SETFIREMODIFIER, EncList.lst[enchant].eve );
        set_enchant_value( enchant, SETICEMODIFIER, EncList.lst[enchant].eve );
        set_enchant_value( enchant, SETZAPMODIFIER, EncList.lst[enchant].eve );
        add_enchant_value( enchant, ADDACCEL, EncList.lst[enchant].eve );
        add_enchant_value( enchant, ADDDEFENSE, EncList.lst[enchant].eve );
        enchant = EncList.lst[enchant].nextenchant;
    }

    return skin;
}

//--------------------------------------------------------------------------------------------
void change_character_full( Uint16 ichr, Uint16 profile, Uint8 skin, Uint8 leavewhich )
{
    // ZF> This function polymorphs a character permanently so that it can be exported properly
    // A character turned into a frog with this function will also export as a frog!
    if ( profile > MAX_PROFILE || !MadList[profile].loaded ) return;

    strncpy(MadList[ChrList.lst[ichr].model].name, MadList[profile].name, SDL_arraysize(MadList[ChrList.lst[ichr].model].name));
    change_character( ichr, profile, skin, leavewhich );
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

    if ( INVALID_MAD(pholder->inst.imad) ) return;
    pholder_mad = MadList + pholder->inst.imad;

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
    cap_t * pcap;
    mad_t * pmad;

    if ( INVALID_MAD(profile) || INVALID_CHR(ichr) ) return;
    pchr = ChrList.lst + ichr;
    pcap = CapList + profile;
    pmad = MadList + profile;

    // Drop left weapon
    sTmp = pchr->holdingwhich[SLOT_LEFT];
    if ( sTmp != MAX_CHR && ( !pcap->slotvalid[SLOT_LEFT] || pcap->ismount ) )
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
    if ( sTmp != MAX_CHR && !pcap->slotvalid[SLOT_RIGHT] )
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
    pchr->model = profile;
    pchr->stoppedby = pcap->stoppedby;
    pchr->lifeheal = pcap->lifeheal;
    pchr->manacost = pcap->manacost;

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
    pchr->ai.type = pmad->ai;
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
    pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;
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
        TeamList[ChrList.lst[character].baseteam].morale--;
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

    team = ChrList.lst[character].team;
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ChrList.lst[cnt].on || team != ChrList.lst[cnt].team ) continue;

        if ( !ChrList.lst[cnt].alive )
        {
            ChrList.lst[cnt].ai.timer  = update_wld + 2;  // Don't let it think too much...
        }

        ChrList.lst[cnt].ai.alert |= ALERTIF_CLEANEDUP;
    }
}

//--------------------------------------------------------------------------------------------
int restock_ammo( Uint16 character, IDSZ idsz )
{
    // ZZ> This function restocks the characters ammo, if it needs ammo and if
    //    either its parent or type idsz match the given idsz.  This
    //    function returns the amount of ammo given.
    int amount, model;

    if ( INVALID_CHR(character) ) return 0;

    amount = 0;
    model = ChrList.lst[character].model;
    if ( INVALID_CAP(model) ) return 0;

    if ( CapList[model].idsz[IDSZ_PARENT] == idsz || CapList[model].idsz[IDSZ_TYPE] == idsz )
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
bool_t add_quest_idsz( const char *whichplayer, IDSZ idsz )
{
    /// @details ZF@> This function writes a IDSZ (With quest level 0) into a player quest.txt file, returns btrue if succeeded

    vfs_FILE *filewrite;
    STRING newloadname;

    // Only add quest IDSZ if it doesnt have it already
    if (check_player_quest(whichplayer, idsz) >= QUEST_BEATEN) return bfalse;

    // Try to open the file in read and append mode
    snprintf(newloadname, SDL_arraysize(newloadname), "players/%s/quest.txt", str_encode_path(whichplayer) );
    filewrite = vfs_openAppend( newloadname );
    if ( !filewrite )
    {
        // Create the file if it does not exist
        filewrite = vfs_openWrite( newloadname );
        if (!filewrite)
        {
            log_warning("Cannot write to %s!\n", newloadname);
            return bfalse;
        }

        vfs_printf( filewrite, "// This file keeps order of all the quests for the player (%s)\n", whichplayer);
        vfs_printf( filewrite, "// The number after the IDSZ shows the quest level. -1 means it is completed.");
    }

    vfs_printf( filewrite, "\n:[%4s] 0", undo_idsz( idsz ));
    vfs_close( filewrite );

    return btrue;
}

//--------------------------------------------------------------------------------------------
Sint16 modify_quest_idsz( const char *whichplayer, IDSZ idsz, Sint16 adjustment )
{
    /// @details ZF@> This function increases or decreases a Quest IDSZ quest level by the amount determined in
    ///     adjustment. It then returns the current quest level it now has.
    ///     It returns QUEST_NONE if failed and if the adjustment is 0, the quest is marked as beaten...

    vfs_FILE *filewrite, *fileread;
    STRING newloadname, copybuffer;
    IDSZ newidsz;
    Sint8 NewQuestLevel = QUEST_NONE, QuestLevel;

    // Now check each expansion until we find correct IDSZ
    if (check_player_quest(whichplayer, idsz) <= QUEST_BEATEN || adjustment == 0)  return QUEST_NONE;
    else
    {
        // modify the CData.quest_file
        char ctmp;

        // create a "tmp_*" copy of the file
        snprintf( newloadname, SDL_arraysize( newloadname ), "players/%s/quest.txt", str_encode_path(whichplayer));
        snprintf( copybuffer, SDL_arraysize( copybuffer ), "players/%s/tmp_quest.txt", str_encode_path(whichplayer));
        vfs_copyFile( newloadname, copybuffer );

        // open the tmp file for reading and overwrite the original file
        fileread  = vfs_openRead( copybuffer );
        filewrite = vfs_openWrite( newloadname );

        // Something went wrong
        if (!fileread || !filewrite)
        {
            log_warning("Could not modify quest IDSZ (%s).\n", newloadname);
            return QUEST_NONE;
        }

        // read the tmp file line-by line
        while ( !vfs_eof(fileread) )
        {
            ctmp = vfs_getc(fileread);
            vfs_ungetc(ctmp, fileread);
            if ( '/' == ctmp )
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
                    QuestLevel = MAX(adjustment, 0);        // Don't get negative
                    NewQuestLevel = QuestLevel;
                }

                vfs_printf(filewrite, "\n:[%s] %i", undo_idsz(newidsz), QuestLevel);
            }
        }
    }

    // clean it up
    vfs_close( fileread );
    vfs_close( filewrite );
    vfs_delete_file( copybuffer );

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
        // Bad! both function return and return to global variable!
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

    vfs_FILE *fileread;
    STRING newloadname;
    IDSZ newidsz;
    bool_t foundidsz = bfalse;
    Sint8 result = QUEST_NONE;

    snprintf( newloadname, SDL_arraysize(newloadname), "players/%s/quest.txt", str_encode_path(whichplayer) );
    fileread = vfs_openRead( newloadname );
    if ( NULL == fileread ) return result;

    // Always return "true" for [NONE] IDSZ checks
    if (idsz == IDSZ_NONE) result = QUEST_BEATEN;

    // Check each expansion
    while ( !foundidsz && goto_colon( NULL, fileread, btrue ) )
    {
        newidsz = fget_idsz( fileread );
        if ( newidsz == idsz )
        {
            foundidsz = btrue;
            result = fget_int( fileread );  // Read value behind colon and IDSZ
        }
    }

    vfs_close( fileread );

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
    if ( CapList[ChrList.lst[who].model].idsz[IDSZ_SKILL]  == whichskill ) result = btrue;
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
bool_t chr_integrate_motion( chr_t * pchr )
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
            ijump = CapList[pchr->model].soundindex[SOUND_JUMP];
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
                    int ijump = CapList[pchr->model].soundindex[SOUND_JUMP];
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
        if ( item != MAX_CHR )
        {
            if ( ( ChrList.lst[item].iskursed || CapList[ChrList.lst[item].model].istoobig ) && !CapList[ChrList.lst[item].model].isequipment )
            {
                // The item couldn't be put away
                ChrList.lst[item].ai.alert |= ALERTIF_NOTPUTAWAY;
                if ( pchr->isplayer && CapList[ChrList.lst[item].model].istoobig )
                {
                    debug_printf( "The %s is too big to be put away...", CapList[ChrList.lst[item].model].classname );
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
            if ( ( ChrList.lst[item].iskursed || CapList[ChrList.lst[item].model].istoobig ) && !CapList[ChrList.lst[item].model].isequipment )
            {
                // The item couldn't be put away
                ChrList.lst[item].ai.alert |= ALERTIF_NOTPUTAWAY;
                if ( pchr->isplayer && CapList[ChrList.lst[item].model].istoobig )
                {
                    debug_printf( "The %s is too big to be put away...", CapList[ChrList.lst[item].model].classname );
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
        // Which weapon?
        weapon = pchr->holdingwhich[SLOT_LEFT];
        if ( weapon == MAX_CHR )
        {
            // Unarmed means character itself is the weapon
            weapon = ichr;
        }

        action = CapList[ChrList.lst[weapon].model].weaponaction;

        // Can it do it?
        allowedtoattack = btrue;

        // First check if reload time and action is okay
        if ( !MadList[pchr->inst.imad].actionvalid[action] || ChrList.lst[weapon].reloadtime > 0 ) allowedtoattack = bfalse;
        else
        {
            // Then check if a skill is needed
            if ( CapList[ChrList.lst[weapon].model].needskillidtouse )
            {
                if (check_skills( ichr, CapList[ChrList.lst[weapon].model].idsz[IDSZ_SKILL]) == bfalse )
                    allowedtoattack = bfalse;
            }
        }
        if ( !allowedtoattack )
        {
            if ( ChrList.lst[weapon].reloadtime == 0 )
            {
                // This character can't use this weapon
                ChrList.lst[weapon].reloadtime = 50;
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
            if ( ChrList.lst[weapon].reloadtime == 0 )
            {
                ChrList.lst[weapon].ai.alert |= ALERTIF_USED;
            }
        }
        if ( allowedtoattack )
        {
            // Rearing mount
            mount = pchr->attachedto;
            if ( mount != MAX_CHR )
            {
                allowedtoattack = CapList[ChrList.lst[mount].model].ridercanattack;
                if ( ChrList.lst[mount].ismount && ChrList.lst[mount].alive && !ChrList.lst[mount].isplayer && ChrList.lst[mount].actionready )
                {
                    if ( ( action != ACTION_PA || !allowedtoattack ) && pchr->actionready )
                    {
                        chr_play_action( pchr->attachedto,  generate_randmask( ACTION_UA, 1 ), bfalse );
                        ChrList.lst[pchr->attachedto].ai.alert |= ALERTIF_USED;
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
                    bool_t mana_paid = cost_mana( ichr, ChrList.lst[weapon].manacost, weapon );

                    if ( mana_paid )
                    {
                        // Check life healing
                        pchr->life += ChrList.lst[weapon].lifeheal;
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
                            ChrList.lst[weapon].ai.alert |= ALERTIF_USED;
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
        // Which weapon?
        weapon = pchr->holdingwhich[SLOT_RIGHT];
        if ( weapon == MAX_CHR )
        {
            // Unarmed means character itself is the weapon
            weapon = ichr;
        }

        action = CapList[ChrList.lst[weapon].model].weaponaction + 2;

        // Can it do it? (other hand)
        allowedtoattack = btrue;

        // First check if reload time and action is okay
        if ( !MadList[pchr->inst.imad].actionvalid[action] || ChrList.lst[weapon].reloadtime > 0 ) allowedtoattack = bfalse;
        else
        {
            // Then check if a skill is needed
            if ( CapList[ChrList.lst[weapon].model].needskillidtouse )
            {
                IDSZ idsz = CapList[ChrList.lst[weapon].model].idsz[IDSZ_SKILL];

                if ( check_skills( ichr, idsz) == bfalse   )
                    allowedtoattack = bfalse;
            }
        }
        if ( !allowedtoattack )
        {
            if ( ChrList.lst[weapon].reloadtime == 0 )
            {
                // This character can't use this weapon
                ChrList.lst[weapon].reloadtime = 50;
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
            if ( ChrList.lst[weapon].reloadtime == 0 )
            {
                ChrList.lst[weapon].ai.alert |= ALERTIF_USED;
                pchr->ai.lastitemused = weapon;
            }
        }
        if ( allowedtoattack )
        {
            // Rearing mount
            mount = pchr->attachedto;
            if ( mount != MAX_CHR )
            {
                allowedtoattack = CapList[ChrList.lst[mount].model].ridercanattack;
                if ( ChrList.lst[mount].ismount && ChrList.lst[mount].alive && !ChrList.lst[mount].isplayer && ChrList.lst[mount].actionready )
                {
                    if ( ( action != ACTION_PC || !allowedtoattack ) && pchr->actionready )
                    {
                        chr_play_action( pchr->attachedto,  generate_randmask( ACTION_UC, 1 ), bfalse );
                        ChrList.lst[pchr->attachedto].ai.alert |= ALERTIF_USED;
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
                    bool_t mana_paid = cost_mana( ichr, ChrList.lst[weapon].manacost, weapon );
                    // Check mana cost
                    if ( mana_paid )
                    {
                        // Check life healing
                        pchr->life += ChrList.lst[weapon].lifeheal;
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
                            ChrList.lst[weapon].ai.alert |= ALERTIF_USED;
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
void move_characters( void )
{
    // ZZ> This function handles character physics

    Uint16 cnt;
    Uint8 twist;
    Uint8 speed, framelip;
    float level, friction_xy, friction_z;
    float dvx, dvy, dvmax;
    bool_t watchtarget;
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

        if ( !ChrList.lst[cnt].on || ChrList.lst[cnt].pack_ispacked ) continue;
        pchr = ChrList.lst + cnt;

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
            chr_t * pplat = ChrList.lst + pchr->onwhichplatform;

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
                    chr_t * pplat = ChrList.lst + pchr->onwhichplatform;

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
                    if ( cnt != pchr->ai.target )
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

        }

        chr_do_latch_button( pchr );

        // Flying
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
            // Flying
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
                        if ( pchr->phys.grounded && pchr->jumptime == 0 ) pchr->jumpnumber = pchr->jumpnumberreset;
                    }
                }
                else if ( pchr->phys.grounded && pchr->jumptime == 0 )
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

        chr_integrate_motion( pchr );

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
                character_swipe( cnt, SLOT_LEFT );
            if ( Md2FrameList[pchr->inst.frame_nxt].framefx&MADFX_ACTRIGHT )
                character_swipe( cnt, SLOT_RIGHT );
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
                if ( VALID_SND( ifoot ) )
                {
                    sound_play_chunk( pchr->pos, chr_get_chunk_ptr( pchr,ifoot ) );
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
        if ( !ChrList.lst[cnt].on || !(ChrList.lst[cnt].ai.poof_time >= 0 && ChrList.lst[cnt].ai.poof_time <= (Sint32)update_wld)  ) continue;

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
void chop_load( Uint16 profile, const char *szLoadname )
{
    // ZZ> This function reads a naming file
    vfs_FILE *fileread;
    int   section, chopinsection;
    char  tmp_chop[32];

    fileread = vfs_openRead( szLoadname );
    if ( NULL == fileread ) return;

    section = 0;
    chopinsection = 0;
    while ( section < MAXSECTION && chop.carat < CHOPDATACHUNK && goto_colon( NULL, fileread, btrue ) )
    {
        fget_string( fileread, tmp_chop, SDL_arraysize(tmp_chop) );

        // convert all the '_' and junk in the string
        str_decode( tmp_chop, SDL_arraysize(tmp_chop), tmp_chop);

        if ( 0 == strcmp(tmp_chop, "STOP") )
        {
            if ( section < MAXSECTION )
            {
                CapList[profile].chop_sectionsize[section]  = chopinsection;
                CapList[profile].chop_sectionstart[section] = chop.count - chopinsection;
            }

            section++;
            chopinsection = 0;
            tmp_chop[0] = '\0';
        }
        else
        {
            int chop_len;

            // fill in the chop data
            chop.start[chop.count] = chop.carat;
            chop_len = snprintf( chop.buffer + chop.carat, CHOPDATACHUNK - chop.carat - 1, "%s", tmp_chop );

            chop.carat += chop_len + 1;
            chop.count++;
            chopinsection++;
            tmp_chop[0] = '\0';
        }
    }

    // handle the case where the chop buffer has overflowed
    // pretend the last command was "STOP"
    if ( '\0' != tmp_chop[0] && section < MAXSECTION )
    {
        CapList[profile].chop_sectionsize[section]  = chopinsection;
        CapList[profile].chop_sectionstart[section] = chop.count - chopinsection;
    }

    vfs_close( fileread );
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

    // Remember any previous color shifts in case of lasting enchantments
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
    pinst->texture   = pmad->tex_ref[skin];
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

            lTmp = toupper( CapList[pchr->model].classname[0] );
            if ( 'A' == lTmp || 'E' == lTmp || 'I' == lTmp || 'O' == lTmp || 'U' == lTmp )
            {
                article = "an";
            }

            snprintf( szName, SDL_arraysize( szName), "%s %s", article, CapList[pchr->model].classname );
        }
    }

    // ? capitalize the name ?
    szName[0] = toupper( szName[0] );

    return szName;
}

//--------------------------------------------------------------------------------------------
Mix_Chunk * chr_get_chunk( Uint16 ichr, int index )
{
	if( INVALID_CHR(ichr) ) return NULL;
	if( INVALID_MAD(ChrList.lst[ichr].inst.imad) ) return NULL;
	if( index < 0 || index >= MAX_WAVE ) return NULL;

	return MadList[ChrList.lst[ichr].inst.imad].wavelist[index];
}

//--------------------------------------------------------------------------------------------
Mix_Chunk * chr_get_chunk_ptr( chr_t * pchr, int index )
{
	if( NULL == pchr || !pchr->on ) return NULL;
	if( INVALID_MAD(pchr->inst.imad) ) return NULL;
	if( index < 0 || index >= MAX_WAVE ) return NULL;

	return MadList[pchr->inst.imad].wavelist[index];
}

//--------------------------------------------------------------------------------------------
bool_t release_one_cap( Uint16 icap )
{
	int tnc;
	cap_t * pcap;

	if( !VALID_CAP_RANGE(icap) ) return bfalse;
	pcap = CapList + icap;

	if( !pcap->loaded ) return btrue;

    memset( pcap, 0, sizeof(cap_t) );

    for ( tnc = 0; tnc < MAXSECTION; tnc++ )
    {
        pcap->chop_sectionstart[tnc] = MAXCHOP;
        pcap->chop_sectionsize[tnc]  = 0;
    }

	pcap->loaded  = bfalse;
	pcap->name[0] = '\0';

	return btrue;
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
    team = ChrList.lst[character].team;
    fanblock = mesh_get_block_int(PMesh, x,y);
    charb = bumplist[fanblock].chr;
    cnt = 0;
    while ( cnt < bumplist[fanblock].chrnum )
    {
      if ( dead != ChrList.lst[charb].alive && ( seeinvisible || FF_MUL( ChrList.lst[charb].inst.alpha, ChrList.lst[charb].inst.max_light ) > INVISIBLE ) ) )
      {
        if ( ( enemies && TeamList[team].hatesteam[ChrList.lst[charb].team] && !ChrList.lst[charb].invictus ) ||
             ( items && ChrList.lst[charb].isitem ) ||
             ( friends && ChrList.lst[charb].baseteam == team ) )
        {
          if ( charb != character && ChrList.lst[character].attachedto != charb )
          {
            if ( !ChrList.lst[charb].isitem || items )
            {
              if ( idsz != IDSZ_NONE )
              {
                if ( CapList[ChrList.lst[charb].model].idsz[IDSZ_PARENT] == idsz ||
                     CapList[ChrList.lst[charb].model].idsz[IDSZ_TYPE] == idsz )
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

  team = ChrList.lst[character].team;
  cnt = 0;
  while ( cnt < MAX_CHR )
  {
    if ( ChrList.lst[cnt].on )
    {
      if ( ChrList.lst[cnt].attachedto == MAX_CHR && !ChrList.lst[cnt].pack_ispacked )
      {
        if ( TeamList[team].hatesteam[ChrList.lst[cnt].team] && ChrList.lst[cnt].alive && !ChrList.lst[cnt].invictus )
        {
          if ( ChrList.lst[character].canseeinvisible || FF_MUL( ChrList.lst[cnt].inst.alpha, ChrList.lst[cnt].inst.max_light ) > INVISIBLE ) )
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
    team = ChrList.lst[character].team;
    fanblock = mesh_get_block_int(PMesh, x,y);
    charb = bumplist[fanblock].chr;
    cnt = 0;
    while ( cnt < bumplist[fanblock].chrnum )
    {
      if ( dead != ChrList.lst[charb].alive && ( seeinvisible || FF_MUL( ChrList.lst[charb].inst.alpha, ChrList.lst[charb].inst.max_light ) > INVISIBLE ) ) )
      {
        if ( ( enemies && TeamList[team].hatesteam[ChrList.lst[charb].team] ) ||
             ( items && ChrList.lst[charb].isitem ) ||
             ( friends && ChrList.lst[charb].team == team ) ||
             ( friends && enemies ) )
        {
          if ( charb != character && ChrList.lst[character].attachedto != charb && ChrList.lst[charb].attachedto == MAX_CHR && !ChrList.lst[charb].pack_ispacked )
          {
            if ( !ChrList.lst[charb].invictus || items )
            {
              if ( idsz != IDSZ_NONE )
              {
                if ( CapList[ChrList.lst[charb].model].idsz[IDSZ_PARENT] == idsz ||
                     CapList[ChrList.lst[charb].model].idsz[IDSZ_TYPE] == idsz )
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
