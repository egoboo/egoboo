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

/// @file game/Graphics/RenderPasses.cpp
/// @brief Implementation of Egoboo's render passes
/// @author Michael Heilmann

#include "game/Graphics/RenderPasses.hpp"
#include "game/graphic.h"
#include "game/graphic_prt.h"
#include "game/graphic_mad.h"
#include "game/graphic_fan.h"
#include "game/graphic_billboard.h"
#include "game/renderer_3d.h"
#include "game/Logic/Player.hpp"
#include "egolib/Script/script.h"
#include "game/script_compile.h"
#include "game/game.h"
#include "game/lighting.h"
#include "game/egoboo.h"
#include "game/mesh.h"
#include "game/Graphics/CameraSystem.hpp"
#include "egolib/FileFormats/Globals.hpp"
#include "game/Module/Module.hpp"
#include "game/Entities/_Include.hpp"

namespace Ego {
namespace Graphics {

namespace Internal {

ElementV2::ElementV2()
    : ElementV2(std::numeric_limits<float>::infinity(), 
                Index1D::Invalid, 
                std::numeric_limits<uint32_t>::max()) {
}

ElementV2::ElementV2(float distance, const Index1D& tileIndex, uint32_t textureIndex)
	: distance(distance), tileIndex(tileIndex), textureIndex(textureIndex) {
}

ElementV2::ElementV2(const ElementV2& other)
	: ElementV2(other.getDistance(), other.getTileIndex(), other.getTextureIndex()) {
}

const ElementV2& ElementV2::operator=(const ElementV2& other) {
	distance = other.getDistance();
	tileIndex = other.getTileIndex();
	textureIndex = other.getTextureIndex();
	return *this;
}

float ElementV2::getDistance() const {
    return distance;
}

const Index1D& ElementV2::getTileIndex() const {
    return tileIndex;
}

uint32_t ElementV2::getTextureIndex() const {
    return textureIndex;
}

bool ElementV2::compare(const ElementV2& x, const ElementV2& y) {
	int result = (int)x.getTextureIndex() - (int)y.getTextureIndex();
	if (result < 0) {
		return true;
	} else if (result > 0) {
		return false;
	} else {
        float result = x.getDistance() - y.getDistance();
		if (result < 0.0f) {
			return true;
		} else {
			return false;
		}
	}
}

void TileListV2::render(ego_mesh_t& mesh, const std::vector<ClippingEntry>& tiles)
{
	size_t tcnt = mesh._tmem.getInfo().getTileCount();

	if (0 == tiles.size()) {
		return;
	}

	// insert the rlst values into lst_vals
	std::vector<ElementV2> lst_vals(tiles.size());
	for (size_t i = 0; i < tiles.size(); ++i)
	{
        uint32_t textureIndex;
		if (tiles[i].getIndex() >= tcnt)
		{
			textureIndex = std::numeric_limits<uint32_t>::max();
		}
		else
		{
			const ego_tile_info_t& tile = mesh._tmem.get(tiles[i].getIndex());

			int img = TILE_GET_LOWER_BITS(tile._img);
			if (tile._type >= tile_dict.offset)
			{
				img += Graphics::MESH_IMG_COUNT;
			}

			textureIndex = img;
		}
        lst_vals[i] = ElementV2(tiles[i].getDistance(), tiles[i].getIndex(), textureIndex);
	}

	std::sort(lst_vals.begin(), lst_vals.end(), ElementV2::compare);

	// restart the mesh texture code
	TileRenderer::invalidate();

	for (size_t i = 0; i < tiles.size(); ++i)
	{
		Index1D tmp_itile = lst_vals[i].getTileIndex();

		gfx_rv render_rv = render_fan(mesh, tmp_itile);
		if (egoboo_config_t::get().debug_developerMode_enable.getValue() && gfx_error == render_rv)
		{
			Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "error rendering tile ", tmp_itile.i(), Log::EndOfEntry);
		}
	}

	// let the mesh texture code know that someone else is in control now
	TileRenderer::invalidate();
}

gfx_rv TileListV2::render_fan(ego_mesh_t& mesh, const Index1D& i) {
    // grab a pointer to the tile
    const ego_tile_info_t& ptile = mesh.getTileInfo(i);

    const tile_mem_t& ptmem = mesh._tmem;

    // do not render the itile if the image image is invalid
    if (ptile.isFanOff())  return gfx_success;

    tile_definition_t *pdef = tile_dict.get(ptile._type);
    if (NULL == pdef) return gfx_fail;

    // bind the correct texture
    TileRenderer::bind(ptile);

    {
        OpenGL::PushClientAttrib pca(GL_CLIENT_VERTEX_ARRAY_BIT);
        {
            // Per-vertex coloring.
            Renderer::get().setGouraudShadingEnabled(gfx.gouraudShading_enable); // GL_LIGHTING_BIT

                                                                                      /// @note claforte@> Put this in an initialization function.
            GL_DEBUG(glEnableClientState)(GL_VERTEX_ARRAY);
            GL_DEBUG(glVertexPointer)(3, GL_FLOAT, 0, &(ptmem._plst[ptile._vrtstart]));

            GL_DEBUG(glEnableClientState)(GL_TEXTURE_COORD_ARRAY);
            GL_DEBUG(glTexCoordPointer)(2, GL_FLOAT, 0, &(ptmem._tlst[ptile._vrtstart]));

            if (gfx.gouraudShading_enable) {
                GL_DEBUG(glEnableClientState)(GL_COLOR_ARRAY);
                GL_DEBUG(glColorPointer)(3, GL_FLOAT, 0, &(ptmem._clst[ptile._vrtstart]));
            } else {
                GL_DEBUG(glDisableClientState)(GL_COLOR_ARRAY);
            }
            // grab some model info
            uint16_t commands = pdef->command_count;

            // Render each command
            for (size_t cnt = 0, entry = 0; cnt < commands; cnt++) {
                uint8_t numEntries = pdef->command_entries[cnt];

                GL_DEBUG(glDrawElements)(GL_TRIANGLE_FAN, numEntries, GL_UNSIGNED_SHORT, &(pdef->command_verts[entry]));
                entry += numEntries;
            }
        }
    }

    if (egoboo_config_t::get().debug_mesh_renderNormals.getValue()) {
        TileRenderer::invalidate();
        auto& renderer = Renderer::get();
        renderer.getTextureUnit().setActivated(nullptr);
        renderer.setColour(Colour4f::white());
        for (size_t i = ptile._vrtstart, j = 0; j < 4; ++i, ++j) {
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

void TileListV2::render_heightmap(ego_mesh_t& mesh, const std::vector<ClippingEntry>& tiles)
{
    for (size_t i = 0; i < tiles.size(); ++i)
    {
        render_heightmap_fan(mesh, tiles[i].getIndex());
    }
}

gfx_rv TileListV2::render_heightmap_fan(ego_mesh_t& mesh, const Index1D& tileIndex) {
    /// @author ZZ
    /// @details This function draws a mesh itile
    GLvertex v[4];

    int cnt, vertex;
    size_t badvertex;
    int ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};

    const tile_mem_t& ptmem = mesh._tmem;

    const ego_tile_info_t& ptile = mesh.getTileInfo(tileIndex);

    /// @author BB
    /// @details the water info is for TILES, not for vertices, so ignore all vertex info and just draw the water
    ///     tile where it's supposed to go
    auto i2 = Grid::map<int>(tileIndex, mesh._info.getTileCountX());

    // vertex is a value from 0-15, for the meshcommandref/u/v variables
    // badvertex is a value that references the actual vertex number

    const uint8_t twist = ptile._twist;

    // Original points
    badvertex = ptile._vrtstart;          // Get big reference value
    for (cnt = 0; cnt < 4; cnt++) {
        float tmp;
        v[cnt].pos[XX] = (i2.x() + ix_off[cnt]) * Info<float>::Grid::Size();
        v[cnt].pos[YY] = (i2.y() + iy_off[cnt]) * Info<float>::Grid::Size();
        v[cnt].pos[ZZ] = ptmem._plst[badvertex][ZZ];

        tmp = g_meshLookupTables.twist_nrm[twist][kZ];
        tmp *= tmp;

        v[cnt].col[RR] = tmp * (tmp + (1.0f - tmp) * g_meshLookupTables.twist_nrm[twist][kX] * g_meshLookupTables.twist_nrm[twist][kX]);
        v[cnt].col[GG] = tmp * (tmp + (1.0f - tmp) * g_meshLookupTables.twist_nrm[twist][kY] * g_meshLookupTables.twist_nrm[twist][kY]);
        v[cnt].col[BB] = tmp;
        v[cnt].col[AA] = 1.0f;

        v[cnt].col[RR] = constrain(v[cnt].col[RR], 0.0f, 1.0f);
        v[cnt].col[GG] = constrain(v[cnt].col[GG], 0.0f, 1.0f);
        v[cnt].col[BB] = constrain(v[cnt].col[BB], 0.0f, 1.0f);

        badvertex++;
    }

    Renderer::get().getTextureUnit().setActivated(nullptr);

    // Render each command
    GL_DEBUG(glBegin)(GL_TRIANGLE_FAN);
    {
        for (vertex = 0; vertex < 4; vertex++) {
            GL_DEBUG(glColor3fv)(v[vertex].col);
            GL_DEBUG(glVertex3fv)(v[vertex].pos);
        }
    }
    GL_DEBUG_END();

    return gfx_success;
}

}







void Reflective1::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
    auto& renderer = Renderer::get();
    // Set projection matrix.
    renderer.setProjectionMatrix(camera.getProjectionMatrix());
    // Set view matrix.
    renderer.setViewMatrix(camera.getViewMatrix());
    // Set world matrix.
    renderer.setWorldMatrix(Matrix4f4f::identity());
    // Disable culling.
    renderer.setCullingMode(CullingMode::None);
    // Perform less-or-equal depth testing.
    renderer.setDepthTestEnabled(true);
    renderer.setDepthFunction(CompareFunction::LessOrEqual);
    // Write to depth buffer.
    renderer.setDepthWriteEnabled(true);
    if (gfx.refon) {
		doReflectionsEnabled(camera, tl, el);
	} else {
		doReflectionsDisabled(camera, tl, el);
	}
}

void Reflective1::doReflectionsEnabled(::Camera& camera, const TileList& tl, const EntityList& el) {
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	{
		// Enable blending.
		auto& renderer = Renderer::get();
		// Enable blending.
		renderer.setBlendingEnabled(true);
		renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::One);

		// reduce texture hashing by loading up each texture only once
		Internal::TileListV2::render(*tl.getMesh(), tl._reflective);
	}
}

/**
 * @todo We could re-use Graphics::RenderPasses::NonReflective for that.
 *       However, things will evolve soon enough ... Egoboo's whole way of
 *       rendering needs to be improved (at least we've improved structure &
 *       terminology for now).
 */
void Reflective1::doReflectionsDisabled(::Camera& camera, const TileList& tl, const EntityList& el) {
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	{
		auto& renderer = Renderer::get();
		// Disable blending.
		renderer.setBlendingEnabled(false);
		// Do not display the completely transparent portion:
		// Use alpha test to allow the thatched roof tiles to look like thatch.
		renderer.setAlphaTestEnabled(true);
		// Speed-up drawing of surfaces with alpha = 0.0f sections
		renderer.setAlphaFunction(CompareFunction::Greater, 0.0f);

		// reduce texture hashing by loading up each texture only once
		Internal::TileListV2::render(*tl.getMesh(), tl._reflective);
	}
}













}
}