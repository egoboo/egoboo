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

/// @file game/Graphics/RenderPasses.hpp
/// @brief Implementation of Egoboo's render passes
/// @author Michael Heilmann

#pragma once

#include "game/Graphics/RenderPass.hpp"
#include "game/Graphics/Vertex.hpp"

namespace Ego {
namespace Graphics {
namespace RenderPasses {

namespace Internal {
void render_fans_by_list(const ego_mesh_t& mesh, const Ego::Graphics::renderlist_lst_t& rlst);
}

/// The first pass for reflective tiles
/// i.e. tiles which do reflect entities.
/// Ran before the pass rendering the reflections of entities.
struct Reflective0 : public RenderPass {
public:
	Reflective0()
		: RenderPass("reflective.0") {
	}
protected:
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
private:
	// Used if reflections are enabled.
	void doReflectionsEnabled(::Camera& cam, const TileList& tl, const EntityList& el);
	// Used if reflections are disabled.
	void doReflectionsDisabled(::Camera& cam, const TileList& tl, const EntityList& el);
	/// Common renderer configuration regardless of if reflections are enabled or disabled.
	void doCommon(::Camera& cam, const TileList& til, const EntityList& el);
};

/// The 2nd pass for reflective tiles
/// i.e. tiles which do reflect entities.
/// Ran after the pass rendering the reflections of entities.
struct Reflective1 : public RenderPass {
public:
	Reflective1()
		: RenderPass("reflective.1") {
	}
protected:
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
private:
	// Used if reflections are enabled.
	void doReflectionsEnabled(::Camera& cam, const TileList& tl, const EntityList& el);
	// Used if reflections are disabled.
	void doReflectionsDisabled(::Camera& cam, const TileList& tl, const EntityList& el);
	/// Common renderer configuration regardless of if reflections are enabled or disabled.
	void doCommon(::Camera& cam, const TileList& tl, const EntityList& el);
};

/// The render pass for the world background.
struct Background : public RenderPass {
public:
	Background()
		: RenderPass("background"),
		  _vertexBuffer(4, VertexFormatDescriptor::get<VertexFormat::P3FT2F>()) {
	}
protected:
	/// A vertex type used by this render pass.
	struct Vertex {
		float x, y, z;
		float s, t;
	};
	/// A vertex buffer used by this render pass.
	VertexBuffer _vertexBuffer;
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for the world foreground.
struct Foreground : public RenderPass {
public:
	Foreground()
		: RenderPass("foreground"),
		  _vertexBuffer(4, VertexFormatDescriptor::get<VertexFormat::P3FT2F>()) {
	}
protected:
	/// A vertex type used by this render pass.
	struct Vertex {
		float x, y, z;
		float s, t;
	};
	/// A vertex buffer used by this render pass.
	VertexBuffer _vertexBuffer;
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for non-reflective tiles
/// i.e. tiles which do not reflect entities.
struct NonReflective : public RenderPass {
public:
	NonReflective()
		: RenderPass("nonReflective") {
	}
protected:
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for water tiles.
struct Water : public RenderPass {
public:
	Water()
		: RenderPass("water") {
	}
protected:
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for solid entities.
struct SolidEntities : public RenderPass {
public:
	SolidEntities()
		: RenderPass("solidEntities") {
	}
protected:
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for transparent entities.
struct TransparentEntities : public RenderPass {
public:
	TransparentEntities()
		: RenderPass("transparentEntities") {
	}
protected:
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for entity reflections.
struct EntityReflections : public RenderPass {
public:
	EntityReflections()
		: RenderPass("entityReflections") {
	}
protected:
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for entity shadows.
struct EntityShadows : public RenderPass {
public:
	EntityShadows()
		: RenderPass("entityShadows"),
		  _vertexBuffer(4, VertexFormatDescriptor::get<VertexFormat::P3FT2F>()) {
	}
protected:
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
private:
	/// A vertex type used by this render pass.
	struct Vertex {
		float x, y, z;
		float s, t;
	};
	/// A vertex buffer used by this render pass.
	VertexBuffer _vertexBuffer;
	// Used if low-quality shadows are enabled.
	void doLowQualityShadow(const CHR_REF character);
	// Used if high-quality shadows are enabled.
	void doHighQualityShadow(const CHR_REF character);
	// Used by all shadow qualities.
	void doShadowSprite(float intensity, VertexBuffer& vertexBuffer);
};

extern TransparentEntities g_transparentEntities;
extern SolidEntities g_solidEntities;
extern Reflective0 g_reflective0;
extern Reflective1 g_reflective1;
extern NonReflective g_nonReflective;
extern EntityShadows g_entityShadows;
extern Water g_water;
extern EntityReflections g_entityReflections;
extern Foreground g_foreground;
extern Background g_background;

}
}
}
