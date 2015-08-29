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
#include "game/bsp.h"
#include "game/player.h"
#include "game/collision.h"
#include "egolib/Script/script.h"
#include "game/input.h"
#include "game/script_compile.h"
#include "game/game.h"
#include "game/lighting.h"
#include "game/egoboo.h"
#include "game/char.h"
#include "game/mesh.h"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Module/Module.hpp"

namespace Ego {
namespace Graphics {
namespace RenderPasses {

void Background::doRun(::Camera& cam, const TileList& tl, const EntityList& el) {
	if (!gfx.draw_background) {
		return;
	}

	grid_mem_t *pgmem = &(_currentModule->getMeshPointer()->gmem);

	// which layer
	water_instance_layer_t *ilayer = water._layers + 0;

	GLvertex vtlist[4];

	// the "official" camera height
	float z0 = 1500;

	// clip the waterlayer uv offset
	ilayer->_tx[XX] = ilayer->_tx[XX] - (float)std::floor(ilayer->_tx[XX]);
	ilayer->_tx[YY] = ilayer->_tx[YY] - (float)std::floor(ilayer->_tx[YY]);

	float xmag, Cx_0, Cx_1;
	float ymag, Cy_0, Cy_1;

	// determine the constants for the x-coordinate
	xmag = water._backgroundrepeat / 4 / (1.0f + z0 * ilayer->_dist[XX]) / GRID_FSIZE;
	Cx_0 = xmag * (1.0f + cam.getPosition()[kZ] * ilayer->_dist[XX]);
	Cx_1 = -xmag * (1.0f + (cam.getPosition()[kZ] - z0) * ilayer->_dist[XX]);

	// determine the constants for the y-coordinate
	ymag = water._backgroundrepeat / 4 / (1.0f + z0 * ilayer->_dist[YY]) / GRID_FSIZE;
	Cy_0 = ymag * (1.0f + cam.getPosition()[kZ] * ilayer->_dist[YY]);
	Cy_1 = -ymag * (1.0f + (cam.getPosition()[kZ] - z0) * ilayer->_dist[YY]);

	float Qx, Qy;

	// Figure out the coordinates of its corners
	Qx = -pgmem->edge_x;
	Qy = -pgmem->edge_y;
	vtlist[0].pos[XX] = Qx;
	vtlist[0].pos[YY] = Qy;
	vtlist[0].pos[ZZ] = cam.getPosition()[kZ] - z0;
	vtlist[0].tex[SS] = Cx_0 * Qx + Cx_1 * cam.getPosition()[kX] + ilayer->_tx[XX];
	vtlist[0].tex[TT] = Cy_0 * Qy + Cy_1 * cam.getPosition()[kY] + ilayer->_tx[YY];

	Qx = 2 * pgmem->edge_x;
	Qy = -pgmem->edge_y;
	vtlist[1].pos[XX] = Qx;
	vtlist[1].pos[YY] = Qy;
	vtlist[1].pos[ZZ] = cam.getPosition()[kZ] - z0;
	vtlist[1].tex[SS] = Cx_0 * Qx + Cx_1 * cam.getPosition()[kX] + ilayer->_tx[XX];
	vtlist[1].tex[TT] = Cy_0 * Qy + Cy_1 * cam.getPosition()[kY] + ilayer->_tx[YY];

	Qx = 2 * pgmem->edge_x;
	Qy = 2 * pgmem->edge_y;
	vtlist[2].pos[XX] = Qx;
	vtlist[2].pos[YY] = Qy;
	vtlist[2].pos[ZZ] = cam.getPosition()[kZ] - z0;
	vtlist[2].tex[SS] = Cx_0 * Qx + Cx_1 * cam.getPosition()[kX] + ilayer->_tx[XX];
	vtlist[2].tex[TT] = Cy_0 * Qy + Cy_1 * cam.getPosition()[kY] + ilayer->_tx[YY];

	Qx = -pgmem->edge_x;
	Qy = 2 * pgmem->edge_y;
	vtlist[3].pos[XX] = Qx;
	vtlist[3].pos[YY] = Qy;
	vtlist[3].pos[ZZ] = cam.getPosition()[kZ] - z0;
	vtlist[3].tex[SS] = Cx_0 * Qx + Cx_1 * cam.getPosition()[kX] + ilayer->_tx[XX];
	vtlist[3].tex[TT] = Cy_0 * Qy + Cy_1 * cam.getPosition()[kY] + ilayer->_tx[YY];

	float light = water._light ? 1.0f : 0.0f;
	float alpha = ilayer->_alpha * INV_FF;

	float intens = 1.0f;

	if (gfx.usefaredge)
	{
		float fcos;

		intens = light_a * ilayer->_light_add;

		fcos = light_nrm[kZ];
		if (fcos > 0.0f)
		{
			intens += fcos * fcos * light_d * ilayer->_light_dir;
		}

		intens = CLIP(intens, 0.0f, 1.0f);
	}

	oglx_texture_t::bind(_currentModule->getWaterTexture(1));

	ATTRIB_PUSH(__FUNCTION__, GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
	{
		auto& renderer = Renderer::get();
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

				GL_DEBUG(glBegin)(GL_TRIANGLE_FAN);
				{
					for (size_t i = 0; i < 4; i++)
					{
						GL_DEBUG(glTexCoord2fv)(vtlist[i].tex);
						GL_DEBUG(glVertex3fv)(vtlist[i].pos);
					}
				}
				GL_DEBUG_END();
			}
			ATTRIB_POP(__FUNCTION__);
		}

		if (light > 0.0f)
		{
			ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
			{
				renderer.setBlendingEnabled(false);

				renderer.setColour(Colour4f(light, light, light, 1.0f));

				GL_DEBUG(glBegin)(GL_TRIANGLE_FAN);
				{
					for (size_t i = 0; i < 4; i++)
					{
						GL_DEBUG(glTexCoord2fv)(vtlist[i].tex);
						GL_DEBUG(glVertex3fv)(vtlist[i].pos);
					}
				}
				GL_DEBUG_END();
			}
			ATTRIB_POP(__FUNCTION__);
		}
	}
	ATTRIB_POP(__FUNCTION__);
}

void Foreground::doRun(::Camera& cam, const TileList& tl, const EntityList& el) {
	if (!gfx.draw_overlay) {
		return;
	}

	water_instance_layer_t *ilayer = water._layers + 1;

	Vector3f vforw_wind(ilayer->_tx_add[XX], ilayer->_tx_add[YY], 0.0f);
	vforw_wind.normalize();

	Vector3f vforw_cam;
	mat_getCamForward(cam.getViewMatrix(), vforw_cam);
	vforw_cam.normalize();

	// make the texture begin to disappear if you are not looking straight down
	float ftmp = vforw_wind.dot(vforw_cam);

	float alpha = (1.0f - ftmp * ftmp) * (ilayer->_alpha * INV_FF);

	if (alpha != 0.0f)
	{
		GLvertex vtlist[4];

		// Figure out the screen coordinates of its corners
		float x = sdl_scr.x << 6;
		float y = sdl_scr.y << 6;
		float z = 0;
		float size = x + y + 1;
		TURN_T default_turn = (3 * 2047) & TRIG_TABLE_MASK;
		float sinsize = turntosin[default_turn] * size;
		float cossize = turntocos[default_turn] * size;
		float loc_foregroundrepeat = water._foregroundrepeat * std::min(x / sdl_scr.x, y / sdl_scr.x);

		vtlist[0].pos[XX] = x + cossize;
		vtlist[0].pos[YY] = y - sinsize;
		vtlist[0].pos[ZZ] = z;
		vtlist[0].tex[SS] = ilayer->_tx[XX];
		vtlist[0].tex[TT] = ilayer->_tx[YY];

		vtlist[1].pos[XX] = x + sinsize;
		vtlist[1].pos[YY] = y + cossize;
		vtlist[1].pos[ZZ] = z;
		vtlist[1].tex[SS] = ilayer->_tx[XX] + loc_foregroundrepeat;
		vtlist[1].tex[TT] = ilayer->_tx[YY];

		vtlist[2].pos[XX] = x - cossize;
		vtlist[2].pos[YY] = y + sinsize;
		vtlist[2].pos[ZZ] = z;
		vtlist[2].tex[SS] = ilayer->_tx[SS] + loc_foregroundrepeat;
		vtlist[2].tex[TT] = ilayer->_tx[TT] + loc_foregroundrepeat;

		vtlist[3].pos[XX] = x - sinsize;
		vtlist[3].pos[YY] = y - cossize;
		vtlist[3].pos[ZZ] = z;
		vtlist[3].tex[SS] = ilayer->_tx[SS];
		vtlist[3].tex[TT] = ilayer->_tx[TT] + loc_foregroundrepeat;

		oglx_texture_t::bind(_currentModule->getWaterTexture(1));

		ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT | GL_HINT_BIT);
		{
			// make sure that the texture is as smooth as possible
			GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT, GL_NICEST);          // GL_HINT_BIT

			auto& renderer = Renderer::get();

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

			oglx_texture_t::bind(_currentModule->getWaterTexture(1));

			renderer.setColour(Colour4f(1.0f, 1.0f, 1.0f, 1.0f - std::abs(alpha)));
			GL_DEBUG(glBegin)(GL_TRIANGLE_FAN);
			for (size_t i = 0; i < 4; i++)
			{
				GL_DEBUG(glTexCoord2fv)(vtlist[i].tex);
				GL_DEBUG(glVertex3fv)(vtlist[i].pos);
			}
			GL_DEBUG_END();
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
		render_fans_by_list(tl._mesh, &(tl._reflective));
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
		render_fans_by_list(tl._mesh, &(tl._reflective));
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
		render_fans_by_list(tl._mesh, &(tl._reflective));
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
		render_fans_by_list(tl._mesh, &(tl._nonReflective));
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
			CHR_REF ichr = el.get(i).ichr;
			if (!VALID_CHR_RANGE(ichr)) continue;
			if (0 == _currentModule->getObjectHandler().get(ichr)->shadow_size) continue;
			doHighQualityShadow(ichr);
			count++;
		}
	} else {
		// Render low-quality shadows.
		for (size_t i = 0; i < el.getSize(); ++i) {
			CHR_REF ichr = el.get(i).ichr;
			if (!VALID_CHR_RANGE(ichr)) continue;
			if (0 == _currentModule->getObjectHandler().get(ichr)->shadow_size) continue;
			doLowQualityShadow(ichr);
			count++;
		}
	}
}

void EntityShadows::doLowQualityShadow(const CHR_REF character) {
	Object *pchr = _currentModule->getObjectHandler().get(character);
	if(pchr->isBeingHeld()) return;

	// If the object is hidden it is not drawn at all, so it has no shadow.
	// If the object's shadow size is qa 0, then it has no shadow.
	if (pchr->is_hidden || 0 == pchr->shadow_size)
	{
		return;
	}
	// No shadow if off the mesh.
	ego_tile_info_t *ptile = _currentModule->getMeshPointer()->get_ptile(pchr->getTile());
	if (!ptile)
	{
		return;
	}
	// No shadow if invalid tile.
	if (TILE_IS_FANOFF(ptile))
	{
		return;
	}

	// No shadow if completely transparent or completely glowing.
	float alpha = (255 == pchr->inst.light) ? pchr->inst.alpha  * INV_FF : (pchr->inst.alpha - pchr->inst.light) * INV_FF;

	/// @test ZF@> previous test didn't work, but this one does
	//if ( alpha * 255 < 1 ) return;
	if (pchr->inst.light <= INVISIBLE || pchr->inst.alpha <= INVISIBLE) return;

	// much reduced shadow if on a reflective tile
	if (0 != ego_mesh_t::test_fx(_currentModule->getMeshPointer(), pchr->getTile(), MAPFX_REFLECTIVE))
	{
		alpha *= 0.1f;
	}
	if (alpha < INV_FF) return;

	// Original points
	float level = pchr->enviro.floor_level + SHADOWRAISE;
	float height = pchr->inst.matrix(2, 3) - level;
	float height_factor = 1.0f - height / (pchr->shadow_size * 5.0f);
	if (height_factor <= 0.0f) return;

	// how much transparency from height
	alpha *= height_factor * 0.5f + 0.25f;
	if (alpha < INV_FF) return;

	float x = pchr->inst.matrix(0, 3); ///< @todo MH: This should be the x/y position of the model.
	float y = pchr->inst.matrix(1, 3); ///<           Use a more self-descriptive method to describe this.

	float size = pchr->shadow_size * height_factor;

	GLvertex v[4];

	v[0].pos[XX] = (float)x + size;
	v[0].pos[YY] = (float)y - size;
	v[0].pos[ZZ] = (float)level;

	v[1].pos[XX] = (float)x + size;
	v[1].pos[YY] = (float)y + size;
	v[1].pos[ZZ] = (float)level;

	v[2].pos[XX] = (float)x - size;
	v[2].pos[YY] = (float)y + size;
	v[2].pos[ZZ] = (float)level;

	v[3].pos[XX] = (float)x - size;
	v[3].pos[YY] = (float)y - size;
	v[3].pos[ZZ] = (float)level;

	// Choose texture and matrix
	oglx_texture_t::bind(ParticleHandler::get().getLightParticleTexture());
	int itex_style = SPRITE_LIGHT; //ZF> Note: index 1 is for SPRITE_LIGHT

	v[0].tex[SS] = CALCULATE_PRT_U0(itex_style, 236);
	v[0].tex[TT] = CALCULATE_PRT_V0(itex_style, 236);

	v[1].tex[SS] = CALCULATE_PRT_U1(itex_style, 253);
	v[1].tex[TT] = CALCULATE_PRT_V0(itex_style, 236);

	v[2].tex[SS] = CALCULATE_PRT_U1(itex_style, 253);
	v[2].tex[TT] = CALCULATE_PRT_V1(itex_style, 253);

	v[3].tex[SS] = CALCULATE_PRT_U0(itex_style, 236);
	v[3].tex[TT] = CALCULATE_PRT_V1(itex_style, 253);

	doShadowSprite(alpha, v);
}

void EntityShadows::doHighQualityShadow(const CHR_REF character) {

	Object *pchr = _currentModule->getObjectHandler().get(character);
	if(pchr->isBeingHeld()) return;

	// if the character is hidden, not drawn at all, so no shadow
	if (pchr->is_hidden || 0 == pchr->shadow_size) return;

	// no shadow if off the mesh
	ego_tile_info_t *ptile = _currentModule->getMeshPointer()->get_ptile(pchr->getTile());
	if (NULL == ptile) return;

	// no shadow if invalid tile image
	if (TILE_IS_FANOFF(ptile)) return;

	// no shadow if completely transparent
	float alpha = (255 == pchr->inst.light) ? pchr->inst.alpha  * INV_FF : (pchr->inst.alpha - pchr->inst.light) * INV_FF;

	/// @test ZF@> The previous test didn't work, but this one does
	//if ( alpha * 255 < 1 ) return;
	if (pchr->inst.light <= INVISIBLE || pchr->inst.alpha <= INVISIBLE) return;

	// much reduced shadow if on a reflective tile
	if (0 != ego_mesh_t::test_fx(_currentModule->getMeshPointer(), pchr->getTile(), MAPFX_REFLECTIVE))
	{
		alpha *= 0.1f;
	}
	if (alpha < INV_FF) return;

	// Original points
	float level = pchr->enviro.floor_level + SHADOWRAISE;
	float height = pchr->inst.matrix(2, 3) - level;
	if (height < 0) height = 0;

	float size_umbra = 1.5f * (pchr->bump.size - height / 30.0f);
	float size_penumbra = 1.5f * (pchr->bump.size + height / 30.0f);

	alpha *= 0.3f;
	GLvertex v[4];

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

		alpha_umbra = CLIP(alpha_umbra, 0.0f, 1.0f);
		alpha_penumbra = CLIP(alpha_penumbra, 0.0f, 1.0f);
	}

	float x = pchr->inst.matrix(0, 3);
	float y = pchr->inst.matrix(1, 3);

	// Choose texture and matrix
	oglx_texture_t::bind(ParticleHandler::get().getLightParticleTexture());
	int itex_style = SPRITE_LIGHT; //ZF> Note: index 1 is for SPRITE_LIGHT

	// GOOD SHADOW
	v[0].tex[SS] = CALCULATE_PRT_U0(itex_style, 238);
	v[0].tex[TT] = CALCULATE_PRT_V0(itex_style, 238);

	v[1].tex[SS] = CALCULATE_PRT_U1(itex_style, 255);
	v[1].tex[TT] = CALCULATE_PRT_V0(itex_style, 238);

	v[2].tex[SS] = CALCULATE_PRT_U1(itex_style, 255);
	v[2].tex[TT] = CALCULATE_PRT_V1(itex_style, 255);

	v[3].tex[SS] = CALCULATE_PRT_U0(itex_style, 238);
	v[3].tex[TT] = CALCULATE_PRT_V1(itex_style, 255);

	if (size_penumbra > 0)
	{
		v[0].pos[XX] = x + size_penumbra;
		v[0].pos[YY] = y - size_penumbra;
		v[0].pos[ZZ] = level;

		v[1].pos[XX] = x + size_penumbra;
		v[1].pos[YY] = y + size_penumbra;
		v[1].pos[ZZ] = level;

		v[2].pos[XX] = x - size_penumbra;
		v[2].pos[YY] = y + size_penumbra;
		v[2].pos[ZZ] = level;

		v[3].pos[XX] = x - size_penumbra;
		v[3].pos[YY] = y - size_penumbra;
		v[3].pos[ZZ] = level;

		doShadowSprite(alpha_penumbra, v);
	};

	if (size_umbra > 0)
	{
		v[0].pos[XX] = x + size_umbra;
		v[0].pos[YY] = y - size_umbra;
		v[0].pos[ZZ] = level + 0.1f;

		v[1].pos[XX] = x + size_umbra;
		v[1].pos[YY] = y + size_umbra;
		v[1].pos[ZZ] = level + 0.1f;

		v[2].pos[XX] = x - size_umbra;
		v[2].pos[YY] = y + size_umbra;
		v[2].pos[ZZ] = level + 0.1f;

		v[3].pos[XX] = x - size_umbra;
		v[3].pos[YY] = y - size_umbra;
		v[3].pos[ZZ] = level + 0.1f;

		doShadowSprite(alpha_umbra, v);
	}
}

void EntityShadows::doShadowSprite(float intensity, GLvertex v[])
{
	if (intensity*255.0f < 1.0f) return;

	Ego::Renderer::get().setColour(Colour4f(intensity, intensity, intensity, 1.0f));

	GL_DEBUG(glBegin)(GL_TRIANGLE_FAN);
	{
		for (size_t i = 0; i < 4; i++)
		{
			GL_DEBUG(glTexCoord2fv)(v[i].tex);
			GL_DEBUG(glVertex3fv)(v[i].pos);
		}
	}
	GL_DEBUG_END();
}

void Water::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
	// Restart the mesh texture code.
	mesh_texture_invalidate();

	// Bottom layer first.
	if (gfx.draw_water_1)
	{
		for (size_t i = 0; i < tl._water.size; ++i)
		{
			render_water_fan(tl._mesh, tl._water.lst[i].index, 1);
		}
	}

	// Top layer second.
	if (gfx.draw_water_0)
	{
		for (size_t i = 0; i < tl._water.size; ++i)
		{
			render_water_fan(tl._mesh, tl._water.lst[i].index, 0);
		}
	}

	// let the mesh texture code know that someone else is in control now
	mesh_texture_invalidate();
}

void EntityReflections::doRun(::Camera& camera, const TileList& tl, const EntityList& el) {
	ego_mesh_t *mesh = tl.getMesh();
	if (!mesh) {
		log_warning("%s:%d: tile list not attached to a mesh - skipping pass\n", __FILE__, __LINE__);
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
			if (INVALID_PRT_REF == el.get(i).iprt && INVALID_CHR_REF != el.get(i).ichr)
			{
				// cull backward facing polygons
				// use couter-clockwise orientation to determine backfaces
				oglx_begin_culling(CullingMode::Back, MAP_REF_CULL);

				// allow transparent objects
				renderer.setBlendingEnabled(true);
				// use the alpha channel to modulate the transparency
				renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);
				CHR_REF ichr = el.get(i).ichr;
				TileIndex itile = _currentModule->getObjectHandler().get(ichr)->getTile();

				if (ego_mesh_t::grid_is_valid(mesh, itile) && (0 != ego_mesh_t::test_fx(mesh, itile, MAPFX_REFLECTIVE)))
				{
					renderer.setColour(Colour4f::white());

					render_one_mad_ref(camera, ichr);
				}
			}
			else if (INVALID_CHR_REF == el.get(i).ichr && INVALID_PRT_REF != el.get(i).iprt)
			{
				// draw draw front and back faces of polygons
				renderer.setCullingMode(CullingMode::None);

				// render_one_prt_ref() actually sets its own blend function, but just to be safe
				// allow transparent objects
				renderer.setBlendingEnabled(true);
				// set the default particle blending
				renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);
				PRT_REF iprt = el.get(i).iprt;
				TileIndex itile = ParticleHandler::get()[iprt]->getTile();

				if (ego_mesh_t::grid_is_valid(mesh, itile) && (0 != ego_mesh_t::test_fx(mesh, itile, MAPFX_REFLECTIVE)))
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

			if (INVALID_PRT_REF == el.get(i).iprt && VALID_CHR_RANGE(el.get(i).ichr))
			{
				render_one_mad_solid(camera, el.get(i).ichr);
			}
			else if (INVALID_CHR_REF == el.get(i).ichr && ParticleHandler::get()[el.get(i).iprt] != nullptr)
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
			if (INVALID_PRT_REF == el.get(j).iprt && INVALID_CHR_REF != el.get(j).ichr)
			{
				render_one_mad_trans(camera, el.get(j).ichr);
			}
			// A particle.
			else if (INVALID_CHR_REF == el.get(j).ichr && INVALID_PRT_REF != el.get(j).iprt)
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