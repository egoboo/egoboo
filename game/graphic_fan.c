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

/// @file graphic_fan.c
/// @brief World mesh drawing.
/// @details

#include "graphic_fan.h"
#include "graphic.h"
#include "mesh.inl"
#include "camera.h"

#include "game.h"
#include "texture.h"

#include "egoboo.h"

#include "SDL_extensions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t           meshnotexture   = bfalse;
TX_REF           meshlasttexture = ( TX_REF )INVALID_TX_TEXTURE;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t animate_tile( ego_mpd_t * pmesh, Uint32 itile );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void animate_all_tiles( ego_mpd_t * pmesh )
{
    Uint32 cnt;
    Uint32 tile_count;

    if ( NULL == pmesh ) return;

    tile_count = pmesh->info.tiles_count;

    for ( cnt = 0; cnt < tile_count; cnt++ )
    {
        animate_tile( pmesh, cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool_t animate_tile( ego_mpd_t * pmesh, Uint32 itile )
{
    /// BB@> animate a given tile

    Uint16 basetile, image;
    Uint16 base_and, frame_and, frame_add;
    Uint8  type;
    tile_mem_t  * ptmem;
    ego_tile_info_t * ptile;

    if ( NULL == pmesh ) return bfalse;
    ptmem  = &( pmesh->tmem );

    if ( !mesh_grid_is_valid( pmesh, itile ) ) return bfalse;
    ptile = ptmem->tile_list + itile;

    // do not render the itile if the image image is invalid
    if ( TILE_IS_FANOFF( *ptile ) )  return btrue;

    image = TILE_GET_LOWER_BITS( ptile->img ); // Tile image
    type  = ptile->type;                       // Command type ( index to points in itile )

    if ( 0 == mesh_test_fx( pmesh, itile, MPDFX_ANIM ) ) return btrue;

    // Animate the tiles
    if ( type >= ( MAXMESHTYPE >> 1 ) )
    {
        // Big tiles
        base_and  = animtile[1].base_and;            // Animation set
        frame_and = animtile[1].frame_and;
        frame_add = animtile[1].frame_add;    // Animated image
    }
    else
    {
        // Small tiles
        base_and  = animtile[0].base_and;            // Animation set
        frame_and = animtile[0].frame_and;
        frame_add = animtile[0].frame_add;         // Animated image
    }

    basetile = image & base_and;
    image    = frame_add + basetile;

    // actually update the animated texture info
    return mesh_set_texture( pmesh, itile, image );
}

//--------------------------------------------------------------------------------------------
gfx_rv render_fan( ego_mpd_t * pmesh, Uint32 itile )
{
    /// @details ZZ@> This function draws a mesh itile
    /// Optimized to use gl*Pointer() and glArrayElement() for vertex presentation

    int    cnt, tnc, entry, vertex;
    Uint16 commands, vertices;

    Uint16 image;
    Uint8  type;
    int    texture;

    tile_mem_t  * ptmem;
    ego_tile_info_t * ptile;

    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL mesh" );
        return gfx_error;
    }

    ptmem  = &( pmesh->tmem );

    if ( !mesh_grid_is_valid( pmesh, itile ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, itile, "invalid grid" );
        return gfx_error;
    }

    ptile = ptmem->tile_list + itile;

    // do not render the itile if the image image is invalid
    if ( TILE_IS_FANOFF( *ptile ) )  return gfx_success;

    image = TILE_GET_LOWER_BITS( ptile->img ); // Tile image
    texture  = (( image >> 6 ) & 3 ) + TX_TILE_0; // 64 tiles in each 256x256 texture
    if ( !meshnotexture && texture != meshlasttexture ) return gfx_success;

    type     = ptile->type;                         // Fan type ( index to points in itile )
    vertices = tile_dict[type].numvertices;      // Number of vertices
    commands = tile_dict[type].command_count;    // Number of commands

    GL_DEBUG( glPushClientAttrib )( GL_CLIENT_VERTEX_ARRAY_BIT );
    {
        // per-vertex coloring
        GL_DEBUG( glShadeModel )( GL_SMOOTH );  // GL_LIGHTING_BIT

        // [claforte] Put this in an initialization function.
        GL_DEBUG( glEnableClientState )( GL_VERTEX_ARRAY );
        GL_DEBUG( glVertexPointer )( 3, GL_FLOAT, 0, ptmem->plst + ptile->vrtstart );

        GL_DEBUG( glEnableClientState )( GL_TEXTURE_COORD_ARRAY );
        GL_DEBUG( glTexCoordPointer )( 2, GL_FLOAT, 0, ptmem->tlst + ptile->vrtstart );

        GL_DEBUG( glEnableClientState )( GL_COLOR_ARRAY );
        GL_DEBUG( glColorPointer )( 3, GL_FLOAT, 0, ptmem->clst + ptile->vrtstart );

        // Render each command
        entry = 0;
        for ( cnt = 0; cnt < commands; cnt++ )
        {
            GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
            {
                for ( tnc = 0; tnc < tile_dict[type].command_entries[cnt]; tnc++, entry++ )
                {
                    vertex = tile_dict[type].command_verts[entry];
                    GL_DEBUG( glArrayElement )( vertex );
                }
            }
            GL_DEBUG_END();
        }
    }
    GL_DEBUG( glPopClientAttrib )();

#if defined(DEBUG_MESH_NORMALS) && defined(_DEBUG)
    GL_DEBUG( glDisable )( GL_TEXTURE_2D );
    GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
    entry = ptile->vrtstart;
    for ( cnt = 0; cnt < 4; cnt++, entry++ )
    {
        GL_DEBUG( glBegin )( GL_LINES );
        {
            GL_DEBUG( glVertex3fv )( ptmem->plst[entry] );
            GL_DEBUG( glVertex3f )(
                ptmem->plst[entry][XX] + GRID_FSIZE*ptmem->ncache[itile][cnt][XX],
                ptmem->plst[entry][YY] + GRID_FSIZE*ptmem->ncache[itile][cnt][YY],
                ptmem->plst[entry][ZZ] + GRID_FSIZE*ptmem->ncache[itile][cnt][ZZ] );

        }
        GL_DEBUG_END();
    }
    GL_DEBUG( glEnable )( GL_TEXTURE_2D );
#endif

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv  render_hmap_fan( ego_mpd_t * pmesh, Uint32 itile )
{
    /// @details ZZ@> This function draws a mesh itile
    GLvertex v[4];

    int cnt, vertex;
    size_t badvertex;
    int ix, iy, ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};
    Uint16 tile;
    Uint8  type, twist;

    ego_mpd_info_t  * pinfo;
    tile_mem_t      * ptmem;
    ego_tile_info_t * ptile;
    grid_mem_t      * pgmem;
    ego_grid_info_t * pgrid;

    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL mesh" );
        return gfx_error;
    }

    ptmem  = &( pmesh->tmem );
    pgmem  = &( pmesh->gmem );
    pinfo  = &( pmesh->info );

    if ( !mesh_grid_is_valid( pmesh, itile ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, itile, "invalid grid" );
        return gfx_error;
    }
    ptile = ptmem->tile_list + itile;
    pgrid = pgmem->grid_list + itile;

    /// @details BB@> the water info is for TILES, not for vertices, so ignore all vertex info and just draw the water
    ///     tile where it's supposed to go

    ix = itile % pmesh->info.tiles_x;
    iy = itile / pmesh->info.tiles_x;

    // vertex is a value from 0-15, for the meshcommandref/u/v variables
    // badvertex is a value that references the actual vertex number

    tile  = TILE_GET_LOWER_BITS( ptile->img ); // Tile
    type  = ptile->type;                     // Command type ( index to points in itile )
    twist = pgrid->twist;

    type &= 0x3F;

    // Original points
    badvertex = ptile->vrtstart;          // Get big reference value
    for ( cnt = 0; cnt < 4; cnt++ )
    {
        float tmp;
        v[cnt].pos[XX] = ( ix + ix_off[cnt] ) * GRID_FSIZE;
        v[cnt].pos[YY] = ( iy + iy_off[cnt] ) * GRID_FSIZE;
        v[cnt].pos[ZZ] = ptmem->plst[badvertex][ZZ];

        tmp = map_twist_nrm[twist].z;
        tmp *= tmp;

        v[cnt].col[RR] = tmp * ( tmp + ( 1.0f - tmp ) * map_twist_nrm[twist].x * map_twist_nrm[twist].x );
        v[cnt].col[GG] = tmp * ( tmp + ( 1.0f - tmp ) * map_twist_nrm[twist].y * map_twist_nrm[twist].y );
        v[cnt].col[BB] = tmp;
        v[cnt].col[AA] = 1.0f;

        v[cnt].col[RR] = CLIP( v[cnt].col[RR], 0.0f, 1.0f );
        v[cnt].col[GG] = CLIP( v[cnt].col[GG], 0.0f, 1.0f );
        v[cnt].col[BB] = CLIP( v[cnt].col[BB], 0.0f, 1.0f );

        badvertex++;
    }

    oglx_texture_Bind( NULL );

    // Render each command
    GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
    {
        for ( vertex = 0; vertex < 4; vertex++ )
        {
            GL_DEBUG( glColor3fv )( v[vertex].col );
            GL_DEBUG( glVertex3fv )( v[vertex].pos );
        }
    }
    GL_DEBUG_END();

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_water_fan( ego_mpd_t * pmesh, Uint32 itile, Uint8 layer )
{
    /// @details ZZ@> This function draws a water itile

    GLvertex v[4];

    int    cnt, tnc;
    size_t badvertex;
    Uint16 type, commands, vertices;
    TX_REF texture;
    Uint16 frame;
    float offu, offv;
    int ix, iy;
    int ix_off[4] = {1, 1, 0, 0}, iy_off[4] = {0, 1, 1, 0};
    int  imap[4];
    float x1, y1, fx_off[4], fy_off[4];
    float falpha;

    ego_mpd_info_t * pinfo;
    tile_mem_t     * ptmem;
    grid_mem_t     * pgmem;
    ego_tile_info_t    * ptile;
    oglx_texture_t   * ptex;

    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL mesh" );
        return gfx_error;
    }

    pinfo = &( pmesh->info );
    ptmem = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );

    if ( !mesh_grid_is_valid( pmesh, itile ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, itile, "invalid grid" );
        return gfx_error;
    }
    ptile = ptmem->tile_list + itile;

    falpha = FF_TO_FLOAT( water.layer[layer].alpha );
    falpha = CLIP( falpha, 0.0f, 1.0f );

    /// @note BB@> the water info is for TILES, not for vertices, so ignore all vertex info and just draw the water
    ///            tile where it's supposed to go

    ix = itile % pinfo->tiles_x;
    iy = itile / pinfo->tiles_x;

    // To make life easier
    type  = 0;                                         // Command type ( index to points in itile )
    offu  = water.layer[layer].tx.x;                   // Texture offsets
    offv  = water.layer[layer].tx.y;
    frame = water.layer[layer].frame;                  // Frame

    texture  = layer + TX_WATER_TOP;                   // Water starts at texture TX_WATER_TOP
    vertices = tile_dict[type].numvertices;            // Number of vertices
    commands = tile_dict[type].command_count;          // Number of commands

    ptex = TxTexture_get_ptr( texture );

    x1 = ( float ) oglx_texture_GetTextureWidth( ptex ) / ( float ) oglx_texture_GetImageWidth( ptex );
    y1 = ( float ) oglx_texture_GetTextureHeight( ptex ) / ( float ) oglx_texture_GetImageHeight( ptex );

    for ( cnt = 0; cnt < 4; cnt ++ )
    {
        fx_off[cnt] = x1 * ix_off[cnt];
        fy_off[cnt] = y1 * iy_off[cnt];

        imap[cnt] = cnt;
    }

    // flip the coordinates around based on the "mode" of the tile
    if ( HAS_NO_BITS( ix, 1 ) )
    {
        SWAP( int, imap[0], imap[3] );
        SWAP( int, imap[1], imap[2] );
    }

    if ( HAS_NO_BITS( iy, 1 ) )
    {
        SWAP( int, imap[0], imap[1] );
        SWAP( int, imap[2], imap[3] );
    }

    // draw draw front and back faces of polygons
    GL_DEBUG( glDisable )( GL_CULL_FACE );

    // Original points
    badvertex = ptile->vrtstart;
    {
        GLXvector3f nrm = {0, 0, 1};
        float alight;

        alight = get_ambient_level() + water.layer->light_add;
        alight = CLIP( alight, 0.0f, 1.0f );

        for ( cnt = 0; cnt < 4; cnt++ )
        {
            float dlight;
            int jx, jy;

            tnc = imap[cnt];

            jx = ix + ix_off[cnt];
            jy = iy + iy_off[cnt];

            v[cnt].pos[XX] = jx * GRID_FSIZE;
            v[cnt].pos[YY] = jy * GRID_FSIZE;
            v[cnt].pos[ZZ] = water.layer_z_add[layer][frame][tnc] + water.layer[layer].z;

            v[cnt].tex[SS] = fx_off[cnt] + offu;
            v[cnt].tex[TT] = fy_off[cnt] + offv;

            // get the lighting info from the grid
            itile = mesh_get_tile_int( pmesh, jx, jy );
            if ( grid_light_one_corner( pmesh, itile, v[cnt].pos[ZZ], nrm, &dlight ) )
            {
                // take the v[cnt].color from the tnc vertices so that it is oriented prroperly
                v[cnt].col[RR] = dlight * INV_FF + alight;
                v[cnt].col[GG] = dlight * INV_FF + alight;
                v[cnt].col[BB] = dlight * INV_FF + alight;
                v[cnt].col[AA] = 1.0f;

                v[cnt].col[RR] = CLIP( v[cnt].col[RR], 0.0f, 1.0f );
                v[cnt].col[GG] = CLIP( v[cnt].col[GG], 0.0f, 1.0f );
                v[cnt].col[BB] = CLIP( v[cnt].col[BB], 0.0f, 1.0f );
            }
            else
            {
                v[cnt].col[RR] = v[cnt].col[GG] = v[cnt].col[BB] = 0.0f;
            }

            // the application of alpha to the tile depends on the blending mode
            if ( water.light )
            {
                // blend using light
                v[cnt].col[RR] *= falpha;
                v[cnt].col[GG] *= falpha;
                v[cnt].col[BB] *= falpha;
                v[cnt].col[AA]  = 1.0f;
            }
            else
            {
                // blend using alpha
                v[cnt].col[AA] = falpha;
            }

            badvertex++;
        }
    }

    // Change texture if need be
    if ( meshlasttexture != texture )
    {
        oglx_texture_Bind( ptex );
        meshlasttexture = texture;
    }

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT );
    {
        GLboolean use_depth_mask = ( !water.light && ( 1.0f == falpha ) ) ? GL_TRUE : GL_FALSE;

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );                                  // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );                                   // GL_DEPTH_BUFFER_BIT

        // only use the depth mask if the tile is NOT transparent
        GL_DEBUG( glDepthMask )( use_depth_mask );                              // GL_DEPTH_BUFFER_BIT

        // cull backward facing polygons
        GL_DEBUG( glEnable )( GL_CULL_FACE );                                   // GL_ENABLE_BIT
        GL_DEBUG( glFrontFace )( GL_CW );                                       // GL_POLYGON_BIT

        // set the blending mode
        if ( water.light )
        {
            GL_DEBUG( glEnable )( GL_BLEND );                                   // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE_MINUS_SRC_COLOR );          // GL_COLOR_BUFFER_BIT
        }
        else
        {
            GL_DEBUG( glEnable )( GL_BLEND );                                    // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );     // GL_COLOR_BUFFER_BIT
        }

        // per-vertex coloring
        GL_DEBUG( glShadeModel )( GL_SMOOTH );                // GL_LIGHTING_BIT

        // Render each command
        GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
        {
            for ( cnt = 0; cnt < 4; cnt++ )
            {
                GL_DEBUG( glColor4fv )( v[cnt].col );         // GL_CURRENT_BIT
                GL_DEBUG( glTexCoord2fv )( v[cnt].tex );
                GL_DEBUG( glVertex3fv )( v[cnt].pos );
            }
        }
        GL_DEBUG_END();
    }
    ATTRIB_POP( __FUNCTION__ );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
void animate_tiles()
{
    /// ZZ@> This function changes the animated tile frame

    // make sure this updates per frame
    if ( 0 == ( true_frame & animtile_update_and ) )
    {
        animtile[0].frame_add = ( animtile[0].frame_add + 1 ) & animtile[0].frame_and;
        animtile[1].frame_add = ( animtile[1].frame_add + 1 ) & animtile[1].frame_and;
    }
}

