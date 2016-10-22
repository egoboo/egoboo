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
namespace RenderPasses {

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

void TileListV2::render(const ego_mesh_t& mesh, const Graphics::renderlist_lst_t& rlst)
{
	size_t tcnt = mesh._tmem.getInfo().getTileCount();

	if (0 == rlst.size) {
		return;
	}

	// insert the rlst values into lst_vals
	std::vector<ElementV2> lst_vals(rlst.size);
	for (size_t i = 0; i < rlst.size; ++i)
	{
        uint32_t textureIndex;
		if (rlst.lst[i]._index >= tcnt)
		{
			textureIndex = std::numeric_limits<uint32_t>::max();
		}
		else
		{
			const ego_tile_info_t& tile = mesh._tmem.get(rlst.lst[i]._index);

			int img = TILE_GET_LOWER_BITS(tile._img);
			if (tile._type >= tile_dict.offset)
			{
				img += Ego::Graphics::MESH_IMG_COUNT;
			}

			textureIndex = img;
		}
        lst_vals[i] = ElementV2(rlst.lst[i]._distance, rlst.lst[i]._index, textureIndex);
	}

	std::sort(lst_vals.begin(), lst_vals.end(), ElementV2::compare);

	// restart the mesh texture code
	TileRenderer::invalidate();

	for (size_t i = 0; i < rlst.size; ++i)
	{
		Index1D tmp_itile = lst_vals[i].getTileIndex();

		gfx_rv render_rv = render_fan(mesh, tmp_itile);
		if (egoboo_config_t::get().debug_developerMode_enable.getValue() && gfx_error == render_rv)
		{
			Log::get().warn("%s - error rendering tile %d.\n", __FUNCTION__, tmp_itile.getI());
		}
	}

	// let the mesh texture code know that someone else is in control now
	TileRenderer::invalidate();
}

gfx_rv TileListV2::render_fan(const ego_mesh_t& mesh, const Index1D& i) {
    /// @author ZZ
    /// @details This function draws a mesh itile
    /// Optimized to use gl*Pointer() and glArrayElement() for vertex presentation

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
        Ego::OpenGL::PushClientAttrib pca(GL_CLIENT_VERTEX_ARRAY_BIT);
        {
            // Per-vertex coloring.
            Ego::Renderer::get().setGouraudShadingEnabled(gfx.gouraudShading_enable); // GL_LIGHTING_BIT

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
        auto& renderer = Ego::Renderer::get();
        renderer.getTextureUnit().setActivated(nullptr);
        renderer.setColour(Ego::Colour4f::white());
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

gfx_rv TileListV2::render_hmap_fan(const ego_mesh_t * mesh, const Index1D& tileIndex) {
    /// @author ZZ
    /// @details This function draws a mesh itile
    GLvertex v[4];

    int cnt, vertex;
    size_t badvertex;
    int ix, iy, ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};

    if (NULL == mesh) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "NULL mesh");
        return gfx_error;
    }

    const tile_mem_t& ptmem = mesh->_tmem;

    const ego_tile_info_t& ptile = mesh->getTileInfo(tileIndex);

    /// @author BB
    /// @details the water info is for TILES, not for vertices, so ignore all vertex info and just draw the water
    ///     tile where it's supposed to go

    ix = tileIndex.getI() % mesh->_info.getTileCountX();
    iy = tileIndex.getI() / mesh->_info.getTileCountX();

    // vertex is a value from 0-15, for the meshcommandref/u/v variables
    // badvertex is a value that references the actual vertex number

    const uint8_t twist = ptile._twist;

    // Original points
    badvertex = ptile._vrtstart;          // Get big reference value
    for (cnt = 0; cnt < 4; cnt++) {
        float tmp;
        v[cnt].pos[XX] = (ix + ix_off[cnt]) * Info<float>::Grid::Size();
        v[cnt].pos[YY] = (iy + iy_off[cnt]) * Info<float>::Grid::Size();
        v[cnt].pos[ZZ] = ptmem._plst[badvertex][ZZ];

        tmp = g_meshLookupTables.twist_nrm[twist][kZ];
        tmp *= tmp;

        v[cnt].col[RR] = tmp * (tmp + (1.0f - tmp) * g_meshLookupTables.twist_nrm[twist][kX] * g_meshLookupTables.twist_nrm[twist][kX]);
        v[cnt].col[GG] = tmp * (tmp + (1.0f - tmp) * g_meshLookupTables.twist_nrm[twist][kY] * g_meshLookupTables.twist_nrm[twist][kY]);
        v[cnt].col[BB] = tmp;
        v[cnt].col[AA] = 1.0f;

        v[cnt].col[RR] = Ego::Math::constrain(v[cnt].col[RR], 0.0f, 1.0f);
        v[cnt].col[GG] = Ego::Math::constrain(v[cnt].col[GG], 0.0f, 1.0f);
        v[cnt].col[BB] = Ego::Math::constrain(v[cnt].col[BB], 0.0f, 1.0f);

        badvertex++;
    }

    Ego::Renderer::get().getTextureUnit().setActivated(nullptr);

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

gfx_rv TileListV2::render_water_fan(ego_mesh_t& mesh, const Index1D& tileIndex, const Uint8 layer) {

    static const int ix_off[4] = {1, 1, 0, 0}, iy_off[4] = {0, 1, 1, 0};

    int    tnc;
    size_t badvertex;
    int  imap[4];
    float fx_off[4], fy_off[4];

    const Ego::MeshInfo& info = mesh._info;

    const ego_tile_info_t& ptile = mesh.getTileInfo(tileIndex);

    float falpha;
    falpha = FF_TO_FLOAT(_currentModule->getWater()._layers[layer]._alpha);
    falpha = Ego::Math::constrain(falpha, 0.0f, 1.0f);

    /// @note BB@> the water info is for TILES, not for vertices, so ignore all vertex info and just draw the water
    ///            tile where it's supposed to go

    int ix = tileIndex.getI() % info.getTileCountX();
    int iy = tileIndex.getI() / info.getTileCountX();

    // To make life easier
    uint16_t type = 0;                                         // Command type ( index to points in tile )
    tile_definition_t *pdef = tile_dict.get(type);
    if (NULL == pdef) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, type, "unknown tile type");
        return gfx_error;
    }
    float offu = _currentModule->getWater()._layers[layer]._tx[XX];               // Texture offsets
    float offv = _currentModule->getWater()._layers[layer]._tx[YY];
    uint16_t frame = _currentModule->getWater()._layers[layer]._frame;                // Frame

    std::shared_ptr<const Ego::Texture> ptex = _currentModule->getWaterTexture(layer);

    float x1 = (float)ptex->getWidth() / (float)ptex->getSourceWidth();
    float y1 = (float)ptex->getHeight() / (float)ptex->getSourceHeight();

    for (size_t cnt = 0; cnt < 4; cnt++) {
        fx_off[cnt] = x1 * ix_off[cnt];
        fy_off[cnt] = y1 * iy_off[cnt];

        imap[cnt] = cnt;
    }

    // flip the coordinates around based on the "mode" of the tile
    if ((ix & 1) == 0) {
        std::swap(imap[0], imap[3]);
        std::swap(imap[1], imap[2]);
    }

    if ((iy & 1) == 0) {
        std::swap(imap[0], imap[1]);
        std::swap(imap[2], imap[3]);
    }

    // draw draw front and back faces of polygons
    Ego::Renderer::get().setCullingMode(Ego::CullingMode::None);

    struct Vertex {
        float x, y, z;
        float r, g, b, a;
        float s, t;
    };
    auto vb = std::make_shared<Ego::VertexBuffer>(4, Ego::VertexFormatFactory::get<Ego::VertexFormat::P3FC4FT2F>());
    Vertex *v = static_cast<Vertex *>(vb->lock());

    // Original points
    badvertex = ptile._vrtstart;
    {
        GLXvector3f nrm = {0, 0, 1};

        float alight = get_ambient_level() + _currentModule->getWater()._layers->_light_add;
        alight = Ego::Math::constrain(alight / 255.0f, 0.0f, 1.0f);

        for (size_t cnt = 0; cnt < 4; cnt++) {
            Vertex& v0 = v[cnt];

            tnc = imap[cnt];

            int jx = ix + ix_off[cnt];
            int jy = iy + iy_off[cnt];

            v0.x = jx * Info<float>::Grid::Size();
            v0.y = jy * Info<float>::Grid::Size();
            v0.z = _currentModule->getWater()._layer_z_add[layer][frame][tnc] + _currentModule->getWater()._layers[layer]._z;

            v0.s = fx_off[cnt] + offu;
            v0.t = fy_off[cnt] + offv;

            float dlight = 0.0f;
            if (jx <= 0 || jy <= 0 || jx >= info.getTileCountX() || jy >= info.getTileCountY()) {
                //All water is dark near edges of the map
                dlight = 0.0f;
            } else {
                //Else interpolate using ligh levels of nearby tiles
                Index1D jtile = mesh.getTileIndex(Index2D(jx, jy));
                GridIllumination::light_corner(mesh, jtile, v0.z, nrm, dlight);
            }

            // take the v[cnt].color from the tnc vertices so that it is oriented properly
            v0.r = Ego::Math::constrain(dlight * INV_FF<float>() + alight, 0.0f, 1.0f);
            v0.g = Ego::Math::constrain(dlight * INV_FF<float>() + alight, 0.0f, 1.0f);
            v0.b = Ego::Math::constrain(dlight * INV_FF<float>() + alight, 0.0f, 1.0f);

            // the application of alpha to the tile depends on the blending mode
            if (_currentModule->getWater()._light) {
                // blend using light
                v0.r *= falpha;
                v0.g *= falpha;
                v0.b *= falpha;
                v0.a = 1.0f;
            } else {
                // blend using alpha
                v0.a = falpha;
            }

            badvertex++;
        }
    }

    vb->unlock();

    // tell the mesh texture code that someone else is controlling the texture
    TileRenderer::invalidate();

    auto& renderer = Ego::Renderer::get();

    // set the texture
    renderer.getTextureUnit().setActivated(ptex.get());

    Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);
    {
        bool use_depth_mask = (!_currentModule->getWater()._light && (1.0f == falpha));

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
        if (_currentModule->getWater()._light) {
            renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::OneMinusSourceColour);
        } else {
            renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);
        }

        // per-vertex coloring
        renderer.setGouraudShadingEnabled(true);
        renderer.render(*vb, Ego::PrimitiveType::TriangleFan, 0, 4);
    }

    return gfx_success;
}

}

void Background::doRun(::Camera& cam, const TileList& tl, const EntityList& el) {
	if (!gfx.draw_background || !_currentModule->getWater()._background_req) {
		return;
	}

	tile_mem_t& tmem = _currentModule->getMeshPointer()->_tmem;

	// which layer
	water_instance_layer_t *ilayer = _currentModule->getWater()._layers + 0;

	// the "official" camera height
	float z0 = 1500;

	// clip the waterlayer uv offset
	ilayer->_tx[XX] = ilayer->_tx[XX] - (float)std::floor(ilayer->_tx[XX]);
	ilayer->_tx[YY] = ilayer->_tx[YY] - (float)std::floor(ilayer->_tx[YY]);

	float xmag, Cx_0, Cx_1;
	float ymag, Cy_0, Cy_1;

	// determine the constants for the x-coordinate
	xmag = _currentModule->getWater()._backgroundrepeat / 4 / (1.0f + z0 * ilayer->_dist[XX]) / Info<float>::Grid::Size();
	Cx_0 = xmag * (1.0f + cam.getPosition()[kZ] * ilayer->_dist[XX]);
	Cx_1 = -xmag * (1.0f + (cam.getPosition()[kZ] - z0) * ilayer->_dist[XX]);

	// determine the constants for the y-coordinate
	ymag = _currentModule->getWater()._backgroundrepeat / 4 / (1.0f + z0 * ilayer->_dist[YY]) / Info<float>::Grid::Size();
	Cy_0 = ymag * (1.0f + cam.getPosition()[kZ] * ilayer->_dist[YY]);
	Cy_1 = -ymag * (1.0f + (cam.getPosition()[kZ] - z0) * ilayer->_dist[YY]);

	float Qx, Qy;

	{
		BufferScopedLock lock(_vertexBuffer);
		Vertex *vertices = lock.get<Vertex>();
		// Figure out the coordinates of its corners
		Qx = -tmem._edge_x;
		Qy = -tmem._edge_y;
		vertices[0].x = Qx;
		vertices[0].y = Qy;
		vertices[0].z = cam.getPosition()[kZ] - z0;
		vertices[0].s = Cx_0 * Qx + Cx_1 * cam.getPosition()[kX] + ilayer->_tx[XX];
		vertices[0].t = Cy_0 * Qy + Cy_1 * cam.getPosition()[kY] + ilayer->_tx[YY];

		Qx = 2 * tmem._edge_x;
		Qy = -tmem._edge_y;
		vertices[1].x = Qx;
		vertices[1].y = Qy;
		vertices[1].z = cam.getPosition()[kZ] - z0;
		vertices[1].s = Cx_0 * Qx + Cx_1 * cam.getPosition()[kX] + ilayer->_tx[XX];
		vertices[1].t = Cy_0 * Qy + Cy_1 * cam.getPosition()[kY] + ilayer->_tx[YY];

		Qx = 2 * tmem._edge_x;
		Qy = 2 * tmem._edge_y;
		vertices[2].x = Qx;
		vertices[2].y = Qy;
		vertices[2].z = cam.getPosition()[kZ] - z0;
		vertices[2].s = Cx_0 * Qx + Cx_1 * cam.getPosition()[kX] + ilayer->_tx[XX];
		vertices[2].t = Cy_0 * Qy + Cy_1 * cam.getPosition()[kY] + ilayer->_tx[YY];

		Qx = -tmem._edge_x;
		Qy = 2 * tmem._edge_y;
		vertices[3].x = Qx;
		vertices[3].y = Qy;
		vertices[3].z = cam.getPosition()[kZ] - z0;
		vertices[3].s = Cx_0 * Qx + Cx_1 * cam.getPosition()[kX] + ilayer->_tx[XX];
		vertices[3].t = Cy_0 * Qy + Cy_1 * cam.getPosition()[kY] + ilayer->_tx[YY];
	}

	float light = _currentModule->getWater()._light ? 1.0f : 0.0f;
	float alpha = ilayer->_alpha * INV_FF<float>();

	float intens = 1.0f;

	if (gfx.usefaredge)
	{
		intens = light_a * ilayer->_light_add;

		float fcos = light_nrm[kZ];
		if (fcos > 0.0f)
		{
			intens += fcos * fcos * light_d * ilayer->_light_dir;
		}

		intens = constrain(intens, 0.0f, 1.0f);
	}

	auto& renderer = Renderer::get();

	renderer.getTextureUnit().setActivated(_currentModule->getWaterTexture(0).get());

    {
        OpenGL::PushAttrib pa(GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
        {
            // flat shading
            renderer.setGouraudShadingEnabled(false);

            // Do not write into the depth buffer.
            renderer.setDepthWriteEnabled(false);

            // Essentially disable the depth test without calling
            // renderer.setDepthTestEnabled(false).
            renderer.setDepthTestEnabled(true);
            renderer.setDepthFunction(CompareFunction::AlwaysPass);

            // draw draw front and back faces of polygons
            renderer.setCullingMode(CullingMode::None);

            if (alpha > 0.0f) {
                {
                    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
                    {
                        renderer.setColour(Colour4f(intens, intens, intens, alpha));

                        if (alpha >= 1.0f) {
                            renderer.setBlendingEnabled(false);
                        } else {
                            renderer.setBlendingEnabled(true);
                            renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);
                        }

                        renderer.render(_vertexBuffer, PrimitiveType::TriangleFan, 0, 4);
                    }
                }
            }

            if (light > 0.0f) {
                {
                    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
                    {
                        renderer.setBlendingEnabled(false);

                        renderer.setColour(Colour4f(light, light, light, 1.0f));

                        renderer.render(_vertexBuffer, PrimitiveType::TriangleFan, 0, 4);
                    }
                }
            }
        }
    }
}

void Foreground::doRun(::Camera& cam, const TileList& tl, const EntityList& el) {
	if (!gfx.draw_overlay || !_currentModule->getWater()._background_req) {
		return;
	}

	water_instance_layer_t *ilayer = _currentModule->getWater()._layers + 1;

	Vector3f vforw_wind(ilayer->_tx_add[XX], ilayer->_tx_add[YY], 0.0f);
	vforw_wind.normalize();

	Vector3f vforw_cam;
	mat_getCamForward(cam.getViewMatrix(), vforw_cam);
	vforw_cam.normalize();

	// make the texture begin to disappear if you are not looking straight down
	float ftmp = vforw_wind.dot(vforw_cam);

	float alpha = (1.0f - ftmp * ftmp) * (ilayer->_alpha * INV_FF<float>());

	if (alpha != 0.0f)
	{
		// Figure out the screen coordinates of its corners
        auto windowSize = Ego::GraphicsSystem::window->getSize();
		float x = windowSize.width() << 6;
		float y = windowSize.height() << 6;
		float z = 0;
		float size = x + y + 1;
		static const Facing default_turn = Facing((3 * 2047) << 2);
		float sinsize = std::sin(default_turn) * size;
		float cossize = std::cos(default_turn) * size;
        // TODO: Shouldn't this be std::min(x / windowSize.width(), y / windowSize.height())?
		float loc_foregroundrepeat = _currentModule->getWater()._foregroundrepeat *
                                     std::min(x / windowSize.width(), y / windowSize.height());

		{
			BufferScopedLock lock(_vertexBuffer);
			Vertex *vertices = lock.get<Vertex>();

			vertices[0].x = x + cossize;
			vertices[0].y = y - sinsize;
			vertices[0].z = z;
			vertices[0].s = ilayer->_tx[XX];
			vertices[0].t = ilayer->_tx[YY];

			vertices[1].x = x + sinsize;
			vertices[1].y = y + cossize;
			vertices[1].z = z;
			vertices[1].s = ilayer->_tx[XX] + loc_foregroundrepeat;
			vertices[1].t = ilayer->_tx[YY];

			vertices[2].x = x - cossize;
			vertices[2].y = y + sinsize;
			vertices[2].z = z;
			vertices[2].s = ilayer->_tx[SS] + loc_foregroundrepeat;
			vertices[2].t = ilayer->_tx[TT] + loc_foregroundrepeat;

			vertices[3].x = x - sinsize;
			vertices[3].y = y - cossize;
			vertices[3].z = z;
			vertices[3].s = ilayer->_tx[SS];
			vertices[3].t = ilayer->_tx[TT] + loc_foregroundrepeat;
		}

		auto& renderer = Renderer::get();

		renderer.getTextureUnit().setActivated(_currentModule->getWaterTexture(1).get());

        {
            Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT | GL_HINT_BIT);
            {
                // make sure that the texture is as smooth as possible
                GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT, GL_NICEST);          // GL_HINT_BIT

                // flat shading
                renderer.setGouraudShadingEnabled(false);                     // GL_LIGHTING_BIT

                // Do not write into the depth buffer.
                renderer.setDepthWriteEnabled(false);

                // Essentially disable the depth test without calling
                // renderer.setDepthTestEnabled(false).
                renderer.setDepthTestEnabled(true);
                renderer.setDepthFunction(CompareFunction::AlwaysPass);

                // draw draw front and back faces of polygons
                renderer.setCullingMode(CullingMode::None);

                // do not display the completely transparent portion
                renderer.setAlphaTestEnabled(true);
                renderer.setAlphaFunction(CompareFunction::Greater, 0.0f);

                // make the texture a filter
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceColour);

                renderer.getTextureUnit().setActivated(_currentModule->getWaterTexture(1).get());

                renderer.setColour(Colour4f(1.0f, 1.0f, 1.0f, 1.0f - std::abs(alpha)));
                renderer.render(_vertexBuffer, PrimitiveType::TriangleFan, 0, 4);
            }
        }
	}
}

void Reflective0::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
	if (gfx.refon) {
		doReflectionsEnabled(camera, tl, el);
	}
	else {
		doReflectionsDisabled(camera, tl, el);
	}
}

void Reflective0::doReflectionsEnabled(::Camera& camera, const TileList& tl, const EntityList& el) {
	/// @details draw the reflective tiles, but turn off the depth buffer
	///          this blanks out any background that might've been drawn

    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	{
		auto& renderer = Renderer::get();
		// DO NOT store the surface depth
		renderer.setDepthWriteEnabled(false);

		// do not draw hidden surfaces
		renderer.setDepthTestEnabled(true);
		renderer.setDepthFunction(CompareFunction::LessOrEqual);

		// black out any backgound, but allow the background to show through any holes in the floor
		renderer.setBlendingEnabled(true);
		// use the alpha channel to modulate the transparency
		renderer.setBlendFunction(BlendFunction::Zero, BlendFunction::OneMinusSourceAlpha);
		// do not display the completely transparent portion
		// use alpha test to allow the thatched roof tiles to look like thatch
		renderer.setAlphaTestEnabled(true);
		// speed-up drawing of surfaces with alpha == 0.0f sections
		renderer.setAlphaFunction(CompareFunction::Greater, 0.0f);
		// reduce texture hashing by loading up each texture only once
		Internal::TileListV2::render(*tl.getMesh(), tl._reflective);
	}
}

void Reflective0::doReflectionsDisabled(::Camera& camera, const TileList& tl, const EntityList& el) {
	/* Intentionally empty. */
}

void Reflective1::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
	if (gfx.refon) {
		doReflectionsEnabled(camera, tl, el);
	} else {
		doReflectionsDisabled(camera, tl, el);
	}
}

void Reflective1::doCommon(::Camera& camera, const TileList& tl, const EntityList& el) {
	auto& renderer = Renderer::get();
	// Disable culling.
	renderer.setCullingMode(CullingMode::None);
	// Perform less-or-equal depth testing.
	renderer.setDepthTestEnabled(true);
	renderer.setDepthFunction(CompareFunction::LessOrEqual);
	// Write to depth buffer.
	renderer.setDepthWriteEnabled(true);
}

void Reflective1::doReflectionsEnabled(::Camera& camera, const TileList& tl, const EntityList& el) {
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	{
		doCommon(camera, tl, el);
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
 * @todo We could re-use Ego::Graphics::RenderPasses::NonReflective for that.
 *       However, things will evolve soon enough ... Egoboo's whole way of
 *       rendering needs to be improved (at least we've improved structure &
 *       terminology for now).
 */
void Reflective1::doReflectionsDisabled(::Camera& camera, const TileList& tl, const EntityList& el) {
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	{
		doCommon(camera, tl, el);
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

void NonReflective::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	{
		auto& renderer = Renderer::get();
		// draw draw front and back faces of polygons
		renderer.setCullingMode(CullingMode::None);
		// Write to the depth buffer.
		renderer.setDepthWriteEnabled(true);
		// Do not draw hidden surfaces.
		renderer.setDepthTestEnabled(true);
		renderer.setDepthFunction(CompareFunction::LessOrEqual);

		// Disable blending.
		renderer.setBlendingEnabled(false);
		// Do not display the completely transparent portion:
		// Use alpha test to allow the thatched roof tiles to look like thatch.
		renderer.setAlphaTestEnabled(true);
		// Speed-up drawing of surfaces with alpha == 0.0f sections.
		renderer.setAlphaFunction(CompareFunction::Greater, 0.0f);

		// reduce texture hashing by loading up each texture only once
		Internal::TileListV2::render(*tl.getMesh(), tl._nonReflective);
	}
	OpenGL::Utilities::isError();
}

void EntityShadows::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
	// If shadows are not enabled, return immediatly.
	if (!gfx.shadows_enable) {
		return;
	}
	// Get the renderer.
	auto& renderer = Renderer::get();
	// Do not write into the depth buffer.
	renderer.setDepthWriteEnabled(false);
	// Enable depth tests.
	renderer.setDepthTestEnabled(true);
	// Enable blending.
	renderer.setBlendingEnabled(true);
	renderer.setBlendFunction(BlendFunction::Zero, BlendFunction::OneMinusSourceColour);

	// Keep track of the number of rendered shadows.
	size_t count = 0;

	if (gfx.shadows_highQuality_enable) {
		// Render high-quality shadows.
		for (size_t i = 0; i < el.getSize(); ++i) {
			ObjectRef ichr = el.get(i).iobj;
			if (ObjectRef::Invalid == ichr) continue;
			if (0 == _currentModule->getObjectHandler().get(ichr)->shadow_size) continue;
			doHighQualityShadow(ichr);
			count++;
		}
	} else {
		// Render low-quality shadows.
		for (size_t i = 0; i < el.getSize(); ++i) {
            ObjectRef ichr = el.get(i).iobj;
			if (ObjectRef::Invalid == ichr) continue;
			if (0 == _currentModule->getObjectHandler().get(ichr)->shadow_size) continue;
			doLowQualityShadow(ichr);
			count++;
		}
	}
}

void EntityShadows::doLowQualityShadow(const ObjectRef character) {
	Object *pchr = _currentModule->getObjectHandler().get(character);
	if(pchr->isBeingHeld()) return;

	// If the object is hidden it is not drawn at all, so it has no shadow.
	// If the object's shadow size is qa 0, then it has no shadow.
	if (pchr->isHidden() || 0 == pchr->shadow_size)
	{
		return;
	}

	// No shadow if completely transparent or completely glowing.
	float alpha = (255 == pchr->inst.light) ? pchr->inst.alpha  * INV_FF<float>() : (pchr->inst.alpha - pchr->inst.light) * INV_FF<float>();

	/// @test ZF@> previous test didn't work, but this one does
	//if ( alpha * 255 < 1 ) return;
	if (pchr->inst.light <= INVISIBLE || pchr->inst.alpha <= INVISIBLE) return;

	// much reduced shadow if on a reflective tile
	auto mesh = _currentModule->getMeshPointer();
	if (0 != mesh->test_fx(pchr->getTile(), MAPFX_REFLECTIVE))
	{
		alpha *= 0.1f;
	}
	if (alpha < INV_FF<float>()) return;

	// Original points
	float level = pchr->getObjectPhysics().getGroundElevation() + SHADOWRAISE;
	float height = pchr->inst.getMatrix()(2, 3) - level;
	float height_factor = 1.0f - height / (pchr->shadow_size * 5.0f);
	if (height_factor <= 0.0f) return;

	// how much transparency from height
	alpha *= height_factor * 0.5f + 0.25f;
	if (alpha < INV_FF<float>()) return;

	float x = pchr->inst.getMatrix()(0, 3); ///< @todo MH: This should be the x/y position of the model.
	float y = pchr->inst.getMatrix()(1, 3); ///<           Use a more self-descriptive method to describe this.

	float size = pchr->shadow_size * height_factor;

	{
		BufferScopedLock lock(_vertexBuffer);
		Vertex *vertices = lock.get<Vertex>();
		vertices[0].x = (float)x + size;
		vertices[0].y = (float)y - size;
		vertices[0].z = (float)level;

		vertices[1].x = (float)x + size;
		vertices[1].y = (float)y + size;
		vertices[1].z = (float)level;

		vertices[2].x = (float)x - size;
		vertices[2].y = (float)y + size;
		vertices[2].z = (float)level;

		vertices[3].x = (float)x - size;
		vertices[3].y = (float)y - size;
		vertices[3].z = (float)level;
	}

	// Choose texture and matrix
	Ego::Renderer::get().getTextureUnit().setActivated(ParticleHandler::get().getLightParticleTexture().get());
	int itex_style = SPRITE_LIGHT; //ZF> Note: index 1 is for SPRITE_LIGHT

	{
		BufferScopedLock lock(_vertexBuffer);
		Vertex *vertices = lock.get<Vertex>();
		vertices[0].s = CALCULATE_PRT_U0(itex_style, 236);
		vertices[0].t = CALCULATE_PRT_V0(itex_style, 236);

		vertices[1].s = CALCULATE_PRT_U1(itex_style, 253);
		vertices[1].t = CALCULATE_PRT_V0(itex_style, 236);

		vertices[2].s = CALCULATE_PRT_U1(itex_style, 253);
		vertices[2].t = CALCULATE_PRT_V1(itex_style, 253);

		vertices[3].s = CALCULATE_PRT_U0(itex_style, 236);
		vertices[3].t = CALCULATE_PRT_V1(itex_style, 253);
	}

	doShadowSprite(alpha, _vertexBuffer);
}

void EntityShadows::doHighQualityShadow(const ObjectRef character) {

	Object *pchr = _currentModule->getObjectHandler().get(character);
	if(pchr->isBeingHeld()) return;

	// if the character is hidden, not drawn at all, so no shadow
	if (pchr->isHidden() || 0 == pchr->shadow_size) return;

	// no shadow if completely transparent
	float alpha = (255 == pchr->inst.light) ? pchr->inst.alpha  * INV_FF<float>() : (pchr->inst.alpha - pchr->inst.light) * INV_FF<float>();

	/// @test ZF@> The previous test didn't work, but this one does
	//if ( alpha * 255 < 1 ) return;
	if (pchr->inst.light <= INVISIBLE || pchr->inst.alpha <= INVISIBLE) return;

	// much reduced shadow if on a reflective tile
	auto mesh = _currentModule->getMeshPointer();
	if (0 != mesh->test_fx(pchr->getTile(), MAPFX_REFLECTIVE))
	{
		alpha *= 0.1f;
	}
	if (alpha < INV_FF<float>()) return;

	// Original points
	float level = pchr->getObjectPhysics().getGroundElevation() + SHADOWRAISE;
	float height = pchr->inst.getMatrix()(2, 3) - level;
	if (height < 0) height = 0;

	float size_umbra = 1.5f * (pchr->bump.size - height / 30.0f);
	float size_penumbra = 1.5f * (pchr->bump.size + height / 30.0f);

	alpha *= 0.3f;

	float   alpha_umbra, alpha_penumbra;
	alpha_umbra = alpha_penumbra = alpha;
	if (height > 0)
	{
		float factor_penumbra = (1.5f) * ((pchr->bump.size) / size_penumbra);
		float factor_umbra = (1.5f) * ((pchr->bump.size) / size_umbra);

		factor_umbra = std::max(1.0f, factor_umbra);
		factor_penumbra = std::max(1.0f, factor_penumbra);

		alpha_umbra *= 1.0f / factor_umbra / factor_umbra / 1.5f;
		alpha_penumbra *= 1.0f / factor_penumbra / factor_penumbra / 1.5f;

		alpha_umbra = constrain(alpha_umbra, 0.0f, 1.0f);
		alpha_penumbra = constrain(alpha_penumbra, 0.0f, 1.0f);
	}

	float x = pchr->inst.getMatrix()(0, 3);
	float y = pchr->inst.getMatrix()(1, 3);

	// Choose texture and matrix
	Ego::Renderer::get().getTextureUnit().setActivated(ParticleHandler::get().getLightParticleTexture().get());
	int itex_style = SPRITE_LIGHT; //ZF> Note: index 1 is for SPRITE_LIGHT

	// GOOD SHADOW
	{
		BufferScopedLock lock(_vertexBuffer);
		Vertex *vertices = lock.get<Vertex>();

		vertices[0].s = CALCULATE_PRT_U0(itex_style, 238);
		vertices[0].t = CALCULATE_PRT_V0(itex_style, 238);

		vertices[1].s = CALCULATE_PRT_U1(itex_style, 255);
		vertices[1].t = CALCULATE_PRT_V0(itex_style, 238);

		vertices[2].s = CALCULATE_PRT_U1(itex_style, 255);
		vertices[2].t = CALCULATE_PRT_V1(itex_style, 255);

		vertices[3].s = CALCULATE_PRT_U0(itex_style, 238);
		vertices[3].t = CALCULATE_PRT_V1(itex_style, 255);
	}
	if (size_penumbra > 0)
	{
		{
			BufferScopedLock lock(_vertexBuffer);
			Vertex *vertices = lock.get<Vertex>();
			
			vertices[0].x = x + size_penumbra;
			vertices[0].y = y - size_penumbra;
			vertices[0].z = level;

			vertices[1].x = x + size_penumbra;
			vertices[1].y = y + size_penumbra;
			vertices[1].z = level;

			vertices[2].x = x - size_penumbra;
			vertices[2].y = y + size_penumbra;
			vertices[2].z = level;

			vertices[3].x = x - size_penumbra;
			vertices[3].y = y - size_penumbra;
			vertices[3].z = level;
		}
		doShadowSprite(alpha_penumbra, _vertexBuffer);
	};

	if (size_umbra > 0)
	{
		{
			BufferScopedLock lock(_vertexBuffer);
			Vertex *vertices = lock.get<Vertex>();

			vertices[0].x = x + size_umbra;
			vertices[0].y = y - size_umbra;
			vertices[0].z = level + 0.1f;

			vertices[1].x = x + size_umbra;
			vertices[1].y = y + size_umbra;
			vertices[1].z = level + 0.1f;

			vertices[2].x = x - size_umbra;
			vertices[2].y = y + size_umbra;
			vertices[2].z = level + 0.1f;

			vertices[3].x = x - size_umbra;
			vertices[3].y = y - size_umbra;
			vertices[3].z = level + 0.1f;
		}

		doShadowSprite(alpha_umbra, _vertexBuffer);
	}
}

void EntityShadows::doShadowSprite(float intensity, VertexBuffer& vertexBuffer)
{
	if (intensity*255.0f < 1.0f) return;

	//Limit the intensity to a valid range
	intensity = Ego::Math::constrain(intensity, 0.0f, 1.0f);

	auto& renderer = Renderer::get();
	renderer.setColour(Colour4f(intensity, intensity, intensity, 1.0f));

	renderer.render(vertexBuffer, PrimitiveType::TriangleFan, 0, 4);
}

void Water::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
	if (!tl.getMesh())
	{
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "tile list not bound to a mesh");
	}
	ego_mesh_t& mesh = *tl.getMesh().get();
	// Restart the mesh texture code.
	TileRenderer::invalidate();

	// Bottom layer first.
	if (gfx.draw_water_1 && _currentModule->getWater()._layer_count > 1)
	{
		for (size_t i = 0; i < tl._water.size; ++i)
		{
			Internal::TileListV2::render_water_fan(mesh, tl._water.lst[i]._index, 1);
		}
	}

	// Top layer second.
	if (gfx.draw_water_0 && _currentModule->getWater()._layer_count > 0)
	{
		for (size_t i = 0; i < tl._water.size; ++i)
		{
			Internal::TileListV2::render_water_fan(mesh, tl._water.lst[i]._index, 0);
		}
	}

	// let the mesh texture code know that someone else is in control now
	TileRenderer::invalidate();
}

void EntityReflections::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
	auto mesh = tl.getMesh();
	if (!mesh) {
		Log::get().warn("%s:%d: tile list not attached to a mesh - skipping pass\n", __FILE__, __LINE__);
		return;
	}

	OpenGL::Utilities::isError();
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT);
	{
		auto& renderer = Renderer::get();
		// don't write into the depth buffer (disable glDepthMask for transparent objects)
		// turn off the depth mask by default. Can cause glitches if used improperly.
		renderer.setDepthWriteEnabled(false);

		// do not draw hidden surfaces
		renderer.setDepthTestEnabled(true);
		// surfaces must be closer to the camera to be drawn
		renderer.setDepthFunction(CompareFunction::LessOrEqual);

		for (size_t j = el.getSize(); j > 0; --j)
		{
			size_t i = j - 1;
			if (ParticleRef::Invalid == el.get(i).iprt && ObjectRef::Invalid != el.get(i).iobj)
			{
				const std::shared_ptr<Object> &object = _currentModule->getObjectHandler()[el.get(i).iobj];
				if(!object || object->isTerminated()) {
					continue;
				}

				// cull backward facing polygons
				// use couter-clockwise orientation to determine backfaces
				oglx_begin_culling(CullingMode::Back, MAP_REF_CULL);

				// allow transparent objects
				renderer.setBlendingEnabled(true);

				// use the alpha channel to modulate the transparency
				renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);
				Index1D itile = object->getTile();

				if (mesh->grid_is_valid(itile) && (0 != mesh->test_fx(itile, MAPFX_REFLECTIVE)))
				{
					renderer.setColour(Colour4f::white());

					MadRenderer::render_ref(camera, object);
				}
			}
			else if (ObjectRef::Invalid == el.get(i).iobj && ParticleRef::Invalid != el.get(i).iprt)
			{
				// draw draw front and back faces of polygons
				renderer.setCullingMode(CullingMode::None);

				// render_one_prt_ref() actually sets its own blend function, but just to be safe
				// allow transparent objects
				renderer.setBlendingEnabled(true);
				// set the default particle blending
				renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);
				ParticleRef iprt = el.get(i).iprt;
				Index1D itile = ParticleHandler::get()[iprt]->getTile();

				if (mesh->grid_is_valid(itile) && (0 != mesh->test_fx(itile, MAPFX_REFLECTIVE)))
				{
					renderer.setColour(Colour4f::white());
					render_one_prt_ref(iprt);
				}
			}
		}
	}
}

void SolidEntities::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	{
		// scan for solid objects
		for (size_t i = 0, n = el.getSize(); i < n; ++i)
		{
			auto& renderer = Renderer::get();
			// solid objects draw into the depth buffer for hidden surface removal
			renderer.setDepthWriteEnabled(true);

			// do not draw hidden surfaces
			renderer.setDepthTestEnabled(true);
			renderer.setDepthFunction(CompareFunction::Less);

			renderer.setAlphaTestEnabled(true);
			renderer.setAlphaFunction(CompareFunction::Greater, 0.0f);

			if (ParticleRef::Invalid == el.get(i).iprt && ObjectRef::Invalid != el.get(i).iobj)
			{
				MadRenderer::render_solid(camera, _currentModule->getObjectHandler()[el.get(i).iobj]);
			}
			else if (ObjectRef::Invalid == el.get(i).iobj && ParticleHandler::get()[el.get(i).iprt] != nullptr)
			{
				// draw draw front and back faces of polygons
				renderer.setCullingMode(CullingMode::None);

				render_one_prt_solid(el.get(i).iprt);
			}
		}
	}
}


void TransparentEntities::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
	{
		auto& renderer = Renderer::get();
		//---- set the the transparency parameters

		// don't write into the depth buffer (disable glDepthMask for transparent objects)
		renderer.setDepthWriteEnabled(false);

		// do not draw hidden surfaces
		renderer.setDepthTestEnabled(true);
		renderer.setDepthFunction(CompareFunction::LessOrEqual);

		// Now render all transparent and light objects
		for (size_t i = el.getSize(); i > 0; --i)
		{
			size_t j = i - 1;
			// A character.
			if (ParticleRef::Invalid == el.get(j).iprt && ObjectRef::Invalid != el.get(j).iobj)
			{
				MadRenderer::render_trans(camera, _currentModule->getObjectHandler()[el.get(j).iobj]);
			}
			// A particle.
			else if (ObjectRef::Invalid == el.get(j).iobj && ParticleRef::Invalid != el.get(j).iprt)
			{
				render_one_prt_trans(el.get(j).iprt);
			}
		}
	}
}

TransparentEntities g_transparentEntities;
SolidEntities g_solidEntities;
Reflective0 g_reflective0;
Reflective1 g_reflective1;
NonReflective g_nonReflective;
EntityShadows g_entityShadows;
Water g_water;
EntityReflections g_entityReflections;
Foreground g_foreground;
Background g_background;

}
}
}