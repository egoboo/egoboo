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
static void chr_instance_update_lighting_base( chr_instance_t * pinst, chr_t * pchr, bool_t force );
//static void chr_instance_update_lighting( chr_instance_t * pinst, chr_t * pchr, Uint8 trans, bool_t do_ambient );
//static void chr_instance_update_lighting_ref( chr_instance_t * pinst, chr_t * pchr, Uint8 trans, bool_t do_ambient );

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
bool_t render_one_mad_enviro( Uint16 character, GLXvector4f tint, Uint32 bits )
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

    ptex = NULL;
    if( 0 != (bits & CHR_PHONG) )
    {
        ptex = TxTexture_get_ptr( TX_PHONG );
    }

    if( NULL == ptex )
    {
        ptex = TxTexture_get_ptr( pinst->texture );
    }

    uoffset = pinst->uoffset - PCamera->turn_z_one;
    voffset = pinst->voffset;

    if( 0 != (bits & CHR_REFLECT) )
    {
        GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
        GL_DEBUG(glPushMatrix)();
        GL_DEBUG(glMultMatrixf)(pinst->ref.matrix.v );
    }
    else
    {
        GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
        GL_DEBUG(glPushMatrix)();
        GL_DEBUG(glMultMatrixf)(pinst->matrix.v );
    }

    // Choose texture and matrix
    oglx_texture_Bind( ptex );

    ATTRIB_PUSH( "render_one_mad_enviro", GL_CURRENT_BIT );
    {
        GLXvector4f curr_color;

        GL_DEBUG(glGetFloatv)( GL_CURRENT_COLOR, curr_color );

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
                    GLfloat     cmax;
                    GLXvector4f col;
                    GLfloat     tex[2];

                    GLvertex * pvrt;

                    if ( entry >= entry_count ) break;

                    vertex = ego_md2_data[pmad->md2_ref].cmd.vrt[entry];
                    pvrt = pinst->vlst + vertex;

                    if ( vertex < vrt_count )
                    {
                        // normalize the color so it can be modulated by the phong/environment map
                        col[RR] = pvrt->color_dir * INV_FF;
                        col[GG] = pvrt->color_dir * INV_FF;
                        col[BB] = pvrt->color_dir * INV_FF;
                        col[AA] = 1.0f;

                        cmax = MAX(MAX(col[RR], col[GG]), col[BB]);

                        if( cmax != 0.0f )
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
                        tex[1] = CLIP(cmax, 0.0f, 1.0f);

                        if( 0 != (bits & CHR_PHONG) )
                        {
                            // determine the phong texture coordinates
                            // the default phong is bright in both the forward and back directions...
                            tex[1] = tex[1] * 0.5f + 0.5f;
                        }

                        GL_DEBUG(glColor4fv   )( col       );
                        GL_DEBUG(glNormal3fv  )( pvrt->nrm );
                        GL_DEBUG(glTexCoord2fv)( tex       );
                        GL_DEBUG(glVertex3fv  )( pvrt->pos );
                    }

                    entry++;
                }
            }
            GL_DEBUG_END();
        }
    }

    ATTRIB_POP( "render_one_mad_enviro" );

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPopMatrix)();

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
bool_t render_one_mad_tex( Uint16 character, GLXvector4f tint, Uint32 bits )
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

    if(  0 != (bits & CHR_REFLECT)  )
    {
        GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
        GL_DEBUG(glPushMatrix)();
        GL_DEBUG(glMultMatrixf)( pinst->ref.matrix.v );
    }
    else
    {
        GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
        GL_DEBUG(glPushMatrix)();
        GL_DEBUG(glMultMatrixf)( pinst->matrix.v );
    }

    // Choose texture and matrix
    oglx_texture_Bind( ptex );

    ATTRIB_PUSH( "render_one_mad_tex", GL_CURRENT_BIT );
    {
        // set the basic tint. if the object is marked with CHR_LIGHT
        // the color will not be set again inside the loop
        GL_DEBUG(glColor4fv)( tint );

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
                    GLXvector2f tex;
                    GLXvector4f col;

                    if ( entry >= entry_count ) break;

                    vertex = ego_md2_data[pmad->md2_ref].cmd.vrt[entry];

                    if ( vertex < vrt_count )
                    {
                        GLvertex * pvrt = pinst->vlst + vertex;

                        // determine the texture coordinates
                        tex[0] = ego_md2_data[pmad->md2_ref].cmd.u[entry] + uoffset;
                        tex[1] = ego_md2_data[pmad->md2_ref].cmd.v[entry] + voffset;

                        // determine the vertex color for objects that have
                        // per vertex lighting
                        if( 0 == (bits & CHR_LIGHT) )
                        {
                            float fcol;

                            fcol   = pvrt->color_dir * INV_FF;
                            col[0] = fcol * tint[0];
                            col[1] = fcol * tint[1];
                            col[2] = fcol * tint[2];
                            col[3] = tint[0];

                            if ( 0 != (bits & CHR_PHONG) )
                            {
                                fcol = pinst->color_amb * INV_FF;

                                col[0] += fcol * tint[0];
                                col[1] += fcol * tint[1];
                                col[2] += fcol * tint[2];
                            }

                            col[0] = CLIP( col[0], 0.0f, 1.0f );
                            col[1] = CLIP( col[1], 0.0f, 1.0f );
                            col[2] = CLIP( col[2], 0.0f, 1.0f );

                            GL_DEBUG(glColor4fv)( col );
                        }

                        GL_DEBUG(glNormal3fv)  ( pvrt->nrm );
                        GL_DEBUG(glTexCoord2fv)( tex );
                        GL_DEBUG(glVertex3fv)  ( pvrt->pos );
                    }

                    entry++;
                }
            }
            GL_DEBUG_END();
        }
    }
    ATTRIB_POP("render_one_mad_tex");

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPopMatrix)();

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
void render_chr_bbox( chr_t * pchr )
{
    if( !ACTIVE_PCHR(pchr) ) return;

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

//--------------------------------------------------------------------------------------------
bool_t render_one_mad( Uint16 character, GLXvector4f tint, Uint32 bits )
{
    /// @details ZZ@> This function picks the actual function to use

    chr_t * pchr;
    bool_t retval;

    if ( !ACTIVE_CHR(character) ) return bfalse;
    pchr = ChrList.lst + character;

    if ( pchr->is_hidden ) return bfalse;

    if ( pchr->inst.enviro || 0 != (bits & CHR_PHONG) )
    {
        retval = render_one_mad_enviro( character, tint, bits );
    }
    else
    {
        retval = render_one_mad_tex( character, tint, bits );
    }

#if defined(USE_DEBUG)
    // don't draw the debug stuff for reflections
    if( 0 == (bits & CHR_REFLECT) )
    {
        render_chr_bbox( pchr );
    }

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
    GLXvector4f tint;

    if ( !ACTIVE_CHR(ichr) ) return bfalse;
    pchr = ChrList.lst + ichr;
    pinst = &(pchr->inst);

    if ( pchr->is_hidden ) return bfalse;

    if( !pinst->ref.matrix_valid )
    {
        if( !apply_reflection_matrix( &(pchr->inst), pchr->enviro.floor_level ) )
        {
            return bfalse;
        }
    }

    ATTRIB_PUSH( "render_one_mad_ref", GL_ENABLE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT );
    {
        GL_DEBUG(glEnable)( GL_CULL_FACE );    // GL_ENABLE_BIT

        // cull face CCW because we are rendering a reflected object
        GL_DEBUG(glFrontFace)( GL_CCW );      // GL_POLYGON_BIT

        if (pinst->ref.alpha != 255 && pinst->ref.light == 255)
        {
            GL_DEBUG(glEnable)( GL_BLEND );                                   // GL_ENABLE_BIT
            GL_DEBUG(glBlendFunc)( GL_ALPHA, GL_ONE_MINUS_SRC_ALPHA );                          // GL_COLOR_BUFFER_BIT

            chr_instance_get_tint( pinst, tint, CHR_ALPHA | CHR_REFLECT );

            // the previous call to chr_instance_update_lighting_ref() has actually set the
            // alpha and light for all vertices 
            render_one_mad( ichr, tint, CHR_ALPHA | CHR_REFLECT );
        }

        if ( pinst->ref.light != 255 )
        {
            GL_DEBUG(glEnable)( GL_BLEND );                                   // GL_ENABLE_BIT
            GL_DEBUG(glBlendFunc)( GL_ONE, GL_ONE );                          // GL_COLOR_BUFFER_BIT

            chr_instance_get_tint( pinst, tint, CHR_LIGHT | CHR_REFLECT );

            // the previous call to chr_instance_update_lighting_ref() has actually set the
            // alpha and light for all vertices 
            render_one_mad( ichr, tint, CHR_LIGHT | CHR_REFLECT );
        }

        if ( gfx.phongon && pinst->sheen > 0 )
        {
            GL_DEBUG(glEnable)( GL_BLEND );
            GL_DEBUG(glBlendFunc)( GL_ONE, GL_ONE );

            chr_instance_get_tint( pinst, tint, CHR_PHONG | CHR_REFLECT );

            render_one_mad( ichr, tint, CHR_PHONG | CHR_REFLECT );
        }
    }
    ATTRIB_POP( "render_one_mad_ref" );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_rv chr_update_instance( chr_t * pchr )
{
    chr_instance_t * pinst;
    egoboo_rv retval;

    if( !ACTIVE_PCHR(pchr) ) return rv_error;
    pinst = &(pchr->inst);

    // make sure that the vertices are interpolated
    retval = chr_instance_update_vertices( pinst, -1, -1, btrue );
    if( rv_error == retval )
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

    Uint8  self_light;
    lighting_cache_t global_light, loc_light;

    GLvertex * vlst;

    mad_t * pmad;

    if ( NULL == pinst || NULL == pchr ) return;
    vlst = pinst->vlst;

    // has this already been calculated this update?
    if( !force && pinst->lighting_update_wld >= update_wld ) return;
    pinst->lighting_update_wld = update_wld;

    // make sure the matrix is valid
    chr_update_matrix( pchr, btrue );

    if ( !LOADED_MAD(pinst->imad) ) return;
    pmad = MadList + pinst->imad;
    pinst->vlst_size = ego_md2_data[pmad->md2_ref].vertices;

    // interpolate the lighting for the origin of the object
    interpolate_grid_lighting( PMesh, &global_light, pchr->pos );

    // rotate the lighting data to body_centered coordinates
    project_lighting( &loc_light, &global_light, pinst->matrix );

    self_light = ( 255 == pinst->light ) ? 0 : pinst->light;

    pinst->color_amb = 0.9f * pinst->color_amb + 0.1f * (self_light + (loc_light.hgh.lighting[6] + loc_light.low.lighting[6])*0.5f);

    pinst->max_light = -255;
    pinst->min_light =  255;
    for ( cnt = 0; cnt < pinst->vlst_size; cnt++ )
    {
        Sint16 lite;

        GLvertex * pvert = pinst->vlst + cnt;

        // a simple "height" measurement
        float hgt = pvert->pos[ZZ] * pinst->matrix.CNV(3, 3) + pinst->matrix.CNV(3, 3);

        if ( pvert->nrm[0] == 0.0f && pvert->nrm[1] == 0.0f && pvert->nrm[2] == 0.0f )
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
            lite  = evaluate_lighting_cache( &loc_light, pvert->nrm, hgt, PMesh->mmem.bbox, NULL, NULL );
        }

        pvert->color_dir = 0.9f * pvert->color_dir + 0.1f * lite;

        pinst->max_light = MAX(pinst->max_light, pvert->color_dir);
        pinst->min_light = MIN(pinst->min_light, pvert->color_dir);
    }

    // ??coerce this to reasonable values in the presence of negative light??
    if( pinst->max_light < 0 ) pinst->max_light = 0;
    if( pinst->min_light < 0 ) pinst->min_light = 0;
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
egoboo_rv chr_instance_needs_update( chr_instance_t * pinst, int vmin, int vmax, bool_t *verts_match, bool_t *frames_match )
{
    /// @details BB@> determine whether some specific vertices of an instance need to be updated
    //                rv_error   means that the function was passed invalid values
    //                rv_fail    means that the instance does not need to be updated
    //                rv_success means that the instance should be updated

    bool_t local_verts_match, flips_match, local_frames_match;

    vlst_cache_t * psave;

    mad_t * pmad;
    int maxvert;

    // ensure that the pointers point to something
    if( NULL == verts_match  ) verts_match  = &local_verts_match;
    if( NULL == frames_match ) frames_match = &local_frames_match;

    // initialize the boolean pointers
    *verts_match  = bfalse;
    *frames_match = bfalse;

    if ( NULL == pinst ) return rv_error;
    psave = &(pinst->save);

    // get the model.
    if ( !LOADED_MAD(pinst->imad) ) return rv_error;
    pmad = MadList + pinst->imad;

    maxvert = ego_md2_data[pmad->md2_ref].vertices - 1;

    // check to make sure the lower bound of the saved data is valid.
    // it is initialized to an invalid value (psave->vmin = psave->vmax = -1) 
    if( psave->vmin < 0 || psave->vmax < 0 ) return rv_success;

    // check to make sure the upper bound of the saved data is valid.
    if( psave->vmin > maxvert || psave->vmax > maxvert ) return rv_success;

     // make sure that the min and max vertices lie in the valid range
    if( vmax < vmin ) SWAP(int, vmax, vmin);
    vmin = CLIP(vmin, 0, maxvert);
    vmax = CLIP(vmax, 0, maxvert);

    // test to see if we have already calculated this data
    (*verts_match) = (vmin >= psave->vmin) && (vmax <= psave->vmax);

    flips_match = ( ABS(psave->flip - pinst->flip) < 0.125f );

    (*frames_match) = ( pinst->frame_nxt == pinst->frame_lst && psave->frame_nxt == pinst->frame_nxt && psave->frame_lst == pinst->frame_lst ) ||
                      ( flips_match && psave->frame_nxt == pinst->frame_nxt && psave->frame_lst == pinst->frame_lst );

    return (!(*verts_match) || !(*frames_match)) ? rv_success : rv_fail;
}


//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_update_vertices( chr_instance_t * pinst, int vmin, int vmax, bool_t force )
{
    int    i, maxvert;
    bool_t vertices_match, frames_match;
    bool_t verts_updated, frames_updated;

    egoboo_rv retval;

    vlst_cache_t * psave;

    mad_t * pmad;

    if ( NULL == pinst ) return rv_error;
    psave = &(pinst->save);

    if( rv_error == chr_instance_update_bbox( pinst ) )
    {
         return rv_error;
    }

    // get the model. try to heal a bad model.
    if ( !LOADED_MAD(pinst->imad) ) return rv_error;
    pmad = MadList + pinst->imad;

    maxvert = ego_md2_data[pmad->md2_ref].vertices - 1;

    // handle the default parameters
    if ( vmin < 0 ) vmin = 0;
    if ( vmax < 0 ) vmax = maxvert;

    // are they in the right order?
    if( vmax < vmin ) SWAP(int, vmax, vmin);

    if( force )
    {
        // do the absolute maximum extent
        vmin = MIN(vmin, psave->vmin);
        vmax = MAX(vmax, psave->vmax);
    }

    // make sure that the vertices are within the max range
    vmin = CLIP(vmin, 0, maxvert);
    vmax = CLIP(vmax, 0, maxvert);

    // do we need to update?
    retval = chr_instance_needs_update( pinst, vmin, vmax, &vertices_match, &frames_match);
    if( rv_error == retval ) return rv_error;
    if( rv_fail  == retval ) return rv_success;

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
            pinst->vlst[i].env[YY] = 0.5f * (1.0f + pinst->vlst[i].nrm[ZZ]);
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
            pinst->vlst[i].env[YY] = 0.5f * (1.0f + pinst->vlst[i].nrm[ZZ]);
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
            pinst->vlst[i].env[YY] = 0.5f * (1.0f + pinst->vlst[i].nrm[ZZ]);
        }
    }

    // this is getting a bit ugly...
    // we need to do this calculation as little as possible, so it is important that the
    // save_* values be tested and stored properly

    // the save_vmin and save_vmax is the most complex
    verts_updated = bfalse;
    if( force )
    {
        // to get here, either the specified range was outside the clean range or
        // the animation was updated. In any case, the only vertices that are 
        // clean are in the range [vmin, vmax]

        psave->vmin = vmin;
        psave->vmax = vmax;
        verts_updated = btrue;
    }
    else if( vertices_match )
    {
        // The only way to get here is to fail the frames_match test, and pass vertices_match

        // This means that all of the vertices were SUPPOSED TO BE updated,
        // but only the ones in the range [vmin, vmax] actually were.
        psave->vmin = vmin;
        psave->vmax = vmax;
        verts_updated = btrue;
    }
    else if( frames_match )
    {
        // The only way to get here is to fail the vertices_match test, and pass frames_match test

        // There was no update to the animation,  but there was an update to some of the vertices
        // The clean verrices should be the union of the sets of the vertices updated this time
        // and the oned updated last time. 
        //
        //If these ranges are disjoint, then only one of them can be saved. Choose the larger set

        if( vmax >= psave->vmin && vmin <= psave->vmax )
        {
            // the old list [save_vmin, save_vmax] and the new list [vmin, vmax]
            // overlap, so we can merge them
            psave->vmin = MIN(psave->vmin, vmin);
            psave->vmax = MAX(psave->vmax, vmax);
            verts_updated = btrue;
        }
        else
        {
            // the old list and the new list are disjoint sets, so we are out of luck
            // save the set with the largest number of members
            if( (psave->vmax - psave->vmin) >= (vmax - vmin) )
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
    if( !frames_match )
    {
        psave->frame_wld = update_wld;
        frames_updated        = btrue;
    }

    // store the time of the last full update
    if( 0 == vmin && maxvert == vmax )
    {
        psave->vert_wld  = update_wld;
    }

    return (verts_updated || frames_updated) ? rv_success : rv_fail;
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

//--------------------------------------------------------------------------------------------
egoboo_rv chr_instance_update_grip_verts( chr_instance_t * pinst, Uint16 vrt_lst[], size_t vrt_count )
{
    int cnt, count, vmin, vmax;
    egoboo_rv retval;

    if( NULL == pinst ) return rv_error;

    if( NULL == vrt_lst || 0 == vrt_count ) return rv_fail;

    // count the valid attachment points
    vmin = 0xFFFF;
    vmax = 0;
    count = 0;
    for( cnt = 0; cnt<vrt_count; cnt++ )
    {
        if( 0xFFFF == vrt_lst[cnt] ) continue;

        vmin = MIN(vmin, vrt_lst[cnt]);
        vmax = MAX(vmax, vrt_lst[cnt]);
        count++;
    }

    // if there are no valid points, there is nothing to do
    if( 0 == count ) return rv_fail;

    // force the vertices to update
    retval = chr_instance_update_vertices( pinst, vmin, vmax, btrue );

    return retval;
}

////--------------------------------------------------------------------------------------------
//void chr_instance_update_lighting( chr_instance_t * pinst, chr_t * pchr, Uint8 trans, bool_t do_ambient )
//{
//    /// @details BB@> take the basic lighting info and add tint and alpha information to it
//
//    Uint16 cnt;
//    Uint8  rs, gs, bs;
//    Uint8  self_light;
//
//    mad_t * pmad;
//
//    if ( NULL == pinst || NULL == pchr ) return;
//
//    if ( !LOADED_MAD(pinst->imad) ) return;
//    pmad = MadList + pinst->imad;
//
//    rs = pinst->redshift;
//    gs = pinst->grnshift;
//    bs = pinst->blushift;
//    self_light = ( 255 == pinst->light ) ? 0 : pinst->light;
//
//    if( pinst->color_amb >= 0 )
//    {
//        pinst->col_amb.r = (float)( pinst->color_amb >> rs ) * INV_FF;
//        pinst->col_amb.g = (float)( pinst->color_amb >> gs ) * INV_FF;
//        pinst->col_amb.b = (float)( pinst->color_amb >> bs ) * INV_FF;
//    }
//    else
//    {
//        pinst->col_amb.r = -(float)( (-pinst->color_amb) >> rs ) * INV_FF;
//        pinst->col_amb.g = -(float)( (-pinst->color_amb) >> gs ) * INV_FF;
//        pinst->col_amb.b = -(float)( (-pinst->color_amb) >> bs ) * INV_FF;
//    }
//    pinst->col_amb.a = (trans * INV_FF) * (pinst->alpha * INV_FF);
//
//    pinst->max_light = 0;
//    pinst->min_light = 255;
//    for ( cnt = 0; cnt < pinst->vlst_size; cnt++ )
//    {
//        int r,g,b;
//        Sint16 light;
//
//        light = do_ambient ? (pinst->color_amb + pinst->vlst[cnt].color_dir) : (pinst->vlst[cnt].color_dir);
//
//        if( light < 0 )
//        {
//            r = -(-light) >> rs;
//            g = -(-light) >> gs;
//            b = -(-light) >> bs;
//        }
//        else
//        {
//            r = light >> rs;
//            g = light >> gs;
//            b = light >> bs;
//        }
//
//        pinst->max_light = MAX(pinst->max_light, MAX(MAX(r,g),b));
//        pinst->min_light = MIN(pinst->min_light, MIN(MIN(r,g),b));
//
//        pinst->vlst[cnt].col[RR] = r * INV_FF;
//        pinst->vlst[cnt].col[GG] = g * INV_FF;
//        pinst->vlst[cnt].col[BB] = b * INV_FF;
//        pinst->vlst[cnt].col[AA] = (trans * INV_FF) * (pinst->alpha * INV_FF);
//
//        // coerce these to valid values
//        pinst->vlst[cnt].col[RR] = CLIP( pinst->vlst[cnt].col[RR], 0.0f, 1.0f);
//        pinst->vlst[cnt].col[GG] = CLIP( pinst->vlst[cnt].col[GG], 0.0f, 1.0f);
//        pinst->vlst[cnt].col[BB] = CLIP( pinst->vlst[cnt].col[BB], 0.0f, 1.0f);
//        pinst->vlst[cnt].col[AA] = CLIP( pinst->vlst[cnt].col[AA], 0.0f, 1.0f);
//    }
//
//    // ??coerce this to reasonable values in the presence of negative light??
//    if( pinst->max_light < 0 ) pinst->max_light = 0;
//    if( pinst->min_light < 0 ) pinst->min_light = 0;
//}

////--------------------------------------------------------------------------------------------
//void chr_instance_update_lighting_ref( chr_instance_t * pinst, chr_t * pchr, Uint8 trans, bool_t do_ambient )
//{
//    /// @details BB@> take the basic lighting info and add tint and alpha information to it (for reflected characters)
//
//    Uint16 cnt;
//    Uint8  rs, gs, bs;
//    Uint8  self_light;
//    mad_t * pmad;
//
//    if ( NULL == pinst || NULL == pchr ) return;
//
//    if ( !LOADED_MAD(pinst->imad) ) return;
//    pmad = MadList + pinst->imad;
//
//    rs = pinst->ref.redshift;
//    gs = pinst->ref.grnshift;
//    bs = pinst->ref.blushift;
//    self_light = ( 255 == pinst->ref.light ) ? 0 : pinst->ref.light;
//
//    pinst->max_light = 0;
//    pinst->min_light = 255;
//    for ( cnt = 0; cnt < pinst->vlst_size; cnt++ )
//    {
//        Sint16 light;
//
//        light = do_ambient ? (pinst->color_amb + pinst->vlst[cnt].color_dir) : (pinst->vlst[cnt].color_dir);
//
//        pinst->vlst[cnt].col[RR] = (light >> rs) * INV_FF;
//        pinst->vlst[cnt].col[GG] = (light >> gs) * INV_FF;
//        pinst->vlst[cnt].col[BB] = (light >> bs) * INV_FF;
//        pinst->vlst[cnt].col[AA] = (trans * INV_FF) * (pinst->ref.alpha * INV_FF);
//
//        // coerce these to valid values
//        pinst->vlst[cnt].col[RR] = CLIP( pinst->vlst[cnt].col[RR], 0.0f, 1.0f);
//        pinst->vlst[cnt].col[GG] = CLIP( pinst->vlst[cnt].col[GG], 0.0f, 1.0f);
//        pinst->vlst[cnt].col[BB] = CLIP( pinst->vlst[cnt].col[BB], 0.0f, 1.0f);
//        pinst->vlst[cnt].col[AA] = CLIP( pinst->vlst[cnt].col[AA], 0.0f, 1.0f);
//    }
//}
