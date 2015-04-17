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

/// @file  game/graphic_fan.c
/// @brief World mesh drawing.
/// @details

#include "game/graphic.h"
#include "game/graphic_fan.h"
#include "game/graphic_texture.h"
#include "game/game.h"
#include "game/renderer_3d.h"
#include "game/egoboo.h"
#include "game/mesh.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool animate_tile( ego_mesh_t * pmesh, Uint32 itile );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void animate_all_tiles( ego_mesh_t * pmesh )
{
    Uint32 cnt, itile;
    Uint32 tile_count, anim_count;
    bool small_tile_update, big_tile_update;

    if ( NULL == pmesh ) return;

    small_tile_update = ( animtile[0].frame_add_old != animtile[0].frame_add );
    big_tile_update   = ( animtile[1].frame_add_old != animtile[1].frame_add );

    // if there are no updates, do nothing
    if ( !small_tile_update && !big_tile_update ) return;

    tile_count = pmesh->tmem.tile_count;
    anim_count = pmesh->fxlists.anm.idx;

    // scan through all the animated tiles
    for ( cnt = 0; cnt < anim_count; cnt++ )
    {
        // get the offset
        itile = pmesh->fxlists.anm.lst[cnt];
        if ( itile >= tile_count ) continue;

        animate_tile( pmesh, itile );
    }
}

//--------------------------------------------------------------------------------------------
bool animate_tile( ego_mesh_t * pmesh, Uint32 itile )
{
    /// @author BB
    /// @details animate a given tile

    Uint16 basetile, image;
    Uint16 base_and, frame_and, frame_add;
    Uint8  type;
    ego_tile_info_t * ptile;

    // do nothing if the tile is not animated
    if ( 0 == ego_mesh_t::test_fx( pmesh, itile, MAPFX_ANIM ) )
    {
        return true;
    }

    // grab a pointer to the tile
    ptile = ego_mesh_t::get_ptile( pmesh, itile );
    if ( NULL == ptile )
    {
        return false;
    }

    image = TILE_GET_LOWER_BITS( ptile->img ); // Tile image
    type  = ptile->type;                       // Command type ( index to points in itile )

    // Animate the tiles
    if ( type >= tile_dict.offset )
    {
        // Big tiles
        base_and  = animtile[1].base_and;     // Animation set
        frame_and = animtile[1].frame_and;
        frame_add = animtile[1].frame_add;    // Animated image
    }
    else
    {
        // Small tiles
        base_and  = animtile[0].base_and;          // Animation set
        frame_and = animtile[0].frame_and;
        frame_add = animtile[0].frame_add;         // Animated image
    }

    basetile = image & base_and;
    image    = frame_add + basetile;

    // actually update the animated texture info
    return ego_mesh_set_texture( pmesh, itile, image );
}

//--------------------------------------------------------------------------------------------
gfx_rv render_fan( const ego_mesh_t * pmesh, const Uint32 itile )
{
    /// @author ZZ
    /// @details This function draws a mesh itile
    /// Optimized to use gl*Pointer() and glArrayElement() for vertex presentation

    int    cnt, tnc, entry, vertex;
    Uint16 commands, vertices;

    tile_definition_t * pdef;
    const tile_mem_t      * ptmem;
    const ego_tile_info_t * ptile;

    // grab a pointer to the tile
    ptile = ego_mesh_t::get_ptile( pmesh, itile );
    if ( NULL == ptile )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, itile, "invalid tile" );
        return gfx_error;
    }

    // get some info from the mesh
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL mesh" );
        return gfx_error;
    }
    ptmem  = &( pmesh->tmem );

    // do not render the itile if the image image is invalid
    if (TILE_IS_FANOFF(ptile))  return gfx_success;

    pdef = TILE_DICT_PTR( tile_dict, ptile->type );
    if ( NULL == pdef ) return gfx_fail;

    // bind the correct texture
    mesh_texture_bind( ptile );

    GL_DEBUG( glPushClientAttrib )( GL_CLIENT_VERTEX_ARRAY_BIT | GL_LIGHTING_BIT );
    {
        // per-vertex coloring
        GL_DEBUG( glShadeModel )( gfx.shading );  // GL_LIGHTING_BIT

        /// @note claforte@> Put this in an initialization function.
        GL_DEBUG( glEnableClientState )( GL_VERTEX_ARRAY );
        GL_DEBUG( glVertexPointer )( 3, GL_FLOAT, 0, ptmem->plst + ptile->vrtstart );

        GL_DEBUG( glEnableClientState )( GL_TEXTURE_COORD_ARRAY );
        GL_DEBUG( glTexCoordPointer )( 2, GL_FLOAT, 0, ptmem->tlst + ptile->vrtstart );

        if ( GL_FLAT != gfx.shading )
        {
            GL_DEBUG( glEnableClientState )( GL_COLOR_ARRAY );
            GL_DEBUG( glColorPointer )( 3, GL_FLOAT, 0, ptmem->clst + ptile->vrtstart );
        }
        else
        {
            GL_DEBUG( glDisableClientState )( GL_COLOR_ARRAY );
        }

        // grab some model info
        vertices = pdef->numvertices;
        commands = pdef->command_count;

        // Render each command
        entry = 0;
        for ( cnt = 0; cnt < commands; cnt++ )
        {
            GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
            {
                for ( tnc = 0; tnc < pdef->command_entries[cnt]; tnc++, entry++ )
                {
                    vertex = pdef->command_verts[entry];
                    GL_DEBUG( glArrayElement )( vertex );
                }
            }
            GL_DEBUG_END();
        }
    }
    GL_DEBUG( glPopClientAttrib )();

#if defined(DEBUG_MESH_NORMALS) && defined(_DEBUG)
    GL_DEBUG( glDisable )( GL_TEXTURE_2D );
	Ego::Renderer::getSingleton()->setColour(Ego::Colour4f::WHITE);
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
gfx_rv  render_hmap_fan( const ego_mesh_t * pmesh, const Uint32 itile )
{
    /// @author ZZ
    /// @details This function draws a mesh itile
    GLvertex v[4];

    int cnt, vertex;
    size_t badvertex;
    int ix, iy, ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};
    Uint16 tile;
    Uint8  type, twist;

    const tile_mem_t      * ptmem;
    const ego_tile_info_t * ptile;
    const ego_grid_info_t * pgrid;

    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL mesh" );
        return gfx_error;
    }

    ptmem  = &( pmesh->tmem );

    ptile = ego_mesh_t::get_ptile( pmesh, itile );
    if ( NULL == ptile )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, itile, "invalid grid" );
        return gfx_error;
    }

    pgrid = ego_mesh_t::get_pgrid( pmesh, itile );
    if ( NULL == pgrid )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, itile, "invalid grid" );
        return gfx_error;
    }

    /// @author BB
    /// @details the water info is for TILES, not for vertices, so ignore all vertex info and just draw the water
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

    oglx_texture_t::bind(nullptr);

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
gfx_rv render_water_fan( const ego_mesh_t * pmesh, const Uint32 itile, const Uint8 layer )
{
    /// @author ZZ
    /// @details This function draws a water itile

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

    const ego_mesh_info_t  * pinfo = NULL;
    const ego_tile_info_t * ptile = NULL;
    oglx_texture_t        * ptex  = NULL;
    tile_definition_t     * pdef  = NULL;

    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL mesh" );
        return gfx_error;
    }

    pinfo = &( pmesh->info );

    ptile = ego_mesh_t::get_ptile( pmesh, itile );
    if ( NULL == ptile )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, itile, "invalid tile" );
        return gfx_error;
    }

    falpha = FF_TO_FLOAT( water.layer[layer].alpha );
    falpha = CLIP( falpha, 0.0f, 1.0f );

    /// @note BB@> the water info is for TILES, not for vertices, so ignore all vertex info and just draw the water
    ///            tile where it's supposed to go

    ix = itile % pinfo->tiles_x;
    iy = itile / pinfo->tiles_x;

    // To make life easier
    type  = 0;                                         // Command type ( index to points in tile )
    pdef = TILE_DICT_PTR( tile_dict, type );
    if ( NULL == pdef )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, type, "unknown tile type" );
        return gfx_error;
    }

    offu  = water.layer[layer].tx.x;                   // Texture offsets
    offv  = water.layer[layer].tx.y;
    frame = water.layer[layer].frame;                  // Frame

    texture  = layer + TX_WATER_TOP;                   // Water starts at texture TX_WATER_TOP
    vertices = pdef->numvertices;            // Number of vertices
    commands = pdef->command_count;          // Number of commands

	ptex = TextureManager::get().get_valid_ptr(texture);

    x1 = ( float ) oglx_texture_t::getTextureWidth( ptex ) / ( float ) oglx_texture_t::getImageWidth( ptex );
    y1 = ( float ) oglx_texture_t::getTextureHeight( ptex ) / ( float ) oglx_texture_t::getImageHeight( ptex );

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
    oglx_end_culling();                        // GL_ENABLE_BIT

    // Original points
    badvertex = ptile->vrtstart;
    {
        GLXvector3f nrm = {0, 0, 1};
        float alight;

        alight = get_ambient_level() + water.layer->light_add;
        alight = CLIP( alight / 255.0f, 0.0f, 1.0f );

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
            TileIndex jtile = ego_mesh_t::get_tile_int(pmesh, PointGrid(jx, jy));
            if ( grid_light_one_corner( pmesh, jtile, v[cnt].pos[ZZ], nrm, &dlight ) )
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

    // tell the mesh texture code that someone else is controlling the texture
    mesh_texture_invalidate();

    // set the texture
    oglx_texture_t::bind( ptex );

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT );
    {
        GLboolean use_depth_mask = ( !water.light && ( 1.0f == falpha ) ) ? GL_TRUE : GL_FALSE;

        // do not draw hidden surfaces
		Ego::Renderer::get().setDepthTestEnabled(true);
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );                                   // GL_DEPTH_BUFFER_BIT

        // only use the depth mask if the tile is NOT transparent
        GL_DEBUG( glDepthMask )( use_depth_mask );                              // GL_DEPTH_BUFFER_BIT

        // cull backward facing polygons
        // use clockwise orientation to determine backfaces
        oglx_begin_culling( GL_BACK, MAP_NRM_CULL );            // GL_ENABLE_BIT | GL_POLYGON_BIT

        // set the blending mode
        Ego::Renderer::get().setBlendingEnabled(true);
        if (water.light)
        {
            GL_DEBUG(glBlendFunc)(GL_ONE, GL_ONE_MINUS_SRC_COLOR);          // GL_COLOR_BUFFER_BIT
        }
        else
        {
            GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);    // GL_COLOR_BUFFER_BIT
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
    /// @author ZZ
    /// @details This function changes the animated tile frame
    animtile_instance_t * patile;

    for ( size_t cnt = 0; cnt < 2; cnt ++ )
    {
        // grab the tile data
        patile = animtile + cnt;

        // skip it if there were no updates
        if ( patile->frame_update_old == true_frame ) continue;

        // save the old frame_add when we update to detect changes
        patile->frame_add_old = patile->frame_add;

        // cycle through all frames since the last time
        for ( Uint32 tnc = patile->frame_update_old + 1; tnc <= true_frame; tnc++ )
        {
            if ( 0 == ( tnc & patile->update_and ) )
            {
                patile->frame_add = ( patile->frame_add + 1 ) & patile->frame_and;
            }
        }

        // save the frame update
        patile->frame_update_old = true_frame;
    }
}
