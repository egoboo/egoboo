#pragma once

#include "game/Graphics/RenderPass.hpp"

namespace Ego {
namespace Graphics {

/// @brief The first render pass for reflective tiles i.e. tiles which do reflect entities.
/// Ran before the pass rendering the reflections of entities.
struct ReflectiveTilesFirstRenderPass : public RenderPass
{
public:
    ReflectiveTilesFirstRenderPass();
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

} // namespace Graphics
} // namespace Ego
