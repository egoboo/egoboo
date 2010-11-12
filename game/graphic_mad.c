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

/// @file graphic_mad.c
/// @brief Character model drawing code.
/// @details

#include "graphic_mad.h"

#include "profile.inl"
#include "char.inl"
#include "mad.h"

#include "md2.inl"
#include "id_md2.h"

#include "log.h"
#include "camera.h"
#include "game.h"
#include "input.h"
#include "texture.h"
#include "lighting.h"

#include "egoboo_setup.h"
#include "egoboo.h"

#include <SDL_opengl.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void chr_instance_update_lighting_base( chr_instance_t * pinst, chr_t * pchr, bool_t force );

static void draw_points( chr_t * pchr, int vrt_offset, int verts );
static void _draw_one_grip_raw( chr_instance_t * pinst, mad_t * pmad, int slot );
static void draw_one_grip( chr_instance_t * pinst, mad_t * pmad, int slot );
static void chr_draw_grips( chr_t * pchr );
static void chr_draw_attached_grip( chr_t * pchr );
static void render_chr_bbox( chr_t * pchr );

static egoboo_rv chr_instance_update_vlst_cache( chr_instance_t * pinst, int vmax, int vmin, bool_t force, bool_t vertices_match, bool_t frames_match );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t render_one_mad_enviro( const CHR_REF character, GLXvector4f tint, Uint32 bits )
{
    /// @details ZZ@> This function draws an environment mapped model

    Uint16 cnt;
    Uint16 vertex;
    float  uoffset, voffset;

    chr_t          * pchr;
    mad_t          * pmad;
    MD2_Model_t    * pmd2;
    chr_instance_t * pinst;
    oglx_texture_t   * ptex;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr  = ChrList.lst + character;
    pinst = &( pchr->inst );

    if ( !LOADED_MAD( pinst->imad ) ) return bfalse;
    pmad = MadStack.lst + pinst->imad;

    pmd2 = pmad->md2_ptr;
    if ( NULL == pmd2 ) return bfalse;

    ptex = NULL;
    if ( 0 != ( bits & CHR_PHONG ) )
    {
        ptex = TxTexture_get_ptr(( TX_REF )TX_PHONG );
    }

    if ( NULL == ptex )
    {
        ptex = TxTexture_get_ptr( pinst->texture );
    }

    uoffset = pinst->uoffset - PCamera->turn_z_one;
    voffset = pinst->voffset;

    if ( 0 != ( bits & CHR_REFLECT ) )
    {
        GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
        GL_DEBUG( glPushMatrix )();
        GL_DEBUG( glMultMatrixf )( pinst->ref.matrix.v );
    }
    else
    {
        GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
        GL_DEBUG( glPushMatrix )();
        GL_DEBUG( glMultMatrixf )( pinst->matrix.v );
    }

    // Choose texture and matrix
    oglx_texture_Bind( ptex );

    ATTRIB_PUSH( __FUNCTION__, GL_CURRENT_BIT );
    {
        int cmd_count;
        MD2_GLCommand_t * glcommand;

        GLXvector4f curr_color;

        GL_DEBUG( glGetFloatv )( GL_CURRENT_COLOR, curr_color );

        // Render each command
        cmd_count   = md2_get_numCommands( pmd2 );
        glcommand   = ( MD2_GLCommand_t * )md2_get_Commands( pmd2 );

        for ( cnt = 0; cnt < cmd_count && NULL != glcommand; cnt++ )
        {
            int count = glcommand->command_count;

            GL_DEBUG( glBegin )( glcommand->gl_mode );
            {
                int tnc;

                for ( tnc = 0; tnc < count; tnc++ )
                {
                    GLfloat     cmax;
                    GLXvector4f col;
                    GLfloat     tex[2];
                    GLvertex   *pvrt;

                    vertex = glcommand->data[tnc].index;
                    if ( vertex >= pinst->vrt_count ) continue;

                    pvrt   = pinst->vrt_lst + vertex;

                    // normalize the color so it can be modulated by the phong/environment map
                    col[RR] = pvrt->color_dir * INV_FF;
                    col[GG] = pvrt->color_dir * INV_FF;
                    col[BB] = pvrt->color_dir * INV_FF;
                    col[AA] = 1.0f;

                    cmax = MAX( MAX( col[RR], col[GG] ), col[BB] );

                    if ( cmax != 0.0f )
                    {
                        col[RR] /= cmax;
                        col[GG] /= cmax;
                        col[BB] /= cmax;
                    }

                    // apply the tint
                    col[RR] *= tint[RR];
                    col[GG] *= tint[GG];
                    col[BB] *= tint[BB];
                    col[AA] *= tint[AA];

                    tex[0] = pvrt->env[XX] + uoffset;
                    tex[1] = CLIP( cmax, 0.0f, 1.0f );

                    if ( 0 != ( bits & CHR_PHONG ) )
                    {
                        // determine the phong texture coordinates
                        // the default phong is bright in both the forward and back directions...
                        tex[1] = tex[1] * 0.5f + 0.5f;
                    }

                    GL_DEBUG( glColor4fv )( col );
                    GL_DEBUG( glNormal3fv )( pvrt->nrm );
                    GL_DEBUG( glTexCoord2fv )( tex );
                    GL_DEBUG( glVertex3fv )( pvrt->pos );
                }

            }
            GL_DEBUG_END();

            glcommand = glcommand->next;
        }
    }

    ATTRIB_POP( __FUNCTION__ );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    return btrue;
}

// Do fog...
/*
if(fogon && pinst->light==255)
{
    // The full fog value
    alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

    for (cnt = 0; cnt < pmad->transvertices; cnt++)
    {
        // Figure out the z position of the vertex...  Not totally accurate
        z = (pinst->vrt_lst[cnt].pos[ZZ]) + pchr->matrix(3,2);

        // Figure out the fog coloring
        if(z < fogtop)
        {
            if(z < fogbottom)
            {
                pinst->vrt_lst[cnt].specular = alpha;
            }
            else
            {
                z = 1.0f - ((z - fogbottom)/fogdistance);  // 0.0f to 1.0f...  Amount of fog to keep
                red = fogred * z;
                grn = foggrn * z;
                blu = fogblu * z;
                fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                pinst->vrt_lst[cnt].specular = fogspec;
            }
        }
        else
        {
            pinst->vrt_lst[cnt].specular = 0;
        }
    }
}
else
{
    for (cnt = 0; cnt < pmad->transvertices; cnt++)
        pinst->vrt_lst[cnt].specular = 0;
}
*/

//--------------------------------------------------------------------------------------------
bool_t render_one_mad_tex( const CHR_REF character, GLXvector4f tint, Uint32 bits )
{
    /// @details ZZ@> This function draws a model

    int    cmd_count;
    int    cnt;
    Uint16 vertex;
    float  uoffset, voffset;

    chr_t          * pchr;
    mad_t          * pmad;
    MD2_Model_t    * pmd2;
    chr_instance_t * pinst;
    oglx_texture_t   * ptex;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr  = ChrList.lst + character;
    pinst = &( pchr->inst );

    if ( !LOADED_MAD( pinst->imad ) ) return bfalse;
    pmad = MadStack.lst + pinst->imad;

    pmd2 = pmad->md2_ptr;
    if ( NULL == pmd2 ) return bfalse;

    // To make life easier
    ptex = TxTexture_get_ptr( pinst->texture );

    uoffset = pinst->uoffset * INV_FFFF;
    voffset = pinst->voffset * INV_FFFF;

    if ( 0 != ( bits & CHR_REFLECT ) )
    {
        GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
        GL_DEBUG( glPushMatrix )();
        GL_DEBUG( glMultMatrixf )( pinst->ref.matrix.v );
    }
    else
    {
        GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
        GL_DEBUG( glPushMatrix )();
        GL_DEBUG( glMultMatrixf )( pinst->matrix.v );
    }

    // Choose texture and matrix
    oglx_texture_Bind( ptex );

    ATTRIB_PUSH( __FUNCTION__, GL_CURRENT_BIT );
    {
        float             base_amb;
        MD2_GLCommand_t * glcommand;

        // set the basic tint. if the object is marked with CHR_LIGHT
        // the color will not be set again inside the loop
        GL_DEBUG( glColor4fv )( tint );

        base_amb = 0.0f;
        if ( 0 == ( bits & CHR_LIGHT ) )
        {
            // convert the "light" parameter to self-lighting for
            // every object that is not being rendered using CHR_LIGHT
            base_amb   = ( 255 == pinst->light ) ? 0 : ( pinst->light * INV_FF );
        }

        // Render each command
        cmd_count   = md2_get_numCommands( pmd2 );
        glcommand   = ( MD2_GLCommand_t * )md2_get_Commands( pmd2 );

        for ( cnt = 0; cnt < cmd_count && NULL != glcommand; cnt++ )
        {
            int count = glcommand->command_count;

            GL_DEBUG( glBegin )( glcommand->gl_mode );
            {
                int tnc;

                for ( tnc = 0; tnc < count; tnc++ )
                {
                    GLXvector2f tex;
                    GLXvector4f col;
                    GLvertex * pvrt;

                    vertex = glcommand->data[tnc].index;
                    if ( vertex >= pinst->vrt_count ) continue;

                    pvrt = pinst->vrt_lst + vertex;

                    // determine the texture coordinates
                    tex[0] = glcommand->data[tnc].s + uoffset;
                    tex[1] = glcommand->data[tnc].t + voffset;

                    // determine the vertex color for objects that have
                    // per vertex lighting
                    if ( 0 == ( bits & CHR_LIGHT ) )
                    {
                        float fcol;

                        // convert the "light" parameter to self-lighting for
                        // every object that is not being rendered using CHR_LIGHT
                        fcol   = pvrt->color_dir * INV_FF;

                        col[0] = fcol * tint[0];
                        col[1] = fcol * tint[1];
                        col[2] = fcol * tint[2];
                        col[3] = tint[0];

                        if ( 0 != ( bits & CHR_PHONG ) )
                        {
                            fcol = base_amb + pinst->color_amb * INV_FF;

                            col[0] += fcol * tint[0];
                            col[1] += fcol * tint[1];
                            col[2] += fcol * tint[2];
                        }

                        col[0] = CLIP( col[0], 0.0f, 1.0f );
                        col[1] = CLIP( col[1], 0.0f, 1.0f );
                        col[2] = CLIP( col[2], 0.0f, 1.0f );

                        GL_DEBUG( glColor4fv )( col );
                    }

                    GL_DEBUG( glNormal3fv )( pvrt->nrm );
                    GL_DEBUG( glTexCoord2fv )( tex );
                    GL_DEBUG( glVertex3fv )( pvrt->pos );
                }
            }
            GL_DEBUG_END();

            glcommand = glcommand->next;
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    return btrue;
}

/*
    // Do fog...
    if(fogon && pinst->light==255)
    {
        // The full fog value
        alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

        for (cnt = 0; cnt < pmad->transvertices; cnt++)
        {
            // Figure out the z position of the vertex...  Not totally accurate
            z = (pinst->vrt_lst[cnt].pos[ZZ]) + pchr->matrix(3,2);

            // Figure out the fog coloring
            if(z < fogtop)
            {
                if(z < fogbottom)
                {
                    pinst->vrt_lst[cnt].specular = alpha;
                }
                else
                {
                    spek = pinst->vrt_lst[cnt].specular & 255;
                    z = (z - fogbottom)/fogdistance;  // 0.0f to 1.0f...  Amount of old to keep
                    fogtokeep = 1.0f-z;  // 0.0f to 1.0f...  Amount of fog to keep
                    spek = spek * z;
                    red = (fogred * fogtokeep) + spek;
                    grn = (foggrn * fogtokeep) + spek;
                    blu = (fogblu * fogtokeep) + spek;
                    fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                    pinst->vrt_lst[cnt].specular = fogspec;
                }
            }
        }
    }
*/

//--------------------------------------------------------------------------------------------
bool_t render_one_mad( const CHR_REF character, GLXvector4f tint, BIT_FIELD bits )
{
    /// @details ZZ@> This function picks the actual function to use

    chr_t * pchr;
    bool_t retval;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    if ( pchr->is_hidden ) return bfalse;

    if ( pchr->inst.enviro || HAS_SOME_BITS(bits, CHR_PHONG) )
    {
        retval = render_one_mad_enviro( character, tint, bits );
    }
    else
    {
        retval = render_one_mad_tex( character, tint, bits );
    }

#if defined(_DEBUG) && defined(DEBUG_CHR_BBOX)
    // don't draw the debug stuff for reflections
    if ( 0 == ( bits & CHR_REFLECT ) )
    {
        render_chr_bbox( pchr );
    }

    // the grips of all objects
    //chr_draw_attached_grip( pchr );

    // draw all the vertices of an object
    //GL_DEBUG( glPointSize( 5 ) );
    //draw_points( pchr, 0, pro_get_pmad(pchr->inst.imad)->md2_data.vertex_lst );
#endif

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t render_one_mad_ref( const CHR_REF ichr )
{
    /// @details ZZ@> This function draws characters reflected in the floor

    chr_t * pchr;
    chr_instance_t * pinst;
    GLXvector4f tint;

    if ( !INGAME_CHR( ichr ) ) return bfalse;
    pchr = ChrList.lst + ichr;
    pinst = &( pchr->inst );

    if ( pchr->is_hidden ) return bfalse;

    if ( !pinst->ref.matrix_valid )
    {
        if ( !apply_reflection_matrix( &( pchr->inst ), pchr->enviro.floor_level ) )
        {
            return bfalse;
        }
    }

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT );
    {
        GL_DEBUG( glEnable )( GL_CULL_FACE );  // GL_ENABLE_BIT

        // cull face CCW because we are rendering a reflected object
        GL_DEBUG( glFrontFace )( GL_CCW );    // GL_POLYGON_BIT

        if ( pinst->ref.alpha != 255 && pinst->ref.light == 255 )
        {
            GL_DEBUG( glEnable )( GL_BLEND );                                 // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_ALPHA, GL_ONE_MINUS_SRC_ALPHA );                        // GL_COLOR_BUFFER_BIT

            chr_instance_get_tint( pinst, tint, CHR_ALPHA | CHR_REFLECT );

            // the previous call to chr_instance_update_lighting_ref() has actually set the
            // alpha and light for all vertices
            render_one_mad( ichr, tint, CHR_ALPHA | CHR_REFLECT );
        }

        if ( pinst->ref.light != 255 )
        {
            GL_DEBUG( glEnable )( GL_BLEND );                                 // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );                        // GL_COLOR_BUFFER_BIT

            chr_instance_get_tint( pinst, tint, CHR_LIGHT | CHR_REFLECT );

            // the previous call to chr_instance_update_lighting_ref() has actually set the
            // alpha and light for all vertices
            render_one_mad( ichr, tint, CHR_LIGHT | CHR_REFLECT );
        }

        if ( gfx.phongon && pinst->sheen > 0 )
        {
            GL_DEBUG( glEnable )( GL_BLEND );
            GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );

            chr_instance_get_tint( pinst, tint, CHR_PHONG | CHR_REFLECT );

            render_one_mad( ichr, tint, CHR_PHONG | CHR_REFLECT );
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_chr_bbox( chr_t * pchr )
{
    if ( !ACTIVE_PCHR( pchr ) ) return;

    // draw the object bounding box as a part of the graphics debug mode F7
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F7 ) )
    {
        GL_DEBUG( glDisable )( GL_TEXTURE_2D );
        {
            oct_bb_t bb;

            oct_bb_add_vector( pchr->chr_prt_cv, pchr->pos.v, &bb );

            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
            render_oct_bb( &bb, btrue, btrue );
        }
        GL_DEBUG( glEnable )( GL_TEXTURE_2D );
    }

    // the grips and vertrices of all objects
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F6 ) )
    {
        chr_draw_attached_grip( pchr );

        // draw all the vertices of an object
        GL_DEBUG( glPointSize( 5 ) );
        draw_points( pchr, 0, pchr->inst.vrt_count );
    }
}

//--------------------------------------------------------------------------------------------
void draw_points( chr_t * pchr, int vrt_offset, int verts )
{
    /// @details BB@> a function that will draw some of the vertices of the given character.
    ///     The original idea was to use this to debug the grip for attached items.

    mad_t * pmad;

    int vmin, vmax, cnt;
    GLboolean texture_1d_enabled, texture_2d_enabled;

    if ( !ACTIVE_PCHR( pchr ) ) return;

    pmad = chr_get_pmad( GET_REF_PCHR( pchr ) );
    if ( NULL == pmad ) return;

    vmin = vrt_offset;
    vmax = vmin + verts;

    if ( vmin < 0 || vmax < 0 ) return;
    if ( vmin > pchr->inst.vrt_count || vmax > pchr->inst.vrt_count ) return;

    texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
    texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if ( texture_1d_enabled ) GL_DEBUG( glDisable ) ( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glDisable ) ( GL_TEXTURE_2D );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glMultMatrixf )( pchr->inst.matrix.v );

    GL_DEBUG( glBegin( GL_POINTS ) );
    {
        for ( cnt = vmin; cnt < vmax; cnt++ )
        {
            GL_DEBUG( glVertex3fv ) ( pchr->inst.vrt_lst[cnt].pos );
        }
    }
    GL_DEBUG_END();

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    if ( texture_1d_enabled ) GL_DEBUG( glEnable ) ( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glEnable ) ( GL_TEXTURE_2D );
}

//--------------------------------------------------------------------------------------------
void draw_one_grip( chr_instance_t * pinst, mad_t * pmad, int slot )
{
    GLboolean texture_1d_enabled, texture_2d_enabled;

    texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
    texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if ( texture_1d_enabled ) GL_DEBUG( glDisable ) ( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glDisable ) ( GL_TEXTURE_2D );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glMultMatrixf )( pinst->matrix.v );

    _draw_one_grip_raw( pinst, pmad, slot );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    if ( texture_1d_enabled ) GL_DEBUG( glEnable ) ( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glEnable ) ( GL_TEXTURE_2D );
}

//--------------------------------------------------------------------------------------------
void _draw_one_grip_raw( chr_instance_t * pinst, mad_t * pmad, int slot )
{
    int vmin, vmax, cnt;

    float red[4] = {1, 0, 0, 1};
    float grn[4] = {0, 1, 0, 1};
    float blu[4] = {0, 0, 1, 1};
    float * col_ary[3];

    col_ary[0] = red;
    col_ary[1] = grn;
    col_ary[2] = blu;

    if ( NULL == pinst || NULL == pmad ) return;

    vmin = ( int )pinst->vrt_count - ( int )slot_to_grip_offset(( slot_t )slot );
    vmax = vmin + GRIP_VERTS;

    if ( vmin >= 0 && vmax >= 0 && vmax <= pinst->vrt_count )
    {
        fvec3_t   src, dst, diff;

        GL_DEBUG( glBegin )( GL_LINES );
        {
            for ( cnt = 1; cnt < GRIP_VERTS; cnt++ )
            {
                src.x = pinst->vrt_lst[vmin].pos[XX];
                src.y = pinst->vrt_lst[vmin].pos[YY];
                src.z = pinst->vrt_lst[vmin].pos[ZZ];

                diff.x = pinst->vrt_lst[vmin+cnt].pos[XX] - src.x;
                diff.y = pinst->vrt_lst[vmin+cnt].pos[YY] - src.y;
                diff.z = pinst->vrt_lst[vmin+cnt].pos[ZZ] - src.z;

                dst.x = src.x + 3 * diff.x;
                dst.y = src.y + 3 * diff.y;
                dst.z = src.z + 3 * diff.z;

                GL_DEBUG( glColor4fv ) ( col_ary[cnt-1] );

                GL_DEBUG( glVertex3fv ) ( src.v );
                GL_DEBUG( glVertex3fv ) ( dst.v );
            }
        }
        GL_DEBUG_END();
    }

    GL_DEBUG( glColor4f ) ( 1, 1, 1, 1 );
}

//--------------------------------------------------------------------------------------------
void chr_draw_attached_grip( chr_t * pchr )
{
    mad_t * pholder_mad;
    cap_t * pholder_cap;
    chr_t * pholder;

    if ( !ACTIVE_PCHR( pchr ) ) return;

    if ( !INGAME_CHR( pchr->attachedto ) ) return;
    pholder = ChrList.lst + pchr->attachedto;

    pholder_cap = pro_get_pcap( pholder->profile_ref );
    if ( NULL == pholder_cap ) return;

    pholder_mad = chr_get_pmad( GET_REF_PCHR( pholder ) );
    if ( NULL == pholder_mad ) return;

    draw_one_grip( &( pholder->inst ), pholder_mad, pchr->inwhich_slot );
}

//--------------------------------------------------------------------------------------------
void chr_draw_grips( chr_t * pchr )
{
    mad_t * pmad;
    cap_t * pcap;

    int slot;
    GLboolean texture_1d_enabled, texture_2d_enabled;

    if ( !ACTIVE_PCHR( pchr ) ) return;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return;

    pmad = chr_get_pmad( GET_REF_PCHR( pchr ) );
    if ( NULL == pmad ) return;

    texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
    texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if ( texture_1d_enabled ) GL_DEBUG( glDisable ) ( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glDisable ) ( GL_TEXTURE_2D );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glMultMatrixf )( pchr->inst.matrix.v );

    slot = SLOT_LEFT;
    if ( pcap->slotvalid[slot] )
    {
        _draw_one_grip_raw( &( pchr->inst ), pmad, slot );
    }

    slot = SLOT_RIGHT;
    if ( pcap->slotvalid[slot] )
    {
        _draw_one_grip_raw( &( pchr->inst ), pmad, slot );
    }

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    if ( texture_1d_enabled ) GL_DEBUG( glEnable ) ( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glEnable ) ( GL_TEXTURE_2D );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_rv chr_update_instance( chr_t * pchr )
{
    chr_instance_t * pinst;
    egoboo_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;
    pinst = &( pchr->inst );

    // make sure that the vertices are interpolated
    retval = chr_instance_update_vertices( pinst, -1, -1, btrue );
    if ( rv_error == retval )
    {
        return rv_error;
    }

    // do the basic lighting
    chr_instance_update_lighting_base( pinst, pchr, bfalse );

    return retval;
}

//--------------------------------------------------------------------------------------------
void chr_instance_update_lighting_base( chr_instance_t * pinst, chr_t * pchr, bool_t force )
{
    /// @details BB@> determine the basic per-vertex lighting

    Uint16 cnt;

    lighting_cache_t global_light, loc_light;

    GLvertex * vrt_lst;

    mad_t * pmad;

    if ( NULL == pinst || NULL == pchr ) return;
    vrt_lst = pinst->vrt_lst;

    // force this function to be evaluated the 1st time through
    if ( 0 == update_wld && 0 == frame_all ) force = btrue;

    // has this already been calculated this update?
    if ( !force && pinst->lighting_update_wld >= update_wld ) return;
    pinst->lighting_update_wld = update_wld;

    // make sure the matrix is valid
    chr_update_matrix( pchr, btrue );

    // has this already been calculated this frame?
    if ( !force && pinst->lighting_frame_all >= frame_all ) return;

    // reduce the amount of updates to an average of about 1 every 2 frames, but dither
    // the updating so that not all objects update on the same frame
    pinst->lighting_frame_all = frame_all + (( frame_all + pchr->obj_base.guid ) & 0x03 );

    if ( !LOADED_MAD( pinst->imad ) ) return;
    pmad = MadStack.lst + pinst->imad;
    pinst->vrt_count = pinst->vrt_count;

    // interpolate the lighting for the origin of the object
    grid_lighting_interpolate( PMesh, &global_light, pchr->pos.x, pchr->pos.y );

    // rotate the lighting data to body_centered coordinates
    lighting_project_cache( &loc_light, &global_light, pinst->matrix );

    pinst->color_amb = 0.9f * pinst->color_amb + 0.1f * ( loc_light.hgh.lighting[LVEC_AMB] + loc_light.low.lighting[LVEC_AMB] ) * 0.5f;

    pinst->max_light = -255;
    pinst->min_light =  255;
    for ( cnt = 0; cnt < pinst->vrt_count; cnt++ )
    {
        Sint16 lite;

        GLvertex * pvert = pinst->vrt_lst + cnt;

        // a simple "height" measurement
        float hgt = pvert->pos[ZZ] * pinst->matrix.CNV( 3, 3 ) + pinst->matrix.CNV( 3, 3 );

        if ( pvert->nrm[0] == 0.0f && pvert->nrm[1] == 0.0f && pvert->nrm[2] == 0.0f )
        {
            // this is the "ambient only" index, but it really means to sum up all the light
            GLfloat tnrm[3];

            tnrm[0] = tnrm[1] = tnrm[2] = 1.0f;
            lite  = lighting_evaluate_cache( &loc_light, tnrm, hgt, PMesh->tmem.bbox, NULL, NULL );

            tnrm[0] = tnrm[1] = tnrm[2] = -1.0f;
            lite += lighting_evaluate_cache( &loc_light, tnrm, hgt, PMesh->tmem.bbox, NULL, NULL );

            // average all the directions
            lite /= 6;
        }
        else
        {
            lite  = lighting_evaluate_cache( &loc_light, pvert->nrm, hgt, PMesh->tmem.bbox, NULL, NULL );
        }

        pvert->color_dir = 0.9f * pvert->color_dir + 0.1f * lite;

        pinst->max_light = MAX( pinst->max_light, pvert->color_dir );
        pinst->min_light = MIN( pinst->min_light, pvert->color_dir );
    }

    // ??coerce this to reasonable values in the presence of negative light??
    if ( pinst->max_light < 0 ) pinst->max_light = 0;
    if ( pinst->min_light < 0 ) pinst->min_light = 0;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_update_bbox( chr_instance_t * pinst )
{
    int           i, frame_count;

    mad_t       * pmad;
    MD2_Model_t * pmd2;
    MD2_Frame_t * frame_list, * pframe_nxt, * pframe_lst;

    if ( NULL == pinst ) return rv_error;

    // get the model. try to heal a bad model.
    if ( !LOADED_MAD( pinst->imad ) ) return rv_error;
    pmad = MadStack.lst + pinst->imad;

    pmd2 = pmad->md2_ptr;
    if ( NULL == pmd2 ) return rv_error;

    frame_count = md2_get_numFrames( pmd2 );
    if ( pinst->frame_nxt >= frame_count ||  pinst->frame_lst >= frame_count ) return rv_error;

    frame_list = ( MD2_Frame_t * )md2_get_Frames( pmd2 );
    pframe_lst = frame_list + pinst->frame_lst;
    pframe_nxt = frame_list + pinst->frame_nxt;

    if ( pinst->frame_nxt == pinst->frame_lst || pinst->flip == 0.0f )
    {
        pinst->bbox = pframe_lst->bb;
    }
    else if ( pinst->flip == 1.0f )
    {
        pinst->bbox = pframe_nxt->bb;
    }
    else
    {
        for ( i = 0; i < OCT_COUNT; i++ )
        {
            pinst->bbox.mins[i] = pframe_lst->bb.mins[i] + ( pframe_nxt->bb.mins[i] - pframe_lst->bb.mins[i] ) * pinst->flip;
            pinst->bbox.maxs[i] = pframe_lst->bb.maxs[i] + ( pframe_nxt->bb.maxs[i] - pframe_lst->bb.maxs[i] ) * pinst->flip;
        }
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_needs_update( chr_instance_t * pinst, int vmin, int vmax, bool_t *verts_match, bool_t *frames_match )
{
    /// @details BB@> determine whether some specific vertices of an instance need to be updated
    //                rv_error   means that the function was passed invalid values
    //                rv_fail    means that the instance does not need to be updated
    //                rv_success means that the instance should be updated

    const float flip_tolerance = 0.25f * 0.5f;  // the flip tolerance is the default flip increment / 2

    bool_t local_verts_match, flips_match, local_frames_match;

    vlst_cache_t * psave;

    mad_t * pmad;
    int maxvert;

    // ensure that the pointers point to something
    if ( NULL == verts_match ) verts_match  = &local_verts_match;
    if ( NULL == frames_match ) frames_match = &local_frames_match;

    // initialize the boolean pointers
    *verts_match  = bfalse;
    *frames_match = bfalse;

    // do we have a valid instance?
    if ( NULL == pinst ) return rv_error;
    psave = &( pinst->save );

    // do we hace a valid mad?
    if ( !LOADED_MAD( pinst->imad ) ) return rv_error;
    pmad = MadStack.lst + pinst->imad;

    // check to see if the vlst_cache has been marked as invalid.
    // in this case, everything needs to be updated
    if ( !psave->valid ) return rv_success;

    // get the last valid vertex from the chr_instance
    maxvert = (( int )pinst->vrt_count ) - 1;

    // check to make sure the lower bound of the saved data is valid.
    // it is initialized to an invalid value (psave->vmin = psave->vmax = -1)
    if ( psave->vmin < 0 || psave->vmax < 0 ) return rv_success;

    // check to make sure the upper bound of the saved data is valid.
    if ( psave->vmin > maxvert || psave->vmax > maxvert ) return rv_success;

    // make sure that the min and max vertices are in the correct order
    if ( vmax < vmin ) SWAP( int, vmax, vmin );

    // test to see if we have already calculated this data
    *verts_match = ( vmin >= psave->vmin ) && ( vmax <= psave->vmax );

    flips_match = ( ABS( psave->flip - pinst->flip ) < flip_tolerance );

    *frames_match = ( pinst->frame_nxt == pinst->frame_lst && psave->frame_nxt == pinst->frame_nxt && psave->frame_lst == pinst->frame_lst ) ||
                    ( flips_match && psave->frame_nxt == pinst->frame_nxt && psave->frame_lst == pinst->frame_lst );

    return ( !( *verts_match ) || !( *frames_match ) ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_update_vertices( chr_instance_t * pinst, int vmin, int vmax, bool_t force )
{
    int    i, maxvert, frame_count;
    bool_t vertices_match, frames_match;

    egoboo_rv retval;

    vlst_cache_t * psave;

    mad_t       * pmad;
    MD2_Model_t * pmd2;
    MD2_Frame_t * frame_list, * pframe_nxt, * pframe_lst;

    if ( NULL == pinst ) return rv_error;
    psave = &( pinst->save );

    if ( rv_error == chr_instance_update_bbox( pinst ) )
    {
        return rv_error;
    }

    // get the model
    if ( !LOADED_MAD( pinst->imad ) ) return rv_error;
    pmad = MadStack.lst + pinst->imad;

    pmd2 = pmad->md2_ptr;
    if ( NULL == pmd2 ) return rv_error;

    // make sure we have valid data
    if ( pinst->vrt_count != md2_get_numVertices( pmd2 ) )
    {
        log_error( "chr_instance_update_vertices() - character instance vertex data does not match its md2\n" );
    }

    // get the vertex list size from the chr_instance
    maxvert = (( int )pinst->vrt_count ) - 1;

    // handle the default parameters
    if ( vmin < 0 ) vmin = 0;
    if ( vmax < 0 ) vmax = maxvert;

    // are they in the right order?
    if ( vmax < vmin ) SWAP( int, vmax, vmin );

    if ( force )
    {
        // force an update of vertices

        // select a range that encompases the requested vertices and the saved vertices
        // if this is the 1st update, the saved vertices may be set to invalid values, as well
        vmin = ( psave->vmin < 0 ) ? vmin : MIN( vmin, psave->vmin );
        vmax = ( psave->vmax < 0 ) ? vmax : MAX( vmax, psave->vmax );

        // force the routine to update
        vertices_match = bfalse;
        frames_match   = bfalse;
    }
    else
    {
        // make sure that the vertices are within the max range
        vmin = CLIP( vmin, 0, maxvert );
        vmax = CLIP( vmax, 0, maxvert );

        // do we need to update?
        retval = chr_instance_needs_update( pinst, vmin, vmax, &vertices_match, &frames_match );
        if ( rv_error == retval ) return rv_error;            // rv_error == retval means some pointer or reference is messed up
        if ( rv_fail  == retval ) return rv_success;          // rv_fail  == retval means we do not need to update this round
    }

    // make sure the frames are in the valid range
    frame_count = md2_get_numFrames( pmd2 );
    if ( pinst->frame_nxt >= frame_count || pinst->frame_lst >= frame_count )
    {
        log_error( "chr_instance_update_vertices() - character instance frame is outside the range of its md2\n" );
    }

    // grab the frame data from the correct model
    frame_list = ( MD2_Frame_t * )md2_get_Frames( pmd2 );
    pframe_nxt = frame_list + pinst->frame_nxt;
    pframe_lst = frame_list + pinst->frame_lst;

    if ( pinst->frame_nxt == pinst->frame_lst || pinst->flip == 0.0f )
    {
        for ( i = vmin; i <= vmax; i++ )
        {
            Uint16 vrta_lst;

            pinst->vrt_lst[i].pos[XX] = pframe_lst->vertex_lst[i].pos.x;
            pinst->vrt_lst[i].pos[YY] = pframe_lst->vertex_lst[i].pos.y;
            pinst->vrt_lst[i].pos[ZZ] = pframe_lst->vertex_lst[i].pos.z;
            pinst->vrt_lst[i].pos[WW] = 1.0f;

            pinst->vrt_lst[i].nrm[XX] = pframe_lst->vertex_lst[i].nrm.x;
            pinst->vrt_lst[i].nrm[YY] = pframe_lst->vertex_lst[i].nrm.y;
            pinst->vrt_lst[i].nrm[ZZ] = pframe_lst->vertex_lst[i].nrm.z;

            vrta_lst = pframe_lst->vertex_lst[i].normal;

            pinst->vrt_lst[i].env[XX] = indextoenvirox[vrta_lst];
            pinst->vrt_lst[i].env[YY] = 0.5f * ( 1.0f + pinst->vrt_lst[i].nrm[ZZ] );
        }
    }
    else if ( pinst->flip == 1.0f )
    {
        for ( i = vmin; i <= vmax; i++ )
        {
            Uint16 vrta_nxt;

            pinst->vrt_lst[i].pos[XX] = pframe_nxt->vertex_lst[i].pos.x;
            pinst->vrt_lst[i].pos[YY] = pframe_nxt->vertex_lst[i].pos.y;
            pinst->vrt_lst[i].pos[ZZ] = pframe_nxt->vertex_lst[i].pos.z;
            pinst->vrt_lst[i].pos[WW] = 1.0f;

            pinst->vrt_lst[i].nrm[XX] = pframe_nxt->vertex_lst[i].nrm.x;
            pinst->vrt_lst[i].nrm[YY] = pframe_nxt->vertex_lst[i].nrm.y;
            pinst->vrt_lst[i].nrm[ZZ] = pframe_nxt->vertex_lst[i].nrm.z;

            vrta_nxt = pframe_nxt->vertex_lst[i].normal;

            pinst->vrt_lst[i].env[XX] = indextoenvirox[vrta_nxt];
            pinst->vrt_lst[i].env[YY] = 0.5f * ( 1.0f + pinst->vrt_lst[i].nrm[ZZ] );
        }
    }
    else
    {
        for ( i = vmin; i <= vmax; i++ )
        {
            Uint16 vrta_lst, vrta_nxt;

            pinst->vrt_lst[i].pos[XX] = pframe_lst->vertex_lst[i].pos.x + ( pframe_nxt->vertex_lst[i].pos.x - pframe_lst->vertex_lst[i].pos.x ) * pinst->flip;
            pinst->vrt_lst[i].pos[YY] = pframe_lst->vertex_lst[i].pos.y + ( pframe_nxt->vertex_lst[i].pos.y - pframe_lst->vertex_lst[i].pos.y ) * pinst->flip;
            pinst->vrt_lst[i].pos[ZZ] = pframe_lst->vertex_lst[i].pos.z + ( pframe_nxt->vertex_lst[i].pos.z - pframe_lst->vertex_lst[i].pos.z ) * pinst->flip;
            pinst->vrt_lst[i].pos[WW] = 1.0f;

            pinst->vrt_lst[i].nrm[XX] = pframe_lst->vertex_lst[i].nrm.x + ( pframe_nxt->vertex_lst[i].nrm.x - pframe_lst->vertex_lst[i].nrm.x ) * pinst->flip;
            pinst->vrt_lst[i].nrm[YY] = pframe_lst->vertex_lst[i].nrm.y + ( pframe_nxt->vertex_lst[i].nrm.y - pframe_lst->vertex_lst[i].nrm.y ) * pinst->flip;
            pinst->vrt_lst[i].nrm[ZZ] = pframe_lst->vertex_lst[i].nrm.z + ( pframe_nxt->vertex_lst[i].nrm.z - pframe_lst->vertex_lst[i].nrm.z ) * pinst->flip;

            vrta_lst = pframe_lst->vertex_lst[i].normal;
            vrta_nxt = pframe_nxt->vertex_lst[i].normal;

            pinst->vrt_lst[i].env[XX] = indextoenvirox[vrta_lst] + ( indextoenvirox[vrta_nxt] - indextoenvirox[vrta_lst] ) * pinst->flip;
            pinst->vrt_lst[i].env[YY] = 0.5f * ( 1.0f + pinst->vrt_lst[i].nrm[ZZ] );
        }
    }

    // update the saved parameters
    return chr_instance_update_vlst_cache( pinst, vmax, vmin, force, vertices_match, frames_match );
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_update_vlst_cache( chr_instance_t * pinst, int vmax, int vmin, bool_t force, bool_t vertices_match, bool_t frames_match )
{
    // this is getting a bit ugly...
    // we need to do this calculation as little as possible, so it is important that the
    // pinst->save.* values be tested and stored properly

    bool_t verts_updated, frames_updated;
    int    maxvert;

    vlst_cache_t * psave;

    if ( NULL == pinst ) return rv_error;
    maxvert = (( int )pinst->vrt_count ) - 1;
    psave   = &( pinst->save );

    // the save_vmin and save_vmax is the most complex
    verts_updated = bfalse;
    if ( force )
    {
        // to get here, either the specified range was outside the clean range or
        // the animation was updated. In any case, the only vertices that are
        // clean are in the range [vmin, vmax]

        psave->vmin   = vmin;
        psave->vmax   = vmax;
        verts_updated = btrue;
    }
    else if ( vertices_match )
    {
        // The only way to get here is to fail the frames_match test, and pass vertices_match

        // This means that all of the vertices were SUPPOSED TO BE updated,
        // but only the ones in the range [vmin, vmax] actually were.
        psave->vmin = vmin;
        psave->vmax = vmax;
        verts_updated = btrue;
    }
    else if ( frames_match )
    {
        // The only way to get here is to fail the vertices_match test, and pass frames_match test

        // There was no update to the animation,  but there was an update to some of the vertices
        // The clean verrices should be the union of the sets of the vertices updated this time
        // and the oned updated last time.
        //
        //If these ranges are disjoint, then only one of them can be saved. Choose the larger set

        if ( vmax >= psave->vmin && vmin <= psave->vmax )
        {
            // the old list [save_vmin, save_vmax] and the new list [vmin, vmax]
            // overlap, so we can merge them
            psave->vmin = MIN( psave->vmin, vmin );
            psave->vmax = MAX( psave->vmax, vmax );
            verts_updated = btrue;
        }
        else
        {
            // the old list and the new list are disjoint sets, so we are out of luck
            // save the set with the largest number of members
            if (( psave->vmax - psave->vmin ) >= ( vmax - vmin ) )
            {
                // obviously no change...
                psave->vmin = psave->vmin;
                psave->vmax = psave->vmax;
                verts_updated = btrue;
            }
            else
            {
                psave->vmin = vmin;
                psave->vmax = vmax;
                verts_updated = btrue;
            }
        }
    }
    else
    {
        // The only way to get here is to fail the vertices_match test, and fail the frames_match test

        // everything was dirty, so just save the new vertex list
        psave->vmin = vmin;
        psave->vmax = vmax;
        verts_updated = btrue;
    }

    psave->frame_nxt = pinst->frame_nxt;
    psave->frame_lst = pinst->frame_lst;
    psave->flip      = pinst->flip;

    // store the last time there was an update to the animation
    frames_updated = bfalse;
    if ( !frames_match )
    {
        psave->frame_wld = update_wld;
        frames_updated   = btrue;
    }

    // store the time of the last full update
    if ( 0 == vmin && maxvert == vmax )
    {
        psave->vert_wld  = update_wld;
    }

    // mark the saved vlst_cache data as valid
    psave->valid = btrue;

    return ( verts_updated || frames_updated ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_update_grip_verts( chr_instance_t * pinst, Uint16 vrt_lst[], size_t vrt_count )
{
    int vmin, vmax;
    Uint32 cnt;
    size_t count;
    egoboo_rv retval;

    if ( NULL == pinst ) return rv_error;

    if ( NULL == vrt_lst || 0 == vrt_count ) return rv_fail;

    // count the valid attachment points
    vmin = 0xFFFF;
    vmax = 0;
    count = 0;
    for ( cnt = 0; cnt < vrt_count; cnt++ )
    {
        if ( 0xFFFF == vrt_lst[cnt] ) continue;

        vmin = MIN( vmin, vrt_lst[cnt] );
        vmax = MAX( vmax, vrt_lst[cnt] );
        count++;
    }

    // if there are no valid points, there is nothing to do
    if ( 0 == count ) return rv_fail;

    // force the vertices to update
    retval = chr_instance_update_vertices( pinst, vmin, vmax, btrue );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_set_action( chr_instance_t * pinst, int action, bool_t action_ready, bool_t override_action )
{
    int action_old;
    mad_t * pmad;

    // did we get a bad pointer?
    if ( NULL == pinst ) return rv_error;

    // is the action in the valid range?
    if ( action < 0 || action > ACTION_COUNT ) return rv_error;

    // do we have a valid model?
    if ( !LOADED_MAD( pinst->imad ) ) return rv_error;
    pmad = MadStack.lst + pinst->imad;

    // is the chosen action valid?
    if ( !pmad->action_valid[ action ] ) return rv_fail;

    // are we going to check action_ready?
    if ( !override_action && !pinst->action_ready ) return rv_fail;

    // save the old action
    action_old = pinst->action_which;

    // set up the action
    pinst->action_which = action;
    pinst->action_next  = ACTION_DA;
    pinst->action_ready = action_ready;

    // invalidate the vertex list if the action has changed
    if ( action_old != action )
    {
        pinst->save.valid = bfalse;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_set_frame( chr_instance_t * pinst, int frame )
{
    mad_t * pmad;

    // did we get a bad pointer?
    if ( NULL == pinst ) return rv_error;

    // is the action in the valid range?
    if ( pinst->action_which < 0 || pinst->action_which > ACTION_COUNT ) return rv_error;

    // do we have a valid model?
    if ( !LOADED_MAD( pinst->imad ) ) return rv_error;
    pmad = MadStack.lst + pinst->imad;

    // is the current action valid?
    if ( !pmad->action_valid[ pinst->action_which ] ) return rv_fail;

    // is the frame within the valid range for this action?
    if ( frame <  pmad->action_stt[ pinst->action_which ] ) return rv_fail;
    if ( frame >= pmad->action_end[ pinst->action_which ] ) return rv_fail;

    // jump to the next frame
    pinst->flip = 0.0f;
    pinst->ilip = 0;
    pinst->frame_lst = pinst->frame_nxt;
    pinst->frame_nxt = frame;

    // invalidate the vlst_cache
    pinst->save.valid = bfalse;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_set_anim( chr_instance_t * pinst, int action, int frame, bool_t action_ready, bool_t override_action )
{
    egoboo_rv retval;

    if ( NULL == pinst ) return rv_error;

    retval = chr_instance_set_action( pinst, action, action_ready, override_action );
    if ( rv_success != retval ) return retval;

    retval = chr_instance_set_frame( pinst, frame );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_start_anim( chr_instance_t * pinst, int action, bool_t action_ready, bool_t override_action )
{
    mad_t * pmad;

    if ( NULL == pinst ) return rv_error;

    if ( action < 0 || action >= ACTION_COUNT ) return rv_error;

    if ( !LOADED_MAD( pinst->imad ) ) return rv_error;
    pmad = MadStack.lst + pinst->imad;

    return chr_instance_set_anim( pinst, action, pmad->action_stt[action], action_ready, override_action );
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_increment_action( chr_instance_t * pinst )
{
    /// @details BB@> This function starts the next action for a character

    egoboo_rv retval;
    mad_t * pmad;
    int     action, action_old;
    bool_t  action_ready;

    if ( NULL == pinst ) return rv_error;

    // save the old action
    action_old = pinst->action_which;

    if ( !LOADED_MAD( pinst->imad ) ) return rv_error;
    pmad = MadStack.lst + pinst->imad;

    // get the correct action
    action = mad_get_action( pinst->imad, pinst->action_next );

    // determine if the action is one of the types that can be broken at any time
    // D == "dance" and "W" == walk
    action_ready = ACTION_IS_TYPE( action, D ) || ACTION_IS_TYPE( action, W );

    retval = chr_instance_start_anim( pinst, action, action_ready, btrue );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_increment_frame( chr_instance_t * pinst, mad_t * pmad, const CHR_REF imount )
{
    /// @details BB@> all the code necessary to move on to the next frame of the animation

    int tmp_action;

    if ( NULL == pinst || NULL == pmad ) return rv_error;

    // Change frames
    pinst->frame_lst = pinst->frame_nxt;
    pinst->frame_nxt++;

    // detect the end of the animation and handle special end conditions
    if ( pinst->frame_nxt >= pmad->action_end[pinst->action_which] )
    {
        // make sure that the frame_nxt points to a valid frame in this action
        pinst->frame_nxt = pmad->action_end[pinst->action_which] - 1;

        if ( pinst->action_keep )
        {
            // Freeze that anumation at the last frame
            pinst->frame_nxt = pinst->frame_lst;

            // Break a kept action at any time
            pinst->action_ready = btrue;
        }
        else if ( pinst->action_loop )
        {
            // Convert the action into a riding action if the character is mounted
            if ( INGAME_CHR( imount ) )
            {
                tmp_action = mad_get_action( pinst->imad, ACTION_MI );
                chr_instance_start_anim( pinst, tmp_action, btrue, btrue );
            }

            // set the frame to the beginning of the action
            pinst->frame_nxt = pmad->action_stt[pinst->action_which];

            // Break a looped action at any time
            pinst->action_ready = btrue;
        }
        else
        {
            // Go on to the next action. don't let just anything interrupt it?
            chr_instance_increment_action( pinst );
        }
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_play_action( chr_instance_t * pinst, int action, bool_t action_ready )
{
    /// @details ZZ@> This function starts a generic action for a character
    mad_t * pmad;

    if ( NULL == pinst ) return rv_error;

    if ( !LOADED_MAD( pinst->imad ) ) return rv_error;
    pmad = MadStack.lst + pinst->imad;

    action = mad_get_action( pinst->imad, action );

    return chr_instance_start_anim( pinst, action, action_ready, btrue );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
vlst_cache_t * vlst_cache_init( vlst_cache_t * pcache )
{
    if ( NULL == pcache ) return NULL;

    memset( pcache, 0, sizeof( *pcache ) );

    pcache->vmin = -1;
    pcache->vmax = -1;

    return pcache;
}

