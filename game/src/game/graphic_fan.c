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
#include "game/game.h"
#include "game/renderer_3d.h"
#include "game/egoboo.h"
#include "game/mesh.h"
#include "game/Module/Module.hpp"
#include "egolib/FileFormats/Globals.hpp"

//--------------------------------------------------------------------------------------------

void animate_all_tiles( ego_mesh_t& mesh )
{
    bool small_tile_update = (animtile[0].frame_add_old != animtile[0].frame_add);
    bool big_tile_update = (animtile[1].frame_add_old != animtile[1].frame_add);

    // If there are no updates, do nothing.
    if (!small_tile_update && !big_tile_update) return;

    size_t tile_count = mesh._tmem.getTileCount();
    size_t anim_count = mesh._fxlists.anm._idx;

    // Scan through all the animated tiles.
    for (size_t i = 0; i < anim_count; ++i)
    {
        // Get the offset
        Uint32 itile = mesh._fxlists.anm._lst[i];
        if (itile >= tile_count) continue;

        animate_tile(mesh, itile);
    }
}

//--------------------------------------------------------------------------------------------
bool animate_tile( ego_mesh_t& mesh, Uint32 itile )
{
    /// @author BB
    /// @details animate a given tile

    Uint16 basetile, image;
    Uint16 base_and, frame_add;
    Uint8  type;

	// do nothing if the tile is not animated
    if ( 0 == mesh.test_fx( itile, MAPFX_ANIM ) )
    {
        return true;
    }

    // grab a pointer to the tile
	ego_tile_info_t& ptile = mesh.getTileInfo(itile);

    image = TILE_GET_LOWER_BITS( ptile._img ); // Tile image
    type  = ptile._type;                       // Command type ( index to points in itile )

    // Animate the tiles
    if ( type >= tile_dict.offset )
    {
        // Big tiles
        base_and  = animtile[1].base_and;     // Animation set
        frame_add = animtile[1].frame_add;    // Animated image
    }
    else
    {
        // Small tiles
        base_and  = animtile[0].base_and;          // Animation set
        frame_add = animtile[0].frame_add;         // Animated image
    }

    basetile = image & base_and;
    image    = frame_add + basetile;

    // actually update the animated texture info
    return mesh.set_texture( itile, image );
}

//--------------------------------------------------------------------------------------------
gfx_rv render_fan( const ego_mesh_t& mesh, const TileIndex& tileIndex )
{
    /// @author ZZ
    /// @details This function draws a mesh itile
    /// Optimized to use gl*Pointer() and glArrayElement() for vertex presentation

    // grab a pointer to the tile
	const ego_tile_info_t& ptile = mesh.getTileInfo(tileIndex);

	const tile_mem_t& ptmem  = mesh._tmem;

    // do not render the itile if the image image is invalid
    if (ptile.isFanOff())  return gfx_success;

	tile_definition_t *pdef = tile_dict.get( ptile._type );
    if ( NULL == pdef ) return gfx_fail;

    // bind the correct texture
    mesh_texture_bind( &ptile );
    
    GL_DEBUG(glPushClientAttrib)(GL_CLIENT_VERTEX_ARRAY_BIT);
    {
        // Per-vertex coloring.
        Ego::Renderer::get().setGouraudShadingEnabled(gfx.gouraudShading_enable); // GL_LIGHTING_BIT

        /// @note claforte@> Put this in an initialization function.
        GL_DEBUG( glEnableClientState )( GL_VERTEX_ARRAY );
        GL_DEBUG( glVertexPointer )( 3, GL_FLOAT, 0, ptmem._plst[ptile._vrtstart] );

        GL_DEBUG( glEnableClientState )( GL_TEXTURE_COORD_ARRAY );
		GL_DEBUG( glTexCoordPointer )( 2, GL_FLOAT, 0, ptmem._tlst[ptile._vrtstart] );

        if (gfx.gouraudShading_enable)
        {
            GL_DEBUG( glEnableClientState )( GL_COLOR_ARRAY );
            GL_DEBUG( glColorPointer )( 3, GL_FLOAT, 0, ptmem._clst[ptile._vrtstart] );
        }
        else
        {
            GL_DEBUG( glDisableClientState )( GL_COLOR_ARRAY );
        }
        // grab some model info
        uint16_t commands = pdef->command_count;

        // Render each command
        for (size_t cnt = 0, entry = 0; cnt < commands; cnt++ )
        {
            uint8_t numEntries = pdef->command_entries[cnt];
            
            GL_DEBUG(glDrawElements)(GL_TRIANGLE_FAN, numEntries, GL_UNSIGNED_SHORT, &pdef->command_verts[entry]);
            entry += numEntries;
        }
    }
    GL_DEBUG(glPopClientAttrib)();

	if (egoboo_config_t::get().debug_mesh_renderNormals.getValue())
	{
		mesh_texture_invalidate();
		auto& renderer = Ego::Renderer::get();
		renderer.getTextureUnit().setActivated(nullptr);
		renderer.setColour(Ego::Colour4f::white());
		for (size_t i = ptile._vrtstart, j = 0; j < 4; ++i, ++j)
		{
			glBegin(GL_LINES);
			{
				glVertex3fv(ptmem._plst[i]);
				glVertex3f
					(
						ptmem._plst[i][XX] + Info<float>::Grid::Size()*(ptile._ncache[j][XX]),
						ptmem._plst[i][YY] + Info<float>::Grid::Size()*(ptile._ncache[j][YY]),
						ptmem._plst[i][ZZ] + Info<float>::Grid::Size()*(ptile._ncache[j][ZZ])
					);

			}
			glEnd();
		}
	}

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv  render_hmap_fan( const ego_mesh_t * mesh, const TileIndex& tileIndex )
{
    /// @author ZZ
    /// @details This function draws a mesh itile
    GLvertex v[4];

    int cnt, vertex;
    size_t badvertex;
    int ix, iy, ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};
    Uint8  type, twist;

    if ( NULL == mesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL mesh" );
        return gfx_error;
    }

	const tile_mem_t& ptmem  = mesh->_tmem;

	const ego_tile_info_t& ptile = mesh->getTileInfo(tileIndex);

	const ego_grid_info_t& pgrid = mesh->getGridInfo(tileIndex);

    /// @author BB
    /// @details the water info is for TILES, not for vertices, so ignore all vertex info and just draw the water
    ///     tile where it's supposed to go

    ix = tileIndex.getI() % mesh->_info.getTileCountX();
    iy = tileIndex.getI() / mesh->_info.getTileCountX();

    // vertex is a value from 0-15, for the meshcommandref/u/v variables
    // badvertex is a value that references the actual vertex number

    type  = ptile._type;                     // Command type ( index to points in itile )
    twist = pgrid._twist;

    type &= 0x3F;

    // Original points
    badvertex = ptile._vrtstart;          // Get big reference value
    for ( cnt = 0; cnt < 4; cnt++ )
    {
        float tmp;
        v[cnt].pos[XX] = ( ix + ix_off[cnt] ) * Info<float>::Grid::Size();
        v[cnt].pos[YY] = ( iy + iy_off[cnt] ) * Info<float>::Grid::Size();
        v[cnt].pos[ZZ] = ptmem._plst[badvertex][ZZ];

        tmp = g_meshLookupTables.twist_nrm[twist][kZ];
        tmp *= tmp;

        v[cnt].col[RR] = tmp * ( tmp + ( 1.0f - tmp ) * g_meshLookupTables.twist_nrm[twist][kX] * g_meshLookupTables.twist_nrm[twist][kX] );
        v[cnt].col[GG] = tmp * ( tmp + ( 1.0f - tmp ) * g_meshLookupTables.twist_nrm[twist][kY] * g_meshLookupTables.twist_nrm[twist][kY] );
        v[cnt].col[BB] = tmp;
        v[cnt].col[AA] = 1.0f;

        v[cnt].col[RR] = CLIP( v[cnt].col[RR], 0.0f, 1.0f );
        v[cnt].col[GG] = CLIP( v[cnt].col[GG], 0.0f, 1.0f );
        v[cnt].col[BB] = CLIP( v[cnt].col[BB], 0.0f, 1.0f );

        badvertex++;
    }

	Ego::Renderer::get().getTextureUnit().setActivated(nullptr);

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
gfx_rv render_water_fan( ego_mesh_t& mesh, const TileIndex& tileIndex, const Uint8 layer )
{
    /// @author ZZ
    /// @details This function draws a water itile
    static const int ix_off[4] = {1, 1, 0, 0}, iy_off[4] = {0, 1, 1, 0};

	int    tnc;
	size_t badvertex;
	int  imap[4];
    float fx_off[4], fy_off[4];

	const Ego::MeshInfo& info = mesh._info;

	const ego_tile_info_t& ptile = mesh.getTileInfo(tileIndex);

	float falpha;
    falpha = FF_TO_FLOAT( water._layers[layer]._alpha );
    falpha = CLIP( falpha, 0.0f, 1.0f );

    /// @note BB@> the water info is for TILES, not for vertices, so ignore all vertex info and just draw the water
    ///            tile where it's supposed to go

    int ix = tileIndex.getI() % info.getTileCountX();
    int iy = tileIndex.getI() / info.getTileCountX();

    // To make life easier
    uint16_t type  = 0;                                         // Command type ( index to points in tile )
	tile_definition_t *pdef = tile_dict.get( type );
    if ( NULL == pdef )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, type, "unknown tile type" );
        return gfx_error;
    }
    float offu  = water._layers[layer]._tx[XX];               // Texture offsets
    float offv  = water._layers[layer]._tx[YY];
	uint16_t frame = water._layers[layer]._frame;                // Frame

	const oglx_texture_t *ptex = _currentModule->getWaterTexture(layer);

    float x1 = (float)ptex->getWidth() / (float)ptex->getSourceWidth();
    float y1 = (float)ptex->getHeight() / (float)ptex->getSourceHeight();

    for (size_t cnt = 0; cnt < 4; cnt ++ )
    {
        fx_off[cnt] = x1 * ix_off[cnt];
        fy_off[cnt] = y1 * iy_off[cnt];

        imap[cnt] = cnt;
    }

    // flip the coordinates around based on the "mode" of the tile
    if ( HAS_NO_BITS( ix, 1 ) )
    {
        std::swap(imap[0], imap[3]);
        std::swap(imap[1], imap[2]);
    }

    if ( HAS_NO_BITS( iy, 1 ) )
    {
        std::swap(imap[0], imap[1]);
        std::swap(imap[2], imap[3]);
    }

    // draw draw front and back faces of polygons
	Ego::Renderer::get().setCullingMode(Ego::CullingMode::None);

    struct Vertex
    {
        float x, y, z;
        float r, g, b, a;
        float s, t;
    };
    auto vb = std::make_shared<Ego::VertexBuffer>(4, Ego::VertexFormatDescriptor::get<Ego::VertexFormat::P3FC4FT2F>());
    Vertex *v = static_cast<Vertex *>(vb->lock());

    // Original points
    badvertex = ptile._vrtstart;
    {
        GLXvector3f nrm = {0, 0, 1};
        float alight;

        alight = get_ambient_level() + water._layers->_light_add;
        alight = CLIP( alight / 255.0f, 0.0f, 1.0f );

        for (size_t cnt = 0; cnt < 4; cnt++ )
        {
            Vertex& v0 = v[cnt];

            tnc = imap[cnt];

            int jx = ix + ix_off[cnt];
            int jy = iy + iy_off[cnt];

            v0.x = jx * Info<float>::Grid::Size();
            v0.y = jy * Info<float>::Grid::Size();
            v0.z = water._layer_z_add[layer][frame][tnc] + water._layers[layer]._z;

            v0.s = fx_off[cnt] + offu;
            v0.t = fy_off[cnt] + offv;

            // get the lighting info from the grid
            TileIndex jtile = mesh.getTileIndex(PointGrid(jx, jy));
            float dlight;
            if ( GridIllumination::light_corner(mesh, jtile, v0.z, nrm, dlight) )
            {
                // take the v[cnt].color from the tnc vertices so that it is oriented prroperly
                v0.r = dlight * INV_FF + alight;
                v0.g = dlight * INV_FF + alight;
                v0.b = dlight * INV_FF + alight;

                v0.r = CLIP(v0.r, 0.0f, 1.0f);
                v0.g = CLIP(v0.g, 0.0f, 1.0f);
                v0.b = CLIP(v0.b, 0.0f, 1.0f);
            }
            else
            {
                v0.r = v0.g = v0.b = 0.0f;
            }

            // the application of alpha to the tile depends on the blending mode
            if ( water._light )
            {
                // blend using light
                v0.r *= falpha;
                v0.g *= falpha;
                v0.b *= falpha;
                v0.a = 1.0f;
            }
            else
            {
                // blend using alpha
                v0.a = falpha;
            }

            badvertex++;
        }
    }

    vb->unlock();

    // tell the mesh texture code that someone else is controlling the texture
    mesh_texture_invalidate();

	auto& renderer = Ego::Renderer::get();

    // set the texture
    renderer.getTextureUnit().setActivated(ptex);

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT );
    {
        bool use_depth_mask = ( !water._light && ( 1.0f == falpha ) );

        // do not draw hidden surfaces
        renderer.setDepthTestEnabled(true);
        renderer.setDepthFunction(Ego::CompareFunction::LessOrEqual);

        // only use the depth mask if the tile is NOT transparent
        renderer.setDepthWriteEnabled(use_depth_mask);

        // cull backward facing polygons
        // use clockwise orientation to determine backfaces
        oglx_begin_culling(Ego::CullingMode::Back, MAP_NRM_CULL);

        // set the blending mode
        renderer.setBlendingEnabled(true);
        if (water._light)
        {
			renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::OneMinusSourceColour);
        }
        else
        {
			renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);
        }

        // per-vertex coloring
        renderer.setGouraudShadingEnabled(true);
        renderer.render(*vb, Ego::PrimitiveType::TriangleFan, 0, 4);
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
        if ( patile->frame_update_old == update_wld ) continue;

        // save the old frame_add when we update to detect changes
        patile->frame_add_old = patile->frame_add;

        // cycle through all frames since the last time
        for ( Uint32 tnc = patile->frame_update_old + 1; tnc <= update_wld; tnc++ )
        {
            if ( 0 == ( tnc & patile->update_and ) )
            {
                patile->frame_add = ( patile->frame_add + 1 ) & patile->frame_and;
            }
        }

        // save the frame update
        patile->frame_update_old = update_wld;
    }
}
