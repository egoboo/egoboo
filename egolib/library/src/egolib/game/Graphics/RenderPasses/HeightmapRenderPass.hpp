#pragma once

#include "egolib/game/Graphics/RenderPass.hpp"

namespace Ego::Graphics {

/// The render pass for the heightmap.
struct HeightmapRenderPass : public RenderPass
{
public:
    HeightmapRenderPass();
protected:
    void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

} // namespace Ego::Graphics
