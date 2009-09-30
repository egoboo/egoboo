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

/* Egoboo - graphic_mad.c
 * Character model drawing code.
 */

#include "graphic.h"

#include "md2.h"
#include "id_md2.h"
#include "camera.h"
#include "char.h"
#include "mad.h"
#include "game.h"
#include "input.h"
#include "texture.h"

#include "egoboo_setup.h"
#include "egoboo.h"

#include <SDL_opengl.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void chr_instance_update( Uint16 character, Uint8 trans, bool_t do_lighting );
static void chr_instance_update_lighting( chr_instance_t * pinst, chr_t * pchr, Uint8 trans, bool_t do_lighting );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/* Storage for blended vertices */
static GLfloat md2_blendedVertices[MD2_MAX_VERTICES][3];
static GLfloat md2_blendedNormals[MD2_MAX_VERTICES][3];

/* blend_md2_vertices
 * Blends the vertices and normals between 2 frames of a md2 model for animation.
 *
 * NOTE: Only meant to be called from draw_textured_md2, which does the necessary
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
void render_one_mad_enviro( Uint16 character, Uint8 trans )
{
    // ZZ> This function draws an environment mapped model

    int    cmd_count, vrt_count, entry_count;
    Uint16 cnt, tnc, entry;
    Uint16 vertex;
    float  uoffset, voffset;

    chr_t          * pchr;
    mad_t          * pmad;
    chr_instance_t * pinst;
    oglx_texture   * ptex;

    if ( !ACTIVE_CHR(character) ) return;
    pchr  = ChrList.lst + character;
    pinst = &(pchr->inst);

    if ( INVALID_MAD(pinst->imad) ) return;
    pmad = MadList + pinst->imad;

    ptex = TxTexture_get_ptr( pinst->texture );

    uoffset = pinst->uoffset - PCamera->turn_z_one;
    voffset = pinst->voffset;

    // prepare the object
    chr_instance_update( character, 255, bfalse );

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPushMatrix)();
    GL_DEBUG(glMultMatrixf)(pinst->matrix.v );

    // Choose texture and matrix
    oglx_texture_Bind( ptex );

    // Render each command
    cmd_count   = MIN(pmad->md2_data.cmd.count,   MAXCOMMAND);
    entry_count = MIN(pmad->md2_data.cmd.entries, MAXCOMMANDENTRIES);
    vrt_count   = MIN(pmad->md2_data.vertices,    MAXVERTICES);
    entry = 0;
    for ( cnt = 0; cnt < cmd_count; cnt++ )
    {
        if ( entry >= entry_count ) break;

        GL_DEBUG(glBegin)(pmad->md2_data.cmd.type[cnt] );
        {
            for ( tnc = 0; tnc < pmad->md2_data.cmd.size[cnt]; tnc++ )
            {
                float     cmax, cmin;
                GLvector4 col;
                GLfloat tex[2];

                if ( entry >= entry_count ) break;

                vertex = pmad->md2_data.cmd.vrt[entry];

                // normalize the color
                cmax = 1.0f;
                cmin = MIN(MIN(pinst->vlst[vertex].col_dir[RR], pinst->vlst[vertex].col_dir[GG]), pinst->vlst[vertex].col_dir[BB]);
                if ( 1.0f != cmin )
                {
                    cmax = MAX(MAX(pinst->vlst[vertex].col_dir[RR], pinst->vlst[vertex].col_dir[GG]), pinst->vlst[vertex].col_dir[BB]);

                    if ( 0.0f == cmax || cmax == cmin )
                    {
                        col.r = col.g = col.b = 1.0f;
                        col.a = 1.0f;
                    }
                    else
                    {
                        col.r = pinst->vlst[vertex].col_dir[RR] / cmax;
                        col.g = pinst->vlst[vertex].col_dir[GG] / cmax;
                        col.b = pinst->vlst[vertex].col_dir[BB] / cmax;
                        col.a = 1.0f;
                    }
                }
                else
                {
                    col.r = col.g = col.b = 1.0f;
                    col.a = 1.0f;
                }

                col.a = (pinst->alpha * INV_FF) * (trans * INV_FF);
                col.a = CLIP( col.a, 0.0f, 1.0f );

                tex[0] = pinst->vlst[vertex].env[XX] + uoffset;
                tex[1] = CLIP(cmax * 0.5f + 0.5f, 0.0f, 1.0f);    // the default phong is bright in both the forward and back directions...

                if ( vertex < vrt_count )
                {
                    GL_DEBUG(glColor4fv)(col.v );
                    GL_DEBUG(glNormal3fv)(pinst->vlst[vertex].nrm );
                    GL_DEBUG(glTexCoord2fv)(tex );
                    GL_DEBUG(glVertex3fv)(pinst->vlst[vertex].pos );
                }

                entry++;
            }
        }
        GL_DEBUG_END();
    }

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPopMatrix)();
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
void render_one_mad_tex( Uint16 character, Uint8 trans )
{
    // ZZ> This function draws a model

    int    cmd_count, vrt_count, entry_count;
    Uint16 cnt, tnc, entry;
    Uint16 vertex;
    float  uoffset, voffset;

    chr_t          * pchr;
    mad_t          * pmad;
    chr_instance_t * pinst;
    oglx_texture   * ptex;

    if ( !ACTIVE_CHR(character) ) return;
    pchr  = ChrList.lst + character;
    pinst = &(pchr->inst);

    if ( INVALID_MAD(pinst->imad) ) return;
    pmad = MadList + pinst->imad;

    // To make life easier
    ptex = TxTexture_get_ptr( pinst->texture );

    uoffset = pinst->uoffset * INV_FFFF;
    voffset = pinst->voffset * INV_FFFF;

    // prepare the object
    chr_instance_update( character, trans, btrue );

    // Choose texture and matrix
    oglx_texture_Bind( ptex );

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPushMatrix)();
    GL_DEBUG(glMultMatrixf)(pinst->matrix.v );

    // Render each command
    cmd_count   = MIN(pmad->md2_data.cmd.count,   MAXCOMMAND);
    entry_count = MIN(pmad->md2_data.cmd.entries, MAXCOMMANDENTRIES);
    vrt_count   = MIN(pmad->md2_data.vertices,    MAXVERTICES);
    entry = 0;
    for (cnt = 0; cnt < cmd_count; cnt++ )
    {
        if ( entry >= entry_count ) break;

        GL_DEBUG(glBegin)(pmad->md2_data.cmd.type[cnt] );
        {
            for ( tnc = 0; tnc < pmad->md2_data.cmd.size[cnt]; tnc++ )
            {
                GLfloat tex[2];

                if ( entry >= entry_count ) break;

                vertex = pmad->md2_data.cmd.vrt[entry];

                if ( vertex < vrt_count )
                {

                    tex[0] = pmad->md2_data.cmd.u[entry] + uoffset;
                    tex[1] = pmad->md2_data.cmd.v[entry] + voffset;

                    GL_DEBUG(glColor4fv)(pinst->vlst[vertex].col );
                    GL_DEBUG(glNormal3fv)(pinst->vlst[vertex].nrm );
                    GL_DEBUG(glTexCoord2fv)(tex );
                    GL_DEBUG(glVertex3fv)(pinst->vlst[vertex].pos );
                }

                entry++;
            }
        }
        GL_DEBUG_END();
    }

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glPopMatrix)();
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
void render_one_mad( Uint16 character, Uint8 trans )
{
    // ZZ> This function picks the actual function to use

    chr_t * pchr;

    if ( !ACTIVE_CHR(character) ) return;
    pchr = ChrList.lst + character;

    if ( pchr->is_hidden ) return;

    if ( pchr->inst.enviro )
    {
        render_one_mad_enviro( character, trans );
    }
    else
    {
        render_one_mad_tex( character, trans );
    }

    // draw the object bounding box as a part of the graphics debug mode F7
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F7 ) )
    {
        GL_DEBUG(glDisable)( GL_TEXTURE_2D );
        {
            int cnt;
            aabb_t bb;

            for ( cnt = 0; cnt < 3; cnt++ )
            {
                bb.mins[cnt] = bb.maxs[cnt] = pchr->pos.v[cnt];
            }

            bb.mins[XX] -= pchr->bump.size;
            bb.mins[YY] -= pchr->bump.size;

            bb.maxs[XX] += pchr->bump.size;
            bb.maxs[YY] += pchr->bump.size;
            bb.maxs[ZZ] += pchr->bump.height;

            GL_DEBUG(glColor4f)(1, 1, 1, 1);
            bbox_gl_draw( &bb );
        }
        GL_DEBUG(glEnable)( GL_TEXTURE_2D );
    }
}

//--------------------------------------------------------------------------------------------
void render_one_mad_ref( int tnc, Uint8 trans )
{
    // ZZ> This function draws characters reflected in the floor

    float level;
    int trans_temp;
    int pos_z;
    Uint8 sheen_save;
    GLvector4 pos_save;
    chr_t * pchr;
    cap_t * pcap;
    chr_instance_t * pinst;

    if ( !ACTIVE_CHR(tnc) ) return;
    pchr = ChrList.lst + tnc;
    pinst = &(pchr->inst);

    if ( pchr->is_hidden ) return;

    // we need to force the calculation of lighting for the the non-reflected
    // object here, or there will be problems later
    //pinst->save_lighting_wldframe = 0;
    chr_instance_update( tnc, trans, !pinst->enviro );

    pcap = chr_get_pcap( tnc );
    if ( NULL == pcap || !pcap->reflect ) return;

    level = pchr->floor_level;
    trans_temp = FF_MUL( pchr->inst.alpha, trans) >> 1;

    pos_z = pinst->matrix.CNV( 3, 2 ) - level;
    if (pos_z < 0) pos_z = 0;

    trans_temp -= pos_z >> 1;
    if ( trans_temp < 0 ) trans_temp = 0;

    trans_temp |= gfx.reffadeor;  // Fix for Riva owners
    trans_temp = CLIP(trans_temp, 0, 255);

    pinst->redshift += 1;
    pinst->grnshift += 1;
    pinst->blushift += 1;

    sheen_save = pinst->sheen;
    pinst->sheen >>= 1;

    pos_save.x = pinst->matrix.CNV( 0, 2 );
    pos_save.y = pinst->matrix.CNV( 1, 2 );
    pos_save.z = pinst->matrix.CNV( 2, 2 );
    pos_save.w = pinst->matrix.CNV( 3, 2 );

    pinst->matrix.CNV( 0, 2 ) = -pinst->matrix.CNV( 0, 2 );
    pinst->matrix.CNV( 1, 2 ) = -pinst->matrix.CNV( 1, 2 );
    pinst->matrix.CNV( 2, 2 ) = -pinst->matrix.CNV( 2, 2 );
    pinst->matrix.CNV( 3, 2 ) = -pinst->matrix.CNV( 3, 2 ) + level + level;

    render_one_mad( tnc, trans_temp );

    pinst->matrix.CNV( 0, 2 ) = pos_save.x;
    pinst->matrix.CNV( 1, 2 ) = pos_save.y;
    pinst->matrix.CNV( 2, 2 ) = pos_save.z;
    pinst->matrix.CNV( 3, 2 ) = pos_save.w;

    pinst->sheen = sheen_save;

    pinst->redshift -= 1;
    pinst->grnshift -= 1;
    pinst->blushift -= 1;

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void chr_instance_update( Uint16 character, Uint8 trans, bool_t do_lighting )
{
    chr_t * pchr;
    chr_instance_t * pinst;

    if ( !ACTIVE_CHR(character) ) return;
    pchr = ChrList.lst + character;
    pinst = &(pchr->inst);

    // make sure that the vertices are interpolated
    chr_instance_update_vertices( pinst, -1, -1 );

    // do the lighting
    chr_instance_update_lighting( pinst, pchr, trans, do_lighting );
}

//--------------------------------------------------------------------------------------------
void chr_instance_update_lighting( chr_instance_t * pinst, chr_t * pchr, Uint8 trans, bool_t do_lighting )
{
    Uint16 cnt;

    Uint16 frame_nxt, frame_lst;
    Uint32 alpha;
    Uint8  rs, gs, bs;
    float  flip;
    Uint8  self_light;
    lighting_cache_t global_light, loc_light;
    float min_light;

    mad_t * pmad;

    if ( NULL == pinst || NULL == pchr ) return;

    // has this already been calculated this update?
    if( pinst->save_lighting_wldframe >= update_wld ) return;
    pinst->save_lighting_wldframe = update_wld;

    // make sure the matrix is valid
    _chr_update_matrix( pchr );

    if ( INVALID_MAD(pinst->imad) ) return;
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

    for (cnt = 0; cnt < 6; cnt++)
    {
        loc_light.lighting_low[cnt] -= min_light;
        loc_light.lighting_hgh[cnt] -= min_light;
    }
    loc_light.max_light -= min_light;

    rs = pinst->redshift;
    gs = pinst->grnshift;
    bs = pinst->blushift;
    flip = pinst->lip / 256.0f;
    self_light = ( 255 == pinst->light ) ? 0 : pinst->light;

    pinst->color_amb = 0.9f * pinst->color_amb + 0.1f * (self_light + min_light);

    pinst->col_amb.a = (alpha * INV_FF) * (pinst->alpha * INV_FF);
    pinst->col_amb.r = (float)( pinst->color_amb >> rs ) * INV_FF;
    pinst->col_amb.g = (float)( pinst->color_amb >> gs ) * INV_FF;
    pinst->col_amb.b = (float)( pinst->color_amb >> bs ) * INV_FF;

    pinst->col_amb.a = CLIP(pinst->col_amb.a, 0, 1);

    pinst->max_light = 0;
    pinst->min_light = 255;
    for ( cnt = 0; cnt < pmad->md2_data.vertices; cnt++ )
    {
        Uint16 lite;

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

        pinst->vlst[cnt].col_dir[RR] = (float)( pinst->vlst[cnt].color_dir >> rs ) * INV_FF;
        pinst->vlst[cnt].col_dir[GG] = (float)( pinst->vlst[cnt].color_dir >> gs ) * INV_FF;
        pinst->vlst[cnt].col_dir[BB] = (float)( pinst->vlst[cnt].color_dir >> bs ) * INV_FF;

        if ( do_lighting )
        {
            Uint16 light = pinst->color_amb + pinst->vlst[cnt].color_dir;
            light = CLIP(light, 0, 255);
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
            light = CLIP(light, 0, 255);

            pinst->max_light = MAX(pinst->max_light, light);
            pinst->min_light = MIN(pinst->min_light, light);

            pinst->vlst[cnt].col[RR] = pinst->vlst[cnt].col_dir[RR];
            pinst->vlst[cnt].col[GG] = pinst->vlst[cnt].col_dir[GG];
            pinst->vlst[cnt].col[BB] = pinst->vlst[cnt].col_dir[BB];
            pinst->vlst[cnt].col[AA] = pinst->col_amb.a;
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t chr_instance_update_vertices( chr_instance_t * pinst, int vmin, int vmax )
{
    int    i;
    float  flip;
    bool_t vertices_match, flips_match, frames_match;

    mad_t * pmad;

    if ( NULL == pinst ) return bfalse;

    // get the model. try to heal a bad model.
    if ( INVALID_MAD(pinst->imad) ) return bfalse;
    pmad = MadList + pinst->imad;

    // handle the default parameters
    if ( vmin < 0 ) vmin = 0;
    if ( vmax < 0 ) vmax = pmad->md2_data.vertices - 1;

    vmin = CLIP(vmin, 0, pmad->md2_data.vertices - 1);
    vmax = CLIP(vmax, 0, pmad->md2_data.vertices - 1);

    flip = pinst->lip / 256.0f;

    // test to see if we have already calculated this data
    vertices_match = (pinst->save_vmin <= vmin) && (pinst->save_vmax >= vmax);

    flips_match = ( pinst->frame_nxt == pinst->frame_lst ) || ( pinst->save_flip == flip );

    frames_match = ( flip == 1.0f && pinst->save_frame_nxt == pinst->frame_nxt ) ||
                   ( flip == 0.0f && pinst->save_frame_lst == pinst->frame_lst ) ||
                   ( flips_match  && pinst->save_frame_nxt == pinst->frame_nxt && pinst->save_frame_lst == pinst->frame_lst );

    if ( frames_match && vertices_match ) return bfalse;

    if ( pinst->frame_nxt == pinst->frame_lst || flip == 0.0f )
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
    else if ( flip == 1.0f )
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

            pinst->vlst[i].pos[XX] = Md2FrameList[pinst->frame_lst].vrtx[i] + (Md2FrameList[pinst->frame_nxt].vrtx[i] - Md2FrameList[pinst->frame_lst].vrtx[i]) * flip;
            pinst->vlst[i].pos[YY] = Md2FrameList[pinst->frame_lst].vrty[i] + (Md2FrameList[pinst->frame_nxt].vrty[i] - Md2FrameList[pinst->frame_lst].vrty[i]) * flip;
            pinst->vlst[i].pos[ZZ] = Md2FrameList[pinst->frame_lst].vrtz[i] + (Md2FrameList[pinst->frame_nxt].vrtz[i] - Md2FrameList[pinst->frame_lst].vrtz[i]) * flip;
            pinst->vlst[i].pos[WW] = 1.0f;

            vrta_lst = Md2FrameList[pinst->frame_lst].vrta[i];
            vrta_nxt = Md2FrameList[pinst->frame_nxt].vrta[i];

            pinst->vlst[i].nrm[XX] = kMd2Normals[vrta_lst][XX] + (kMd2Normals[vrta_nxt][XX] - kMd2Normals[vrta_lst][XX]) * flip;
            pinst->vlst[i].nrm[YY] = kMd2Normals[vrta_lst][YY] + (kMd2Normals[vrta_nxt][YY] - kMd2Normals[vrta_lst][YY]) * flip;
            pinst->vlst[i].nrm[ZZ] = kMd2Normals[vrta_lst][ZZ] + (kMd2Normals[vrta_nxt][ZZ] - kMd2Normals[vrta_lst][ZZ]) * flip;

            pinst->vlst[i].env[XX] = indextoenvirox[vrta_lst] + (indextoenvirox[vrta_nxt] - indextoenvirox[vrta_lst]) * flip;
        }
    }

    pinst->save_frame     = update_wld;
    pinst->save_vmin      = MIN(pinst->save_vmin, vmin);
    pinst->save_vmax      = MAX(pinst->save_vmax, vmax);
    pinst->save_frame_nxt = pinst->frame_nxt;
    pinst->save_frame_lst = pinst->frame_lst;

    return btrue;
}
