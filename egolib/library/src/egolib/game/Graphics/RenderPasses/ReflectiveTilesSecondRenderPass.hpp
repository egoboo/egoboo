#pragma once

#include "egolib/game/Graphics/RenderPass.hpp"

namespace Ego::Graphics {

/// @brief The 2nd pass for reflective tiles i.e. tiles which do reflect entities.
/// Ran after the pass rendering the reflections of entities.
struct ReflectiveTilesSecondRenderPass : public RenderPass
{
public:
    ReflectiveTilesSecondRenderPass();
protected:
    void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
private:
    // Used if reflections are enabled.
    void doReflectionsEnabled(::Camera& cam, const TileList& tl, const EntityList& el);
    // Used if reflections are disabled.
    void doReflectionsDisabled(::Camera& cam, const TileList& tl, const EntityList& el);
};

} // namespace Ego::Graphics
