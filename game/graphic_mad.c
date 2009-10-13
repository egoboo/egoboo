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

#include "graphic.h"

#include "profile.h"
#include "char.h"
#include "mad.h"

#include "md2.h"
#include "id_md2.h"
#include "camera.h"
#include "game.h"
#include "input.h"
#include "texture.h"

#include "egoboo_setup.h"
#include "egoboo.h"

#include <SDL_opengl.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static egoboo_rv chr_instance_update( Uint16 character, Uint8 trans, bool_t do_ambient );
static void      chr_instance_update_lighting( chr_instance_t * pinst, chr_t * pchr, Uint8 trans, bool_t do_ambient, bool_t force );
static void      chr_instance_update_lighting_ref( chr_instance_t * pinst, chr_t * pchr, Uint8 trans );

static void draw_points( chr_t * pchr, int vrt_offset, int verts );
static void _draw_one_grip_raw( chr_instance_t * pinst, mad_t * pmad, int slot );
static void draw_one_grip( chr_instance_t * pinst, mad_t * pmad, int slot );
static void chr_draw_grips( chr_t * pchr );
static void chr_draw_attached_grip( chr_t * pchr );
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/* Storage for blended vertices */
static GLfloat md2_blendedVertices[MD2_MAX_VERTICES][3];
static GLfloat md2_blendedNormals[MD2_MAX_VERTICES][3];

/* blend_md2_vertices
 * Blends the vertices and normals between 2 frames of a md2 model for animation.
 *
 * @note Only meant to be called from draw_textured_md2, which does the necessary
 * checks to make sure that the inputs are valid.  So this function itself assumes
 * that they are valid.  User beware!
 */
static void blend_md2_vertices( const Md2Model *model, int from_, int to_, float lerp )
{
    struct Md2Frame *from, *to;
    int numVertices, i;

    from = &model->frames[from_];
    to = &model->frames[to_];
    numVertices = model->numVertices;
    if ( lerp <= 0 )
    {
        // copy the vertices in frame 'from' over
        for ( i = 0; i < numVertices; i++ )
        {
            md2_blendedVertices[i][0] = from->vertices[i].x;
            md2_blendedVertices[i][1] = from->vertices[i].y;
            md2_blendedVertices[i][2] = from->vertices[i].z;

            md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][0];
            md2_blendedNormals[i][1] = kMd2Normals[from->vertices[i].normal][1];
            md2_blendedNormals[i][2] = kMd2Normals[from->vertices[i].normal][2];
        }
    }
    else if ( lerp >= 1.0f )
    {
        // copy the vertices in frame 'to'
        for ( i = 0; i < numVertices; i++ )
        {
            md2_blendedVertices[i][0] = to->vertices[i].x;
            md2_blendedVertices[i][1] = to->vertices[i].y;
            md2_blendedVertices[i][2] = to->vertices[i].z;

            md2_blendedNormals[i][0] = kMd2Normals[to->vertices[i].normal][0];
            md2_blendedNormals[i][1] = kMd2Normals[to->vertices[i].normal][1];
            md2_blendedNormals[i][2] = kMd2Normals[to->vertices[i].normal][2];
        }
    }
    else
    {
        // mix the vertices
        for ( i = 0; i < numVertices; i++ )
        {
            md2_blendedVertices[i][0] = from->vertices[i].x +
                                        ( to->vertices[i].x - from->vertices[i].x ) * lerp;
            md2_blendedVertices[i][1] = from->vertices[i].y +
                                        ( to->vertices[i].y - from->vertices[i].y ) * lerp;
            md2_blendedVertices[i][2] = from->vertices[i].z +
                                        ( to->vertices[i].z - from->vertices[i].z ) * lerp;

            md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][0] +
                                       ( kMd2Normals[to->vertices[i].normal][0] - kMd2Normals[from->vertices[i].normal][0] ) * lerp;
            md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][1] +
                                       ( kMd2Normals[to->vertices[i].normal][1] - kMd2Normals[from->vertices[i].normal][1] ) * lerp;
            md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][2] +
                                       ( kMd2Normals[to->vertices[i].normal][2] - kMd2Normals[from->vertices[i].normal][2] ) * lerp;
        }
    }
}

//--------------------------------------------------------------------------------------------
/* draw_textured_md2
 * Draws a Md2Model in the new format
 */
void draw_textured_md2( const Md2Model *model, int from_, int to_, float lerp )
{
    int i, numTriangles;
    const struct Md2TexCoord *tc;
    const struct Md2Triangle *triangles;
    const struct Md2Triangle *tri;
    if ( model == NULL ) return;
    if ( from_ < 0 || from_ >= model->numFrames ) return;
    if ( to_ < 0 || to_ >= model->numFrames ) return;

    blend_md2_vertices( model, from_, to_, lerp );

    numTriangles = model->numTriangles;
    tc = model->texCoords;
    triangles = model->triangles;

    GL_DEBUG(glEnableClientState)(GL_VERTEX_ARRAY );
    GL_DEBUG(glEnableClientState)(GL_NORMAL_ARRAY );

    GL_DEBUG(glVertexPointer)(3, GL_FLOAT, 0, md2_blendedVertices );
    GL_DEBUG(glNormalPointer)(GL_FLOAT, 0, md2_blendedNormals );

    GL_DEBUG(glBegin)(GL_TRIANGLES );
    {
        for ( i = 0; i < numTriangles; i++ )
        {
            tri = &triangles[i];

            GL_DEBUG(glTexCoord2fv)( ( const GLfloat* )&( tc[tri->texCoordIndices[0]] ) );
            GL_DEBUG(glArrayElement)(tri->vertexIndices[0] );

            GL_DEBUG(glTexCoord2fv)( ( const GLfloat* )&( tc[tri->texCoordIndices[1]] ) );
            GL_DEBUG(glArrayElement)(tri->vertexIndices[1] );

            GL_DEBUG(glTexCoord2fv)( ( const GLfloat* )&( tc[tri->texCoordIndices[2]] ) );
            GL_DEBUG(glArrayElement)(tri->vertexIndices[2] );
        }
    }
    GL_DEBUG_END();

    GL_DEBUG(glDisableClientState)(GL_VERTEX_ARRAY );
    GL_DEBUG(glDisableClientState)(GL_NORMAL_ARRAY );
}

//--------------------------------------------------------------------------------------------
bool_t render_one_mad_enviro( Uint16 character, Uint8 trans, bool_t use_reflection )
{
    /// @details ZZ@> This function draws an environment mapped model

    int    cmd_count, vrt_count, entry_count;
    Uint16 cnt, tnc, entry;
    Uint16 vertex;
    float  uoffset, voffset;

    chr_t          * pchr;
    mad_t          * pmad;
    chr_instance_t * pinst;
    oglx_texture   * ptex;

    if ( !ACTIVE_CHR(character) ) return bfalse;
    pchr  = ChrList.lst + character;
    pinst = &(pchr->inst);

    if ( !LOADED_MAD(pinst->imad) ) return bfalse;
    pmad = MadList + pinst->imad;

    ptex = TxTexture_get_ptr( pinst->texture );

    uoffset = pinst->uoffset - PCamera->turn_z_one;
    voffset = pinst->voffset;

    // prepare the object
    chr_instance_update( character, 255, bfalse );

    if( use_reflection )
    {
        // tint the vertices correctly
        chr_instance_update_lighting_ref( pinst, pchr, trans );
    }

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPushMatrix)();

    if( use_reflection )
    {
        GL_DEBUG(glMultMatrixf)(pinst->ref_matrix.v );
    }
    else
    {
        GL_DEBUG(glMultMatrixf)(pinst->matrix.v );
    }

    // Choose texture and matrix
    oglx_texture_Bind( ptex );

    // Render each command
    cmd_count   = MIN(ego_md2_data[pmad->md2_ref].cmd.count,   MAXCOMMAND);
    entry_count = MIN(ego_md2_data[pmad->md2_ref].cmd.entries, MAXCOMMANDENTRIES);
    vrt_count   = MIN(ego_md2_data[pmad->md2_ref].vertices,    MAXVERTICES);

    entry = 0;
    for ( cnt = 0; cnt < cmd_count; cnt++ )
    {
        if ( entry >= entry_count ) break;

        GL_DEBUG(glBegin)(ego_md2_data[pmad->md2_ref].cmd.type[cnt] );
        {
            for ( tnc = 0; tnc < ego_md2_data[pmad->md2_ref].cmd.size[cnt]; tnc++ )
            {
                float     cmax, cmin;
                fvec4_t   col;
                GLfloat tex[2];
                Uint8 tmp_alpha;

                GLvertex * pvrt;

                if ( entry >= entry_count ) break;

                vertex = ego_md2_data[pmad->md2_ref].cmd.vrt[entry];
                pvrt = pinst->vlst + vertex;

                // normalize the color
                cmax = 1.0f;
                cmin = MIN(MIN(pvrt->col_dir[RR], pvrt->col_dir[GG]), pvrt->col_dir[BB]);
                if ( 1.0f != cmin )
                {
                    cmax = MAX(MAX(pvrt->col_dir[RR], pvrt->col_dir[GG]), pvrt->col_dir[BB]);

                    if ( 0.0f == cmax || cmax == cmin )
                    {
                        col.r = col.g = col.b = 1.0f;
                        col.a = 1.0f;
                    }
                    else
                    {
                        col.r = pvrt->col_dir[RR] / cmax;
                        col.g = pvrt->col_dir[GG] / cmax;
                        col.b = pvrt->col_dir[BB] / cmax;
                        col.a = 1.0f;
                    }
                }
                else
                {
                    col.r = col.g = col.b = 1.0f;
                    col.a = 1.0f;
                }

                tmp_alpha = use_reflection ? pinst->ref_alpha : pinst->alpha;
                col.a = (tmp_alpha * trans * INV_FF);
                col.a = CLIP( col.a, 0.0f, 1.0f );

                tex[0] = pvrt->env[XX] + uoffset;
                tex[1] = CLIP(cmax * 0.5f + 0.5f, 0.0f, 1.0f);    // the default phong is bright in both the forward and back directions...

                if ( vertex < vrt_count )
                {
                    GL_DEBUG(glColor4fv)(col.v );
                    GL_DEBUG(glNormal3fv)(pvrt->nrm );
                    GL_DEBUG(glTexCoord2fv)(tex );
                    GL_DEBUG(glVertex3fv)(pvrt->pos );
                }

                entry++;
            }
        }
        GL_DEBUG_END();
    }

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPopMatrix)();

    if( use_reflection )
    {
        chr_instance_update_lighting( pinst, pchr, trans, bfalse, btrue );
    }

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
        z = (pinst->vlst[cnt].pos[ZZ]) + pchr->matrix(3,2);

        // Figure out the fog coloring
        if(z < fogtop)
        {
            if(z < fogbottom)
            {
                pinst->vlst[cnt].specular = alpha;
            }
            else
            {
                z = 1.0f - ((z - fogbottom)/fogdistance);  // 0.0f to 1.0f...  Amount of fog to keep
                red = fogred * z;
                grn = foggrn * z;
                blu = fogblu * z;
                fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                pinst->vlst[cnt].specular = fogspec;
            }
        }
        else
        {
            pinst->vlst[cnt].specular = 0;
        }
    }
}
else
{
    for (cnt = 0; cnt < pmad->transvertices; cnt++)
        pinst->vlst[cnt].specular = 0;
}
*/

//--------------------------------------------------------------------------------------------
bool_t render_one_mad_tex( Uint16 character, Uint8 trans, bool_t use_reflection )
{
    /// @details ZZ@> This function draws a model

    int    cmd_count, vrt_count, entry_count;
    Uint16 cnt, tnc, entry;
    Uint16 vertex;
    float  uoffset, voffset;

    chr_t          * pchr;
    mad_t          * pmad;
    chr_instance_t * pinst;
    oglx_texture   * ptex;

    if ( !ACTIVE_CHR(character) ) return bfalse;
    pchr  = ChrList.lst + character;
    pinst = &(pchr->inst);

    if ( !LOADED_MAD(pinst->imad) ) return bfalse;
    pmad = MadList + pinst->imad;

    // To make life easier
    ptex = TxTexture_get_ptr( pinst->texture );

    uoffset = pinst->uoffset * INV_FFFF;
    voffset = pinst->voffset * INV_FFFF;

    // prepare the object
    chr_instance_update( character, trans, btrue );

    if( use_reflection )
    {
        // tint the vertices correctly
        chr_instance_update_lighting_ref( pinst, pchr, trans );
    }

    // Choose texture and matrix
    oglx_texture_Bind( ptex );

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPushMatrix)();

    if( use_reflection )
    {
        GL_DEBUG(glMultMatrixf)(pinst->ref_matrix.v );
    }
    else
    {
        GL_DEBUG(glMultMatrixf)(pinst->matrix.v );
    }

    // Render each command
    cmd_count   = MIN(ego_md2_data[pmad->md2_ref].cmd.count,   MAXCOMMAND);
    entry_count = MIN(ego_md2_data[pmad->md2_ref].cmd.entries, MAXCOMMANDENTRIES);
    vrt_count   = MIN(ego_md2_data[pmad->md2_ref].vertices,    MAXVERTICES);
    entry = 0;
    for (cnt = 0; cnt < cmd_count; cnt++ )
    {
        if ( entry >= entry_count ) break;

        GL_DEBUG(glBegin)(ego_md2_data[pmad->md2_ref].cmd.type[cnt] );
        {
            for ( tnc = 0; tnc < ego_md2_data[pmad->md2_ref].cmd.size[cnt]; tnc++ )
            {
                GLfloat tex[2];

                if ( entry >= entry_count ) break;

                vertex = ego_md2_data[pmad->md2_ref].cmd.vrt[entry];

                if ( vertex < vrt_count )
                {
                    tex[0] = ego_md2_data[pmad->md2_ref].cmd.u[entry] + uoffset;
                    tex[1] = ego_md2_data[pmad->md2_ref].cmd.v[entry] + voffset;

                    GL_DEBUG(glColor4fv)   ( pinst->vlst[vertex].col );
                    GL_DEBUG(glNormal3fv)  ( pinst->vlst[vertex].nrm );
                    GL_DEBUG(glTexCoord2fv)( tex );
                    GL_DEBUG(glVertex3fv)  ( pinst->vlst[vertex].pos );
                }

                entry++;
            }
        }
        GL_DEBUG_END();
    }

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPopMatrix)();

    if( use_reflection )
    {
        chr_instance_update_lighting( pinst, pchr, trans, btrue, btrue );
    }

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
            z = (pinst->vlst[cnt].pos[ZZ]) + pchr->matrix(3,2);

            // Figure out the fog coloring
            if(z < fogtop)
            {
                if(z < fogbottom)
                {
                    pinst->vlst[cnt].specular = alpha;
                }
                else
                {
                    spek = pinst->vlst[cnt].specular & 255;
                    z = (z - fogbottom)/fogdistance;  // 0.0f to 1.0f...  Amount of old to keep
                    fogtokeep = 1.0f-z;  // 0.0f to 1.0f...  Amount of fog to keep
                    spek = spek * z;
                    red = (fogred * fogtokeep) + spek;
                    grn = (foggrn * fogtokeep) + spek;
                    blu = (fogblu * fogtokeep) + spek;
                    fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                    pinst->vlst[cnt].specular = fogspec;
                }
            }
        }
    }
*/

//--------------------------------------------------------------------------------------------
bool_t render_one_mad( Uint16 character, Uint8 trans, bool_t use_reflection )
{
    /// @details ZZ@> This function picks the actual function to use

    chr_t * pchr;
    bool_t retval;

    if ( !ACTIVE_CHR(character) ) return bfalse;
    pchr = ChrList.lst + character;

    if ( pchr->is_hidden ) return bfalse;

    if ( pchr->inst.enviro )
    {
        retval = render_one_mad_enviro( character, trans, use_reflection );
    }
    else
    {
        retval = render_one_mad_tex( character, trans, use_reflection );
    }

    // don't draw the debug stuff for reflections
    if( !use_reflection )
    {
        //// draw the object bounding box as a part of the graphics debug mode F7
        //if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F7 ) )
        //{
        //    GL_DEBUG(glDisable)( GL_TEXTURE_2D );
        //    {
        //        int cnt;
        //        aabb_t bb;

        //        for ( cnt = 0; cnt < 3; cnt++ )
        //        {
        //            bb.mins[cnt] = bb.maxs[cnt] = pchr->pos.v[cnt];
        //        }

        //        bb.mins[XX] -= pchr->chr_prt_cv.size;
        //        bb.mins[YY] -= pchr->chr_prt_cv.size;

        //        bb.maxs[XX] += pchr->chr_prt_cv.size;
        //        bb.maxs[YY] += pchr->chr_prt_cv.size;
        //        bb.maxs[ZZ] += pchr->chr_prt_cv.height;

        //        GL_DEBUG(glColor4f)(1, 1, 1, 1);
        //        render_aabb( &bb );
        //    }
        //    GL_DEBUG(glEnable)( GL_TEXTURE_2D );
        //}

        // draw the object bounding box as a part of the graphics debug mode F7
        if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F7 ) )
        {
            GL_DEBUG(glDisable)( GL_TEXTURE_2D );
            {
                oct_bb_t bb;

                bb.mins[OCT_X ] = pchr->chr_prt_cv.min_x  + pchr->pos.x;
                bb.mins[OCT_Y ] = pchr->chr_prt_cv.min_y  + pchr->pos.y;
                bb.mins[OCT_Z ] = pchr->chr_prt_cv.min_z  + pchr->pos.z;
                bb.mins[OCT_XY] = pchr->chr_prt_cv.min_xy + ( pchr->pos.x + pchr->pos.y);
                bb.mins[OCT_YX] = pchr->chr_prt_cv.min_yx + (-pchr->pos.x + pchr->pos.y);

                bb.maxs[OCT_X ] = pchr->chr_prt_cv.max_x  + pchr->pos.x;
                bb.maxs[OCT_Y ] = pchr->chr_prt_cv.max_y  + pchr->pos.y;
                bb.maxs[OCT_Z ] = pchr->chr_prt_cv.max_z  + pchr->pos.z;
                bb.maxs[OCT_XY] = pchr->chr_prt_cv.max_xy + ( pchr->pos.x + pchr->pos.y);
                bb.maxs[OCT_YX] = pchr->chr_prt_cv.max_yx + (-pchr->pos.x + pchr->pos.y);

                GL_DEBUG(glColor4f)(1, 1, 1, 1);
                render_oct_bb( &bb, btrue, btrue );
            }
            GL_DEBUG(glEnable)( GL_TEXTURE_2D );
        }

        // the grips and vertrices of all objects
        if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F6 ) )
        {
		    chr_draw_attached_grip( pchr );

		    // draw all the vertices of an object
		    GL_DEBUG( glPointSize( 5 ) );
		    draw_points( pchr, 0, ego_md2_data[pro_get_pmad(pchr->inst.imad)->md2_ref].vertices );
	    }
    }

#if defined(USE_DEBUG)
    // the grips of all objects
    //chr_draw_attached_grip( pchr );

    // draw all the vertices of an object
    //GL_DEBUG( glPointSize( 5 ) );
    //draw_points( pchr, 0, pro_get_pmad(pchr->inst.imad)->md2_data.vertices );
#endif

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t render_one_mad_ref( int ichr )
{
    /// @details ZZ@> This function draws characters reflected in the floor

    chr_t * pchr;
    chr_instance_t * pinst;

    if ( !ACTIVE_CHR(ichr) ) return bfalse;
    pchr = ChrList.lst + ichr;
    pinst = &(pchr->inst);

    if ( pchr->is_hidden ) return bfalse;

    // we need to force the calculation of lighting for the the non-reflected
    // object here, or there will be problems later
    // pinst->save_lighting_update_wld = 0;
    chr_instance_update( ichr, 255, !pinst->enviro );

    if( !pinst->ref_matrix_valid )
    {
        if( !apply_one_reflection( pchr ) )
        {
            return bfalse;
        }
    }

    return render_one_mad( ichr, 255, btrue );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_update( Uint16 character, Uint8 trans, bool_t do_ambient )
{
    chr_t * pchr;
    chr_instance_t * pinst;
    egoboo_rv retval;

    if ( !ACTIVE_CHR(character) ) return bfalse;
    pchr = ChrList.lst + character;
    pinst = &(pchr->inst);

    // make sure that the vertices are interpolated
    retval = chr_instance_update_vertices( pinst, -1, -1 );
    if( rv_error == retval )
    {
        return rv_error;
    }
    else if( rv_success == retval )
    {
        retval = chr_update_collision_size( pchr, btrue );
        if( rv_error == retval )
        {
            return rv_error;
        }
    }

    // do the lighting
    chr_instance_update_lighting( pinst, pchr, trans, do_ambient, bfalse );

    return retval;
}

//--------------------------------------------------------------------------------------------
void chr_instance_update_lighting( chr_instance_t * pinst, chr_t * pchr, Uint8 trans, bool_t do_ambient, bool_t force )
{
    Uint16 cnt;

    Uint16 frame_nxt, frame_lst;
    Uint32 alpha;
    Uint8  rs, gs, bs;
    Uint8  self_light;
    lighting_cache_t global_light, loc_light;
    float min_light;

    mad_t * pmad;

    if ( NULL == pinst || NULL == pchr ) return;

    // has this already been calculated this update?
    if( !force && pinst->save_lighting_update_wld >= update_wld ) return;
    pinst->save_lighting_update_wld = update_wld;

    // make sure the matrix is valid
    chr_update_matrix( pchr, btrue );

    if ( !LOADED_MAD(pinst->imad) ) return;
    pmad = MadList + pinst->imad;

    // To make life easier
    frame_nxt = pinst->frame_nxt;
    frame_lst = pinst->frame_lst;
    alpha = trans;

    // interpolate the lighting for the origin of the object
    interpolate_grid_lighting( PMesh, &global_light, pchr->pos );

    // rotate the lighting data to body_centered coordinates
    project_lighting( &loc_light, &global_light, pinst->matrix );

    min_light = loc_light.max_light;
    for (cnt = 0; cnt < 6; cnt++)
    {
        min_light = MIN(min_light, loc_light.lighting_low[cnt]);
        min_light = MIN(min_light, loc_light.lighting_hgh[cnt]);
    }

    if( min_light > 0 )
    {
        for (cnt = 0; cnt < 6; cnt++)
        {
            loc_light.lighting_low[cnt] -= min_light;
            loc_light.lighting_hgh[cnt] -= min_light;
        }
        loc_light.max_light -= min_light;
    }

    rs = pinst->redshift;
    gs = pinst->grnshift;
    bs = pinst->blushift;
    self_light = ( 255 == pinst->light ) ? 0 : pinst->light;

    pinst->color_amb = 0.9f * pinst->color_amb + 0.1f * (self_light + min_light);

    pinst->col_amb.a = (alpha * INV_FF) * (pinst->alpha * INV_FF);
    if( pinst->color_amb > 0 )
    {
        pinst->col_amb.r = (float)( pinst->color_amb >> rs ) * INV_FF;
        pinst->col_amb.g = (float)( pinst->color_amb >> gs ) * INV_FF;
        pinst->col_amb.b = (float)( pinst->color_amb >> bs ) * INV_FF;
    }
    else
    {
        pinst->col_amb.r = -(float)( (-pinst->color_amb) >> rs ) * INV_FF;
        pinst->col_amb.g = -(float)( (-pinst->color_amb) >> gs ) * INV_FF;
        pinst->col_amb.b = -(float)( (-pinst->color_amb) >> bs ) * INV_FF;
    }

    pinst->col_amb.a = CLIP(pinst->col_amb.a, 0, 1);

    pinst->max_light = 0;
    pinst->min_light = 255;
    for ( cnt = 0; cnt < ego_md2_data[pmad->md2_ref].vertices; cnt++ )
    {
        Sint16 lite;

        // a simple "height" measurement
        float hgt = pinst->vlst[cnt].pos[ZZ] * pinst->matrix.CNV(3, 3) + pinst->matrix.CNV(3, 3);

        if ( pinst->vlst[cnt].nrm[0] == 0.0f && pinst->vlst[cnt].nrm[1] == 0.0f && pinst->vlst[cnt].nrm[2] == 0.0f )
        {
            // this is the "ambient only" index, but it really means to sum up all the light
            GLfloat tnrm[3];
            tnrm[0] = tnrm[1] = tnrm[2] = 1.0f;
            lite  = evaluate_lighting_cache( &loc_light, tnrm, hgt, PMesh->mmem.bbox, NULL, NULL );

            tnrm[0] = tnrm[1] = tnrm[2] = -1.0f;
            lite += evaluate_lighting_cache( &loc_light, tnrm, hgt, PMesh->mmem.bbox, NULL, NULL );

            // average all the directions
            lite /= 6;
        }
        else
        {
            lite  = evaluate_lighting_cache( &loc_light, pinst->vlst[cnt].nrm, hgt, PMesh->mmem.bbox, NULL, NULL );
        }

        pinst->vlst[cnt].color_dir = 0.9f * pinst->vlst[cnt].color_dir + 0.1f * lite;

        if( pinst->vlst[cnt].color_dir > 0 )
        {
            pinst->vlst[cnt].col_dir[RR] = (float)( pinst->vlst[cnt].color_dir >> rs ) * INV_FF;
            pinst->vlst[cnt].col_dir[GG] = (float)( pinst->vlst[cnt].color_dir >> gs ) * INV_FF;
            pinst->vlst[cnt].col_dir[BB] = (float)( pinst->vlst[cnt].color_dir >> bs ) * INV_FF;
        }
        else
        {
            pinst->vlst[cnt].col_dir[RR] = -(float)( (-pinst->vlst[cnt].color_dir) >> rs ) * INV_FF;
            pinst->vlst[cnt].col_dir[GG] = -(float)( (-pinst->vlst[cnt].color_dir) >> gs ) * INV_FF;
            pinst->vlst[cnt].col_dir[BB] = -(float)( (-pinst->vlst[cnt].color_dir) >> bs ) * INV_FF;
        }

        if ( do_ambient )
        {
            Sint16 light = pinst->color_amb + pinst->vlst[cnt].color_dir;
            light = CLIP(light, -255, 255);
            pinst->max_light = MAX(pinst->max_light, light);
            pinst->min_light = MIN(pinst->min_light, light);

            pinst->vlst[cnt].col[RR] = pinst->col_amb.r + pinst->vlst[cnt].col_dir[RR];
            pinst->vlst[cnt].col[GG] = pinst->col_amb.g + pinst->vlst[cnt].col_dir[GG];
            pinst->vlst[cnt].col[BB] = pinst->col_amb.b + pinst->vlst[cnt].col_dir[BB];
            pinst->vlst[cnt].col[AA] = pinst->col_amb.a;
        }
        else
        {
            Uint16 light;

            light = 255 * MAX(MAX(pinst->vlst[cnt].col_dir[RR], pinst->vlst[cnt].col_dir[GG]), pinst->vlst[cnt].col_dir[BB]);
            light = CLIP(light, -255, 255);

            pinst->max_light = MAX(pinst->max_light, light);
            pinst->min_light = MIN(pinst->min_light, light);

            pinst->vlst[cnt].col[RR] = pinst->vlst[cnt].col_dir[RR];
            pinst->vlst[cnt].col[GG] = pinst->vlst[cnt].col_dir[GG];
            pinst->vlst[cnt].col[BB] = pinst->vlst[cnt].col_dir[BB];
            pinst->vlst[cnt].col[AA] = pinst->col_amb.a;
        }

         // coerce these to valid values
         pinst->vlst[cnt].col[RR] = CLIP( pinst->vlst[cnt].col[RR], 0.0f, 1.0f);
         pinst->vlst[cnt].col[GG] = CLIP( pinst->vlst[cnt].col[GG], 0.0f, 1.0f);
         pinst->vlst[cnt].col[BB] = CLIP( pinst->vlst[cnt].col[BB], 0.0f, 1.0f);
         pinst->vlst[cnt].col[AA] = CLIP( pinst->vlst[cnt].col[AA], 0.0f, 1.0f);
    }

    // ??coerce this to reasonable values in the presence of negative light??
    if( pinst->max_light < 0 ) pinst->max_light = 0;
    if( pinst->min_light < 0 ) pinst->min_light = 0;
}

//--------------------------------------------------------------------------------------------
void chr_instance_update_lighting_ref( chr_instance_t * pinst, chr_t * pchr, Uint8 trans )
{
    Uint16 cnt;

    Uint8 tmp_alpha;
    GLfloat tint[4];

    mad_t * pmad;

    if ( NULL == pchr ) return;

    if( NULL == pinst ||  !pinst->ref_matrix_valid ) return;

    // has this already been calculated this update?
    if( pinst->ref_save_lighting_update_wld >= update_wld ) return;
    pinst->ref_save_lighting_update_wld = update_wld;

    // since we are overwriting the vertex lighting, and the
    // non-reflected characters are actually drawn after the
    // reflected characters, we need to force the re-evaluation
    // of the lighting
    pinst->save_lighting_update_wld     = update_wld-1;

    tint[0] = (1 << pinst->redshift) / (float) (1 << pinst->ref_redshift);
    tint[1] = (1 << pinst->grnshift) / (float) (1 << pinst->ref_grnshift);
    tint[2] = (1 << pinst->blushift) / (float) (1 << pinst->ref_blushift);

    tint[3] = 1.0f;
    tmp_alpha = trans *  pinst->alpha * INV_FF;
    if( tmp_alpha != 0 )
    {
        tint[3] = trans * pinst->ref_alpha * INV_FF / (float)tmp_alpha;
    }

    if( tint[0] == 1.0f && tint[1] == 1.0f && tint[2] == 1.0f && tint[3] == 1.0f ) return;

    if ( !LOADED_MAD(pinst->imad) ) return;
    pmad = MadList + pinst->imad;

    // we only really need to tint the color of the vertices
    for ( cnt = 0; cnt < ego_md2_data[pmad->md2_ref].vertices; cnt++ )
    {
        pinst->vlst[cnt].col[RR] *= tint[0];
        pinst->vlst[cnt].col[GG] *= tint[1];
        pinst->vlst[cnt].col[BB] *= tint[2];
        pinst->vlst[cnt].col[AA] *= tint[3];
    }
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_update_bbox( chr_instance_t * pinst )
{
    int    i;
    mad_t * pmad;

    if ( NULL == pinst ) return rv_error;

    // get the model. try to heal a bad model.
    if ( !LOADED_MAD(pinst->imad) ) return rv_error;
    pmad = MadList + pinst->imad;

    if ( pinst->frame_nxt == pinst->frame_lst || pinst->flip == 0.0f )
    {
        pinst->bbox = Md2FrameList[pinst->frame_lst].bbox;
    }
    else if ( pinst->flip == 1.0f )
    {
        pinst->bbox = Md2FrameList[pinst->frame_nxt].bbox;
    }
    else
    {
        for( i=0; i<OCT_COUNT; i++ )
        {
            pinst->bbox.mins[i] = Md2FrameList[pinst->frame_lst].bbox.mins[i] + (Md2FrameList[pinst->frame_nxt].bbox.mins[i] - Md2FrameList[pinst->frame_lst].bbox.mins[i]) * pinst->flip;
            pinst->bbox.maxs[i] = Md2FrameList[pinst->frame_lst].bbox.maxs[i] + (Md2FrameList[pinst->frame_nxt].bbox.maxs[i] - Md2FrameList[pinst->frame_lst].bbox.maxs[i]) * pinst->flip;
        }
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_update_vertices( chr_instance_t * pinst, int vmin, int vmax )
{
    int    i;
    bool_t vertices_match, flips_match, frames_match, verts_updated;

    mad_t * pmad;

    if ( NULL == pinst ) return rv_error;

    if( rv_error == chr_instance_update_bbox( pinst ) )
    {
         return rv_error;
    }

    // get the model. try to heal a bad model.
    if ( !LOADED_MAD(pinst->imad) ) return rv_error;
    pmad = MadList + pinst->imad;

    // handle the default parameters
    if ( vmin < 0 ) vmin = 0;
    if ( vmax < 0 ) vmax = ego_md2_data[pmad->md2_ref].vertices - 1;

    vmin = CLIP(vmin, 0, ego_md2_data[pmad->md2_ref].vertices - 1);
    vmax = CLIP(vmax, 0, ego_md2_data[pmad->md2_ref].vertices - 1);

    // test to see if we have already calculated this data
    vertices_match = (pinst->save_vmin <= vmin) && (pinst->save_vmax >= vmax);

    flips_match = ( pinst->frame_nxt == pinst->frame_lst ) || ( ABS(pinst->save_flip - pinst->flip) < 0.125f );

    frames_match = ( pinst->flip == 1.0f && pinst->save_frame_nxt == pinst->frame_nxt ) ||
                   ( pinst->flip == 0.0f && pinst->save_frame_lst == pinst->frame_lst ) ||
                   ( flips_match  && pinst->save_frame_nxt == pinst->frame_nxt && pinst->save_frame_lst == pinst->frame_lst );

    if ( frames_match && vertices_match ) return bfalse;

    if ( pinst->frame_nxt == pinst->frame_lst || pinst->flip == 0.0f )
    {
        for ( i = vmin; i <= vmax; i++)
        {
            Uint16 vrta_lst;

            pinst->vlst[i].pos[XX] = Md2FrameList[pinst->frame_lst].vrtx[i];
            pinst->vlst[i].pos[YY] = Md2FrameList[pinst->frame_lst].vrty[i];
            pinst->vlst[i].pos[ZZ] = Md2FrameList[pinst->frame_lst].vrtz[i];
            pinst->vlst[i].pos[WW] = 1.0f;

            vrta_lst = Md2FrameList[pinst->frame_lst].vrta[i];

            pinst->vlst[i].nrm[XX] = kMd2Normals[vrta_lst][XX];
            pinst->vlst[i].nrm[YY] = kMd2Normals[vrta_lst][YY];
            pinst->vlst[i].nrm[ZZ] = kMd2Normals[vrta_lst][ZZ];

            pinst->vlst[i].env[XX] = indextoenvirox[vrta_lst];
        }
    }
    else if ( pinst->flip == 1.0f )
    {
        for ( i = vmin; i <= vmax; i++)
        {
            Uint16 vrta_nxt;

            pinst->vlst[i].pos[XX] = Md2FrameList[pinst->frame_nxt].vrtx[i];
            pinst->vlst[i].pos[YY] = Md2FrameList[pinst->frame_nxt].vrty[i];
            pinst->vlst[i].pos[ZZ] = Md2FrameList[pinst->frame_nxt].vrtz[i];
            pinst->vlst[i].pos[WW] = 1.0f;

            vrta_nxt = Md2FrameList[pinst->frame_nxt].vrta[i];

            pinst->vlst[i].nrm[XX] = kMd2Normals[vrta_nxt][XX];
            pinst->vlst[i].nrm[YY] = kMd2Normals[vrta_nxt][YY];
            pinst->vlst[i].nrm[ZZ] = kMd2Normals[vrta_nxt][ZZ];

            pinst->vlst[i].env[XX] = indextoenvirox[vrta_nxt];
        }
    }
    else
    {
        for ( i = vmin; i <= vmax; i++)
        {
            Uint16 vrta_lst, vrta_nxt;

            pinst->vlst[i].pos[XX] = Md2FrameList[pinst->frame_lst].vrtx[i] + (Md2FrameList[pinst->frame_nxt].vrtx[i] - Md2FrameList[pinst->frame_lst].vrtx[i]) * pinst->flip;
            pinst->vlst[i].pos[YY] = Md2FrameList[pinst->frame_lst].vrty[i] + (Md2FrameList[pinst->frame_nxt].vrty[i] - Md2FrameList[pinst->frame_lst].vrty[i]) * pinst->flip;
            pinst->vlst[i].pos[ZZ] = Md2FrameList[pinst->frame_lst].vrtz[i] + (Md2FrameList[pinst->frame_nxt].vrtz[i] - Md2FrameList[pinst->frame_lst].vrtz[i]) * pinst->flip;
            pinst->vlst[i].pos[WW] = 1.0f;

            vrta_lst = Md2FrameList[pinst->frame_lst].vrta[i];
            vrta_nxt = Md2FrameList[pinst->frame_nxt].vrta[i];

            pinst->vlst[i].nrm[XX] = kMd2Normals[vrta_lst][XX] + (kMd2Normals[vrta_nxt][XX] - kMd2Normals[vrta_lst][XX]) * pinst->flip;
            pinst->vlst[i].nrm[YY] = kMd2Normals[vrta_lst][YY] + (kMd2Normals[vrta_nxt][YY] - kMd2Normals[vrta_lst][YY]) * pinst->flip;
            pinst->vlst[i].nrm[ZZ] = kMd2Normals[vrta_lst][ZZ] + (kMd2Normals[vrta_nxt][ZZ] - kMd2Normals[vrta_lst][ZZ]) * pinst->flip;

            pinst->vlst[i].env[XX] = indextoenvirox[vrta_lst] + (indextoenvirox[vrta_nxt] - indextoenvirox[vrta_lst]) * pinst->flip;
        }
    }

    // this is getting a bit ugly...
    // we need to do this calculation as little as possible, so it is important that the
    // save_* values be tested and stored properly

    // the save_vmin and save_vmax is the most complex
    verts_updated = bfalse;
    if( vertices_match )
    {
        // vmin and vmax are within the old vertex range.
        // This means that vertices outside the range [vmin, vmax]
        // were also SUPPOSED TO BE updated, but weren't.
        // Mark it so we know those vertices are dirty.
        pinst->save_vmin = vmin;
        pinst->save_vmax = vmax;
        verts_updated = btrue;
    }
    else if( frames_match )
    {
        // all of the vertices in the old range [save_vmin, save_vmax]
        // are still clean.

        if( vmax >= pinst->save_vmin && vmin <= pinst->save_vmax )
        {
            // the old list [save_vmin, save_vmax] and the new list [vmin, vmax]
            // overlap, so we can merge them
            pinst->save_vmin = MIN(pinst->save_vmin, vmin);
            pinst->save_vmax = MAX(pinst->save_vmax, vmax);
            verts_updated = btrue;
        }
        else
        {
            // the old list and the nre list are disjoint sets, so we are out of luck
            // save the set with the largest number of members
            if( (pinst->save_vmax - pinst->save_vmin) >= (vmax - vmin) )
            {
                pinst->save_vmin = pinst->save_vmin;
                pinst->save_vmax = pinst->save_vmax;
                verts_updated = btrue;
            }
            else
            {
                pinst->save_vmin = vmin;
                pinst->save_vmax = vmax;
                verts_updated = btrue;
            }
        }
    }
    else
    {
        // everything was dirty, so just save the new vertex list
        pinst->save_vmin = vmin;
        pinst->save_vmax = vmax;
        verts_updated = btrue;
    }

    pinst->save_frame     = update_wld;
    pinst->save_frame_nxt = pinst->frame_nxt;
    pinst->save_frame_lst = pinst->frame_lst;
    pinst->save_flip      = pinst->flip;

    // store the time of the last full update
    if( (vmin == 0) && (vmax == ego_md2_data[pmad->md2_ref].vertices - 1) )
    {
        pinst->save_update_wld  = update_wld;
    }

    return (verts_updated || !frames_match) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
void draw_points( chr_t * pchr, int vrt_offset, int verts )
{
    /// @details BB@> a function that will draw some of the vertices of the given character.
    ///     The original idea was to use this to debug the grip for attached items.

    mad_t * pmad;
    int vmin, vmax, cnt;
    GLboolean texture_1d_enabled, texture_2d_enabled;

    if( !ACTIVE_PCHR( pchr ) ) return;

    pmad = chr_get_pmad( GET_INDEX( pchr, MAX_CHR ) );
    if( NULL == pmad ) return;

    vmin = vrt_offset;
    vmax = vmin + verts;

    if( vmin < 0 || vmax < 0 ) return;
    if( vmin > ego_md2_data[pmad->md2_ref].vertices || vmax > ego_md2_data[pmad->md2_ref].vertices ) return;

    texture_1d_enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_1D);
    texture_2d_enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_2D);

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if( texture_1d_enabled ) glDisable( GL_TEXTURE_1D );
    if( texture_2d_enabled ) glDisable( GL_TEXTURE_2D );

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPushMatrix)();
    GL_DEBUG(glMultMatrixf)( pchr->inst.matrix.v );

    GL_DEBUG( glBegin( GL_POINTS ) );
    {
        for(cnt=vmin; cnt<vmax; cnt++)
        {
            glVertex3fv( pchr->inst.vlst[cnt].pos );
        }
    }
    GL_DEBUG_END();

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPopMatrix)();

    if( texture_1d_enabled ) glEnable( GL_TEXTURE_1D );
    if( texture_2d_enabled ) glEnable( GL_TEXTURE_2D );
}

//--------------------------------------------------------------------------------------------
void draw_one_grip( chr_instance_t * pinst, mad_t * pmad, int slot )
{
    GLboolean texture_1d_enabled, texture_2d_enabled;

    texture_1d_enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_1D);
    texture_2d_enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_2D);

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if( texture_1d_enabled ) glDisable( GL_TEXTURE_1D );
    if( texture_2d_enabled ) glDisable( GL_TEXTURE_2D );

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPushMatrix)();
    GL_DEBUG(glMultMatrixf)( pinst->matrix.v );

    _draw_one_grip_raw( pinst, pmad, slot );

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPopMatrix)();

    if( texture_1d_enabled ) glEnable( GL_TEXTURE_1D );
    if( texture_2d_enabled ) glEnable( GL_TEXTURE_2D );
}

//--------------------------------------------------------------------------------------------
void _draw_one_grip_raw( chr_instance_t * pinst, mad_t * pmad, int slot )
{
    int vmin, vmax, cnt;

    float red[4] = {1,0,0,1};
    float grn[4] = {0,1,0,1};
    float blu[4] = {0,0,1,1};
    float * col_ary[3];

    col_ary[0] = red;
    col_ary[1] = grn;
    col_ary[2] = blu;

    if( NULL == pinst || NULL == pmad ) return;

    vmin = ego_md2_data[pmad->md2_ref].vertices - slot_to_grip_offset( slot );
    vmax = vmin + GRIP_VERTS;

    if( vmin >= 0 && vmax >= 0 && vmax <= ego_md2_data[pmad->md2_ref].vertices )
    {
        fvec3_t   src, dst, diff;

        GL_DEBUG(glBegin)( GL_LINES );
        {
            for( cnt = 1; cnt < GRIP_VERTS; cnt++ )
            {
                src.x = pinst->vlst[vmin].pos[XX];
                src.y = pinst->vlst[vmin].pos[YY];
                src.z = pinst->vlst[vmin].pos[ZZ];

                diff.x = pinst->vlst[vmin+cnt].pos[XX] - src.x;
                diff.y = pinst->vlst[vmin+cnt].pos[YY] - src.y;
                diff.z = pinst->vlst[vmin+cnt].pos[ZZ] - src.z;

                dst.x = src.x + 3 * diff.x;
                dst.y = src.y + 3 * diff.y;
                dst.z = src.z + 3 * diff.z;

                glColor4fv( col_ary[cnt-1] );

                glVertex3fv( src.v );
                glVertex3fv( dst.v );
            }
        }
        GL_DEBUG_END();
    }

    glColor4f( 1,1,1,1 );
}

//--------------------------------------------------------------------------------------------
void chr_draw_attached_grip( chr_t * pchr )
{
    mad_t * pholder_mad;
    cap_t * pholder_cap;
    chr_t * pholder;

    if( !ACTIVE_PCHR( pchr ) ) return;

    if( !ACTIVE_CHR(pchr->attachedto) ) return;
    pholder = ChrList.lst + pchr->attachedto;

    pholder_cap = pro_get_pcap( pholder->iprofile );
    if( NULL == pholder_cap ) return;

    pholder_mad = chr_get_pmad( GET_INDEX( pholder, MAX_CHR ) );
    if( NULL == pholder_mad ) return;

    draw_one_grip( &(pholder->inst), pholder_mad, pchr->inwhich_slot );
}

//--------------------------------------------------------------------------------------------
void chr_draw_grips( chr_t * pchr )
{
    mad_t * pmad;
    cap_t * pcap;

    int slot;
    GLboolean texture_1d_enabled, texture_2d_enabled;

    if( !ACTIVE_PCHR( pchr ) ) return;

    pcap = pro_get_pcap( pchr->iprofile );
    if( NULL == pcap ) return;

    pmad = chr_get_pmad( GET_INDEX( pchr, MAX_CHR ) );
    if( NULL == pmad ) return;

    texture_1d_enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_1D);
    texture_2d_enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_2D);

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if( texture_1d_enabled ) glDisable( GL_TEXTURE_1D );
    if( texture_2d_enabled ) glDisable( GL_TEXTURE_2D );

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPushMatrix)();
    GL_DEBUG(glMultMatrixf)( pchr->inst.matrix.v );

    slot = SLOT_LEFT;
    if( pcap->slotvalid[slot] )
    {
        _draw_one_grip_raw( &(pchr->inst), pmad, slot );
    }

    slot = SLOT_RIGHT;
    if( pcap->slotvalid[slot] )
    {
        _draw_one_grip_raw( &(pchr->inst), pmad, slot );
    }

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPopMatrix)();

    if( texture_1d_enabled ) glEnable( GL_TEXTURE_1D );
    if( texture_2d_enabled ) glEnable( GL_TEXTURE_2D );
}