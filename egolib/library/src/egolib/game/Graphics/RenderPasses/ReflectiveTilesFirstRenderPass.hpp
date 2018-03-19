#pragma once

#include "egolib/game/Graphics/RenderPass.hpp"

namespace Ego::Graphics {

/// @brief The first render pass for reflective tiles i.e. tiles which do reflect entities.
/// Ran before the pass rendering the reflections of entities.
struct ReflectiveTilesFirstRenderPass : public RenderPass
{
public:
    ReflectiveTilesFirstRenderPass();
protected:
    void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

} // namespace Ego::Graphics
