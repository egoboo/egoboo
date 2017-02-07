#pragma once

#include "game/Graphics/RenderPass.hpp"

namespace Ego {
namespace Graphics {

/// The render pass for the heightmap.
struct HeightmapRenderPass : public RenderPass
{
public:
    HeightmapRenderPass();
protected:
    void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

} // namespace Graphics
} // namespace Ego
