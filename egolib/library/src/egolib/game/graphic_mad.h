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

/// @file egolib/game/graphic_mad.h

#pragma once

#include "egolib/game/egoboo.h"
#include "egolib/Entities/Forward.hpp"

class Camera;
namespace Ego::Graphics {
class ObjectGraphics; 
}

struct ObjectGraphicsRenderer {
	static gfx_rv render(Camera& cam, const std::shared_ptr<Object>& object, GLXvector4f tint, const BIT_FIELD bits);
    
	/// @brief Draw object reflected in the floor.
	static gfx_rv render_ref(Camera& cam, const std::shared_ptr<Object>& object);

	/// @brief Dispatch rendering of transparent objects to the correct function.
	/// @remark Does not handle reflections in the floor.
	static gfx_rv render_trans(Camera& cam, const std::shared_ptr<Object>& object);
	static gfx_rv render_solid(Camera& cam, const std::shared_ptr<Object>& object);

private:
	/// Draw model with environment mapping.
	static gfx_rv render_enviro(Camera& cam, const std::shared_ptr<Object>& object, GLXvector4f tint, const BIT_FIELD bits);
	/// Draw model with texturing.
	static gfx_rv render_tex(Camera& cam, const std::shared_ptr<Object>& object, GLXvector4f tint, const BIT_FIELD bits);

#if _DEBUG
    static void draw_chr_verts(const std::shared_ptr<Object>&pchr, int vrt_offset, int verts);
    static void _draw_one_grip_raw(Ego::Graphics::ObjectGraphics * pinst, int slot);
    static void draw_one_grip(Ego::Graphics::ObjectGraphics * pinst, int slot);
    //static void draw_chr_grips( Object * pchr );
    static void draw_chr_attached_grip(const std::shared_ptr<Object>& pchr);
    static void draw_chr_bbox(const std::shared_ptr<Object>& pchr);
#endif

};
