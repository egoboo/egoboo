#pragma once

#include "game/Graphics/RenderPass.hpp"
#include "game/Graphics/Vertex.hpp"

namespace Ego {
namespace Graphics {
namespace RenderPasses {

/// The first pass for reflective tiles
/// i.e. tiles which do reflect entities.
/// Ran before the pass rendering the reflections of entities.
struct Reflective0 : public Graphics::RenderPass {
public:
	Reflective0()
		: Graphics::RenderPass("reflective.0") {
	}
protected:
	void doRun(Camera& cam, const Ego::Graphics::TileList& tl, const EntityList& el) override;
private:
	// Used if reflections are enabled.
	void doReflectionsEnabled(Camera& cam, const TileList& tl, const EntityList& el);
	// Used if reflections are disabled.
	void doReflectionsDisabled(Camera& cam, const TileList& tl, const EntityList& el);
	/// Common renderer configuration regardless of if reflections are enabled or disabled.
	void doCommon(Camera& cam, const TileList& til, const EntityList& el);
};

/// The 2nd pass for reflective tiles
/// i.e. tiles which do reflect entities.
/// Ran after the pass rendering the reflections of entities.
struct Reflective1 : public Graphics::RenderPass {
public:
	Reflective1()
		: Graphics::RenderPass("reflective.1") {
	}
protected:
	void doRun(Camera& cam, const TileList& tl, const EntityList& el) override;
private:
	// Used if reflections are enabled.
	void doReflectionsEnabled(Camera& cam, const TileList& tl, const EntityList& el);
	// Used if reflections are disabled.
	void doReflectionsDisabled(Camera& cam, const TileList& tl, const EntityList& el);
	/// Common renderer configuration regardless of if reflections are enabled or disabled.
	void doCommon(Camera& cam, const TileList& tl, const EntityList& el);
};

/// The render pass for the world background.
struct Background : public Graphics::RenderPass {
public:
	Background()
		: Graphics::RenderPass("background") {
	}
protected:
	void doRun(Camera& cam, const Graphics::TileList& tl, const EntityList& el) override;
};

/// The render pass for the world foreground.
struct Foreground : public Graphics::RenderPass {
public:
	Foreground()
		: Graphics::RenderPass("foreground") {
	}
protected:
	void doRun(Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for non-reflective tiles
/// i.e. tiles which do not reflect entities.
struct NonReflective : public Graphics::RenderPass {
public:
	NonReflective()
		: Graphics::RenderPass("nonReflective") {
	}
protected:
	void doRun(Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for water tiles.
struct Water : public Graphics::RenderPass {
public:
	Water()
		: Graphics::RenderPass("water") {
	}
protected:
	void doRun(Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for solid entities.
struct SolidEntities : public Graphics::RenderPass {
public:
	SolidEntities()
		: Graphics::RenderPass("solidEntities") {
	}
protected:
	void doRun(Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for transparent entities.
struct TransparentEntities : public Graphics::RenderPass {
public:
	TransparentEntities()
		: Graphics::RenderPass("transparentEntities") {
	}
protected:
	void doRun(Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for entity reflections.
struct EntityReflections : public Graphics::RenderPass {
public:
	EntityReflections()
		: Graphics::RenderPass("entityReflections") {
	}
protected:
	void doRun(Camera& cam, const TileList& tl, const EntityList& el) override;
};

/// The render pass for entity shadows.
struct EntityShadows : public Graphics::RenderPass {
public:
	EntityShadows()
		: Graphics::RenderPass("entityShadows") {
	}
protected:
	void doRun(Camera& cam, const TileList& tl, const EntityList& el) override;
private:
	// Used if low-quality shadows are enabled.
	void doLowQualityShadow(const CHR_REF character);
	// Used if high-quality shadows are enabled.
	void doHighQualityShadow(const CHR_REF character);
	// Used by all shadow qualities.
	void doShadowSprite(float intensity, GLvertex v[]);
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
