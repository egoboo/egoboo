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
#include "game/renderer_2d.h"
#include "game/renderer_3d.h"
#include "game/Logic/Player.hpp"
#include "egolib/Script/script.h"
#include "game/input.h"
#include "game/script_compile.h"
#include "game/game.h"
#include "game/lighting.h"
#include "game/egoboo.h"
#include "game/char.h"
#include "game/mesh.h"
#include "game/Graphics/CameraSystem.hpp"
#include "egolib/FileFormats/Globals.hpp"
#include "game/Module/Module.hpp"

namespace Ego {
namespace Graphics {
namespace RenderPasses {

namespace Internal {

struct by_element2_t {
	float _distance;
	Index1D _tileIndex;
	uint32_t _textureIndex;
	by_element2_t()
		: by_element2_t(std::numeric_limits<float>::infinity(), Index1D::Invalid, std::numeric_limits<uint32_t>::max()) {
	}
	by_element2_t(float distance, const Index1D& tileIndex, uint32_t textureIndex)
		: _distance(distance), _tileIndex(tileIndex), _textureIndex(textureIndex) {
	}
	by_element2_t(const by_element2_t& other)
		: by_element2_t(other._distance, other._tileIndex, other._textureIndex) {
	}
	by_element2_t& operator=(const by_element2_t& other) {
		_distance = other._distance;
		_tileIndex = other._tileIndex;
		_textureIndex = other._textureIndex;
		return *this;
	}
	static bool compare(const by_element2_t& x, const by_element2_t& y) {
		int result = (int)x._textureIndex - (int)y._textureIndex;
		if (result < 0) {
			return true;
		} else if (result > 0) {
			return false;
		} else {
			float result = x._distance - y._distance;
			if (result < 0.0f) {
				return true;
			} else {
				return false;
			}
		}
	}
};

void render_fans_by_list(const ego_mesh_t& mesh, const Ego::Graphics::renderlist_lst_t& rlst)
{
	size_t tcnt = mesh._tmem.getInfo().getTileCount();

	if (0 == rlst.size) {
		return;
	}

	// insert the rlst values into lst_vals
	std::vector<by_element2_t> lst_vals(rlst.size);
	for (size_t i = 0; i < rlst.size; ++i)
	{
		lst_vals[i]._tileIndex = rlst.lst[i]._index;
		lst_vals[i]._distance = rlst.lst[i]._distance;

		if (rlst.lst[i]._index >= tcnt)
		{
			lst_vals[i]._textureIndex = std::numeric_limits<uint32_t>::max();
		}
		else
		{
			const ego_tile_info_t& tile = mesh._tmem.get(rlst.lst[i]._index);

			int img = TILE_GET_LOWER_BITS(tile._img);
			if (tile._type >= tile_dict.offset)
			{
				img += Ego::Graphics::MESH_IMG_COUNT;
			}

			lst_vals[i]._textureIndex = img;
		}
	}

	std::sort(lst_vals.begin(), lst_vals.end(), by_element2_t::compare);

	// restart the mesh texture code
	TileRenderer::invalidate();

	for (size_t i = 0; i < rlst.size; ++i)
	{
		Index1D tmp_itile = lst_vals[i]._tileIndex;

		gfx_rv render_rv = render_fan(mesh, tmp_itile);
		if (egoboo_config_t::get().debug_developerMode_enable.getValue() && gfx_error == render_rv)
		{
			Log::get().warn("%s - error rendering tile %d.\n", __FUNCTION__, tmp_itile.getI());
		}
	}

	// let the mesh texture code know that someone else is in control now
	TileRenderer::invalidate();
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
		VertexBufferScopedLock lock(_vertexBuffer);
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

	ATTRIB_PUSH(__FUNCTION__, GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
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

		if (alpha > 0.0f)
		{
			ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
			{
				renderer.setColour(Colour4f(intens, intens, intens, alpha));

				if (alpha >= 1.0f)
				{
					renderer.setBlendingEnabled(false);
				}
				else
				{
					renderer.setBlendingEnabled(true);
					renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);
				}

				renderer.render(_vertexBuffer, PrimitiveType::TriangleFan, 0, 4);
			}
			ATTRIB_POP(__FUNCTION__);
		}

		if (light > 0.0f)
		{
			ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
			{
				renderer.setBlendingEnabled(false);

				renderer.setColour(Colour4f(light, light, light, 1.0f));

				renderer.render(_vertexBuffer, PrimitiveType::TriangleFan, 0, 4);
			}
			ATTRIB_POP(__FUNCTION__);
		}
	}
	ATTRIB_POP(__FUNCTION__);
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
		float x = sdl_scr.x << 6;
		float y = sdl_scr.y << 6;
		float z = 0;
		float size = x + y + 1;
		TURN_T default_turn = (3 * 2047) & TRIG_TABLE_MASK;
		float sinsize = turntosin[default_turn] * size;
		float cossize = turntocos[default_turn] * size;
		float loc_foregroundrepeat = _currentModule->getWater()._foregroundrepeat * std::min(x / sdl_scr.x, y / sdl_scr.x);

		{
			VertexBufferScopedLock lock(_vertexBuffer);
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

		ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT | GL_HINT_BIT);
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
		ATTRIB_POP(__FUNCTION__);
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

	ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
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
		if (tl._mesh) {
			Internal::render_fans_by_list(*tl._mesh, tl._reflective);
		}
	}
	ATTRIB_POP(__FUNCTION__);
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
	ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	{
		doCommon(camera, tl, el);
		// Enable blending.
		auto& renderer = Renderer::get();
		// Enable blending.
		renderer.setBlendingEnabled(true);
		renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::One);

		// reduce texture hashing by loading up each texture only once
		if (tl._mesh) {
			Internal::render_fans_by_list(*tl._mesh, tl._reflective);
		}
	}
	ATTRIB_POP(__FUNCTION__);
}

/**
 * @todo We could re-use Ego::Graphics::RenderPasses::NonReflective for that.
 *       However, things will evolve soon enough ... Egoboo's whole way of
 *       rendering needs to be improved (at least we've improved structure &
 *       terminology for now).
 */
void Reflective1::doReflectionsDisabled(::Camera& camera, const TileList& tl, const EntityList& el) {
	ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
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
		if (tl._mesh) {
			Internal::render_fans_by_list(*tl._mesh, tl._reflective);
		}
	}
	ATTRIB_POP(__FUNCTION__);
}

void NonReflective::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
	ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
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
		if (tl._mesh) {
			Internal::render_fans_by_list(*tl._mesh, tl._nonReflective);
		}
	}
	ATTRIB_POP(__FUNCTION__);
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
	float height = pchr->inst.matrix(2, 3) - level;
	float height_factor = 1.0f - height / (pchr->shadow_size * 5.0f);
	if (height_factor <= 0.0f) return;

	// how much transparency from height
	alpha *= height_factor * 0.5f + 0.25f;
	if (alpha < INV_FF<float>()) return;

	float x = pchr->inst.matrix(0, 3); ///< @todo MH: This should be the x/y position of the model.
	float y = pchr->inst.matrix(1, 3); ///<           Use a more self-descriptive method to describe this.

	float size = pchr->shadow_size * height_factor;

	{
		VertexBufferScopedLock lock(_vertexBuffer);
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
		VertexBufferScopedLock lock(_vertexBuffer);
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
	float height = pchr->inst.matrix(2, 3) - level;
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

	float x = pchr->inst.matrix(0, 3);
	float y = pchr->inst.matrix(1, 3);

	// Choose texture and matrix
	Ego::Renderer::get().getTextureUnit().setActivated(ParticleHandler::get().getLightParticleTexture().get());
	int itex_style = SPRITE_LIGHT; //ZF> Note: index 1 is for SPRITE_LIGHT

	// GOOD SHADOW
	{
		VertexBufferScopedLock lock(_vertexBuffer);
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
			VertexBufferScopedLock lock(_vertexBuffer);
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
			VertexBufferScopedLock lock(_vertexBuffer);
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
			render_water_fan(mesh, tl._water.lst[i]._index, 1);
		}
	}

	// Top layer second.
	if (gfx.draw_water_0 && _currentModule->getWater()._layer_count > 0)
	{
		for (size_t i = 0; i < tl._water.size; ++i)
		{
			render_water_fan(mesh, tl._water.lst[i]._index, 0);
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
	ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT);
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
				// cull backward facing polygons
				// use couter-clockwise orientation to determine backfaces
				oglx_begin_culling(CullingMode::Back, MAP_REF_CULL);

				// allow transparent objects
				renderer.setBlendingEnabled(true);
				// use the alpha channel to modulate the transparency
				renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);
				ObjectRef ichr = el.get(i).iobj;
				Index1D itile = _currentModule->getObjectHandler().get(ichr)->getTile();

				if (mesh->grid_is_valid(itile) && (0 != mesh->test_fx(itile, MAPFX_REFLECTIVE)))
				{
					renderer.setColour(Colour4f::white());

					MadRenderer::render_ref(camera, ichr);
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
	ATTRIB_POP(__FUNCTION__);
}

void SolidEntities::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
	ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT)
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
				MadRenderer::render_solid(camera, el.get(i).iobj);
			}
			else if (ObjectRef::Invalid == el.get(i).iobj && ParticleHandler::get()[el.get(i).iprt] != nullptr)
			{
				// draw draw front and back faces of polygons
				renderer.setCullingMode(CullingMode::None);

				render_one_prt_solid(el.get(i).iprt);
			}
		}
	}
	ATTRIB_POP(__FUNCTION__);
}


void TransparentEntities::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
	ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT)
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
				MadRenderer::render_trans(camera, el.get(j).iobj);
			}
			// A particle.
			else if (ObjectRef::Invalid == el.get(j).iobj && ParticleRef::Invalid != el.get(j).iprt)
			{
				render_one_prt_trans(el.get(j).iprt);
			}
		}
	}
	ATTRIB_POP(__FUNCTION__);
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