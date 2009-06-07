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

/* Egoboo - graphic_mad.c
 * Character model drawing code.
 */

#include "graphic.h"

#include "Md2.h"
#include "id_md2.h"
#include "camera.h"
#include "char.h"
#include "mad.h"

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

    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_NORMAL_ARRAY );

    glVertexPointer( 3, GL_FLOAT, 0, md2_blendedVertices );
    glNormalPointer( GL_FLOAT, 0, md2_blendedNormals );

    glBegin( GL_TRIANGLES );
    {
        for ( i = 0; i < numTriangles; i++ )
        {
            tri = &triangles[i];

            glTexCoord2fv( ( const GLfloat* )&( tc[tri->texCoordIndices[0]] ) );
            glArrayElement( tri->vertexIndices[0] );

            glTexCoord2fv( ( const GLfloat* )&( tc[tri->texCoordIndices[1]] ) );
            glArrayElement( tri->vertexIndices[1] );

            glTexCoord2fv( ( const GLfloat* )&( tc[tri->texCoordIndices[2]] ) );
            glArrayElement( tri->vertexIndices[2] );
        }
    }
    glEnd();

    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_NORMAL_ARRAY );
}

//--------------------------------------------------------------------------------------------
void render_one_mad_enviro( Uint16 character, Uint8 trans )
{
    // ZZ> This function draws an environment mapped model

    int    cmd_count, vrt_count, entry_count;
    Uint16 cnt, tnc, entry;
    Uint16 vertex;

    Uint16 texture;
    float  uoffset, voffset;

    chr_t * pchr;
    mad_t * pmad;
    chr_instance_t * pinst;

    if( INVALID_CHR(character) ) return;
    pchr  = ChrList + character;
    pinst = &(pchr->inst);

    if( INVALID_MAD(pinst->imad) ) return;
    pmad = MadList + pinst->imad;

    texture = pinst->texture;

    uoffset = pinst->uoffset - gCamera.turn_z_one;
    voffset = pinst->voffset;

    // prepare the object
    chr_instance_update( character, 255, bfalse );

    glMatrixMode( GL_MODELVIEW_MATRIX );
    glPushMatrix();
    glMultMatrixf( pinst->matrix.v );

    // Choose texture and matrix
    GLXtexture_Bind( TxTexture + texture );

    // Render each command
    cmd_count   = MIN(pmad->md2.cmd.count,   MAXCOMMAND);
    entry_count = MIN(pmad->md2.cmd.entries, MAXCOMMANDENTRIES);
    vrt_count   = MIN(pmad->md2.vertices,    MAXVERTICES);
    entry = 0;
    for ( cnt = 0; cnt < cmd_count; cnt++ )
    {
        if( entry >= entry_count ) break;

        glBegin ( pmad->md2.cmd.type[cnt] );
        {
            for ( tnc = 0; tnc < pmad->md2.cmd.size[cnt]; tnc++ )
            {
                float     cmax, cmin;
                GLvector4 col;
                GLfloat tex[2];

                if( entry >= entry_count ) break;

                vertex = pmad->md2.cmd.vrt[entry];

                // normalize the color
                cmax = 1.0f;
                cmin = MIN(MIN(pinst->vlst[vertex].col_dir[RR], pinst->vlst[vertex].col_dir[GG]), pinst->vlst[vertex].col_dir[BB]);
                if( 1.0f != cmin )
                {
                    cmax = MAX(MAX(pinst->vlst[vertex].col_dir[RR], pinst->vlst[vertex].col_dir[GG]), pinst->vlst[vertex].col_dir[BB]);

                    if( 0.0f == cmax || cmax == cmin )
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
                    glColor4fv  ( col.v );
                    glNormal3fv ( pinst->vlst[vertex].nrm );
                    glTexCoord2fv ( tex );
                    glVertex3fv ( pinst->vlst[vertex].pos );
                }

                entry++;
            }
        }
        glEnd ();
    }

    glMatrixMode( GL_MODELVIEW_MATRIX );
    glPopMatrix();
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

    Uint16 texture;
    float  uoffset, voffset;

    chr_t * pchr;
    mad_t * pmad;
    chr_instance_t * pinst;

    if( INVALID_CHR(character) ) return;
    pchr  = ChrList + character;
    pinst = &(pchr->inst);

    if( INVALID_MAD(pinst->imad) ) return;
    pmad = MadList + pinst->imad;

    // To make life easier
    texture = pinst->texture;

    uoffset = pinst->uoffset * INV_FFFF;
    voffset = pinst->voffset * INV_FFFF;

    // prepare the object
    chr_instance_update( character, trans, btrue );

    // Choose texture and matrix
    GLXtexture_Bind( TxTexture + texture );

    glMatrixMode( GL_MODELVIEW_MATRIX );
    glPushMatrix();
    glMultMatrixf( pinst->matrix.v );

    // Render each command
    cmd_count   = MIN(pmad->md2.cmd.count,   MAXCOMMAND);
    entry_count = MIN(pmad->md2.cmd.entries, MAXCOMMANDENTRIES);
    vrt_count   = MIN(pmad->md2.vertices,    MAXVERTICES);
    entry = 0;
    for (cnt = 0; cnt < cmd_count; cnt++ )
    {
        if( entry >= entry_count ) break;

        glBegin ( pmad->md2.cmd.type[cnt] );
        {
            for ( tnc = 0; tnc < pmad->md2.cmd.size[cnt]; tnc++ )
            {
                GLfloat tex[2];

                if( entry >= entry_count ) break;

                vertex = pmad->md2.cmd.vrt[entry];

                if ( vertex < vrt_count )
                {

                    tex[0] = pmad->md2.cmd.u[entry] + uoffset;
                    tex[1] = pmad->md2.cmd.v[entry] + voffset;

                    glColor4fv  ( pinst->vlst[vertex].col );
                    glNormal3fv ( pinst->vlst[vertex].nrm );
                    glTexCoord2fv ( tex );
                    glVertex3fv ( pinst->vlst[vertex].pos );
                }

                entry++;
            }
        }
        glEnd ();
    }

    glMatrixMode( GL_MODELVIEW_MATRIX );
    glPopMatrix();
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

    if( INVALID_CHR(character) ) return;
    pchr = ChrList + character;

    if ( pchr->is_hidden ) return;

    if ( pchr->inst.enviro )
    {
        render_one_mad_enviro( character, trans );
    }
    else
    {
        render_one_mad_tex( character, trans );
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
    chr_instance_t * pinst;

    if( INVALID_CHR(tnc) ) return;
    pchr = ChrList + tnc;
    pinst = &(pchr->inst);

    if ( pchr->is_hidden ) return;

    if ( INVALID_CAP(pchr->model) || !CapList[pchr->model].reflect ) return;

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

    if( INVALID_CHR(character) ) return;
    pchr = ChrList + character;
    pinst = &(pchr->inst);

    // make sure that the vertices are interpolated
    chr_instance_update_vertices( pinst, -1, -1 );

    // do the lighting
    chr_instance_update_lighting( pinst, pchr, trans, do_lighting );
}

//--------------------------------------------------------------------------------------------
bool_t project_sum_lighting( light_cache_t * dst, light_cache_t * src, GLvector3 vec, int dir )
{
    if( NULL == src || NULL == dst ) return bfalse;

    if( dir < 0 || dir > 4 || 0 != (dir&1) ) 
        return bfalse;

    if( vec.x > 0 )
    {
        dst->lighting[dir+0] += ABS(vec.x) * src->lighting[0];
        dst->lighting[dir+1] += ABS(vec.x) * src->lighting[1];
    }
    else if (vec.x < 0)
    {
        dst->lighting[dir+0] += ABS(vec.x) * src->lighting[1];
        dst->lighting[dir+1] += ABS(vec.x) * src->lighting[0];
    }

    if( vec.y > 0 )
    {
        dst->lighting[dir+0] += ABS(vec.y) * src->lighting[2];
        dst->lighting[dir+1] += ABS(vec.y) * src->lighting[3];
    }
    else if (vec.y < 0)
    {
        dst->lighting[dir+0] += ABS(vec.y) * src->lighting[3];                            
        dst->lighting[dir+1] += ABS(vec.y) * src->lighting[2];                            
    }

    if( vec.z > 0 )
    {
        dst->lighting[dir+0] += ABS(vec.z) * src->lighting[4];
        dst->lighting[dir+1] += ABS(vec.z) * src->lighting[5];
    }
    else if (vec.z < 0)
    {
        dst->lighting[dir+0] += ABS(vec.z) * src->lighting[5];  
        dst->lighting[dir+1] += ABS(vec.z) * src->lighting[4];
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t project_lighting( light_cache_t * dst, light_cache_t * src, GLmatrix mat )
{
    int cnt;
    GLvector3 fwd, right, up;

    // blank the destination lighting
    if( NULL == dst ) return bfalse;

    dst->max_light = 0.0f;
    for( cnt = 0; cnt<6; cnt++)
    {
        dst->lighting[cnt] = 0.0f;
    }

    if( NULL == src ) return bfalse;
    if( src->max_light <= 0.0f ) return btrue;

    // grab the character directions
    fwd   = VNormalize( mat_getChrForward( mat ) );         // along body-fixed +y-axis
    right = VNormalize( mat_getChrRight( mat ) );        // along body-fixed +x-axis
    up    = VNormalize( mat_getChrUp( mat ) );            // along body-fixed +z axis   

    // split the lighting cache up
    project_sum_lighting( dst, src, right, 0 );
    project_sum_lighting( dst, src, fwd,   2 );
    project_sum_lighting( dst, src, up,    4 );

    // determine the maximum lighting amount
    dst->max_light = 0.0f;
    for( cnt=0; cnt<6; cnt++ )
    {
        dst->max_light = MAX(dst->max_light, dst->lighting[cnt]);
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t interpolate_lighting( light_cache_t * dst, light_cache_t * src[], float u, float v )
{
    int   cnt, tnc;
    float wt_sum;

    if( NULL == dst ) return bfalse;

    dst->max_light = 0.0f;
    for( cnt = 0; cnt < 6; cnt++ )
    {
        dst->lighting[cnt] = 0.0f;
    }

    if( NULL == src ) return bfalse;

    u = CLIP(u, 0, 1);
    v = CLIP(v, 0, 1);

    wt_sum = 0.0f;
    if( NULL != src[0] )
    {
        float wt = (1-u)*(1-v);
        for(tnc = 0; tnc<6; tnc++)
        {
            dst->lighting[tnc] += src[0]->lighting[tnc] * wt;
        }
        wt_sum += wt;
    }

    if( NULL != src[1] )
    {
        float wt = u*(1-v);
        for(tnc = 0; tnc<6; tnc++)
        {
            dst->lighting[tnc] += src[1]->lighting[tnc] * wt;
        }
        wt_sum += wt;
    }

    if( NULL != src[2] )
    {
        float wt = (1-u)*v;
        for(tnc = 0; tnc<6; tnc++)
        {
            dst->lighting[tnc] += src[2]->lighting[tnc] * wt;
        }
        wt_sum += wt;
    }

    if( NULL != src[3] )
    {
        float wt = u*v;
        for(tnc = 0; tnc<6; tnc++)
        {
            dst->lighting[tnc] += src[3]->lighting[tnc] * wt;
        }
        wt_sum += wt;
    }

    if( wt_sum > 0.0f )
    {
        for(tnc = 0; tnc<6; tnc++)
        {
            dst->lighting[tnc] /= wt_sum;
            dst->max_light = MAX(dst->max_light, dst->lighting[tnc]);
        }
    }

    return wt_sum > 0.0f;
}



//--------------------------------------------------------------------------------------------
bool_t interpolate_mesh_lighting( mesh_t * pmesh, light_cache_t * dst, GLvector3 pos )
{
    light_cache_t * cache_list[4];
    int ix, iy, cnt;
    Uint32 fan[4];
    float u,v, min_x,max_x, min_y,max_y;

    fan[0] = mesh_get_tile( pmesh, pos.x,             pos.y             );
    fan[1] = mesh_get_tile( pmesh, pos.x + TILE_SIZE, pos.y             );
    fan[2] = mesh_get_tile( pmesh, pos.x,             pos.y + TILE_SIZE );
    fan[3] = mesh_get_tile( pmesh, pos.x + TILE_SIZE, pos.y + TILE_SIZE );

    for( cnt = 0; cnt<4; cnt++ )
    {
        cache_list[cnt] = VALID_TILE(pmesh, fan[cnt]) ? pmesh->mem.cache + fan[cnt] : NULL;
    }

    ix    = floor( pos.x / TILE_SIZE );
    min_x = ix * TILE_SIZE;
    max_x = (ix + 1) * TILE_SIZE;

    iy    = floor( pos.y / TILE_SIZE );
    min_y = iy * TILE_SIZE;
    max_y = (iy + 1) * TILE_SIZE;

    u = (pos.x - min_x) / (max_x - min_x);
    v = (pos.y - min_y) / (max_y - min_y);

    return interpolate_lighting( dst, cache_list, u, v );
}

//--------------------------------------------------------------------------------------------
float evaluate_mesh_lighting( light_cache_t * src, GLfloat nrm[] )
{
    float lighting;

    if( NULL == src || NULL == nrm ) return 0.0f;

    if( src->max_light <= 0.0f ) return 0.0f;

    lighting = 0.0f;

    if( nrm[XX] > 0 )
    {
        lighting += ABS(nrm[XX]) * src->lighting[0];
    }
    else if (nrm[XX] < 0)
    {
        lighting += ABS(nrm[XX]) * src->lighting[1];
    }

    if( nrm[YY] > 0 )
    {
        lighting += ABS(nrm[YY]) * src->lighting[2];
    }
    else if (nrm[YY] < 0)
    {
        lighting += ABS(nrm[YY]) * src->lighting[3];                                                        
    }

    if( nrm[ZZ] > 0 )
    {
        lighting += ABS(nrm[ZZ]) * src->lighting[4];
    }
    else if (nrm[ZZ] < 0)
    {
        lighting += ABS(nrm[ZZ]) * src->lighting[5];  
    }

    return lighting;
}


//--------------------------------------------------------------------------------------------
void chr_instance_update_lighting( chr_instance_t * pinst, chr_t * pchr, Uint8 trans, bool_t do_lighting )
{
    Uint16 cnt;

    Uint16 frame_nxt, frame_lst;
    Uint8  lightrotation;
    Uint32 alpha;
    Uint16  lightlevel_dir, lightlevel_amb;
    Uint8  rs, gs, bs;
    float  flip;
    Uint8  self_light;
    light_cache_t global_light, loc_light;
    float min_light;

    mad_t * pmad;

    if( NULL == pinst || NULL == pchr ) return;

    if( INVALID_MAD(pinst->imad) ) return;
    pmad = MadList + pinst->imad;

    // To make life easier
    frame_nxt = pinst->frame_nxt;
    frame_lst = pinst->frame_lst;
    lightrotation = FP8_TO_INT( pchr->turn_z + pchr->inst.light_turn_z );
    lightlevel_dir = pinst->lightlevel_dir;
    lightlevel_amb = pinst->lightlevel_amb;
    alpha = trans;

    // interpolate the lighting for the origin of the object
    interpolate_mesh_lighting( PMesh, &global_light, pchr->pos );

    // rotate the lighting data to body_centered coordinates
    project_lighting( &loc_light, &global_light, pinst->matrix );

    min_light = loc_light.max_light;
    for(cnt = 0; cnt<6; cnt++)
    {
        min_light = MIN(min_light, loc_light.lighting[cnt]);
    }

    for(cnt = 0; cnt<6; cnt++)
    {
        loc_light.lighting[cnt] -= min_light;
    }

    rs = pinst->redshift;
    gs = pinst->grnshift;
    bs = pinst->blushift;
    flip = pinst->lip / 256.0f;
    self_light = ( 255 == pinst->light ) ? 0 : pinst->light;
    
    pinst->color_amb = 0.9f * pinst->color_amb + 0.1f * (self_light + min_light + light_a);

    pinst->col_amb.a = (alpha * INV_FF) * (pinst->alpha * INV_FF);
    pinst->col_amb.r = ( float )( pinst->color_amb >> rs ) * INV_FF;
    pinst->col_amb.g = ( float )( pinst->color_amb >> gs ) * INV_FF;
    pinst->col_amb.b = ( float )( pinst->color_amb >> bs ) * INV_FF;

    pinst->col_amb.a = CLIP(pinst->col_amb.a, 0, 1);

    pinst->max_light = 0;
    pinst->min_light = 255;
    for ( cnt = 0; cnt < pmad->md2.vertices; cnt++ )
    {
        Uint16 lite;

        if( pinst->vlst[cnt].nrm[0] == 0.0f && pinst->vlst[cnt].nrm[1] == 0.0f && pinst->vlst[cnt].nrm[2] == 0.0f )
        {
            // this is the "ambient only" index, but it really means to sum up all the light
            GLfloat tnrm[3];
            tnrm[0] = tnrm[1] = tnrm[2] = 1.0f;
            lite  = evaluate_mesh_lighting( &loc_light, tnrm );

            tnrm[0] = tnrm[1] = tnrm[2] = -1.0f;
            lite += evaluate_mesh_lighting( &loc_light, tnrm );

            // average all the directions
            lite /= 6;
        }
        else
        {
            lite = evaluate_mesh_lighting( &loc_light, pinst->vlst[cnt].nrm );
        }

        pinst->vlst[cnt].color_dir = 0.9f * pinst->vlst[cnt].color_dir + 0.1f * lite;

        pinst->vlst[cnt].col_dir[RR] = ( float )( pinst->vlst[cnt].color_dir >> rs ) * INV_FF;
        pinst->vlst[cnt].col_dir[GG] = ( float )( pinst->vlst[cnt].color_dir >> gs ) * INV_FF;
        pinst->vlst[cnt].col_dir[BB] = ( float )( pinst->vlst[cnt].color_dir >> bs ) * INV_FF;

        if( do_lighting )
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